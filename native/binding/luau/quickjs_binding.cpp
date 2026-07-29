// native/binding/luau/quickjs_binding.cpp
// Phase 7: QuickJS Engine Luau Bindings

#include "quickjs_binding.h"
#include "luau_binding.h"
#include <quickjs/js_runtime.h>
#include <quickjs/js_context.h>
#include <quickjs/js_value.h>
#include <quickjs/js_exception.h>
#include <runtime.h>
#include <quickjs.h>
#include <string>
#include <vector>

namespace nrp::luau {

// ===================================================================
// JSValue (Luau)
// ===================================================================

static const char* MT_JSVALUE  = "luandro.js.JSValue";
static const char* MT_CONTEXT  = "luandro.js.Context";
static const char* MT_RUNTIME  = "luandro.js.Runtime";

// Forward declarations
static void push_jsvalue(lua_State* L, Handle h);

static int jsv_isUndefined(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:isUndefined", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_JSVALUE);
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        lua_pushboolean(L, (!v || v->isUndefined()) ? 1 : 0);
        return 1;
    });
}

static int jsv_isNull(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:isNull", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_JSVALUE);
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        lua_pushboolean(L, (v && v->isNull()) ? 1 : 0);
        return 1;
    });
}

static int jsv_isObject(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:isObject", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_JSVALUE);
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        lua_pushboolean(L, (v && v->isObject()) ? 1 : 0);
        return 1;
    });
}

static int jsv_isArray(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:isArray", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_JSVALUE);
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        lua_pushboolean(L, (v && v->isArray()) ? 1 : 0);
        return 1;
    });
}

static int jsv_isFunction(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:isFunction", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_JSVALUE);
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        lua_pushboolean(L, (v && v->isFunction()) ? 1 : 0);
        return 1;
    });
}

static int jsv_isNumber(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:isNumber", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_JSVALUE);
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        lua_pushboolean(L, (v && v->isNumber()) ? 1 : 0);
        return 1;
    });
}

static int jsv_isString(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:isString", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_JSVALUE);
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        lua_pushboolean(L, (v && v->isString()) ? 1 : 0);
        return 1;
    });
}

static int jsv_toBool(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:toBool", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_JSVALUE);
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        if (!v) { luaL_error(L, "JSValue is closed or invalid"); return 0; }
        lua_pushboolean(L, v->toBool() ? 1 : 0);
        return 1;
    });
}

static int jsv_toInt(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:toInt", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_JSVALUE);
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        if (!v) { luaL_error(L, "JSValue is closed or invalid"); return 0; }
        lua_pushinteger(L, v->toInt());
        return 1;
    });
}

static int jsv_toNumber(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:toNumber", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_JSVALUE);
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        if (!v) { luaL_error(L, "JSValue is closed or invalid"); return 0; }
        lua_pushnumber(L, v->toDouble());
        return 1;
    });
}

static int jsv_toString(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:toString", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_JSVALUE);
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        if (!v) { lua_pushstring(L, "(closed)"); return 1; }
        std::string s = v->toString();
        lua_pushlstring(L, s.data(), s.size());
        return 1;
    });
}

static int jsv_getProperty(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:getProperty", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_JSVALUE);
        size_t len = 0;
        const char* name = luaL_checklstring(L, 2, &len);
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        if (!v) { luaL_error(L, "JSValue is closed or invalid"); return 0; }
        Handle prop_h = v->getProperty(v->parent_context(), std::string(name, len));
        push_jsvalue(L, prop_h);
        return 1;
    });
}

static int jsv_setProperty(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:setProperty", [&]() {
        Handle h    = check_handle_userdata(L, 1, MT_JSVALUE);
        Handle vh   = check_handle_userdata(L, 3, MT_JSVALUE);
        size_t len = 0;
        const char* name = luaL_checklstring(L, 2, &len);
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        if (!v) { luaL_error(L, "JSValue is closed or invalid"); return 0; }
        v->setProperty(std::string(name, len), vh);
        return 0;
    });
}

static int jsv_getAt(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:getAt", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_JSVALUE);
        int idx  = static_cast<int>(luaL_checkinteger(L, 2)) - 1; // Lua 1-indexed
        auto* v  = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        if (!v) { luaL_error(L, "JSValue is closed or invalid"); return 0; }
        Handle prop_h = v->getPropertyAt(v->parent_context(), idx);
        push_jsvalue(L, prop_h);
        return 1;
    });
}

static int jsv_length(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:length", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_JSVALUE);
        auto* v  = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(h);
        if (!v) { luaL_error(L, "JSValue is closed or invalid"); return 0; }
        lua_pushinteger(L, v->length());
        return 1;
    });
}

static int jsv_call(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:call", [&]() {
        Handle func_h  = check_handle_userdata(L, 1, MT_JSVALUE);
        Handle this_h  = kInvalidHandle;
        if (!lua_isnil(L, 2)) {
            this_h = check_handle_userdata(L, 2, MT_JSVALUE);
        }
        // Gather remaining arguments as JSValue handles
        int nargs = lua_gettop(L) - 2;
        std::vector<Handle> arg_handles;
        for (int i = 0; i < nargs; ++i) {
            arg_handles.push_back(check_handle_userdata(L, 3 + i, MT_JSVALUE));
        }
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(func_h);
        if (!v) { luaL_error(L, "JSValue is closed or invalid"); return 0; }
        Handle result_h = v->call(v->parent_context(), this_h, arg_handles);
        push_jsvalue(L, result_h);
        return 1;
    });
}

static int jsv_free(lua_State* L) {
    return with_lua_exceptions(L, "JSValue:free", [&]() {
        auto* lh = static_cast<LuaHandle*>(luaL_checkudata(L, 1, MT_JSVALUE));
        if (lh && lh->handle != kInvalidHandle) {
            auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(lh->handle);
            if (v) {
                Handle parent_ctx = v->parent_context();
                if (parent_ctx != kInvalidHandle) {
                    auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(parent_ctx);
                    if (ctx) ctx->untrack_child(lh->handle);
                }
                v->free();
            }
            nrp::Runtime::get().objects().destroy(lh->handle);
            lh->handle = kInvalidHandle;
        }
        return 0;
    });
}

static int jsv_gc(lua_State* L) {
    auto* lh = static_cast<LuaHandle*>(luaL_checkudata(L, 1, MT_JSVALUE));
    if (lh && lh->handle != kInvalidHandle) {
        try {
            auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(lh->handle);
            if (v) v->free();
            nrp::Runtime::get().objects().destroy(lh->handle);
        } catch (...) {}
        lh->handle = kInvalidHandle;
    }
    return 0;
}

static int jsv_newindex(lua_State* L) {
    return read_only_newindex(L, MT_JSVALUE);
}

static const luaL_Reg jsv_meta[]  = {
    {"__gc",       jsv_gc},
    {"__tostring", jsv_toString},
    {"__newindex", jsv_newindex},
    {nullptr,      nullptr}
};
static const luaL_Reg jsv_index[] = {
    {"isUndefined",  jsv_isUndefined},
    {"isNull",       jsv_isNull},
    {"isObject",     jsv_isObject},
    {"isArray",      jsv_isArray},
    {"isFunction",   jsv_isFunction},
    {"isNumber",     jsv_isNumber},
    {"isString",     jsv_isString},
    {"toBool",       jsv_toBool},
    {"toInt",        jsv_toInt},
    {"toNumber",     jsv_toNumber},
    {"toString",     jsv_toString},
    {"getProperty",  jsv_getProperty},
    {"setProperty",  jsv_setProperty},
    {"getAt",        jsv_getAt},
    {"length",       jsv_length},
    {"call",         jsv_call},
    {"free",         jsv_free},
    {nullptr,        nullptr}
};

static void push_jsvalue(lua_State* L, Handle h) {
    if (h != kInvalidHandle) {
        push_handle_userdata(L, h, MT_JSVALUE);
    } else {
        lua_pushnil(L);
    }
}

// ===================================================================
// Context (Luau)
// ===================================================================

static int ctx_eval(lua_State* L) {
    return with_lua_exceptions(L, "Context:eval", [&]() {
        Handle h     = check_handle_userdata(L, 1, MT_CONTEXT);
        size_t len   = 0;
        const char*  code = luaL_checklstring(L, 2, &len);
        std::string filename = "<eval>";
        if (lua_gettop(L) >= 3 && lua_isstring(L, 3)) {
            size_t fl = 0;
            filename = std::string(luaL_checklstring(L, 3, &fl), fl);
        }
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        Handle result_h = ctx->eval(h, std::string(code, len), filename);
        push_jsvalue(L, result_h);
        return 1;
    });
}

static int ctx_evalModule(lua_State* L) {
    return with_lua_exceptions(L, "Context:evalModule", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_CONTEXT);
        size_t cl = 0, fl = 0;
        const char* code     = luaL_checklstring(L, 2, &cl);
        const char* filename = luaL_checklstring(L, 3, &fl);
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        Handle result_h = ctx->evalModule(h, std::string(code, cl), std::string(filename, fl));
        push_jsvalue(L, result_h);
        return 1;
    });
}

static int ctx_setGlobal(lua_State* L) {
    return with_lua_exceptions(L, "Context:setGlobal", [&]() {
        Handle h  = check_handle_userdata(L, 1, MT_CONTEXT);
        Handle vh = check_handle_userdata(L, 3, MT_JSVALUE);
        size_t nl = 0;
        const char* name = luaL_checklstring(L, 2, &nl);
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        ctx->setGlobal(std::string(name, nl), vh);
        return 0;
    });
}

static int ctx_getGlobal(lua_State* L) {
    return with_lua_exceptions(L, "Context:getGlobal", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_CONTEXT);
        size_t nl = 0;
        const char* name = luaL_checklstring(L, 2, &nl);
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        Handle result_h = ctx->getGlobal(h, std::string(name, nl));
        push_jsvalue(L, result_h);
        return 1;
    });
}

static int ctx_parseJSON(lua_State* L) {
    return with_lua_exceptions(L, "Context:parseJSON", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_CONTEXT);
        size_t jl = 0;
        const char* json = luaL_checklstring(L, 2, &jl);
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        Handle result_h = ctx->parseJSON(h, std::string(json, jl));
        push_jsvalue(L, result_h);
        return 1;
    });
}

static int ctx_stringifyJSON(lua_State* L) {
    return with_lua_exceptions(L, "Context:stringifyJSON", [&]() {
        Handle h  = check_handle_userdata(L, 1, MT_CONTEXT);
        Handle vh = check_handle_userdata(L, 2, MT_JSVALUE);
        int indent = 0;
        if (lua_gettop(L) >= 3) indent = static_cast<int>(luaL_optinteger(L, 3, 0));
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        std::string result = ctx->stringifyJSON(vh, indent);
        lua_pushlstring(L, result.data(), result.size());
        return 1;
    });
}

static int ctx_newObject(lua_State* L) {
    return with_lua_exceptions(L, "Context:newObject", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_CONTEXT);
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        push_jsvalue(L, ctx->newObject(h));
        return 1;
    });
}

static int ctx_newArray(lua_State* L) {
    return with_lua_exceptions(L, "Context:newArray", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_CONTEXT);
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        push_jsvalue(L, ctx->newArray(h));
        return 1;
    });
}

static int ctx_newString(lua_State* L) {
    return with_lua_exceptions(L, "Context:newString", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_CONTEXT);
        size_t sl = 0;
        const char* s = luaL_checklstring(L, 2, &sl);
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        push_jsvalue(L, ctx->newString(h, std::string(s, sl)));
        return 1;
    });
}

static int ctx_newInt(lua_State* L) {
    return with_lua_exceptions(L, "Context:newInt", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_CONTEXT);
        int val  = static_cast<int>(luaL_checkinteger(L, 2));
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        push_jsvalue(L, ctx->newInt(h, val));
        return 1;
    });
}

static int ctx_newDouble(lua_State* L) {
    return with_lua_exceptions(L, "Context:newDouble", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_CONTEXT);
        double val = static_cast<double>(luaL_checknumber(L, 2));
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        push_jsvalue(L, ctx->newDouble(h, val));
        return 1;
    });
}

static int ctx_newBool(lua_State* L) {
    return with_lua_exceptions(L, "Context:newBool", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_CONTEXT);
        if (!lua_isboolean(L, 2)) { luaL_error(L, "Context:newBool: expected boolean"); return 0; }
        bool val = lua_toboolean(L, 2) != 0;
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        push_jsvalue(L, ctx->newBool(h, val));
        return 1;
    });
}

static int ctx_undefined(lua_State* L) {
    return with_lua_exceptions(L, "Context:undefined", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_CONTEXT);
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        push_jsvalue(L, ctx->jsUndefined(h));
        return 1;
    });
}

static int ctx_null_val(lua_State* L) {
    return with_lua_exceptions(L, "Context:null_val", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_CONTEXT);
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        push_jsvalue(L, ctx->jsNull(h));
        return 1;
    });
}

static int ctx_executePendingJobs(lua_State* L) {
    return with_lua_exceptions(L, "Context:executePendingJobs", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_CONTEXT);
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        lua_pushinteger(L, ctx->executePendingJobs());
        return 1;
    });
}

static int ctx_setModuleLoader(lua_State* L) {
    return with_lua_exceptions(L, "Context:setModuleLoader", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_CONTEXT);
        luaL_checktype(L, 2, LUA_TFUNCTION);
        // Store function reference
        lua_pushvalue(L, 2);
        int func_ref = lua_ref(L, LUA_REGISTRYINDEX);
        lua_State* L_ref = L;
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(h);
        if (!ctx) { luaL_error(L, "Context is closed or invalid"); return 0; }
        ctx->setModuleLoader([L_ref, func_ref](const std::string& name, const std::string& base) -> std::string {
            lua_rawgeti(L_ref, LUA_REGISTRYINDEX, func_ref);
            lua_pushlstring(L_ref, name.data(), name.size());
            lua_pushlstring(L_ref, base.data(), base.size());
            if (lua_pcall(L_ref, 2, 1, 0) != 0) {
                lua_pop(L_ref, 1);
                return "";
            }
            std::string result;
            if (lua_isstring(L_ref, -1)) {
                size_t len = 0;
                const char* s = lua_tolstring(L_ref, -1, &len);
                if (s) result = std::string(s, len);
            }
            lua_pop(L_ref, 1);
            return result;
        });
        return 0;
    });
}

static int ctx_close(lua_State* L) {
    return with_lua_exceptions(L, "Context:close", [&]() {
        auto* lh = static_cast<LuaHandle*>(luaL_checkudata(L, 1, MT_CONTEXT));
        if (lh && lh->handle != kInvalidHandle) {
            auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(lh->handle);
            if (ctx) {
                Handle rt_h = ctx->runtime();
                if (rt_h != kInvalidHandle) {
                    auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(rt_h);
                    if (rt) rt->untrack_child(lh->handle);
                }
                ctx->close();
            }
            nrp::Runtime::get().objects().destroy(lh->handle);
            lh->handle = kInvalidHandle;
        }
        return 0;
    });
}

static int ctx_gc(lua_State* L) {
    auto* lh = static_cast<LuaHandle*>(luaL_checkudata(L, 1, MT_CONTEXT));
    if (lh && lh->handle != kInvalidHandle) {
        try {
            auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(lh->handle);
            if (ctx) ctx->close();
            nrp::Runtime::get().objects().destroy(lh->handle);
        } catch (...) {}
        lh->handle = kInvalidHandle;
    }
    return 0;
}

static int ctx_newindex(lua_State* L) { return read_only_newindex(L, MT_CONTEXT); }

static const luaL_Reg ctx_meta[]  = {
    {"__gc",       ctx_gc},
    {"__newindex", ctx_newindex},
    {nullptr,      nullptr}
};
static const luaL_Reg ctx_index[] = {
    {"eval",               ctx_eval},
    {"evalModule",         ctx_evalModule},
    {"setGlobal",          ctx_setGlobal},
    {"getGlobal",          ctx_getGlobal},
    {"parseJSON",          ctx_parseJSON},
    {"stringifyJSON",      ctx_stringifyJSON},
    {"newObject",          ctx_newObject},
    {"newArray",           ctx_newArray},
    {"newString",          ctx_newString},
    {"newInt",             ctx_newInt},
    {"newDouble",          ctx_newDouble},
    {"newBool",            ctx_newBool},
    {"undefined",          ctx_undefined},
    {"null_val",           ctx_null_val},
    {"executePendingJobs", ctx_executePendingJobs},
    {"setModuleLoader",    ctx_setModuleLoader},
    {"close",              ctx_close},
    {nullptr,              nullptr}
};

// ===================================================================
// Runtime (Luau)
// ===================================================================

static int rt_newContext(lua_State* L) {
    return with_lua_exceptions(L, "Runtime:newContext", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_RUNTIME);
        auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(h);
        if (!rt) { luaL_error(L, "Runtime is closed or invalid"); return 0; }
        Handle ctx_h = rt->newContext(h);
        push_handle_userdata(L, ctx_h, MT_CONTEXT);
        return 1;
    });
}

static int rt_gc_js(lua_State* L) {
    return with_lua_exceptions(L, "Runtime:gc", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_RUNTIME);
        auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(h);
        if (!rt) { luaL_error(L, "Runtime is closed or invalid"); return 0; }
        rt->gc();
        return 0;
    });
}

static int rt_setMemoryLimit(lua_State* L) {
    return with_lua_exceptions(L, "Runtime:setMemoryLimit", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_RUNTIME);
        size_t limit = static_cast<size_t>(luaL_checkinteger(L, 2));
        auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(h);
        if (!rt) { luaL_error(L, "Runtime is closed or invalid"); return 0; }
        rt->setMemoryLimit(limit);
        return 0;
    });
}

static int rt_memoryUsed(lua_State* L) {
    return with_lua_exceptions(L, "Runtime:memoryUsed", [&]() {
        Handle h = check_handle_userdata(L, 1, MT_RUNTIME);
        auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(h);
        if (!rt) { luaL_error(L, "Runtime is closed or invalid"); return 0; }
        lua_pushinteger(L, static_cast<lua_Integer>(rt->memoryUsed()));
        return 1;
    });
}

static int rt_close(lua_State* L) {
    return with_lua_exceptions(L, "Runtime:close", [&]() {
        auto* lh = static_cast<LuaHandle*>(luaL_checkudata(L, 1, MT_RUNTIME));
        if (lh && lh->handle != kInvalidHandle) {
            auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(lh->handle);
            if (rt) rt->close();
            nrp::Runtime::get().objects().destroy(lh->handle);
            lh->handle = kInvalidHandle;
        }
        return 0;
    });
}

static int rt_gc(lua_State* L) {
    auto* lh = static_cast<LuaHandle*>(luaL_checkudata(L, 1, MT_RUNTIME));
    if (lh && lh->handle != kInvalidHandle) {
        try {
            auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(lh->handle);
            if (rt) rt->close();
            nrp::Runtime::get().objects().destroy(lh->handle);
        } catch (...) {}
        lh->handle = kInvalidHandle;
    }
    return 0;
}

static int rt_newindex(lua_State* L) { return read_only_newindex(L, MT_RUNTIME); }

static const luaL_Reg rt_meta[]  = {
    {"__gc",       rt_gc},
    {"__newindex", rt_newindex},
    {nullptr,      nullptr}
};
static const luaL_Reg rt_index[] = {
    {"newContext",     rt_newContext},
    {"gc",             rt_gc_js},
    {"setMemoryLimit", rt_setMemoryLimit},
    {"memoryUsed",     rt_memoryUsed},
    {"close",          rt_close},
    {nullptr,          nullptr}
};

// ===================================================================
// js global module
// ===================================================================

static int js_newRuntime(lua_State* L) {
    return with_lua_exceptions(L, "js.newRuntime", [&]() {
        JSRuntime* rt = JS_NewRuntime();
        if (!rt) { luaL_error(L, "js.newRuntime: out of memory"); return 0; }
        auto rt_obj = std::make_unique<nrp::js::Runtime>(rt);
        Handle h = nrp::Runtime::get().handles().allocate(nrp::js::Runtime::type_tag);
        nrp::Runtime::get().objects().insert<nrp::js::Runtime>(h, std::move(rt_obj));
        push_handle_userdata(L, h, MT_RUNTIME);
        return 1;
    });
}

static int js_newRuntimeWithLimit(lua_State* L) {
    return with_lua_exceptions(L, "js.newRuntimeWithLimit", [&]() {
        size_t limit = static_cast<size_t>(luaL_checkinteger(L, 1));
        JSRuntime* rt = JS_NewRuntime();
        if (!rt) { luaL_error(L, "js.newRuntimeWithLimit: out of memory"); return 0; }
        JS_SetMemoryLimit(rt, limit);
        auto rt_obj = std::make_unique<nrp::js::Runtime>(rt);
        Handle h = nrp::Runtime::get().handles().allocate(nrp::js::Runtime::type_tag);
        nrp::Runtime::get().objects().insert<nrp::js::Runtime>(h, std::move(rt_obj));
        push_handle_userdata(L, h, MT_RUNTIME);
        return 1;
    });
}

static const luaL_Reg js_module[] = {
    {"newRuntime",          js_newRuntime},
    {"newRuntimeWithLimit", js_newRuntimeWithLimit},
    {nullptr,               nullptr}
};

// ===================================================================
// luaopen_js — module entry point
// ===================================================================

int luaopen_js(lua_State* L) {
    // Register JSValue metatable
    register_metatable(L, MT_JSVALUE, jsv_meta, jsv_index);

    // Register Context metatable
    register_metatable(L, MT_CONTEXT, ctx_meta, ctx_index);

    // Register Runtime metatable
    register_metatable(L, MT_RUNTIME, rt_meta, rt_index);

    // Create global 'js' table
    luaL_register(L, "js", js_module);

    return 1;
}

} // namespace nrp::luau
