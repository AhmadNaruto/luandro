// nrp_init.cpp — Luandro Native Runtime Platform
// Phase 3: Runtime Core

#include <jni.h>
#include <android/log.h>
#include "runtime.h"

#define LOG_TAG "LuandroNRP"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT jstring JNICALL
Java_io_github_luandro_NativeRuntime_nativeVersion(JNIEnv* env, jobject) {
    return env->NewStringUTF("0.1.0-phase3");
}

// JNI_OnLoad — called when the native library is loaded
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    LOGI("Luandro NRP native library loading (Phase 3)");
    nrp::Runtime::initialize();
    return JNI_VERSION_1_6;
}

// JNI_OnUnload — called when the native library is unloaded
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* /*vm*/, void* /*reserved*/) {
    LOGI("Luandro NRP native library unloading");
    nrp::Runtime::destroy();
}

