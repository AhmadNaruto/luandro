// native/regex/matcher.h
// Phase 6: Regex Engine Matcher

#pragma once

#include <handle_manager/handle.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <cstdint>

namespace nrp::regex {

struct JSString {
    bool is_wide_char = false;
    uint32_t len = 0;
    std::string bstr;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> rev_indices;
    std::vector<uint16_t> str16;
    std::vector<uint8_t> str8;

    JSString() = default;
    explicit JSString(const std::string& input);
    void fallback_to_byte_string(const std::string& input);
};

class Matcher {
public:
    static constexpr uint16_t type_tag = 0x0202;

    Matcher(Handle pat_h, const std::string& inp);
    ~Matcher();

    [[nodiscard]] Handle pattern() const noexcept { return pattern_handle; }
    [[nodiscard]] const std::string& input() const noexcept { return js_input.bstr; }
    [[nodiscard]] bool hasMatch() const noexcept { return has_match; }

    bool matches();
    bool find();
    bool findFrom(int startIndex);
    bool lookingAt();

    std::string group();
    std::string groupByIndex(int groupIndex);
    bool isGroupMatched(int groupIndex);
    int groupCount() const;
    int start();
    int end();

    void reset();
    void resetWithInput(const std::string& newInput);

    std::string replaceAll(const std::string& replacement);
    std::string replaceFirst(const std::string& replacement);

    Handle toMatchResult(Handle self_handle);
    void close(Handle self_handle = kInvalidHandle);

    void track_child(Handle h);
    void untrack_child(Handle h);

private:
    void checkClosed() const;

    Handle pattern_handle;
    JSString js_input;
    bool has_match = false;
    bool is_closed = false;
    uint32_t last_index = 0;
    
    // Capture pointers from the last match
    uint8_t* capture[256 * 2] = {nullptr};
    int capture_count = 0;
    std::unordered_set<Handle> child_handles;
};

} // namespace nrp::regex
