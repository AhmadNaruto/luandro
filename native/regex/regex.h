// native/regex/regex.h
// Phase 6: Regex Engine Entry Point

#pragma once

#include <handle_manager/handle.h>
#include <string>
#include <vector>

namespace nrp::regex {

class Regex {
public:
    static Handle compile(const std::string& pattern, const std::string& flags);
    static bool matches(const std::string& pattern, const std::string& input);
    static Handle find(Handle self_handle, const std::string& pattern, const std::string& input);
    static std::vector<Handle> findAll(Handle self_handle, const std::string& pattern, const std::string& input);
    static std::string replace(const std::string& pattern, const std::string& input, const std::string& replacement);
    static std::string replaceAll(const std::string& pattern, const std::string& input, const std::string& replacement);
    static std::vector<std::string> split(const std::string& pattern, const std::string& input);
};

} // namespace nrp::regex
