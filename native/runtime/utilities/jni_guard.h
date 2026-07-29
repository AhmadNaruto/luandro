// native/runtime/utilities/jni_guard.h
// Phase 3: Runtime Core

#pragma once

#include <jni.h>
#include <string>
#include <string_view>
#include "../exceptions/exception_manager.h"

namespace nrp {

// RAII wrapper for GetStringUTFChars / ReleaseStringUTFChars.
class JniStringGuard {
public:
    JniStringGuard(JNIEnv* env, jstring js) : env_(env), js_(js) {
        if (js) {
            chars_ = env->GetStringUTFChars(js, nullptr);
        }
    }

    ~JniStringGuard() {
        if (js_ && chars_) {
            env_->ReleaseStringUTFChars(js_, chars_);
        }
    }

    [[nodiscard]] std::string_view view() const {
        if (!chars_) {
            throw NrpException("null jstring or failed to retrieve chars");
        }
        return {chars_};
    }

    [[nodiscard]] std::string str() const {
        return std::string(view());
    }

    // Non-copyable, non-movable
    JniStringGuard(const JniStringGuard&) = delete;
    JniStringGuard& operator=(const JniStringGuard&) = delete;

private:
    JNIEnv*     env_;
    jstring     js_;
    const char* chars_ = nullptr;
};

// RAII wrapper for local JNI references.
class JniLocalRef {
public:
    JniLocalRef(JNIEnv* env, jobject ref) : env_(env), ref_(ref) {}

    ~JniLocalRef() {
        if (ref_) {
            env_->DeleteLocalRef(ref_);
        }
    }

    [[nodiscard]] jobject get() const noexcept {
        return ref_;
    }

    operator bool() const noexcept {
        return ref_ != nullptr;
    }

    // Non-copyable, non-movable
    JniLocalRef(const JniLocalRef&) = delete;
    JniLocalRef& operator=(const JniLocalRef&) = delete;

private:
    JNIEnv* env_;
    jobject ref_;
};

} // namespace nrp
