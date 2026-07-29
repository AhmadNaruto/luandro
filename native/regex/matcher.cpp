// native/regex/matcher.cpp
// Phase 6: Regex Engine Matcher

#include "matcher.h"
#include "pattern.h"
#include "match_result.h"
#include <runtime.h>
#include <exceptions/exception_manager.h>
extern "C" {
#include <libregexp.h>
}

namespace nrp::regex {

static int nrp_unicode_from_utf8(const uint8_t *p, int max_len, const uint8_t **pp) {
    uint8_t c = *p;
    if (c < 0x80) {
        *pp = p + 1;
        return c;
    }
    int len;
    uint32_t val;
    if ((c & 0xE0) == 0xC0) {
        len = 2;
        val = c & 0x1F;
    } else if ((c & 0xF0) == 0xE0) {
        len = 3;
        val = c & 0x0F;
    } else if ((c & 0xF8) == 0xF0) {
        len = 4;
        val = c & 0x07;
    } else {
        *pp = p + 1;
        return -1;
    }
    if (len > max_len) {
        *pp = p + 1;
        return -1;
    }
    for (int i = 1; i < len; i++) {
        uint8_t b = p[i];
        if ((b & 0xC0) != 0x80) {
            *pp = p + 1;
            return -1;
        }
        val = (val << 6) | (b & 0x3F);
    }
    *pp = p + len;
    return static_cast<int>(val);
}

JSString::JSString(const std::string& input) {
    bstr = input;
    uint32_t n = input.length();
    if (n == 0) {
        is_wide_char = false;
        len = 0;
        str8 = {0};
        return;
    }
    
    bool wide = false;
    for (char c : input) {
        if (static_cast<unsigned char>(c) >= 128) {
            wide = true;
            break;
        }
    }

    if (!wide) {
        is_wide_char = false;
        len = n;
        str8.assign(input.begin(), input.end());
        str8.push_back(0);
        return;
    } else {
        is_wide_char = true;
        indices.resize(n + 1, 0);
        rev_indices.resize(n + 1, 0);
        str16.resize(n + 1, 0);

        uint32_t q_idx = 0;
        const uint8_t* pos = reinterpret_cast<const uint8_t*>(input.c_str());
        const uint8_t* start = pos;
        
        while (*pos) {
            if (q_idx >= indices.size()) {
                indices.resize(q_idx * 2);
            }
            indices[q_idx] = pos - start;
            rev_indices[pos - start] = q_idx;

            int c = nrp_unicode_from_utf8(pos, 6, &pos);
            if (c == -1) {
                fallback_to_byte_string(input);
                return;
            }
            if (c > 0xffff) {
                c -= 0x10000;
                if (q_idx + 2 >= str16.size()) {
                    str16.resize(str16.size() * 2);
                }
                str16[q_idx++] = 0xd800 | (c >> 10);
                str16[q_idx++] = 0xdc00 | (c & 0x3ff);
            } else {
                if (q_idx + 1 >= str16.size()) {
                    str16.resize(str16.size() * 2);
                }
                str16[q_idx++] = c & 0xffff;
            }
        }
        str16.resize(q_idx + 1);
        str16[q_idx] = 0;
        indices.resize(q_idx + 1);
        indices[q_idx] = n;
        rev_indices.resize(n + 1);
        rev_indices[n] = q_idx;
        len = q_idx;
    }
}

void JSString::fallback_to_byte_string(const std::string& input) {
    is_wide_char = false;
    uint32_t n = input.length();
    len = n;
    str8.resize(n + 1);
    std::memcpy(str8.data(), input.data(), n);
    str8[n] = 0;
    
    indices.resize(n + 1);
    rev_indices.resize(n + 1);
    for (uint32_t i = 0; i <= n; ++i) {
        indices[i] = i;
        rev_indices[i] = i;
    }
}

// Helper to expand $0, $1... backreferences during replacements
static std::string format_replacement(const std::string& replacement, Matcher* matcher) {
    std::string result;
    size_t i = 0;
    while (i < replacement.length()) {
        if (replacement[i] == '$' && i + 1 < replacement.length()) {
            char next = replacement[i + 1];
            if (std::isdigit(next)) {
                int g_idx = next - '0';
                if (g_idx <= matcher->groupCount()) {
                    result += matcher->groupByIndex(g_idx);
                }
                i += 2;
            } else {
                result += '$';
                i++;
            }
        } else {
            result += replacement[i];
            i++;
        }
    }
    return result;
}

Matcher::Matcher(Handle pat_h, const std::string& inp)
    : pattern_handle(pat_h), js_input(inp) {
    auto* pat = Runtime::get().objects().get<Pattern>(pattern_handle);
    if (!pat) throw NrpException("Pattern is invalid or closed");
    
    capture_count = lre_get_capture_count(pat->bytecode());
}

Matcher::~Matcher() {
    close();
}

void Matcher::checkClosed() const {
    if (is_closed) throw NrpException("MatcherClosedException: Matcher is closed");
}

bool Matcher::matches() {
    checkClosed();
    reset();
    bool found = find();
    if (found) {
        int s = start();
        int e = end();
        if (s == 0 && e == static_cast<int>(js_input.bstr.length())) {
            return true;
        }
    }
    has_match = false;
    return false;
}

bool Matcher::find() {
    checkClosed();
    const auto* pat = Runtime::get().objects().get<Pattern>(pattern_handle);
    if (!pat) throw NrpException("Pattern is closed or invalid");

    if (last_index > js_input.len) {
        has_match = false;
        return false;
    }

    const uint8_t* input_ptr = js_input.is_wide_char ? 
        reinterpret_cast<const uint8_t*>(js_input.str16.data()) : 
        reinterpret_cast<const uint8_t*>(js_input.str8.data());

    int ret = lre_exec(capture, pat->bytecode(), input_ptr, last_index,
                       js_input.len, js_input.is_wide_char ? 1 : 0, nullptr);

    if (ret <= 0 || !capture[0] || !capture[1] || capture[0] < input_ptr || capture[1] < input_ptr) {
        has_match = false;
        return false;
    }

    has_match = true;
    
    uint32_t end_u16_or_u8;
    uint32_t start_u16_or_u8;
    if (js_input.is_wide_char) {
        end_u16_or_u8 = static_cast<uint32_t>((capture[1] - input_ptr) / 2);
        start_u16_or_u8 = static_cast<uint32_t>((capture[0] - input_ptr) / 2);
    } else {
        end_u16_or_u8 = static_cast<uint32_t>(capture[1] - input_ptr);
        start_u16_or_u8 = static_cast<uint32_t>(capture[0] - input_ptr);
    }

    if (end_u16_or_u8 == start_u16_or_u8) {
        last_index = end_u16_or_u8 + 1;
    } else {
        last_index = end_u16_or_u8;
    }

    return true;
}

bool Matcher::findFrom(int startIndex) {
    checkClosed();
    if (startIndex < 0 || startIndex > static_cast<int>(js_input.len)) {
        throw NrpException("IndexOutOfBoundsException: startIndex out of range");
    }
    reset();
    last_index = static_cast<uint32_t>(startIndex);
    return find();
}

bool Matcher::lookingAt() {
    checkClosed();
    reset();
    return find();
}

std::string Matcher::group() {
    return groupByIndex(0);
}

std::string Matcher::groupByIndex(int groupIndex) {
    checkClosed();
    if (!has_match) throw NrpException("IllegalStateException: No match found yet");
    if (groupIndex < 0 || groupIndex >= capture_count) {
        throw NrpException("IndexOutOfBoundsException: Group index out of range");
    }

    uint8_t* start_ptr = capture[2 * groupIndex];
    uint8_t* end_ptr = capture[2 * groupIndex + 1];

    if (!start_ptr || !end_ptr || end_ptr < start_ptr) {
        return ""; // Group did not participate in match
    }

    const uint8_t* input_ptr = js_input.is_wide_char ? 
        reinterpret_cast<const uint8_t*>(js_input.str16.data()) : 
        reinterpret_cast<const uint8_t*>(js_input.str8.data());

    if (start_ptr < input_ptr || end_ptr < input_ptr) {
        return "";
    }

    size_t a, b;
    if (js_input.is_wide_char) {
        size_t idx_a = (start_ptr - input_ptr) / 2;
        size_t idx_b = (end_ptr - input_ptr) / 2;
        if (idx_a >= js_input.indices.size() || idx_b >= js_input.indices.size()) return "";
        a = js_input.indices[idx_a];
        b = js_input.indices[idx_b];
    } else {
        a = start_ptr - input_ptr;
        b = end_ptr - input_ptr;
    }

    if (a > js_input.bstr.length() || b > js_input.bstr.length() || b < a) {
        return "";
    }

    return js_input.bstr.substr(a, b - a);
}

bool Matcher::isGroupMatched(int groupIndex) {
    checkClosed();
    if (!has_match) throw NrpException("IllegalStateException: No match found yet");
    if (groupIndex < 0 || groupIndex >= capture_count) {
        throw NrpException("IndexOutOfBoundsException: Group index out of range");
    }
    return capture[2 * groupIndex] != nullptr && capture[2 * groupIndex + 1] != nullptr;
}

int Matcher::groupCount() const {
    checkClosed();
    return capture_count - 1; // capture_count includes group 0 (full match)
}

int Matcher::start() {
    checkClosed();
    if (!has_match || !capture[0]) return 0;
    const uint8_t* input_ptr = js_input.is_wide_char ? 
        reinterpret_cast<const uint8_t*>(js_input.str16.data()) : 
        reinterpret_cast<const uint8_t*>(js_input.str8.data());
    
    if (capture[0] < input_ptr) return 0;

    if (js_input.is_wide_char) {
        size_t u16_idx = (capture[0] - input_ptr) / 2;
        if (u16_idx >= js_input.indices.size()) return static_cast<int>(js_input.bstr.length());
        return static_cast<int>(js_input.indices[u16_idx]);
    } else {
        size_t offset = capture[0] - input_ptr;
        if (offset > js_input.bstr.length()) return static_cast<int>(js_input.bstr.length());
        return static_cast<int>(offset);
    }
}

int Matcher::end() {
    checkClosed();
    if (!has_match || !capture[1]) return 0;
    const uint8_t* input_ptr = js_input.is_wide_char ? 
        reinterpret_cast<const uint8_t*>(js_input.str16.data()) : 
        reinterpret_cast<const uint8_t*>(js_input.str8.data());
    
    if (capture[1] < input_ptr) return 0;

    if (js_input.is_wide_char) {
        size_t u16_idx = (capture[1] - input_ptr) / 2;
        if (u16_idx >= js_input.indices.size()) return static_cast<int>(js_input.bstr.length());
        return static_cast<int>(js_input.indices[u16_idx]);
    } else {
        size_t offset = capture[1] - input_ptr;
        if (offset > js_input.bstr.length()) return static_cast<int>(js_input.bstr.length());
        return static_cast<int>(offset);
    }
}

void Matcher::reset() {
    checkClosed();
    has_match = false;
    last_index = 0;
    std::memset(capture, 0, sizeof(capture));
}

void Matcher::resetWithInput(const std::string& newInput) {
    checkClosed();
    js_input = JSString(newInput);
    reset();
}

std::string Matcher::replaceAll(const std::string& replacement) {
    checkClosed();
    reset();
    std::string result;
    int last_copied = 0;
    while (find()) {
        int s = start();
        result.append(js_input.bstr.data() + last_copied, s - last_copied);
        result.append(format_replacement(replacement, this));
        last_copied = end();
    }
    result.append(js_input.bstr.data() + last_copied, js_input.bstr.length() - last_copied);
    return result;
}

std::string Matcher::replaceFirst(const std::string& replacement) {
    checkClosed();
    reset();
    std::string result;
    if (find()) {
        int s = start();
        result.append(js_input.bstr.data(), s);
        result.append(format_replacement(replacement, this));
        result.append(js_input.bstr.data() + end(), js_input.bstr.length() - end());
        return result;
    }
    return js_input.bstr;
}

Handle Matcher::toMatchResult(Handle self_handle) {
    checkClosed();
    if (!has_match) throw NrpException("IllegalStateException: No match found yet");

    std::vector<std::pair<bool, std::string>> groups;
    for (int i = 0; i < capture_count; ++i) {
        if (capture[2 * i] == nullptr || capture[2 * i + 1] == nullptr) {
            groups.push_back({false, ""});
        } else {
            groups.push_back({true, groupByIndex(i)});
        }
    }

    auto result = std::make_unique<MatchResult>(group(), start(), end(), groups, self_handle);
    Handle h = Runtime::get().handles().allocate(0x0203);
    Runtime::get().objects().insert<MatchResult>(h, std::move(result));
    track_child(h);
    return h;
}

void Matcher::close(Handle self_handle) {
    if (is_closed) return;
    is_closed = true;

    // Untrack from Pattern
    if (self_handle != kInvalidHandle && pattern_handle != kInvalidHandle) {
        try {
            auto* pat = Runtime::get().objects().get<Pattern>(pattern_handle);
            if (pat) {
                pat->untrack_child(self_handle);
            }
        } catch (...) {}
    }

    // Destroy children MatchResults
    auto children = child_handles;
    for (Handle h : children) {
        try {
            Runtime::get().objects().destroy(h);
        } catch (...) {}
    }
    child_handles.clear();
}

void Matcher::track_child(Handle h) {
    child_handles.insert(h);
}

void Matcher::untrack_child(Handle h) {
    child_handles.erase(h);
}

} // namespace nrp::regex
