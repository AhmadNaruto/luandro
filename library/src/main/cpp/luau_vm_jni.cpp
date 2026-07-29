// library/src/main/cpp/luau_vm_jni.cpp
// Phase 8: LuauVM JNI Bridge

#include <jni.h>
#include <luau/luau_vm.h>
#include <runtime.h>
#include <utilities/jni_utils.h>
#include <string>
#include <vector>
#include <memory>

using nrp::luau::LuauVM;
using nrp::luau::LuauValue;

// ===================================================================
// Exception translation helper
// ===================================================================

namespace {

template<typename Func>
auto withLuauExceptions(JNIEnv* env, Func&& func) -> decltype(func()) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> decltype(func()) {
        try {
            return func();
        } catch (const nrp::luau::LuauCompileException& e) {
            jclass cls = env->FindClass("io/github/luandro/luau/LuauCompileException");
            if (cls) env->ThrowNew(cls, e.what());
            else env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
            return decltype(func()){};
        } catch (const nrp::luau::LuauRuntimeException& e) {
            jclass cls = env->FindClass("io/github/luandro/luau/LuauRuntimeException");
            if (cls) env->ThrowNew(cls, e.what());
            else env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
            return decltype(func()){};
        } catch (const nrp::luau::VMClosedException& e) {
            jclass cls = env->FindClass("java/lang/IllegalStateException");
            if (cls) env->ThrowNew(cls, e.what());
            return decltype(func()){};
        }
    });
}

// Convert a LuauValue to a Kotlin LuauValue object
jobject luau_value_to_jobject(JNIEnv* env, const LuauValue& v) {
    jclass cls = env->FindClass("io/github/luandro/luau/LuauValue");
    if (!cls) return nullptr;

    jmethodID method = nullptr;
    jobject result = nullptr;

    switch (v.type) {
        case LuauValue::Type::Nil: {
            method = env->GetStaticMethodID(cls, "nil", "()Lio/github/luandro/luau/LuauValue;");
            result = env->CallStaticObjectMethod(cls, method);
            break;
        }
        case LuauValue::Type::Bool: {
            method = env->GetStaticMethodID(cls, "of", "(Z)Lio/github/luandro/luau/LuauValue;");
            result = env->CallStaticObjectMethod(cls, method, static_cast<jboolean>(v.b));
            break;
        }
        case LuauValue::Type::Number: {
            method = env->GetStaticMethodID(cls, "of", "(D)Lio/github/luandro/luau/LuauValue;");
            result = env->CallStaticObjectMethod(cls, method, static_cast<jdouble>(v.n));
            break;
        }
        case LuauValue::Type::String: {
            method = env->GetStaticMethodID(cls, "of", "(Ljava/lang/String;)Lio/github/luandro/luau/LuauValue;");
            jstring js = env->NewStringUTF(v.s.c_str());
            result = env->CallStaticObjectMethod(cls, method, js);
            env->DeleteLocalRef(js);
            break;
        }
    }
    return result;
}

// Convert a Kotlin LuauValue object to a native LuauValue
LuauValue jobject_to_luau_value(JNIEnv* env, jobject obj) {
    if (!obj) return LuauValue::nil();

    jclass cls = env->GetObjectClass(obj);

    // Check type
    jmethodID is_nil    = env->GetMethodID(cls, "isNil",    "()Z");
    jmethodID is_bool   = env->GetMethodID(cls, "isBool",   "()Z");
    jmethodID is_number = env->GetMethodID(cls, "isNumber", "()Z");
    jmethodID is_string = env->GetMethodID(cls, "isString", "()Z");

    if (env->CallBooleanMethod(obj, is_nil)) {
        return LuauValue::nil();
    }
    if (env->CallBooleanMethod(obj, is_bool)) {
        jmethodID to_bool = env->GetMethodID(cls, "toBoolean", "()Z");
        return LuauValue::of(env->CallBooleanMethod(obj, to_bool) != 0);
    }
    if (env->CallBooleanMethod(obj, is_number)) {
        jmethodID to_double = env->GetMethodID(cls, "toDouble", "()D");
        return LuauValue::of(static_cast<double>(env->CallDoubleMethod(obj, to_double)));
    }
    if (env->CallBooleanMethod(obj, is_string)) {
        jmethodID to_str = env->GetMethodID(cls, "toString", "()Ljava/lang/String;");
        auto jstr = static_cast<jstring>(env->CallObjectMethod(obj, to_str));
        if (!jstr) return LuauValue::nil();
        const char* cstr = env->GetStringUTFChars(jstr, nullptr);
        std::string result = cstr ? cstr : "";
        env->ReleaseStringUTFChars(jstr, cstr);
        env->DeleteLocalRef(jstr);
        return LuauValue::of(result);
    }
    return LuauValue::nil();
}

} // anonymous namespace

extern "C" {

// ==========================================
// LuauVM Factory
// ==========================================

JNIEXPORT jlong JNICALL
Java_io_github_luandro_luau_LuauVM_nativeCreate(JNIEnv* env, jclass cls) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        auto vm = LuauVM::create();
        nrp::Handle h = nrp::Runtime::get().handles().allocate(LuauVM::type_tag);
        nrp::Runtime::get().objects().insert<LuauVM>(h, std::move(vm));
        return static_cast<jlong>(h);
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_luau_LuauVM_nativeCreateWithMemoryLimit(JNIEnv* env, jclass cls, jlong maxBytes) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        if (maxBytes <= 0) {
            env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"),
                          "maxHeapBytes must be > 0");
            return 0L;
        }
        auto vm = LuauVM::createWithMemoryLimit(static_cast<size_t>(maxBytes));
        nrp::Handle h = nrp::Runtime::get().handles().allocate(LuauVM::type_tag);
        nrp::Runtime::get().objects().insert<LuauVM>(h, std::move(vm));
        return static_cast<jlong>(h);
    });
}

// ==========================================
// Script execution
// ==========================================

JNIEXPORT jobject JNICALL
Java_io_github_luandro_luau_LuauVM_nativeExecute(JNIEnv* env, jobject thiz,
                                                   jlong handle, jstring script, jstring chunk_name) {
    return withLuauExceptions(env, [&]() -> jobject {
        auto* vm = nrp::Runtime::get().objects().get<LuauVM>(handle);
        if (!vm) throw nrp::luau::VMClosedException();
        nrp::jni::JStringUTF script_guard(env, script);
        nrp::jni::JStringUTF name_guard(env, chunk_name);
        LuauValue result = vm->execute(script_guard.str(), name_guard.str());
        return luau_value_to_jobject(env, result);
    });
}

JNIEXPORT jbyteArray JNICALL
Java_io_github_luandro_luau_LuauVM_nativeCompile(JNIEnv* env, jobject thiz,
                                                   jlong handle, jstring script, jstring chunk_name) {
    return withLuauExceptions(env, [&]() -> jbyteArray {
        auto* vm = nrp::Runtime::get().objects().get<LuauVM>(handle);
        if (!vm) throw nrp::luau::VMClosedException();
        nrp::jni::JStringUTF script_guard(env, script);
        nrp::jni::JStringUTF name_guard(env, chunk_name);
        std::vector<uint8_t> bytecode = vm->compile(script_guard.str(), name_guard.str());
        jbyteArray result = env->NewByteArray(static_cast<jsize>(bytecode.size()));
        env->SetByteArrayRegion(result, 0, static_cast<jsize>(bytecode.size()),
                                reinterpret_cast<const jbyte*>(bytecode.data()));
        return result;
    });
}

JNIEXPORT jobject JNICALL
Java_io_github_luandro_luau_LuauVM_nativeExecuteCompiled(JNIEnv* env, jobject thiz,
                                                          jlong handle, jbyteArray bytecode,
                                                          jstring chunk_name) {
    return withLuauExceptions(env, [&]() -> jobject {
        auto* vm = nrp::Runtime::get().objects().get<LuauVM>(handle);
        if (!vm) throw nrp::luau::VMClosedException();
        nrp::jni::JStringUTF name_guard(env, chunk_name);
        jsize len = env->GetArrayLength(bytecode);
        jbyte* raw = env->GetByteArrayElements(bytecode, nullptr);
        std::vector<uint8_t> bc(reinterpret_cast<uint8_t*>(raw),
                                reinterpret_cast<uint8_t*>(raw) + len);
        env->ReleaseByteArrayElements(bytecode, raw, JNI_ABORT);
        LuauValue result = vm->executeCompiled(bc, name_guard.str());
        return luau_value_to_jobject(env, result);
    });
}

// ==========================================
// Global state
// ==========================================

JNIEXPORT void JNICALL
Java_io_github_luandro_luau_LuauVM_nativeSetGlobal(JNIEnv* env, jobject thiz,
                                                    jlong handle, jstring name, jobject value) {
    withLuauExceptions(env, [&]() {
        auto* vm = nrp::Runtime::get().objects().get<LuauVM>(handle);
        if (!vm) throw nrp::luau::VMClosedException();
        nrp::jni::JStringUTF name_guard(env, name);
        LuauValue v = jobject_to_luau_value(env, value);
        vm->setGlobal(name_guard.str(), v);
    });
}

JNIEXPORT jobject JNICALL
Java_io_github_luandro_luau_LuauVM_nativeGetGlobal(JNIEnv* env, jobject thiz,
                                                    jlong handle, jstring name) {
    return withLuauExceptions(env, [&]() -> jobject {
        auto* vm = nrp::Runtime::get().objects().get<LuauVM>(handle);
        if (!vm) throw nrp::luau::VMClosedException();
        nrp::jni::JStringUTF name_guard(env, name);
        LuauValue v = vm->getGlobal(name_guard.str());
        return luau_value_to_jobject(env, v);
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_luau_LuauVM_nativeRemoveGlobal(JNIEnv* env, jobject thiz,
                                                       jlong handle, jstring name) {
    withLuauExceptions(env, [&]() {
        auto* vm = nrp::Runtime::get().objects().get<LuauVM>(handle);
        if (!vm) throw nrp::luau::VMClosedException();
        nrp::jni::JStringUTF name_guard(env, name);
        vm->removeGlobal(name_guard.str());
    });
}

// ==========================================
// Lifecycle
// ==========================================

JNIEXPORT void JNICALL
Java_io_github_luandro_luau_LuauVM_nativeDestroy(JNIEnv* env, jobject thiz, jlong handle) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* vm = nrp::Runtime::get().objects().get<LuauVM>(handle);
        if (vm) vm->close();
        nrp::Runtime::get().objects().destroy(handle);
    });
}

} // extern "C"
