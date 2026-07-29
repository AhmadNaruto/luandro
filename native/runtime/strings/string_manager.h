// native/runtime/strings/string_manager.h
// Phase 3: Runtime Core

#pragma once

#include <string>
#include <string_view>
#include <unordered_set>
#include <shared_mutex>
#include <jni.h>

namespace nrp {

class StringManager {
public:
    StringManager() = default;
    ~StringManager() { clear_intern_table(); }

    // JNI -> C++: returns UTF-8 std::string, throws on null.
    [[nodiscard]] std::string from_jstring(JNIEnv* env, jstring js) const;

    // C++ -> JNI: returns local jstring reference.
    [[nodiscard]] jstring to_jstring(JNIEnv* env, std::string_view sv) const;

    // Intern a string: returns a stable string_view into the intern table.
    // Interned strings live for the lifetime of the StringManager.
    [[nodiscard]] std::string_view intern(std::string_view sv);

    // Clear the intern table (called at runtime shutdown).
    void clear_intern_table() noexcept;

private:
    std::unordered_set<std::string> intern_table_;
    mutable std::shared_mutex       mutex_;
};

} // namespace nrp
