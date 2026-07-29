// native/luau/luau_vm.cpp
// Phase 8: Luau Engine — LuauVM implementation

#include "luau_vm.h"

// Luau VM headers
#include <lua.h>
#include <lualib.h>
#include <luacode.h>       // luau_compile()
#include <luaconf.h>

// NRP native module registrars
#include <binding/luau/lexsoup_binding.h>
#include <binding/luau/regex_binding.h>
#include <binding/luau/quickjs_binding.h>

#include <stdexcept>
#include <cstring>

namespace nrp::luau {

// ===================================================================
// LuauValue::toString
// ===================================================================

std::string LuauValue::toString() const {
    switch (type) {
        case Type::Nil:    return "nil";
        case Type::Bool:   return b ? "true" : "false";
        case Type::Number: {
            // Integer if no fractional part
            if (n == static_cast<double>(static_cast<int64_t>(n))) {
                return std::to_string(static_cast<int64_t>(n));
            }
            return std::to_string(n);
        }
        case Type::String: return s;
    }
    return "nil";
}

// ===================================================================
// Memory limit allocator
// ===================================================================

struct MemLimitState {
    size_t limit;
    size_t used;
};

static void* mem_limit_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    auto* st = static_cast<MemLimitState*>(ud);
    if (nsize == 0) {
        st->used -= osize;
        free(ptr);
        return nullptr;
    }
    size_t new_used = st->used - osize + nsize;
    if (st->limit > 0 && new_used > st->limit) {
        return nullptr;  // OOM — Lua will handle it
    }
    void* np = realloc(ptr, nsize);
    if (np) st->used = new_used;
    return np;
}

// ===================================================================
// Factory methods
// ===================================================================

std::unique_ptr<LuauVM> LuauVM::create() {
    lua_State* L = luaL_newstate();
    if (!L) throw std::bad_alloc();
    return std::unique_ptr<LuauVM>(new LuauVM(L, 0));
}

std::unique_ptr<LuauVM> LuauVM::createWithMemoryLimit(size_t max_bytes) {
    if (max_bytes == 0) throw std::invalid_argument("maxHeapBytes must be > 0");
    // Use custom allocator that enforces limit
    auto* st = new MemLimitState{max_bytes, 0};
    lua_State* L = lua_newstate(mem_limit_alloc, st);
    if (!L) {
        delete st;
        throw std::bad_alloc();
    }
    return std::unique_ptr<LuauVM>(new LuauVM(L, max_bytes));
}

// ===================================================================
// Constructor / Destructor
// ===================================================================

LuauVM::LuauVM(lua_State* L, size_t max_bytes)
    : L_(L), max_bytes_(max_bytes)
{
    open_libs();
    register_native_modules();
}

LuauVM::~LuauVM() {
    if (!is_closed_) close();
}

// ===================================================================
// Standard library setup + native module registration
// ===================================================================

// Helper for Luau module requiring
static void luaL_requiref(lua_State* L, const char* modname, lua_CFunction openf, int glb) {
    openf(L);
    if (glb) {
        lua_pushvalue(L, -1);
        lua_setglobal(L, modname);
    }
}

void LuauVM::open_libs() {
    // Open Luau standard libraries
    luaL_openlibs(L_);
}

void LuauVM::register_native_modules() {
    // Register lexsoup global
    luaL_requiref(L_, "lexsoup", luaopen_lexsoup, 1);
    lua_pop(L_, 1);

    // Register regex global
    luaL_requiref(L_, "regex", luaopen_regex, 1);
    lua_pop(L_, 1);

    // Register js global
    luaL_requiref(L_, "js", luaopen_js, 1);
    lua_pop(L_, 1);
}

// ===================================================================
// Stack helpers
// ===================================================================

void LuauVM::push_value(const LuauValue& v) {
    switch (v.type) {
        case LuauValue::Type::Nil:    lua_pushnil(L_);             break;
        case LuauValue::Type::Bool:   lua_pushboolean(L_, v.b ? 1 : 0); break;
        case LuauValue::Type::Number: lua_pushnumber(L_, v.n);     break;
        case LuauValue::Type::String: lua_pushlstring(L_, v.s.data(), v.s.size()); break;
    }
}

LuauValue LuauVM::pop_value(int stack_idx) {
    if (lua_isnone(L_, stack_idx)) return LuauValue::nil();

    switch (lua_type(L_, stack_idx)) {
        case LUA_TNIL:      return LuauValue::nil();
        case LUA_TBOOLEAN:  return LuauValue::of(lua_toboolean(L_, stack_idx) != 0);
        case LUA_TNUMBER:   return LuauValue::of(static_cast<double>(lua_tonumber(L_, stack_idx)));
        case LUA_TSTRING: {
            size_t len = 0;
            const char* s = lua_tolstring(L_, stack_idx, &len);
            return LuauValue::of(std::string(s ? s : "", s ? len : 0));
        }
        case LUA_TUSERDATA:
        case LUA_TLIGHTUSERDATA:
        case LUA_TTABLE:
        case LUA_TFUNCTION:
        case LUA_TTHREAD:
            // Return string representation for complex types
            return LuauValue::of(std::string(lua_typename(L_, lua_type(L_, stack_idx))));
        default:
            return LuauValue::nil();
    }
}

// ===================================================================
// Script execution
// ===================================================================

LuauValue LuauVM::execute(const std::string& script, const std::string& chunk_name) {
    if (is_closed_) throw VMClosedException();

    // Step 1: Compile to bytecode
    std::vector<uint8_t> bytecode = compile(script, chunk_name);

    // Step 2: Execute
    return executeCompiled(bytecode, chunk_name);
}

std::vector<uint8_t> LuauVM::compile(const std::string& script, const std::string& chunk_name) {
    if (is_closed_) throw VMClosedException();

    size_t bytecode_size = 0;
    // luau_compile returns a malloc'd buffer or sets size=0 on error
    char* bytecode_raw = luau_compile(
        script.c_str(), script.size(),
        nullptr,           // compile options (use defaults)
        &bytecode_size
    );

    if (!bytecode_raw || bytecode_size == 0) {
        // Compilation error — the error message is in bytecode_raw (byte 0 == '\0')
        std::string err_msg = "LuauCompileException: failed to compile " + chunk_name;
        if (bytecode_raw && bytecode_size > 1) {
            // When bytecode[0] == 0, the rest is the error string
            err_msg = std::string(bytecode_raw + 1, bytecode_size - 1);
        }
        free(bytecode_raw);
        throw LuauCompileException(err_msg);
    }

    // Check for compilation error (QuickJS convention: bytecode[0] == 0 means error)
    if (static_cast<unsigned char>(bytecode_raw[0]) == 0 && bytecode_size > 1) {
        std::string err_msg(bytecode_raw + 1, bytecode_size - 1);
        free(bytecode_raw);
        throw LuauCompileException("LuauCompileException: " + err_msg);
    }

    std::vector<uint8_t> result(bytecode_raw, bytecode_raw + bytecode_size);
    free(bytecode_raw);
    return result;
}

LuauValue LuauVM::executeCompiled(const std::vector<uint8_t>& bytecode, const std::string& chunk_name) {
    if (is_closed_) throw VMClosedException();

    int top_before = lua_gettop(L_);

    // Load bytecode as a Lua chunk
    int load_result = luau_load(
        L_,
        chunk_name.c_str(),
        reinterpret_cast<const char*>(bytecode.data()),
        bytecode.size(),
        0  // env: 0 means use global env
    );

    if (load_result != 0) {
        // Error on stack
        size_t len = 0;
        const char* err = lua_tolstring(L_, -1, &len);
        std::string err_msg = err ? std::string(err, len) : "LuauRuntimeException: load failed";
        lua_settop(L_, top_before);
        throw LuauRuntimeException(err_msg);
    }

    // Call the loaded chunk (0 args, 1 return value, no error handler)
    int call_result = lua_pcall(L_, 0, 1, 0);

    if (call_result != 0) {
        size_t len = 0;
        const char* err = lua_tolstring(L_, -1, &len);
        std::string err_msg = err ? std::string(err, len) : "LuauRuntimeException: runtime error";
        lua_settop(L_, top_before);
        throw LuauRuntimeException(err_msg);
    }

    // Extract return value
    LuauValue result = pop_value(-1);
    lua_settop(L_, top_before);  // restore stack
    return result;
}

// ===================================================================
// Global state
// ===================================================================

void LuauVM::setGlobal(const std::string& name, const LuauValue& value) {
    if (is_closed_) throw VMClosedException();
    push_value(value);
    lua_setglobal(L_, name.c_str());
}

LuauValue LuauVM::getGlobal(const std::string& name) {
    if (is_closed_) throw VMClosedException();
    lua_getglobal(L_, name.c_str());
    LuauValue v = pop_value(-1);
    lua_pop(L_, 1);
    return v;
}

void LuauVM::removeGlobal(const std::string& name) {
    if (is_closed_) throw VMClosedException();
    lua_pushnil(L_);
    lua_setglobal(L_, name.c_str());
}

// ===================================================================
// Lifecycle
// ===================================================================

void LuauVM::close() {
    if (is_closed_) return;
    is_closed_ = true;
    if (L_) {
        // Free custom allocator state if present
        if (max_bytes_ > 0) {
            void* ud = nullptr;
            lua_Alloc alloc_fn = lua_getallocf(L_, &ud);
            (void)alloc_fn;
            lua_close(L_);
            if (ud) delete static_cast<MemLimitState*>(ud);
        } else {
            lua_close(L_);
        }
        L_ = nullptr;
    }
}

} // namespace nrp::luau
