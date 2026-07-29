// native/binding/luau/lexsoup_binding.h
// Phase 5: LexSoup Engine Luau Bindings

#pragma once

#include <lua.h>
#include <lualib.h>

namespace nrp::luau {

/**
 * Registers the lexsoup global module and its Document/Element/Elements metatables.
 */
int luaopen_lexsoup(lua_State* L);

} // namespace nrp::luau
