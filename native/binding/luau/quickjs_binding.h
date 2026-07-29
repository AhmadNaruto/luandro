// native/binding/luau/quickjs_binding.h
// Phase 7: QuickJS Engine Luau Bindings

#pragma once

#include <lua.h>
#include <lualib.h>

namespace nrp::luau {

/**
 * Registers the 'js' global module and Runtime/Context/JSValue metatables.
 */
int luaopen_js(lua_State* L);

} // namespace nrp::luau
