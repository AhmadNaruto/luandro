// native/regex/pattern.h
// Phase 6: Regex Engine Pattern

#pragma once

#include <handle_manager/handle.h>
#include <string>
#include <vector>
#include <unordered_set>

namespace nrp::regex {

class Pattern {
public:
    static constexpr uint16_t type_tag = 0x0201;

    Pattern(const std::string& pat, const std::string& fl, uint8_t* bc_buf, int len)
        : pattern_str(pat), flags_str(fl), bc(bc_buf), bc_len(len) {}
    
    ~Pattern();

    [[nodiscard]] const std::string& pattern() const noexcept { return pattern_str; }
    [[nodiscard]] const std::string& flags() const noexcept { return flags_str; }
    [[nodiscard]] const uint8_t* bytecode() const noexcept { return bc; }
    [[nodiscard]] int bytecode_len() const noexcept { return bc_len; }

    Handle matcher(Handle self_handle, const std::string& input);
    bool matches(const std::string& input);
    Handle find(Handle self_handle, const std::string& input);
    std::vector<Handle> findAll(Handle self_handle, const std::string& input);
    std::string replace(const std::string& input, const std::string& replacement);
    std::string replaceAll(const std::string& input, const std::string& replacement);
    std::vector<std::string> split(const std::string& input);

    void close();

    void track_child(Handle h);
    void untrack_child(Handle h);

private:
    std::string pattern_str;
    std::string flags_str;
    uint8_t* bc = nullptr;
    int bc_len = 0;
    bool is_closed = false;
    std::unordered_set<Handle> child_handles;
};

} // namespace nrp::regex
