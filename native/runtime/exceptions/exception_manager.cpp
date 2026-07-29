// native/runtime/exceptions/exception_manager.cpp
// Phase 3: Runtime Core

#include "exception_manager.h"

namespace nrp {

namespace ExceptionManager {

static void throw_to_java_with_class(JNIEnv* env, const char* class_name, const std::string& msg) noexcept {
    if (!env) return;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
    jclass clazz = env->FindClass(class_name);
    if (clazz) {
        env->ThrowNew(clazz, msg.c_str());
        env->DeleteLocalRef(clazz);
    } else {
        // Fallback to RuntimeException if the specific exception class is not found in JVM
        env->ExceptionClear();
        jclass fallback = env->FindClass("java/lang/RuntimeException");
        if (fallback) {
            std::string fallback_msg = std::string("[") + class_name + "] " + msg;
            env->ThrowNew(fallback, fallback_msg.c_str());
            env->DeleteLocalRef(fallback);
        }
    }
}

void throw_to_java(JNIEnv* env, const NrpException& e) noexcept {
    const char* class_name = "io/github/luandro/NrpException";
    std::string code = e.code();

    if (code == "NRP_HANDLE_ERROR") {
        class_name = "io/github/luandro/NrpHandleException";
    } else if (code == "NRP_TYPE_ERROR") {
        class_name = "io/github/luandro/NrpTypeException";
    } else if (code == "NRP_PARSE_ERROR") {
        class_name = "io/github/luandro/NrpParseException";
    } else if (code == "NRP_SCRIPT_ERROR") {
        class_name = "io/github/luandro/NrpScriptException";
    } else if (code == "NRP_MEMORY_ERROR") {
        class_name = "io/github/luandro/NrpMemoryException";
    }

    throw_to_java_with_class(env, class_name, e.what());
}

void throw_to_java(JNIEnv* env, const std::exception& e) noexcept {
    throw_to_java_with_class(env, "java/lang/RuntimeException", e.what());
}

void throw_unknown_to_java(JNIEnv* env) noexcept {
    throw_to_java_with_class(env, "java/lang/RuntimeException", "Unknown native exception");
}

} // namespace ExceptionManager

} // namespace nrp
