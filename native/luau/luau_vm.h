// native/luau/luau_vm.h
// Phase 8: Luau Engine — LuauVM wrapper

#pragma once

#include <handle_manager/handle.h>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

// Forward-declare Luau state to avoid exposing lua.h in headers
struct lua_State;

namespace nrp::luau {

/**
 * Typed value returned by / passed to LuauVM.
 * Mirrors LuauValue on the Kotlin side.
 */
struct LuauValue {
    enum class Type { Nil, Bool, Number, String };

    Type type = Type::Nil;
    bool   b     = false;
    double n     = 0.0;
    std::string s;

    static LuauValue nil()              { return {}; }
    static LuauValue of(bool v)         { LuauValue r; r.type = Type::Bool;   r.b = v;  return r; }
    static LuauValue of(double v)       { LuauValue r; r.type = Type::Number; r.n = v;  return r; }
    static LuauValue of(const std::string& v) { LuauValue r; r.type = Type::String; r.s = v; return r; }

    [[nodiscard]] bool   isNil()    const noexcept { return type == Type::Nil; }
    [[nodiscard]] bool   isBool()   const noexcept { return type == Type::Bool; }
    [[nodiscard]] bool   isNumber() const noexcept { return type == Type::Number; }
    [[nodiscard]] bool   isString() const noexcept { return type == Type::String; }
    [[nodiscard]] bool   toBool()   const noexcept { return type == Type::Bool ? b : (type != Type::Nil); }
    [[nodiscard]] double toDouble() const noexcept { return n; }
    [[nodiscard]] int    toInt()    const noexcept { return static_cast<int>(n); }
    [[nodiscard]] std::string toString() const;
};

/**
 * Exception thrown when Luau source has syntax errors.
 */
class LuauCompileException : public std::exception {
public:
    explicit LuauCompileException(std::string msg) : msg_(std::move(msg)) {}
    [[nodiscard]] const char* what() const noexcept override { return msg_.c_str(); }
private:
    std::string msg_;
};

/**
 * Exception thrown when a Luau script throws an unhandled error.
 */
class LuauRuntimeException : public std::exception {
public:
    explicit LuauRuntimeException(std::string msg) : msg_(std::move(msg)) {}
    [[nodiscard]] const char* what() const noexcept override { return msg_.c_str(); }
private:
    std::string msg_;
};

/**
 * Exception thrown when a method is called on a closed LuauVM.
 */
class VMClosedException : public std::exception {
public:
    [[nodiscard]] const char* what() const noexcept override {
        return "VMClosedException: LuauVM has been closed";
    }
};

/**
 * The Luau virtual machine.
 * Wraps a lua_State and auto-registers lexsoup, regex, and js modules.
 * NOT thread-safe — use from a single thread only.
 */
class LuauVM {
public:
    static constexpr uint16_t type_tag = 0x0401;

    /**
     * Creates a LuauVM with no memory limit and all native modules registered.
     */
    static std::unique_ptr<LuauVM> create();

    /**
     * Creates a LuauVM with a custom memory limit (in bytes).
     */
    static std::unique_ptr<LuauVM> createWithMemoryLimit(size_t max_bytes);

    ~LuauVM();

    // Non-copyable, non-moveable
    LuauVM(const LuauVM&) = delete;
    LuauVM& operator=(const LuauVM&) = delete;

    // ---- Script execution ----

    /**
     * Compiles and executes a Luau script.
     * @param script     Luau source code
     * @param chunk_name name for error messages
     * @return the return value of the script
     */
    LuauValue execute(const std::string& script, const std::string& chunk_name = "=(chunk)");

    /**
     * Compiles Luau source to bytecode without executing.
     * @return compiled bytecode
     */
    std::vector<uint8_t> compile(const std::string& script, const std::string& chunk_name = "=(chunk)");

    /**
     * Executes precompiled Luau bytecode.
     */
    LuauValue executeCompiled(const std::vector<uint8_t>& bytecode, const std::string& chunk_name = "=(chunk)");

    // ---- Global state ----

    void       setGlobal(const std::string& name, const LuauValue& value);
    LuauValue  getGlobal(const std::string& name);
    void       removeGlobal(const std::string& name);

    // ---- Lifecycle ----
    void close();
    [[nodiscard]] bool is_closed() const noexcept { return is_closed_; }

    [[nodiscard]] lua_State* raw() const noexcept { return L_; }

private:
    explicit LuauVM(lua_State* L, size_t max_bytes = 0);

    // Opens standard Luau libraries and registers all NRP native modules
    void open_libs();
    void register_native_modules();

    // Push/pop LuauValue to/from Lua stack
    void        push_value(const LuauValue& v);
    LuauValue   pop_value(int stack_idx = -1);

    lua_State* L_ = nullptr;
    bool       is_closed_ = false;

    // Memory tracking for limited VMs
    size_t max_bytes_ = 0;
};

} // namespace nrp::luau
