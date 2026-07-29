// native/regex/match_result.h
// Phase 6: Regex Engine MatchResult

#pragma once

#include <handle_manager/handle.h>
#include <string>
#include <vector>

namespace nrp::regex {

class MatchResult {
public:
    static constexpr uint16_t type_tag = 0x0203;

    MatchResult(const std::string& val, int start, int end, 
                const std::vector<std::pair<bool, std::string>>& groups, Handle parent)
        : matched_value(val), start_idx(start), end_idx(end), captured_groups(groups), parent_handle(parent) {}

    ~MatchResult();

    [[nodiscard]] const std::string& value() const noexcept { return matched_value; }
    [[nodiscard]] int start() const noexcept { return start_idx; }
    [[nodiscard]] int end() const noexcept { return end_idx; }
    [[nodiscard]] int groupCount() const noexcept { return static_cast<int>(captured_groups.size()); }
    [[nodiscard]] Handle parent() const noexcept { return parent_handle; }

    std::string groupValue(int index);
    bool isGroupMatched(int index);
    void close(Handle self_handle = kInvalidHandle);

private:
    std::string matched_value;
    int start_idx = 0;
    int end_idx = 0;
    // pair.first = participating, pair.second = string value
    std::vector<std::pair<bool, std::string>> captured_groups;
    Handle parent_handle;
    bool is_closed = false;
};

} // namespace nrp::regex
