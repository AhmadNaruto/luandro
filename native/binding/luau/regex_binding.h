// native/binding/luau/regex_binding.h
// Phase 6: Regex Engine Luau Bindings

#pragma once

#include <lua.h>
#include <lualib.h>

namespace nrp::luau {

/**
 * Registers the regex global module and its Pattern/Matcher/MatchResult metatables.
 */
int luaopen_regex(lua_State* L);

} // namespace nrp::luau
