// native/binding/luau/luau_binding.h
// Phase 4: Binding Infrastructure

#pragma once

#include <lua.h>
#include <lualib.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "handle_manager/handle.h"
#include "object_manager/object_manager.h"
#include "runtime.h"
#include "exceptions/exception_manager.h"

namespace nrp::luau {

// Reusable struct layout for all NRP handles in Luau
struct LuaHandle {
    Handle handle;
};

// Reusable helper to register a metatable for a userdata type
inline void register_metatable(lua_State* L,
                               const char* mt_name,
                               const luaL_Reg* meta_funcs,
                               const luaL_Reg* index_funcs) {
    // Create metatable and store in registry: registry[mt_name] = {}
    luaL_newmetatable(L, mt_name);

    // Set metamethods (__gc, __tostring, __len, __newindex)
    if (meta_funcs) {
        luaL_register(L, nullptr, meta_funcs);
    }

    // Create __index table and populate with methods
    if (index_funcs) {
        lua_newtable(L);
        luaL_register(L, nullptr, index_funcs);
        lua_setfield(L, -2, "__index");
    }

    lua_pop(L, 1);  // pop metatable
}

// Reusable helper to allocate userdata with a given metatable
inline void* push_userdata(lua_State* L, size_t size, const char* mt_name) {
    void* ud = lua_newuserdata(L, size);
    luaL_getmetatable(L, mt_name);
    lua_setmetatable(L, -2);
    return ud;
}

// Reusable helper to push an NRP handle as userdata
inline void push_handle_userdata(lua_State* L, Handle h, const char* mt_name) {
    auto* lh = static_cast<LuaHandle*>(push_userdata(L, sizeof(LuaHandle), mt_name));
    lh->handle = h;
}

// Reusable helper to check and retrieve an NRP handle from userdata
inline Handle check_handle_userdata(lua_State* L, int idx, const char* mt_name) {
    auto* lh = static_cast<LuaHandle*>(luaL_checkudata(L, idx, mt_name));
    if (!lh || lh->handle == kInvalidHandle) {
        luaL_error(L, "%s: object is already closed or invalid", mt_name);
    }
    return lh->handle;
}

// Reusable __gc metamethod implementation template
inline int gc_metamethod(lua_State* L, const char* mt_name) {
    auto* lh = static_cast<LuaHandle*>(luaL_checkudata(L, 1, mt_name));
    if (lh && lh->handle != kInvalidHandle) {
        try {
            Runtime::get().objects().destroy(lh->handle);
        } catch (...) {
            // Destructors inside GC must never throw
        }
        lh->handle = kInvalidHandle;
    }
    return 0;
}

// Reusable explicit close() method implementation template
inline int close_method(lua_State* L, const char* mt_name) {
    auto* lh = static_cast<LuaHandle*>(luaL_checkudata(L, 1, mt_name));
    if (lh && lh->handle != kInvalidHandle) {
        try {
            Runtime::get().objects().destroy(lh->handle);
        } catch (const std::exception& e) {
            luaL_error(L, "%s:close failed: %s", mt_name, e.what());
            return 0;
        }
        lh->handle = kInvalidHandle;
    }
    return 0;
}

// Reusable __newindex metamethod to enforce read-only properties
inline int read_only_newindex(lua_State* L, const char* mt_name) {
    luaL_error(L, "%s: fields are read-only", mt_name);
    return 0;
}

// Reusable try-catch execution wrapper for lua_CFunction
template <typename Func>
int with_lua_exceptions(lua_State* L, const char* context_name, Func&& func) {
    try {
        return func();
    } catch (const std::bad_alloc&) {
        luaL_error(L, "%s: out of memory", context_name);
        return 0;
    } catch (const std::exception& e) {
        luaL_error(L, "%s: %s", context_name, e.what());
        return 0;
    } catch (...) {
        luaL_error(L, "%s: unknown native error", context_name);
        return 0;
    }
}

} // namespace nrp::luau
