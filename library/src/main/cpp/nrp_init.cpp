// nrp_init.cpp — Luandro Native Runtime Platform
// Phase 0: Minimal stub to verify the build system works.
// This file will be replaced in Phase 3 (Runtime Core).
//
// DO NOT add business logic here.

#include <jni.h>
#include <android/log.h>

#define LOG_TAG "LuandroNRP"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// JNI_OnLoad — called when the native library is loaded
// Phase 0: stub only
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    LOGI("Luandro NRP native library loaded (Phase 0 stub)");
    return JNI_VERSION_1_6;
}
