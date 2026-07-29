// native/regex/match_result.cpp
// Phase 6: Regex Engine MatchResult

#include "match_result.h"
#include "matcher.h"
#include <runtime.h>
#include <exceptions/exception_manager.h>

namespace nrp::regex {

MatchResult::~MatchResult() {
    close();
}

std::string MatchResult::groupValue(int index) {
    if (is_closed) throw NrpException("MatchResult is closed");
    if (index < 0 || index >= static_cast<int>(captured_groups.size())) {
        throw NrpException("IndexOutOfBoundsException: Group index out of range");
    }
    if (!captured_groups[index].first) {
        return ""; // Not participating
    }
    return captured_groups[index].second;
}

bool MatchResult::isGroupMatched(int index) {
    if (is_closed) throw NrpException("MatchResult is closed");
    if (index < 0 || index >= static_cast<int>(captured_groups.size())) {
        throw NrpException("IndexOutOfBoundsException: Group index out of range");
    }
    return captured_groups[index].first;
}

void MatchResult::close(Handle self_handle) {
    if (is_closed) return;
    is_closed = true;

    // Untrack from Matcher
    if (self_handle != kInvalidHandle && parent_handle != kInvalidHandle) {
        try {
            auto* matcher = Runtime::get().objects().get<Matcher>(parent_handle);
            if (matcher) {
                matcher->untrack_child(self_handle);
            }
        } catch (...) {}
    }
}

} // namespace nrp::regex
