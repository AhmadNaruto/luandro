// native/regex/regex.cpp
// Phase 6: Regex Engine Entry Point

#include "regex.h"
#include "pattern.h"
#include "matcher.h"
#include "match_result.h"
#include <runtime.h>
#include <exceptions/exception_manager.h>
#include <libregexp/libregexp.h>
#include <libregexp/cutils.h>

namespace nrp::regex {

Handle Regex::compile(const std::string& pattern, const std::string& flags) {
    char error_msg[128];
    int len = 0;
    int re_flags = 0;

    // Check for malformed unicode sequence (same as in jsregexp.c)
    if (pattern.find(static_cast<char>(0xfd)) != std::string::npos) {
        throw NrpException("PatternSyntaxException: malformed unicode");
    }

    // Check if there are non-BMP characters to set LRE_FLAG_UNICODE automatically
    bool has_non_bmp = false;
    for (size_t i = 0; i < pattern.length(); ++i) {
        if (static_cast<unsigned char>(pattern[i]) >= 0xf0) {
            has_non_bmp = true;
            break;
        }
    }
    if (has_non_bmp) {
        re_flags |= LRE_FLAG_UNICODE;
    }

    for (char c : flags) {
        switch (c) {
            case 'd': re_flags |= LRE_FLAG_INDICES; break;
            case 'i': re_flags |= LRE_FLAG_IGNORECASE; break;
            case 'g': re_flags |= LRE_FLAG_GLOBAL; break;
            case 'm': re_flags |= LRE_FLAG_MULTILINE; break;
            case 'n': re_flags |= LRE_FLAG_NAMED_GROUPS; break;
            case 's': re_flags |= LRE_FLAG_DOTALL; break;
            case 'u': re_flags |= LRE_FLAG_UNICODE; break;
            case 'v': re_flags |= LRE_FLAG_UNICODE_SETS; break;
            case 'y': re_flags |= LRE_FLAG_STICKY; break;
            default: break;
        }
    }

    uint8_t* bc = lre_compile(&len, error_msg, sizeof(error_msg),
                              pattern.c_str(), pattern.length(), re_flags, nullptr);

    if (!bc) {
        throw NrpException(std::string("PatternSyntaxException: ") + error_msg);
    }

    auto pat_obj = std::make_unique<Pattern>(pattern, flags, bc, len);
    Handle h = Runtime::get().handles().allocate(0x0201);
    Runtime::get().objects().insert<Pattern>(h, std::move(pat_obj));
    return h;
}

bool Regex::matches(const std::string& pattern, const std::string& input) {
    Handle pat_h = compile(pattern, "");
    auto* pat = Runtime::get().objects().get<Pattern>(pat_h);
    bool res = pat->matches(input);
    Runtime::get().objects().destroy(pat_h);
    return res;
}

Handle Regex::find(Handle self_handle, const std::string& pattern, const std::string& input) {
    Handle pat_h = compile(pattern, "");
    auto* pat = Runtime::get().objects().get<Pattern>(pat_h);
    Handle res = pat->find(self_handle, input);
    Runtime::get().objects().destroy(pat_h);
    return res;
}

std::vector<Handle> Regex::findAll(Handle self_handle, const std::string& pattern, const std::string& input) {
    Handle pat_h = compile(pattern, "");
    auto* pat = Runtime::get().objects().get<Pattern>(pat_h);
    std::vector<Handle> res = pat->findAll(self_handle, input);
    Runtime::get().objects().destroy(pat_h);
    return res;
}

std::string Regex::replace(const std::string& pattern, const std::string& input, const std::string& replacement) {
    Handle pat_h = compile(pattern, "");
    auto* pat = Runtime::get().objects().get<Pattern>(pat_h);
    std::string res = pat->replace(input, replacement);
    Runtime::get().objects().destroy(pat_h);
    return res;
}

std::string Regex::replaceAll(const std::string& pattern, const std::string& input, const std::string& replacement) {
    Handle pat_h = compile(pattern, "");
    auto* pat = Runtime::get().objects().get<Pattern>(pat_h);
    std::string res = pat->replaceAll(input, replacement);
    Runtime::get().objects().destroy(pat_h);
    return res;
}

std::vector<std::string> Regex::split(const std::string& pattern, const std::string& input) {
    Handle pat_h = compile(pattern, "");
    auto* pat = Runtime::get().objects().get<Pattern>(pat_h);
    std::vector<std::string> res = pat->split(input);
    Runtime::get().objects().destroy(pat_h);
    return res;
}

} // namespace nrp::regex
