# STANDARDS.md — Luandro Native Runtime Platform
# Engineering Standards & Development Guidelines
# Phase 0.5 — Project Standards & Guidelines
#
# This document is the authoritative reference for ALL development on the Luandro NRP.
# Every subsequent phase MUST follow these standards.
# No implementation may violate these rules unless this document is updated first.

---

## 1. Core Principles

| Rule | Description |
|------|-------------|
| **Native is truth** | All business logic MUST exist only in C++ native code |
| **Kotlin is thin** | Kotlin is a public API wrapper only — no logic |
| **Luau is thin** | Luau is a second frontend — no logic, same native implementation |
| **No duplication** | Every feature is implemented exactly ONCE in native |
| **Handle model** | Raw pointers NEVER cross language boundaries |

---

## 2. C++ Coding Style

### Standard & Compiler
- Language: **C++20** (required, not optional)
- Compiler: **Clang** (NDK 28.2, LLVM 21.1)
- Flags: `-std=c++20 -Wall -Wextra -Wpedantic`

### Design Rules
```cpp
// ✅ CORRECT — RAII, smart pointers, no raw ownership
class DocumentManager {
    std::unique_ptr<lxb_html_document_t, Deleter> doc_;
public:
    explicit DocumentManager(std::string_view html);
    ~DocumentManager() = default;  // RAII handles cleanup
};

// ❌ WRONG — raw pointer ownership
class DocumentManager {
    lxb_html_document_t* doc_;  // who frees this?
public:
    ~DocumentManager() { free(doc_); }  // error-prone
};
```

### Rules
- **RAII**: every resource has a clear owner via RAII
- **Smart pointers**: `std::unique_ptr` or `std::shared_ptr` internally
- **No raw pointer ownership**: raw pointers for observation only
- **Small, focused classes**: single responsibility per class
- **SOLID principles**: where appropriate
- **Composition > Inheritance**: prefer composition
- **`constexpr`**: use where applicable
- **No macros**: except for platform compatibility
- **`std::string_view`**: prefer over `const std::string&` for read-only strings
- **`[[nodiscard]]`**: on functions where ignoring return value is a bug

### Error Handling (C++)
```cpp
// ✅ CORRECT — exceptions inside native, never crossing JNI
namespace nrp {
class ParseException : public std::runtime_error {
public:
    explicit ParseException(std::string_view msg)
        : std::runtime_error(std::string(msg)) {}
};
}

// ✅ CORRECT — JNI catches and converts to Java exception
// (see JNI standards below)
```

---

## 3. Kotlin Coding Style

### Rules
- Follow official [Kotlin coding conventions](https://kotlinlang.org/docs/coding-conventions.html)
- **No business logic** — all logic lives in native
- **No DOM manipulation** — handled by native
- **No parsing logic** — handled by native
- Public API must feel **idiomatic to Kotlin developers**

### API Example
```kotlin
// ✅ CORRECT — idiomatic Kotlin, no logic
class Document internal constructor(private val handle: Long) : Closeable {
    fun title(): String = nativeTitle(handle)
    fun select(cssQuery: String): Elements = Elements(nativeSelect(handle, cssQuery))
    override fun close() = nativeDestroy(handle)

    private external fun nativeTitle(handle: Long): String
    private external fun nativeSelect(handle: Long, query: String): Long
    private external fun nativeDestroy(handle: Long)
}

// ❌ WRONG — business logic in Kotlin
class Document(private val handle: Long) {
    fun select(query: String): Elements {
        if (query.isEmpty()) return Elements.empty()  // logic!
        val raw = nativeSelect(handle, query)
        return if (raw == 0L) Elements.empty() else Elements(raw)  // logic!
    }
}
```

---

## 4. JNI Standards

### Responsibilities (ONLY these, nothing else)
```cpp
// ✅ CORRECT — JNI is only a thin conversion layer
extern "C" JNIEXPORT jstring JNICALL
Java_io_github_luandro_lexsoup_Document_nativeTitle(JNIEnv* env, jobject, jlong handle) {
    return nrp::jni::withExceptionTranslation(env, [&]() {
        auto* doc = nrp::HandleManager::get<nrp::Document>(handle);
        return nrp::jni::toJString(env, doc->title());  // call runtime, convert result
    });
}

// ❌ WRONG — business logic in JNI
extern "C" JNIEXPORT jstring JNICALL
Java_io_github_luandro_lexsoup_Document_nativeTitle(JNIEnv* env, jobject, jlong handle) {
    auto* doc = nrp::HandleManager::get<nrp::Document>(handle);
    auto title = doc->title();
    if (title.empty()) return env->NewStringUTF("(no title)");  // logic in JNI!
    return nrp::jni::toJString(env, title);
}
```

### JNI Allowed Operations
| Allowed | Example |
|---------|---------|
| String conversion | `jstring → std::string`, `std::string → jstring` |
| Primitive conversion | `jint → int`, `jlong → long` |
| Array conversion | `jbyteArray → std::vector<uint8_t>` |
| Handle conversion | `jlong → T*` via HandleManager |
| Exception translation | native exception → Java exception |

### JNI Forbidden
- Business logic of any kind
- DOM manipulation
- Parsing logic
- String processing beyond encoding conversion

---

## 5. Luau Binding Standards

### Responsibilities (ONLY these)
```lua
-- ✅ CORRECT — Luau binding only converts and forwards
-- (implemented in C++ as lua binding function)
-- read lua args → call runtime → push result
static int lexsoup_parse(lua_State* L) {
    std::string_view html = nrp::lua::checkString(L, 1);
    Handle handle = nrp::Document::parse(html);       // call runtime
    nrp::lua::pushHandle(L, handle, "Document");      // push result
    return 1;
}

-- ❌ WRONG — duplicated logic in Lua binding
static int lexsoup_parse(lua_State* L) {
    std::string_view html = nrp::lua::checkString(L, 1);
    if (html.empty()) {
        lua_pushnil(L);  // logic in binding!
        return 1;
    }
    Handle handle = nrp::Document::parse(html);
    nrp::lua::pushHandle(L, handle, "Document");
    return 1;
}
```

---

## 6. Object Ownership Model

```
Native (C++) owns ALL objects
       │
       ├── Kotlin only stores handles (Long/jlong)
       │
       └── Luau only stores handles (userdata)

Objects are created by Runtime
Objects are destroyed only by Runtime
Handles become invalid after destruction
```

### Handle Rules
- All handles managed by `HandleManager`
- Handles MUST be validated before use
- Destroyed handles MUST become invalid (no dangling)
- Handle reuse MUST be safe

---

## 7. Memory Management

| Rule | Requirement |
|------|-------------|
| Centralized allocation | Use `SharedAllocator` |
| No heap duplication | Never duplicate DOM structures |
| Clear ownership | Every allocation has exactly one owner |
| Zero leaks | Memory leaks are unacceptable |
| Minimal allocation | Avoid unnecessary object creation |
| No string copying | Prefer `std::string_view` |

---

## 8. Thread Safety

- Document every thread-safe component explicitly
- Avoid global mutable state
- Runtime managers must clearly define synchronization behavior
- Document whether each class is: **thread-safe**, **thread-confined**, or **not thread-safe**

---

## 9. Testing Requirements

Every module MUST include:

| Test Type | Description |
|-----------|-------------|
| Unit Tests | Test each component in isolation |
| Integration Tests | Test interaction between components |
| JNI Tests | Test JNI bridge from Kotlin |
| Memory Leak Tests | Verify no leaks (Valgrind / ASan) |
| Stress Tests | High-load execution tests |
| Performance Benchmarks | Measure parsing, execution, JNI overhead |

> **Rule**: New features require corresponding tests BEFORE merging.

---

## 10. Documentation Requirements

Every public class requires:
```kotlin
/**
 * Represents a parsed HTML document backed by Lexbor.
 *
 * ## Lifecycle
 * Created by [LexSoup.parse]. Must be closed after use.
 *
 * ## Thread Safety
 * Not thread-safe. Do not share across threads.
 *
 * ## Example
 * ```kotlin
 * val doc = LexSoup.parse("<html><title>Hello</title></html>")
 * println(doc.title())  // Hello
 * doc.close()
 * ```
 */
class Document internal constructor(private val handle: Long) : Closeable { ... }
```

Every Runtime component requires:
- **Overview** — what it does
- **Responsibilities** — exactly what it manages
- **Lifecycle** — creation, usage, destruction
- **Usage Examples** — code examples

---

## 11. Logging Standards

```cpp
// Logging levels — all removable in Release builds
NRP_LOG_DEBUG("ParseDocument: html_size={}", html.size());
NRP_LOG_INFO("Document created: handle={}", handle);
NRP_LOG_WARN("Selector returned empty: query={}", query);
NRP_LOG_ERROR("HandleManager: invalid handle={}", handle);
```

| Level | When to use |
|-------|-------------|
| `DEBUG` | Detailed execution info (disabled in Release) |
| `INFO` | Significant events (object creation/destruction) |
| `WARN` | Unexpected but recoverable states |
| `ERROR` | Errors that affect functionality |

---

## 12. Code Review Checklist

Before accepting any implementation, verify ALL of the following:

- [ ] Architecture is respected (native is source of truth)
- [ ] No duplicated logic exists
- [ ] JNI contains NO business logic
- [ ] Kotlin remains thin (no logic)
- [ ] Luau remains thin (no logic)
- [ ] Runtime owns all objects
- [ ] Memory ownership is correct (no leaks, no raw ownership)
- [ ] Tests are included (unit + integration + JNI)
- [ ] Documentation is updated
- [ ] No breaking API changes (or explicitly documented)
- [ ] Thread safety is documented

---

## 13. Implementation Policy

1. Implementation proceeds **incrementally**
2. Every phase **must compile** before continuing
3. No phase may introduce **unfinished APIs**
4. No placeholder implementations unless **explicitly documented**
5. If architectural improvements are discovered:
   1. Update this document
   2. Review the impact
   3. Apply the necessary patches
   4. Continue implementation

> **Architecture always takes precedence over implementation speed.**
