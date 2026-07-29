// native/runtime/utilities/jni_utils.h
// Phase 4: Binding Infrastructure

#pragma once

#include <jni.h>
#include "../exceptions/exception_manager.h"
#include "jni_guard.h"
#include <type_traits>

namespace nrp::jni {

// Reusable JStringUTF RAII guard matching JNI.md conventions
using JStringUTF = JniStringGuard;

/**
 * Reusable try-catch exception boundary translator for JNI entry points.
 * Automatically translates native C++ exceptions to JVM exceptions and returns default values.
 */
template <typename Func>
auto withExceptionTranslation(JNIEnv* env, Func&& func) noexcept {
    using ReturnType = decltype(func());
    try {
        return func();
    }
    catch (const NrpException& e) {
        ExceptionManager::throw_to_java(env, e);
    }
    catch (const std::exception& e) {
        ExceptionManager::throw_to_java(env, e);
    }
    catch (...) {
        ExceptionManager::throw_unknown_to_java(env);
    }

    if constexpr (std::is_same_v<ReturnType, void>) {
        return;
    } else {
        return ReturnType{};
    }
}

} // namespace nrp::jni
