// native/runtime/strings/string_manager.cpp
// Phase 3: Runtime Core

#include "string_manager.h"
#include "../exceptions/exception_manager.h"

namespace nrp {

std::string StringManager::from_jstring(JNIEnv* env, jstring js) const {
    if (!js) {
        throw NrpException("null jstring passed to from_jstring");
    }

    const char* chars = env->GetStringUTFChars(js, nullptr);
    if (!chars) {
        throw NrpException("GetStringUTFChars returned null");
    }

    std::string result(chars);
    env->ReleaseStringUTFChars(js, chars);
    return result;
}

jstring StringManager::to_jstring(JNIEnv* env, std::string_view sv) const {
    std::string s(sv);
    return env->NewStringUTF(s.c_str());
}

std::string_view StringManager::intern(std::string_view sv) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    std::string s(sv);
    auto [it, inserted] = intern_table_.insert(s);
    return *it;
}

void StringManager::clear_intern_table() noexcept {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    intern_table_.clear();
}

} // namespace nrp
