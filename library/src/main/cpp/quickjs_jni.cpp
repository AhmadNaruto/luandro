// library/src/main/cpp/quickjs_jni.cpp
// Phase 7: QuickJS Engine JNI Bindings

#include <jni.h>
#include <quickjs/quickjs_jni.h>
#include <quickjs/js_runtime.h>
#include <quickjs/js_context.h>
#include <quickjs/js_value.h>
#include <quickjs/js_exception.h>
#include <runtime.h>
#include <utilities/jni_utils.h>
#include <string>
#include <vector>

// ===================================================================
// Exception translation helpers (extends base withExceptionTranslation)
// ===================================================================

namespace {

// Translates nrp::js exceptions into JVM exceptions
template<typename Func>
auto withJSExceptionTranslation(JNIEnv* env, Func&& func) -> decltype(func()) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> decltype(func()) {
        try {
            return func();
        } catch (const nrp::js::JSException& e) {
            jclass cls = env->FindClass("io/github/luandro/js/JSException");
            if (cls) env->ThrowNew(cls, e.what());
            else env->ThrowNew(env->FindClass("java/lang/RuntimeException"), e.what());
            return decltype(func()){};
        } catch (const nrp::js::RuntimeClosedException& e) {
            jclass cls = env->FindClass("java/lang/IllegalStateException");
            if (cls) env->ThrowNew(cls, e.what());
            return decltype(func()){};
        } catch (const nrp::js::ContextClosedException& e) {
            jclass cls = env->FindClass("java/lang/IllegalStateException");
            if (cls) env->ThrowNew(cls, e.what());
            return decltype(func()){};
        }
    });
}

} // anonymous namespace

extern "C" {

// ==========================================
// JS (static factory) JNI
// ==========================================

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_JS_nativeCreateRuntime(JNIEnv* env, jclass cls) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        JSRuntime* rt = JS_NewRuntime();
        if (!rt) {
            env->ThrowNew(env->FindClass("java/lang/OutOfMemoryError"),
                          "OutOfMemoryError: failed to allocate JS Runtime");
            return 0L;
        }
        auto rt_obj = std::make_unique<nrp::js::Runtime>(rt);
        nrp::Handle h = nrp::Runtime::get().handles().allocate(nrp::js::Runtime::type_tag);
        nrp::Runtime::get().objects().insert<nrp::js::Runtime>(h, std::move(rt_obj));
        return static_cast<jlong>(h);
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_JS_nativeCreateRuntimeWithLimit(JNIEnv* env, jclass cls, jlong maxHeapBytes) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jlong {
        if (maxHeapBytes <= 0) {
            env->ThrowNew(env->FindClass("java/lang/IllegalArgumentException"),
                          "IllegalArgumentException: maxHeapBytes must be > 0");
            return 0L;
        }
        JSRuntime* rt = JS_NewRuntime();
        if (!rt) {
            env->ThrowNew(env->FindClass("java/lang/OutOfMemoryError"),
                          "OutOfMemoryError: failed to allocate JS Runtime");
            return 0L;
        }
        JS_SetMemoryLimit(rt, static_cast<size_t>(maxHeapBytes));
        auto rt_obj = std::make_unique<nrp::js::Runtime>(rt);
        nrp::Handle h = nrp::Runtime::get().handles().allocate(nrp::js::Runtime::type_tag);
        nrp::Runtime::get().objects().insert<nrp::js::Runtime>(h, std::move(rt_obj));
        return static_cast<jlong>(h);
    });
}

// ==========================================
// Runtime JNI
// ==========================================

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_Runtime_nativeNewContext(JNIEnv* env, jobject thiz, jlong handle) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(handle);
        if (!rt) throw nrp::js::RuntimeClosedException();
        nrp::Handle ctx_h = rt->newContext(static_cast<nrp::Handle>(handle));
        return static_cast<jlong>(ctx_h);
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_js_Runtime_nativeGc(JNIEnv* env, jobject thiz, jlong handle) {
    withJSExceptionTranslation(env, [&]() {
        auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(handle);
        if (!rt) throw nrp::js::RuntimeClosedException();
        rt->gc();
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_js_Runtime_nativeSetMemoryLimit(JNIEnv* env, jobject thiz, jlong handle, jlong maxBytes) {
    withJSExceptionTranslation(env, [&]() {
        auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(handle);
        if (!rt) throw nrp::js::RuntimeClosedException();
        rt->setMemoryLimit(static_cast<size_t>(maxBytes));
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_js_Runtime_nativeSetStackSize(JNIEnv* env, jobject thiz, jlong handle, jlong stackBytes) {
    withJSExceptionTranslation(env, [&]() {
        auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(handle);
        if (!rt) throw nrp::js::RuntimeClosedException();
        rt->setStackSize(static_cast<size_t>(stackBytes));
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_Runtime_nativeMemoryUsed(JNIEnv* env, jobject thiz, jlong handle) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(handle);
        if (!rt) throw nrp::js::RuntimeClosedException();
        return static_cast<jlong>(rt->memoryUsed());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_js_Runtime_nativeIsLive(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(handle);
        return static_cast<jboolean>(rt && rt->isLive());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_js_Runtime_nativeDestroy(JNIEnv* env, jobject thiz, jlong handle) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(handle);
        if (rt) rt->close();
        nrp::Runtime::get().objects().destroy(handle);
    });
}

// ==========================================
// Context JNI
// ==========================================

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_Context_nativeEval(JNIEnv* env, jobject thiz, jlong handle, jstring code) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        nrp::jni::JStringUTF code_guard(env, code);
        nrp::Handle h = ctx->eval(static_cast<nrp::Handle>(handle), code_guard.str());
        return static_cast<jlong>(h);
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_Context_nativeEvalModule(JNIEnv* env, jobject thiz, jlong handle, jstring code, jstring filename) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        nrp::jni::JStringUTF code_guard(env, code);
        nrp::jni::JStringUTF file_guard(env, filename);
        nrp::Handle h = ctx->evalModule(static_cast<nrp::Handle>(handle),
                                        code_guard.str(), file_guard.str());
        return static_cast<jlong>(h);
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_js_Context_nativeSetGlobal(JNIEnv* env, jobject thiz, jlong handle, jstring name, jlong value_handle) {
    withJSExceptionTranslation(env, [&]() {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        nrp::jni::JStringUTF name_guard(env, name);
        ctx->setGlobal(name_guard.str(), static_cast<nrp::Handle>(value_handle));
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_Context_nativeGetGlobal(JNIEnv* env, jobject thiz, jlong handle, jstring name) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        nrp::jni::JStringUTF name_guard(env, name);
        nrp::Handle h = ctx->getGlobal(static_cast<nrp::Handle>(handle), name_guard.str());
        return static_cast<jlong>(h);
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_Context_nativeParseJSON(JNIEnv* env, jobject thiz, jlong handle, jstring json) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        nrp::jni::JStringUTF json_guard(env, json);
        nrp::Handle h = ctx->parseJSON(static_cast<nrp::Handle>(handle), json_guard.str());
        return static_cast<jlong>(h);
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_js_Context_nativeStringifyJSON(JNIEnv* env, jobject thiz, jlong handle, jlong value_handle, jint indent) {
    return withJSExceptionTranslation(env, [&]() -> jstring {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        std::string result = ctx->stringifyJSON(static_cast<nrp::Handle>(value_handle),
                                                static_cast<int>(indent));
        return env->NewStringUTF(result.c_str());
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_Context_nativeNewObject(JNIEnv* env, jobject thiz, jlong handle) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        return static_cast<jlong>(ctx->newObject(static_cast<nrp::Handle>(handle)));
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_Context_nativeNewArray(JNIEnv* env, jobject thiz, jlong handle) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        return static_cast<jlong>(ctx->newArray(static_cast<nrp::Handle>(handle)));
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_Context_nativeNewBool(JNIEnv* env, jobject thiz, jlong handle, jboolean value) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        return static_cast<jlong>(ctx->newBool(static_cast<nrp::Handle>(handle), value != 0));
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_Context_nativeNewInt(JNIEnv* env, jobject thiz, jlong handle, jint value) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        return static_cast<jlong>(ctx->newInt(static_cast<nrp::Handle>(handle), static_cast<int>(value)));
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_Context_nativeNewDouble(JNIEnv* env, jobject thiz, jlong handle, jdouble value) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        return static_cast<jlong>(ctx->newDouble(static_cast<nrp::Handle>(handle), static_cast<double>(value)));
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_Context_nativeNewString(JNIEnv* env, jobject thiz, jlong handle, jstring value) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        nrp::jni::JStringUTF val_guard(env, value);
        return static_cast<jlong>(ctx->newString(static_cast<nrp::Handle>(handle), val_guard.str()));
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_Context_nativeUndefined(JNIEnv* env, jobject thiz, jlong handle) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        return static_cast<jlong>(ctx->jsUndefined(static_cast<nrp::Handle>(handle)));
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_Context_nativeNull(JNIEnv* env, jobject thiz, jlong handle) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        return static_cast<jlong>(ctx->jsNull(static_cast<nrp::Handle>(handle)));
    });
}

JNIEXPORT jint JNICALL
Java_io_github_luandro_js_Context_nativeExecutePendingJobs(JNIEnv* env, jobject thiz, jlong handle) {
    return withJSExceptionTranslation(env, [&]() -> jint {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        return static_cast<jint>(ctx->executePendingJobs());
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_js_Context_nativeSetModuleLoader(JNIEnv* env, jobject thiz, jlong handle, jobject loader_obj) {
    withJSExceptionTranslation(env, [&]() {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (!ctx) throw nrp::js::ContextClosedException();
        // Create a global reference to the loader object
        jobject global_loader = env->NewGlobalRef(loader_obj);
        JavaVM* jvm = nullptr;
        env->GetJavaVM(&jvm);
        ctx->setModuleLoader([jvm, global_loader](const std::string& name, const std::string& base) -> std::string {
            JNIEnv* jni_env = nullptr;
            bool attached = false;
            jint res = jvm->GetEnv(reinterpret_cast<void**>(&jni_env), JNI_VERSION_1_6);
            if (res == JNI_EDETACHED) {
                jvm->AttachCurrentThread(reinterpret_cast<JNIEnv**>(&jni_env), nullptr);
                attached = true;
            }
            if (!jni_env) return "";
            jclass loader_cls = jni_env->GetObjectClass(global_loader);
            jmethodID load_method = jni_env->GetMethodID(loader_cls, "load",
                "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
            jstring jname = jni_env->NewStringUTF(name.c_str());
            jstring jbase = jni_env->NewStringUTF(base.c_str());
            auto result_obj = static_cast<jstring>(
                jni_env->CallObjectMethod(global_loader, load_method, jname, jbase));
            jni_env->DeleteLocalRef(jname);
            jni_env->DeleteLocalRef(jbase);
            jni_env->DeleteLocalRef(loader_cls);
            std::string result;
            if (result_obj) {
                const char* cstr = jni_env->GetStringUTFChars(result_obj, nullptr);
                if (cstr) { result = cstr; jni_env->ReleaseStringUTFChars(result_obj, cstr); }
                jni_env->DeleteLocalRef(result_obj);
            }
            if (attached) jvm->DetachCurrentThread();
            return result;
        });
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_js_Context_nativeDestroy(JNIEnv* env, jobject thiz, jlong handle) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(handle);
        if (ctx) {
            nrp::Handle rt_h = ctx->runtime();
            if (rt_h != nrp::kInvalidHandle) {
                auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(rt_h);
                if (rt) rt->untrack_child(static_cast<nrp::Handle>(handle));
            }
            ctx->close();
        }
        nrp::Runtime::get().objects().destroy(handle);
    });
}

// ==========================================
// JSValue JNI
// ==========================================

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_js_JSValue_nativeIsUndefined(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        return static_cast<jboolean>(v && v->isUndefined());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_js_JSValue_nativeIsNull(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        return static_cast<jboolean>(v && v->isNull());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_js_JSValue_nativeIsBool(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        return static_cast<jboolean>(v && v->isBool());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_js_JSValue_nativeIsNumber(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        return static_cast<jboolean>(v && v->isNumber());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_js_JSValue_nativeIsString(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        return static_cast<jboolean>(v && v->isString());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_js_JSValue_nativeIsObject(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        return static_cast<jboolean>(v && v->isObject());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_js_JSValue_nativeIsArray(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        return static_cast<jboolean>(v && v->isArray());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_js_JSValue_nativeIsFunction(JNIEnv* env, jobject thiz, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() -> jboolean {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        return static_cast<jboolean>(v && v->isFunction());
    });
}

JNIEXPORT jboolean JNICALL
Java_io_github_luandro_js_JSValue_nativeToBool(JNIEnv* env, jobject thiz, jlong handle) {
    return withJSExceptionTranslation(env, [&]() -> jboolean {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        if (!v) throw nrp::js::JSException("JSValue is closed or invalid");
        return static_cast<jboolean>(v->toBool());
    });
}

JNIEXPORT jint JNICALL
Java_io_github_luandro_js_JSValue_nativeToInt(JNIEnv* env, jobject thiz, jlong handle) {
    return withJSExceptionTranslation(env, [&]() -> jint {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        if (!v) throw nrp::js::JSException("JSValue is closed or invalid");
        return static_cast<jint>(v->toInt());
    });
}

JNIEXPORT jdouble JNICALL
Java_io_github_luandro_js_JSValue_nativeToDouble(JNIEnv* env, jobject thiz, jlong handle) {
    return withJSExceptionTranslation(env, [&]() -> jdouble {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        if (!v) throw nrp::js::JSException("JSValue is closed or invalid");
        return static_cast<jdouble>(v->toDouble());
    });
}

JNIEXPORT jstring JNICALL
Java_io_github_luandro_js_JSValue_nativeToString(JNIEnv* env, jobject thiz, jlong handle) {
    return withJSExceptionTranslation(env, [&]() -> jstring {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        if (!v) throw nrp::js::JSException("JSValue is closed or invalid");
        std::string s = v->toString();
        return env->NewStringUTF(s.c_str());
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_JSValue_nativeGetProperty(JNIEnv* env, jobject thiz, jlong handle, jstring name) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        if (!v) throw nrp::js::JSException("JSValue is closed or invalid");
        nrp::jni::JStringUTF name_guard(env, name);
        nrp::Handle prop_h = v->getProperty(v->parent_context(), name_guard.str());
        return static_cast<jlong>(prop_h);
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_js_JSValue_nativeSetProperty(JNIEnv* env, jobject thiz, jlong handle, jstring name, jlong value_handle) {
    withJSExceptionTranslation(env, [&]() {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        if (!v) throw nrp::js::JSException("JSValue is closed or invalid");
        nrp::jni::JStringUTF name_guard(env, name);
        v->setProperty(name_guard.str(), static_cast<nrp::Handle>(value_handle));
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_JSValue_nativeGetPropertyAt(JNIEnv* env, jobject thiz, jlong handle, jint index) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        if (!v) throw nrp::js::JSException("JSValue is closed or invalid");
        nrp::Handle prop_h = v->getPropertyAt(v->parent_context(), static_cast<int>(index));
        return static_cast<jlong>(prop_h);
    });
}

JNIEXPORT jint JNICALL
Java_io_github_luandro_js_JSValue_nativeLength(JNIEnv* env, jobject thiz, jlong handle) {
    return withJSExceptionTranslation(env, [&]() -> jint {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        if (!v) throw nrp::js::JSException("JSValue is closed or invalid");
        return static_cast<jint>(v->length());
    });
}

JNIEXPORT jlong JNICALL
Java_io_github_luandro_js_JSValue_nativeCall(JNIEnv* env, jobject thiz, jlong handle, jlong this_handle, jlongArray args) {
    return withJSExceptionTranslation(env, [&]() -> jlong {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        if (!v) throw nrp::js::JSException("JSValue is closed or invalid");

        std::vector<nrp::Handle> arg_handles;
        if (args) {
            jsize len = env->GetArrayLength(args);
            jlong* arr = env->GetLongArrayElements(args, nullptr);
            for (jsize i = 0; i < len; ++i) {
                arg_handles.push_back(static_cast<nrp::Handle>(arr[i]));
            }
            env->ReleaseLongArrayElements(args, arr, JNI_ABORT);
        }

        nrp::Handle result_h = v->call(v->parent_context(),
                                       static_cast<nrp::Handle>(this_handle),
                                       arg_handles);
        return static_cast<jlong>(result_h);
    });
}

JNIEXPORT void JNICALL
Java_io_github_luandro_js_JSValue_nativeFree(JNIEnv* env, jobject thiz, jlong handle) {
    nrp::jni::withExceptionTranslation(env, [&]() {
        auto* v = nrp::Runtime::get().objects().get<nrp::js::JSValueWrapper>(handle);
        if (v) {
            nrp::Handle parent_ctx = v->parent_context();
            if (parent_ctx != nrp::kInvalidHandle) {
                auto* ctx = nrp::Runtime::get().objects().get<nrp::js::Context>(parent_ctx);
                if (ctx) ctx->untrack_child(static_cast<nrp::Handle>(handle));
            }
            v->free();
        }
        nrp::Runtime::get().objects().destroy(handle);
    });
}

} // extern "C"
