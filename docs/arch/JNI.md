# JNI Layer Architecture — Luandro NRP

> **Document:** `docs/arch/JNI.md`
> **Status:** Living document — update when new JNI modules are added.
> **Scope:** All `extern "C"` JNI functions bridging Kotlin/JVM ↔ C++ native layer.

---

## Table of Contents

1. [Design Principles](#1-design-principles)
2. [Naming Convention](#2-naming-convention)
3. [Type Conversion Reference](#3-type-conversion-reference)
4. [String Conversion Pattern](#4-string-conversion-pattern)
5. [Handle Passing via jlong](#5-handle-passing-via-jlong)
6. [Exception Handling](#6-exception-handling)
7. [ThrowJavaException Utility](#7-throwjavaexception-utility)
8. [JNI_OnLoad / JNI_OnUnload](#8-jni_onload--jni_onunload)
9. [Native Method Registration](#9-native-method-registration)
10. [Array Marshaling](#10-array-marshaling)
11. [Return Value Conventions](#11-return-value-conventions)
12. [JNIEnv Safety](#12-jnienv-safety)
13. [Global References](#13-global-references)
14. [Example: Document.parse()](#14-example-documentparse)
15. [Example: Element.select()](#15-example-elementselect)
16. [JNI Checklist](#16-jni-checklist)

---

## 1. Design Principles

### The Thin Layer Mandate

> **The JNI layer does exactly two things: type conversion and dispatch.**
> It contains **no business logic**. All logic lives in C++ core classes.

```
 Kotlin                 JNI (thin)              C++ Core
 ──────                 ──────────              ────────
 doc.select(css)  ────► convert args      ────► Document::select(css)
                        resolve handle
                        convert result   ◄────  return vector<Element*>
                  ◄────  build jobjectArray
```

If you find yourself writing an `if/else` tree or a loop **inside** a JNI function, stop — that logic belongs in C++.

### Design Rules

| Rule | Rationale |
|---|---|
| No business logic in JNI functions | Keeps C++ testable without JVM |
| No stored `JNIEnv*` between calls | `JNIEnv*` is thread-local and call-scoped |
| No raw Java object construction in JNI | Build result objects in Kotlin via callbacks |
| One JNI file per Kotlin class | Mirrors package structure; easy to locate |
| All JNI functions declared `extern "C"` | Prevents C++ name mangling |
| Wrap every C++ call in try/catch | Prevents C++ exceptions from crossing JNI boundary |

---

## 2. Naming Convention

### Pattern

```
Java_<reversed_package>_<ClassName>_<methodName>
```

Package: `io.github.luandro.<module>`

### Examples

| Kotlin Class | Kotlin Method | JNI Function Name |
|---|---|---|
| `lexsoup.Document` | `parse` | `Java_io_github_luandro_lexsoup_Document_parse` |
| `lexsoup.Document` | `nativeDestroy` | `Java_io_github_luandro_lexsoup_Document_nativeDestroy` |
| `lexsoup.Element` | `select` | `Java_io_github_luandro_lexsoup_Element_select` |
| `regex.Pattern` | `compile` | `Java_io_github_luandro_regex_Pattern_compile` |
| `regex.Matcher` | `next` | `Java_io_github_luandro_regex_Matcher_next` |
| `js.JSContext` | `eval` | `Java_io_github_luandro_js_JSContext_eval` |

### Naming Rules

- Use **camelCase** for method names, exactly matching the Kotlin `external fun` name.
- For overloaded methods, append the JNI signature suffix (avoid overloads when possible).
- For `companion object` methods (static): use `jclass` as the second parameter, not `jobject`.
- For instance methods: use `jobject thiz` as the second parameter.

```cpp
// Static (companion object / @JvmStatic)
extern "C" JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Document_parse(JNIEnv* env, jclass clazz, jstring html);

// Instance method
extern "C" JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Document_nativeSelect(JNIEnv* env, jobject thiz,
                                                      jlong handle, jstring selector);
```

---

## 3. Type Conversion Reference

### Primitive Types

| JNI Type | C++ Type | Notes |
|---|---|---|
| `jboolean` | `bool` | JNI `JNI_TRUE` / `JNI_FALSE` |
| `jbyte` | `int8_t` | Signed 8-bit |
| `jshort` | `int16_t` | Signed 16-bit |
| `jint` | `int32_t` | Signed 32-bit |
| `jlong` | `int64_t` / `handle_t` | Used for handles |
| `jfloat` | `float` | IEEE 754 single |
| `jdouble` | `double` | IEEE 754 double |

### Reference Types

| JNI Type | Kotlin/Java Type | C++ Equivalent |
|---|---|---|
| `jstring` | `String` | `std::string` (via conversion) |
| `jbyteArray` | `ByteArray` | `std::vector<uint8_t>` |
| `jintArray` | `IntArray` | `std::vector<int32_t>` |
| `jlongArray` | `LongArray` | `std::vector<int64_t>` |
| `jobjectArray` | `Array<T>` | `std::vector<jobject>` |
| `jobject` | `Any` / custom class | Opaque object reference |

### Handle Type

```cpp
// handle_t is always transmitted as jlong
typedef uint64_t handle_t;

// Conversion helpers
inline handle_t jlong_to_handle(jlong j) { return static_cast<handle_t>(j); }
inline jlong    handle_to_jlong(handle_t h) { return static_cast<jlong>(h); }
```

---

## 4. String Conversion Pattern

### C++ → Kotlin (jstring)

```cpp
// nrp/jni/util/jni_string.h

inline jstring to_jstring(JNIEnv* env, const std::string& s) {
    return env->NewStringUTF(s.c_str());
}

inline jstring to_jstring(JNIEnv* env, std::string_view sv) {
    return env->NewStringUTF(std::string(sv).c_str());
}
```

### Kotlin → C++ (jstring)

```cpp
// RAII wrapper — prevents leaks if exceptions occur mid-function
class JStringUTF {
public:
    JStringUTF(JNIEnv* env, jstring jstr)
        : env_(env), jstr_(jstr),
          chars_(jstr ? env->GetStringUTFChars(jstr, nullptr) : nullptr) {}

    ~JStringUTF() {
        if (chars_) env_->ReleaseStringUTFChars(jstr_, chars_);
    }

    const char* c_str() const { return chars_ ? chars_ : ""; }
    std::string str()   const { return chars_ ? std::string(chars_) : ""; }

    // Non-copyable
    JStringUTF(const JStringUTF&) = delete;
    JStringUTF& operator=(const JStringUTF&) = delete;

private:
    JNIEnv*     env_;
    jstring     jstr_;
    const char* chars_;
};
```

Usage:

```cpp
extern "C" JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Document_parse(JNIEnv* env, jclass, jstring jhtml) {
    JStringUTF html(env, jhtml);     // acquires chars
    // ... use html.str()
}                                    // destructor releases chars automatically
```

**Never** call `GetStringUTFChars` without a matching `ReleaseStringUTFChars`. Use the RAII wrapper above.

---

## 5. Handle Passing via jlong

### Rationale

`jlong` (64-bit signed integer) is the only JNI primitive wide enough to hold a `uint64_t` handle without truncation or pointer aliasing.

### Pattern

```kotlin
// Kotlin side — handle stored in companion field or instance field
class Document private constructor(internal val handle: Long) : AutoCloseable {
    external fun nativeSelect(handle: Long, css: String): LongArray
}
```

```cpp
// C++ JNI side
extern "C" JNIEXPORT jlongArray JNICALL
Java_io_github_luandro_lexsoup_Document_nativeSelect(
        JNIEnv* env, jobject /*thiz*/, jlong jhandle, jstring jcss) {

    handle_t h = jlong_to_handle(jhandle);
    Document* doc = ObjectManager::instance().resolve<Document>(h);
    if (!doc) {
        ThrowJavaException(env, ExceptionType::IllegalState,
                           "Document handle is invalid or already closed");
        return nullptr;
    }
    // ...
}
```

### Security: Opaqueness

The Kotlin layer must treat the `Long` handle as an opaque cookie:
- Do not serialize it to storage.
- Do not send it over IPC.
- Do not compare two handles for object equality — they may alias only coincidentally.

---

## 6. Exception Handling

### The Boundary Rule

> **C++ exceptions must never cross the JNI boundary.**
> Every JNI function must catch all exceptions and convert them to Java exceptions.

```
C++ world         │  JNI boundary  │  JVM world
─────────────────────────────────────────────────
throw std::runtime_error("bad")
                  │                │
    [catch block] │                │
                  │  env->ThrowNew │ ─► Kotlin catches RuntimeException
```

### Exception Mapping

| C++ Exception | Java Exception | Class |
|---|---|---|
| `std::invalid_argument` | `IllegalArgumentException` | `java/lang/IllegalArgumentException` |
| `InvalidHandleException` | `IllegalStateException` | `java/lang/IllegalStateException` |
| `std::bad_alloc` | `OutOfMemoryError` | `java/lang/OutOfMemoryError` |
| `std::runtime_error` | `RuntimeException` | `java/lang/RuntimeException` |
| `std::exception` (catch-all) | `RuntimeException` | `java/lang/RuntimeException` |
| `...` (catch-all) | `RuntimeException` | `java/lang/RuntimeException` |

### Boilerplate Wrapper

```cpp
// Every JNI function body is wrapped in this pattern:
extern "C" JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Document_parse(JNIEnv* env, jclass, jstring jhtml) {
    try {
        JStringUTF html(env, jhtml);
        handle_t h = lexsoup::parse(html.str());
        return handle_to_jlong(h);

    } catch (const std::invalid_argument& e) {
        ThrowJavaException(env, ExceptionType::IllegalArgument, e.what());
    } catch (const std::bad_alloc& e) {
        ThrowJavaException(env, ExceptionType::OutOfMemory, "Out of memory");
    } catch (const std::exception& e) {
        ThrowJavaException(env, ExceptionType::Runtime, e.what());
    } catch (...) {
        ThrowJavaException(env, ExceptionType::Runtime, "Unknown native error");
    }
    return 0L;  // error sentinel
}
```

After `ThrowJavaException()`, the function must return a zero/null value immediately. The JVM sees the pending exception and re-throws it upon returning to Kotlin.

---

## 7. ThrowJavaException Utility

### Declaration

```cpp
// nrp/jni/util/jni_exception.h
#pragma once
#include <jni.h>
#include <string_view>

enum class ExceptionType {
    IllegalArgument,   // java/lang/IllegalArgumentException
    IllegalState,      // java/lang/IllegalStateException
    OutOfMemory,       // java/lang/OutOfMemoryError
    Runtime,           // java/lang/RuntimeException
    IO,                // java/io/IOException
    Unsupported,       // java/lang/UnsupportedOperationException
};

void ThrowJavaException(JNIEnv* env, ExceptionType type, std::string_view message);
```

### Implementation

```cpp
// nrp/jni/util/jni_exception.cpp
#include "jni_exception.h"

void ThrowJavaException(JNIEnv* env, ExceptionType type, std::string_view message) {
    const char* class_name = nullptr;

    switch (type) {
        case ExceptionType::IllegalArgument:
            class_name = "java/lang/IllegalArgumentException"; break;
        case ExceptionType::IllegalState:
            class_name = "java/lang/IllegalStateException"; break;
        case ExceptionType::OutOfMemory:
            class_name = "java/lang/OutOfMemoryError"; break;
        case ExceptionType::Runtime:
            class_name = "java/lang/RuntimeException"; break;
        case ExceptionType::IO:
            class_name = "java/io/IOException"; break;
        case ExceptionType::Unsupported:
            class_name = "java/lang/UnsupportedOperationException"; break;
    }

    // Clear any existing exception before throwing a new one
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }

    jclass clazz = env->FindClass(class_name);
    if (clazz) {
        env->ThrowNew(clazz, std::string(message).c_str());
        env->DeleteLocalRef(clazz);
    }
}
```

---

## 8. JNI_OnLoad / JNI_OnUnload

### JNI_OnLoad

Called once when the `.so` library is loaded (`System.loadLibrary`). Used to:
1. Validate JVM version.
2. Cache global class and method references.
3. Register native methods.

```cpp
// nrp/jni/nrp_jni.cpp

JavaVM* g_jvm = nullptr;  // stored for background thread attachment

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    g_jvm = vm;

    JNIEnv* env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    // Cache global references
    if (!CacheGlobalRefs(env)) return JNI_ERR;

    // Register native methods for each class
    if (!RegisterLexsoupMethods(env)) return JNI_ERR;
    if (!RegisterRegexMethods(env))   return JNI_ERR;
    if (!RegisterJsMethods(env))      return JNI_ERR;

    return JNI_VERSION_1_6;
}
```

### JNI_OnUnload

Called when the ClassLoader that loaded the library is GC'd (rare in practice for app main library).

```cpp
extern "C" JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* /*reserved*/) {
    JNIEnv* env = nullptr;
    vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);

    if (env) {
        ReleaseGlobalRefs(env);
    }

    // Shutdown ObjectManager
    ObjectManager::instance().shutdownAll();

    g_jvm = nullptr;
}
```

---

## 9. Native Method Registration

### Preferred: RegisterNatives

Explicit registration via `RegisterNatives` is preferred over the default name-mangling lookup because it:
- Fails fast at load time if any signature is wrong.
- Does not require `extern "C"` naming (though we keep it for readability).
- Allows the same C++ function to bind to multiple Java method names.

```cpp
// nrp/jni/lexsoup/document_jni.cpp

// Forward declarations
extern "C" {
    jlong  Java_io_github_luandro_lexsoup_Document_parse(JNIEnv*, jclass, jstring);
    void   Java_io_github_luandro_lexsoup_Document_nativeDestroy(JNIEnv*, jclass, jlong);
    jlongArray Java_io_github_luandro_lexsoup_Document_nativeSelect(JNIEnv*, jobject, jlong, jstring);
}

// Method registration table
static const JNINativeMethod kDocumentMethods[] = {
    { "parse",         "(Ljava/lang/String;)J",
       (void*)Java_io_github_luandro_lexsoup_Document_parse         },

    { "nativeDestroy", "(J)V",
       (void*)Java_io_github_luandro_lexsoup_Document_nativeDestroy },

    { "nativeSelect",  "(JLjava/lang/String;)[J",
       (void*)Java_io_github_luandro_lexsoup_Document_nativeSelect  },
};

bool RegisterDocumentMethods(JNIEnv* env) {
    jclass clazz = env->FindClass("io/github/luandro/lexsoup/Document");
    if (!clazz) return false;

    jint result = env->RegisterNatives(
        clazz,
        kDocumentMethods,
        sizeof(kDocumentMethods) / sizeof(kDocumentMethods[0])
    );
    env->DeleteLocalRef(clazz);
    return result == JNI_OK;
}
```

### JNI Signature Quick Reference

| Kotlin Type | JNI Signature |
|---|---|
| `Int` | `I` |
| `Long` | `J` |
| `Boolean` | `Z` |
| `String` | `Ljava/lang/String;` |
| `ByteArray` | `[B` |
| `LongArray` | `[J` |
| `IntArray` | `[I` |
| `Array<String>` | `[Ljava/lang/String;` |
| `Unit` (void) | `V` |

---

## 10. Array Marshaling

### jlongArray (handles)

```cpp
// Returning a list of element handles as jlongArray
jlongArray handles_to_jlongArray(JNIEnv* env, const std::vector<handle_t>& handles) {
    jlongArray arr = env->NewLongArray(static_cast<jsize>(handles.size()));
    if (!arr) return nullptr;  // OOM

    // handles are uint64_t; jlong is int64_t — safe bitcast for handles < 2^63
    env->SetLongArrayRegion(arr, 0, static_cast<jsize>(handles.size()),
                            reinterpret_cast<const jlong*>(handles.data()));
    return arr;
}
```

### jbyteArray (binary data)

```cpp
// C++ vector<uint8_t> → Java ByteArray
jbyteArray bytes_to_jbyteArray(JNIEnv* env, const std::vector<uint8_t>& data) {
    jbyteArray arr = env->NewByteArray(static_cast<jsize>(data.size()));
    if (!arr) return nullptr;
    env->SetByteArrayRegion(arr, 0, static_cast<jsize>(data.size()),
                            reinterpret_cast<const jbyte*>(data.data()));
    return arr;
}

// Java ByteArray → C++ vector<uint8_t>
std::vector<uint8_t> jbyteArray_to_bytes(JNIEnv* env, jbyteArray jarr) {
    jsize len = env->GetArrayLength(jarr);
    std::vector<uint8_t> result(static_cast<size_t>(len));
    env->GetByteArrayRegion(jarr, 0, len, reinterpret_cast<jbyte*>(result.data()));
    return result;
}
```

### jobjectArray (strings)

```cpp
// C++ vector<string> → Java Array<String>
jobjectArray strings_to_jobjectArray(JNIEnv* env, const std::vector<std::string>& strs) {
    jclass strClass = env->FindClass("java/lang/String");
    jobjectArray arr = env->NewObjectArray(static_cast<jsize>(strs.size()),
                                           strClass, nullptr);
    for (jsize i = 0; i < static_cast<jsize>(strs.size()); ++i) {
        jstring js = env->NewStringUTF(strs[i].c_str());
        env->SetObjectArrayElement(arr, i, js);
        env->DeleteLocalRef(js);
    }
    env->DeleteLocalRef(strClass);
    return arr;
}
```

---

## 11. Return Value Conventions

| Return Scenario | Return Value |
|---|---|
| Success, returns handle | Positive `jlong` |
| Success, no return value | `void` |
| Success, returns bool | `JNI_TRUE` / `JNI_FALSE` |
| Error (exception thrown) | `0L` / `nullptr` / `JNI_FALSE` |
| OOM (array alloc failed) | `nullptr` — JVM already has a pending exception |

Always return immediately after calling `ThrowJavaException()`. Do not attempt further operations with a pending exception.

---

## 12. JNIEnv Safety

### Never Store JNIEnv*

`JNIEnv*` is **thread-local**. It is valid only within the JNI call frame on the thread that received it.

```cpp
// WRONG ✗
static JNIEnv* g_env = nullptr;
extern "C" JNIEXPORT void JNICALL SomeMethod(JNIEnv* env, ...) {
    g_env = env;  // DANGER: invalid on any other thread
}

// CORRECT ✓ — retrieve env from JavaVM when needed on a background thread
JNIEnv* GetEnvForCurrentThread() {
    JNIEnv* env = nullptr;
    jint result = g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        g_jvm->AttachCurrentThread(&env, nullptr);
    }
    return env;
}
```

### Attach / Detach Background Threads

If a C++ background thread needs to call back into Java:

```cpp
class JniEnvGuard {
public:
    JniEnvGuard() : attached_(false) {
        jint r = g_jvm->GetEnv(reinterpret_cast<void**>(&env_), JNI_VERSION_1_6);
        if (r == JNI_EDETACHED) {
            g_jvm->AttachCurrentThread(&env_, nullptr);
            attached_ = true;
        }
    }
    ~JniEnvGuard() {
        if (attached_) g_jvm->DetachCurrentThread();
    }
    JNIEnv* env() { return env_; }
private:
    JNIEnv* env_;
    bool    attached_;
};
```

---

## 13. Global References

### When to Use Global References

Cache `jclass`, `jmethodID`, and `jfieldID` as global references in `JNI_OnLoad`. These are stable for the lifetime of the library.

```cpp
// nrp/jni/global_refs.h
struct GlobalRefs {
    jclass    Document_class;
    jmethodID Document_init;
    jclass    Element_class;
    jmethodID Element_init;
    // ...
};

extern GlobalRefs g_refs;

bool CacheGlobalRefs(JNIEnv* env);
void ReleaseGlobalRefs(JNIEnv* env);
```

```cpp
// nrp/jni/global_refs.cpp
GlobalRefs g_refs = {};

bool CacheGlobalRefs(JNIEnv* env) {
    jclass local = env->FindClass("io/github/luandro/lexsoup/Document");
    if (!local) return false;
    g_refs.Document_class = static_cast<jclass>(env->NewGlobalRef(local));
    env->DeleteLocalRef(local);

    g_refs.Document_init = env->GetMethodID(g_refs.Document_class, "<init>", "(J)V");
    if (!g_refs.Document_init) return false;

    // ... repeat for other classes
    return true;
}

void ReleaseGlobalRefs(JNIEnv* env) {
    if (g_refs.Document_class) {
        env->DeleteGlobalRef(g_refs.Document_class);
        g_refs.Document_class = nullptr;
    }
    // ...
}
```

### Local References

Local references are created within a JNI call and automatically freed when the call returns. For functions that create many local refs (e.g., building arrays), use `PushLocalFrame`/`PopLocalFrame`.

---

## 14. Example: Document.parse()

### Kotlin Declaration

```kotlin
// io/github/luandro/lexsoup/Document.kt
companion object {
    @JvmStatic
    external fun parse(html: String): Long  // returns handle
}
```

### JNI Implementation

```cpp
// nrp/jni/lexsoup/document_jni.cpp
extern "C" JNIEXPORT jlong JNICALL
Java_io_github_luandro_lexsoup_Document_parse(JNIEnv* env, jclass /*clazz*/,
                                               jstring jhtml) {
    try {
        if (!jhtml) {
            ThrowJavaException(env, ExceptionType::IllegalArgument,
                               "html string must not be null");
            return 0L;
        }

        JStringUTF html(env, jhtml);

        // Business logic lives in C++ core — JNI just dispatches
        handle_t h = ObjectManager::instance().create<Document>(html.str());

        return handle_to_jlong(h);

    } catch (const std::bad_alloc&) {
        ThrowJavaException(env, ExceptionType::OutOfMemory, "Out of memory parsing HTML");
    } catch (const std::exception& e) {
        ThrowJavaException(env, ExceptionType::Runtime, e.what());
    } catch (...) {
        ThrowJavaException(env, ExceptionType::Runtime, "Unknown native error in parse()");
    }
    return 0L;
}
```

---

## 15. Example: Element.select()

### Kotlin Declaration

```kotlin
// io/github/luandro/lexsoup/Document.kt
external fun nativeSelect(handle: Long, css: String): LongArray  // array of element handles
```

### JNI Implementation

```cpp
extern "C" JNIEXPORT jlongArray JNICALL
Java_io_github_luandro_lexsoup_Document_nativeSelect(JNIEnv* env, jobject /*thiz*/,
                                                      jlong jhandle, jstring jcss) {
    try {
        if (!jcss) {
            ThrowJavaException(env, ExceptionType::IllegalArgument,
                               "css selector must not be null");
            return nullptr;
        }

        handle_t h = jlong_to_handle(jhandle);
        Document* doc = ObjectManager::instance().resolve<Document>(h);

        if (!doc) {
            ThrowJavaException(env, ExceptionType::IllegalState,
                               "Document handle is invalid or already closed");
            return nullptr;
        }

        JStringUTF css(env, jcss);

        // C++ returns element handles (child objects registered in ObjectManager)
        std::vector<handle_t> elem_handles = doc->select(css.str());

        return handles_to_jlongArray(env, elem_handles);

    } catch (const std::invalid_argument& e) {
        ThrowJavaException(env, ExceptionType::IllegalArgument, e.what());
    } catch (const std::exception& e) {
        ThrowJavaException(env, ExceptionType::Runtime, e.what());
    } catch (...) {
        ThrowJavaException(env, ExceptionType::Runtime, "Unknown native error in select()");
    }
    return nullptr;
}
```

---

## 16. JNI Checklist

Use this checklist when adding a new JNI function:

- [ ] Function is declared `extern "C"` with correct name-mangled signature
- [ ] All C++ calls wrapped in `try { ... } catch (...)` 
- [ ] `ThrowJavaException()` called for every exception type
- [ ] Function returns zero/null sentinel after every exception path
- [ ] `jstring` arguments converted via `JStringUTF` RAII wrapper
- [ ] `handle_t` passed as `jlong`, converted with `jlong_to_handle()`
- [ ] No `JNIEnv*` stored beyond function scope
- [ ] Local references deleted with `DeleteLocalRef()` or `PushLocalFrame`/`PopLocalFrame`
- [ ] Method added to `JNINativeMethod` table in `RegisterNatives` call
- [ ] Kotlin `external fun` signature matches JNI signature exactly

---

*Last updated: 2026-07 · Maintainer: NRP Core Team*
