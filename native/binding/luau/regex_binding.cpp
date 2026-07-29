// native/binding/luau/regex_binding.cpp
// Phase 6: Regex Engine Luau Bindings

#include "regex_binding.h"
#include "luau_binding.h"
#include <regex/regex.h>
#include <regex/pattern.h>
#include <regex/matcher.h>
#include <regex/match_result.h>
#include <runtime.h>

namespace nrp::luau {

// ==========================================
// MatchResult Luau Bindings
// ==========================================

static int mr_value(lua_State* L) {
    return with_lua_exceptions(L, "MatchResult:value", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.MatchResult");
        auto* mr = Runtime::get().objects().get<nrp::regex::MatchResult>(h);
        if (!mr) { luaL_error(L, "MatchResult is closed or invalid"); return 0; }
        const std::string& v = mr->value();
        lua_pushlstring(L, v.data(), v.size());
        return 1;
    });
}

static int mr_start(lua_State* L) {
    return with_lua_exceptions(L, "MatchResult:start", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.MatchResult");
        auto* mr = Runtime::get().objects().get<nrp::regex::MatchResult>(h);
        if (!mr) { luaL_error(L, "MatchResult is closed or invalid"); return 0; }
        lua_pushinteger(L, mr->start() + 1); // 1-indexed for Lua
        return 1;
    });
}

static int mr_finish(lua_State* L) {
    return with_lua_exceptions(L, "MatchResult:finish", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.MatchResult");
        auto* mr = Runtime::get().objects().get<nrp::regex::MatchResult>(h);
        if (!mr) { luaL_error(L, "MatchResult is closed or invalid"); return 0; }
        lua_pushinteger(L, mr->end()); // end is exclusive, same as Lua string end
        return 1;
    });
}

static int mr_groupCount(lua_State* L) {
    return with_lua_exceptions(L, "MatchResult:groupCount", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.MatchResult");
        auto* mr = Runtime::get().objects().get<nrp::regex::MatchResult>(h);
        if (!mr) { luaL_error(L, "MatchResult is closed or invalid"); return 0; }
        lua_pushinteger(L, mr->groupCount());
        return 1;
    });
}

static int mr_group(lua_State* L) {
    return with_lua_exceptions(L, "MatchResult:group", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.MatchResult");
        auto* mr = Runtime::get().objects().get<nrp::regex::MatchResult>(h);
        if (!mr) { luaL_error(L, "MatchResult is closed or invalid"); return 0; }
        int idx = static_cast<int>(luaL_checkinteger(L, 2)) - 1; // 1-indexed Lua → 0-indexed C++
        std::string val = mr->groupValue(idx);
        lua_pushlstring(L, val.data(), val.size());
        return 1;
    });
}

static int mr_close(lua_State* L) {
    return close_method(L, "regex.MatchResult");
}

static int mr_gc(lua_State* L) {
    return gc_metamethod(L, "regex.MatchResult");
}

static int mr_tostring(lua_State* L) {
    return with_lua_exceptions(L, "MatchResult:__tostring", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.MatchResult");
        auto* mr = Runtime::get().objects().get<nrp::regex::MatchResult>(h);
        if (!mr) {
            lua_pushstring(L, "MatchResult(closed)");
        } else {
            const std::string& v = mr->value();
            lua_pushfstring(L, "MatchResult(%s)", v.c_str());
        }
        return 1;
    });
}

static int mr_newindex(lua_State* L) {
    return read_only_newindex(L, "regex.MatchResult");
}

static const luaL_Reg mr_meta[] = {
    {"__gc",       mr_gc},
    {"__tostring", mr_tostring},
    {"__newindex", mr_newindex},
    {nullptr,      nullptr}
};

static const luaL_Reg mr_index[] = {
    {"value",      mr_value},
    {"start",      mr_start},
    {"finish",     mr_finish},
    {"groupCount", mr_groupCount},
    {"group",      mr_group},
    {"close",      mr_close},
    {nullptr,      nullptr}
};

// ==========================================
// Matcher Luau Bindings
// ==========================================

static void push_match_result(lua_State* L, Handle mrh) {
    if (mrh != kInvalidHandle) {
        push_handle_userdata(L, mrh, "regex.MatchResult");
    } else {
        lua_pushnil(L);
    }
}

static int mat_matches(lua_State* L) {
    return with_lua_exceptions(L, "Matcher:matches", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Matcher");
        auto* m = Runtime::get().objects().get<nrp::regex::Matcher>(h);
        if (!m) { luaL_error(L, "Matcher is closed or invalid"); return 0; }
        lua_pushboolean(L, m->matches() ? 1 : 0);
        return 1;
    });
}

static int mat_find(lua_State* L) {
    return with_lua_exceptions(L, "Matcher:find", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Matcher");
        auto* m = Runtime::get().objects().get<nrp::regex::Matcher>(h);
        if (!m) { luaL_error(L, "Matcher is closed or invalid"); return 0; }
        lua_pushboolean(L, m->find() ? 1 : 0);
        return 1;
    });
}

static int mat_findFrom(lua_State* L) {
    return with_lua_exceptions(L, "Matcher:findFrom", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Matcher");
        auto* m = Runtime::get().objects().get<nrp::regex::Matcher>(h);
        if (!m) { luaL_error(L, "Matcher is closed or invalid"); return 0; }
        int start = static_cast<int>(luaL_checkinteger(L, 2)) - 1; // Lua 1-indexed
        lua_pushboolean(L, m->findFrom(start) ? 1 : 0);
        return 1;
    });
}

static int mat_lookingAt(lua_State* L) {
    return with_lua_exceptions(L, "Matcher:lookingAt", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Matcher");
        auto* m = Runtime::get().objects().get<nrp::regex::Matcher>(h);
        if (!m) { luaL_error(L, "Matcher is closed or invalid"); return 0; }
        lua_pushboolean(L, m->lookingAt() ? 1 : 0);
        return 1;
    });
}

static int mat_group(lua_State* L) {
    return with_lua_exceptions(L, "Matcher:group", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Matcher");
        auto* m = Runtime::get().objects().get<nrp::regex::Matcher>(h);
        if (!m) { luaL_error(L, "Matcher is closed or invalid"); return 0; }
        std::string g = m->group();
        lua_pushlstring(L, g.data(), g.size());
        return 1;
    });
}

static int mat_groupByIndex(lua_State* L) {
    return with_lua_exceptions(L, "Matcher:groupByIndex", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Matcher");
        auto* m = Runtime::get().objects().get<nrp::regex::Matcher>(h);
        if (!m) { luaL_error(L, "Matcher is closed or invalid"); return 0; }
        int idx = static_cast<int>(luaL_checkinteger(L, 2));
        std::string g = m->groupByIndex(idx);
        lua_pushlstring(L, g.data(), g.size());
        return 1;
    });
}

static int mat_groupCount(lua_State* L) {
    return with_lua_exceptions(L, "Matcher:groupCount", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Matcher");
        auto* m = Runtime::get().objects().get<nrp::regex::Matcher>(h);
        if (!m) { luaL_error(L, "Matcher is closed or invalid"); return 0; }
        lua_pushinteger(L, m->groupCount());
        return 1;
    });
}

static int mat_start(lua_State* L) {
    return with_lua_exceptions(L, "Matcher:start", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Matcher");
        auto* m = Runtime::get().objects().get<nrp::regex::Matcher>(h);
        if (!m) { luaL_error(L, "Matcher is closed or invalid"); return 0; }
        lua_pushinteger(L, m->start() + 1); // 1-indexed
        return 1;
    });
}

static int mat_end(lua_State* L) {
    return with_lua_exceptions(L, "Matcher:finish", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Matcher");
        auto* m = Runtime::get().objects().get<nrp::regex::Matcher>(h);
        if (!m) { luaL_error(L, "Matcher is closed or invalid"); return 0; }
        lua_pushinteger(L, m->end());
        return 1;
    });
}

static int mat_reset(lua_State* L) {
    return with_lua_exceptions(L, "Matcher:reset", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Matcher");
        auto* m = Runtime::get().objects().get<nrp::regex::Matcher>(h);
        if (!m) { luaL_error(L, "Matcher is closed or invalid"); return 0; }
        if (lua_gettop(L) >= 2 && lua_isstring(L, 2)) {
            size_t len = 0;
            const char* input = luaL_checklstring(L, 2, &len);
            m->resetWithInput(std::string(input, len));
        } else {
            m->reset();
        }
        lua_pushvalue(L, 1); // return self for chaining
        return 1;
    });
}

static int mat_replaceAll(lua_State* L) {
    return with_lua_exceptions(L, "Matcher:replaceAll", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Matcher");
        auto* m = Runtime::get().objects().get<nrp::regex::Matcher>(h);
        if (!m) { luaL_error(L, "Matcher is closed or invalid"); return 0; }
        size_t len = 0;
        const char* rep = luaL_checklstring(L, 2, &len);
        std::string result = m->replaceAll(std::string(rep, len));
        lua_pushlstring(L, result.data(), result.size());
        return 1;
    });
}

static int mat_replaceFirst(lua_State* L) {
    return with_lua_exceptions(L, "Matcher:replaceFirst", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Matcher");
        auto* m = Runtime::get().objects().get<nrp::regex::Matcher>(h);
        if (!m) { luaL_error(L, "Matcher is closed or invalid"); return 0; }
        size_t len = 0;
        const char* rep = luaL_checklstring(L, 2, &len);
        std::string result = m->replaceFirst(std::string(rep, len));
        lua_pushlstring(L, result.data(), result.size());
        return 1;
    });
}

static int mat_toMatchResult(lua_State* L) {
    return with_lua_exceptions(L, "Matcher:toMatchResult", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Matcher");
        auto* m = Runtime::get().objects().get<nrp::regex::Matcher>(h);
        if (!m) { luaL_error(L, "Matcher is closed or invalid"); return 0; }
        Handle mrh = m->toMatchResult(h);
        push_match_result(L, mrh);
        return 1;
    });
}

static int mat_close(lua_State* L) {
    return close_method(L, "regex.Matcher");
}

static int mat_gc(lua_State* L) {
    return gc_metamethod(L, "regex.Matcher");
}

static int mat_tostring(lua_State* L) {
    return with_lua_exceptions(L, "Matcher:__tostring", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Matcher");
        auto* m = Runtime::get().objects().get<nrp::regex::Matcher>(h);
        if (!m) {
            lua_pushstring(L, "Matcher(closed)");
        } else {
            lua_pushfstring(L, "Matcher(hasMatch=%s)", m->hasMatch() ? "true" : "false");
        }
        return 1;
    });
}

static int mat_newindex(lua_State* L) {
    return read_only_newindex(L, "regex.Matcher");
}

static const luaL_Reg mat_meta[] = {
    {"__gc",       mat_gc},
    {"__tostring", mat_tostring},
    {"__newindex", mat_newindex},
    {nullptr,      nullptr}
};

static const luaL_Reg mat_index[] = {
    {"matches",       mat_matches},
    {"find",          mat_find},
    {"findFrom",      mat_findFrom},
    {"lookingAt",     mat_lookingAt},
    {"group",         mat_group},
    {"groupByIndex",  mat_groupByIndex},
    {"groupCount",    mat_groupCount},
    {"start",         mat_start},
    {"finish",        mat_end},
    {"reset",         mat_reset},
    {"replaceAll",    mat_replaceAll},
    {"replaceFirst",  mat_replaceFirst},
    {"toMatchResult", mat_toMatchResult},
    {"close",         mat_close},
    {nullptr,         nullptr}
};

// ==========================================
// Pattern Luau Bindings
// ==========================================

static int pat_pattern(lua_State* L) {
    return with_lua_exceptions(L, "Pattern:pattern", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Pattern");
        auto* pat = Runtime::get().objects().get<nrp::regex::Pattern>(h);
        if (!pat) { luaL_error(L, "Pattern is closed or invalid"); return 0; }
        const std::string& p = pat->pattern();
        lua_pushlstring(L, p.data(), p.size());
        return 1;
    });
}

static int pat_flags(lua_State* L) {
    return with_lua_exceptions(L, "Pattern:flags", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Pattern");
        auto* pat = Runtime::get().objects().get<nrp::regex::Pattern>(h);
        if (!pat) { luaL_error(L, "Pattern is closed or invalid"); return 0; }
        const std::string& f = pat->flags();
        lua_pushlstring(L, f.data(), f.size());
        return 1;
    });
}

static int pat_matcher(lua_State* L) {
    return with_lua_exceptions(L, "Pattern:matcher", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Pattern");
        auto* pat = Runtime::get().objects().get<nrp::regex::Pattern>(h);
        if (!pat) { luaL_error(L, "Pattern is closed or invalid"); return 0; }
        size_t len = 0;
        const char* input = luaL_checklstring(L, 2, &len);
        Handle mh = pat->matcher(h, std::string(input, len));
        push_handle_userdata(L, mh, "regex.Matcher");
        return 1;
    });
}

static int pat_matches(lua_State* L) {
    return with_lua_exceptions(L, "Pattern:matches", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Pattern");
        auto* pat = Runtime::get().objects().get<nrp::regex::Pattern>(h);
        if (!pat) { luaL_error(L, "Pattern is closed or invalid"); return 0; }
        size_t len = 0;
        const char* input = luaL_checklstring(L, 2, &len);
        lua_pushboolean(L, pat->matches(std::string(input, len)) ? 1 : 0);
        return 1;
    });
}

static int pat_find(lua_State* L) {
    return with_lua_exceptions(L, "Pattern:find", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Pattern");
        auto* pat = Runtime::get().objects().get<nrp::regex::Pattern>(h);
        if (!pat) { luaL_error(L, "Pattern is closed or invalid"); return 0; }
        size_t len = 0;
        const char* input = luaL_checklstring(L, 2, &len);
        Handle mrh = pat->find(h, std::string(input, len));
        push_match_result(L, mrh);
        return 1;
    });
}

static int pat_findAll(lua_State* L) {
    return with_lua_exceptions(L, "Pattern:findAll", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Pattern");
        auto* pat = Runtime::get().objects().get<nrp::regex::Pattern>(h);
        if (!pat) { luaL_error(L, "Pattern is closed or invalid"); return 0; }
        size_t len = 0;
        const char* input = luaL_checklstring(L, 2, &len);
        std::vector<Handle> handles = pat->findAll(h, std::string(input, len));
        // Return as Lua table of MatchResult userdata
        lua_newtable(L);
        for (size_t i = 0; i < handles.size(); ++i) {
            push_handle_userdata(L, handles[i], "regex.MatchResult");
            lua_rawseti(L, -2, static_cast<int>(i + 1));
        }
        return 1;
    });
}

static int pat_replace(lua_State* L) {
    return with_lua_exceptions(L, "Pattern:replace", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Pattern");
        auto* pat = Runtime::get().objects().get<nrp::regex::Pattern>(h);
        if (!pat) { luaL_error(L, "Pattern is closed or invalid"); return 0; }
        size_t inp_len = 0, rep_len = 0;
        const char* input = luaL_checklstring(L, 2, &inp_len);
        const char* rep   = luaL_checklstring(L, 3, &rep_len);
        std::string result = pat->replace(std::string(input, inp_len), std::string(rep, rep_len));
        lua_pushlstring(L, result.data(), result.size());
        return 1;
    });
}

static int pat_replaceAll(lua_State* L) {
    return with_lua_exceptions(L, "Pattern:replaceAll", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Pattern");
        auto* pat = Runtime::get().objects().get<nrp::regex::Pattern>(h);
        if (!pat) { luaL_error(L, "Pattern is closed or invalid"); return 0; }
        size_t inp_len = 0, rep_len = 0;
        const char* input = luaL_checklstring(L, 2, &inp_len);
        const char* rep   = luaL_checklstring(L, 3, &rep_len);
        std::string result = pat->replaceAll(std::string(input, inp_len), std::string(rep, rep_len));
        lua_pushlstring(L, result.data(), result.size());
        return 1;
    });
}

static int pat_split(lua_State* L) {
    return with_lua_exceptions(L, "Pattern:split", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Pattern");
        auto* pat = Runtime::get().objects().get<nrp::regex::Pattern>(h);
        if (!pat) { luaL_error(L, "Pattern is closed or invalid"); return 0; }
        size_t len = 0;
        const char* input = luaL_checklstring(L, 2, &len);
        std::vector<std::string> parts = pat->split(std::string(input, len));
        lua_newtable(L);
        for (size_t i = 0; i < parts.size(); ++i) {
            lua_pushlstring(L, parts[i].data(), parts[i].size());
            lua_rawseti(L, -2, static_cast<int>(i + 1));
        }
        return 1;
    });
}

static int pat_close(lua_State* L) {
    return close_method(L, "regex.Pattern");
}

static int pat_gc(lua_State* L) {
    return gc_metamethod(L, "regex.Pattern");
}

static int pat_tostring(lua_State* L) {
    return with_lua_exceptions(L, "Pattern:__tostring", [&]() {
        Handle h = check_handle_userdata(L, 1, "regex.Pattern");
        auto* pat = Runtime::get().objects().get<nrp::regex::Pattern>(h);
        if (!pat) {
            lua_pushstring(L, "Pattern(closed)");
        } else {
            lua_pushfstring(L, "Pattern(%s)", pat->pattern().c_str());
        }
        return 1;
    });
}

static int pat_newindex(lua_State* L) {
    return read_only_newindex(L, "regex.Pattern");
}

static const luaL_Reg pat_meta[] = {
    {"__gc",       pat_gc},
    {"__tostring", pat_tostring},
    {"__newindex", pat_newindex},
    {nullptr,      nullptr}
};

static const luaL_Reg pat_index[] = {
    {"pattern",    pat_pattern},
    {"flags",      pat_flags},
    {"matcher",    pat_matcher},
    {"matches",    pat_matches},
    {"find",       pat_find},
    {"findAll",    pat_findAll},
    {"replace",    pat_replace},
    {"replaceAll", pat_replaceAll},
    {"split",      pat_split},
    {"close",      pat_close},
    {nullptr,      nullptr}
};

// ==========================================
// Regex Global Module Functions
// ==========================================

static int regex_compile(lua_State* L) {
    return with_lua_exceptions(L, "regex.compile", [&]() {
        size_t pat_len = 0;
        const char* pattern = luaL_checklstring(L, 1, &pat_len);
        std::string flags;
        if (lua_gettop(L) >= 2 && lua_isstring(L, 2)) {
            size_t fl_len = 0;
            const char* fl = luaL_checklstring(L, 2, &fl_len);
            flags = std::string(fl, fl_len);
        }
        Handle h = nrp::regex::Regex::compile(std::string(pattern, pat_len), flags);
        push_handle_userdata(L, h, "regex.Pattern");
        return 1;
    });
}

static int regex_matches(lua_State* L) {
    return with_lua_exceptions(L, "regex.matches", [&]() {
        size_t pat_len = 0, inp_len = 0;
        const char* pattern = luaL_checklstring(L, 1, &pat_len);
        const char* input   = luaL_checklstring(L, 2, &inp_len);
        bool result = nrp::regex::Regex::matches(std::string(pattern, pat_len), std::string(input, inp_len));
        lua_pushboolean(L, result ? 1 : 0);
        return 1;
    });
}

static int regex_find(lua_State* L) {
    return with_lua_exceptions(L, "regex.find", [&]() {
        size_t pat_len = 0, inp_len = 0;
        const char* pattern = luaL_checklstring(L, 1, &pat_len);
        const char* input   = luaL_checklstring(L, 2, &inp_len);
        Handle ph = nrp::regex::Regex::compile(std::string(pattern, pat_len), "");
        Handle mrh = nrp::regex::Regex::find(ph, std::string(pattern, pat_len), std::string(input, inp_len));
        Runtime::get().objects().destroy(ph);
        push_match_result(L, mrh);
        return 1;
    });
}

static int regex_findAll(lua_State* L) {
    return with_lua_exceptions(L, "regex.findAll", [&]() {
        size_t pat_len = 0, inp_len = 0;
        const char* pattern = luaL_checklstring(L, 1, &pat_len);
        const char* input   = luaL_checklstring(L, 2, &inp_len);
        Handle ph = nrp::regex::Regex::compile(std::string(pattern, pat_len), "");
        std::vector<Handle> handles = nrp::regex::Regex::findAll(ph, std::string(pattern, pat_len), std::string(input, inp_len));
        Runtime::get().objects().destroy(ph);
        lua_newtable(L);
        for (size_t i = 0; i < handles.size(); ++i) {
            push_handle_userdata(L, handles[i], "regex.MatchResult");
            lua_rawseti(L, -2, static_cast<int>(i + 1));
        }
        return 1;
    });
}

static int regex_replace(lua_State* L) {
    return with_lua_exceptions(L, "regex.replace", [&]() {
        size_t pat_len = 0, inp_len = 0, rep_len = 0;
        const char* pattern = luaL_checklstring(L, 1, &pat_len);
        const char* input   = luaL_checklstring(L, 2, &inp_len);
        const char* rep     = luaL_checklstring(L, 3, &rep_len);
        std::string result = nrp::regex::Regex::replace(
            std::string(pattern, pat_len), std::string(input, inp_len), std::string(rep, rep_len));
        lua_pushlstring(L, result.data(), result.size());
        return 1;
    });
}

static int regex_replaceAll(lua_State* L) {
    return with_lua_exceptions(L, "regex.replaceAll", [&]() {
        size_t pat_len = 0, inp_len = 0, rep_len = 0;
        const char* pattern = luaL_checklstring(L, 1, &pat_len);
        const char* input   = luaL_checklstring(L, 2, &inp_len);
        const char* rep     = luaL_checklstring(L, 3, &rep_len);
        std::string result = nrp::regex::Regex::replaceAll(
            std::string(pattern, pat_len), std::string(input, inp_len), std::string(rep, rep_len));
        lua_pushlstring(L, result.data(), result.size());
        return 1;
    });
}

static int regex_split(lua_State* L) {
    return with_lua_exceptions(L, "regex.split", [&]() {
        size_t pat_len = 0, inp_len = 0;
        const char* pattern = luaL_checklstring(L, 1, &pat_len);
        const char* input   = luaL_checklstring(L, 2, &inp_len);
        std::vector<std::string> parts = nrp::regex::Regex::split(
            std::string(pattern, pat_len), std::string(input, inp_len));
        lua_newtable(L);
        for (size_t i = 0; i < parts.size(); ++i) {
            lua_pushlstring(L, parts[i].data(), parts[i].size());
            lua_rawseti(L, -2, static_cast<int>(i + 1));
        }
        return 1;
    });
}

static const luaL_Reg regex_module[] = {
    {"compile",    regex_compile},
    {"matches",    regex_matches},
    {"find",       regex_find},
    {"findAll",    regex_findAll},
    {"replace",    regex_replace},
    {"replaceAll", regex_replaceAll},
    {"split",      regex_split},
    {nullptr,      nullptr}
};

// ==========================================
// luaopen_regex — module entry point
// ==========================================

int luaopen_regex(lua_State* L) {
    // Register MatchResult metatable
    register_metatable(L, "regex.MatchResult", mr_meta, mr_index);

    // Register Matcher metatable
    register_metatable(L, "regex.Matcher", mat_meta, mat_index);

    // Register Pattern metatable
    register_metatable(L, "regex.Pattern", pat_meta, pat_index);

    // Create global 'regex' table
    luaL_register(L, "regex", regex_module);

    return 1;
}

} // namespace nrp::luau
