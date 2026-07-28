# NRP Memory Management

> **Rule #1**: Native owns all memory. Kotlin holds only opaque `jlong` handles.  
> **Rule #2**: Every allocation has a single, explicit owner.  
> **Rule #3**: Destruction is explicit (`close()` from Kotlin or `__gc` from Luau).

---

## Table of Contents

1. [Ownership Model](#1-ownership-model)
2. [Handle Lifecycle Diagram](#2-handle-lifecycle-diagram)
3. [RAII Patterns](#3-raii-patterns)
4. [Allocation Strategy — SharedAllocator](#4-allocation-strategy--sharedallocator)
5. [Memory Leak Prevention Checklist](#5-memory-leak-prevention-checklist)
6. [Smart Pointer Usage Guide](#6-smart-pointer-usage-guide)
7. [String Memory Management](#7-string-memory-management)
8. [JNI Reference Management](#8-jni-reference-management)

---

## 1. Ownership Model

```
┌──────────────────────────────────────────────────────────┐
│                  Kotlin / JVM Heap                        │
│                                                           │
│  LexSoupDocument(handle: Long = 0x0001000100000003L)      │
│      ↑ GC-managed Kotlin object                           │
│      │ holds ONLY a jlong                                 │
└──────┼───────────────────────────────────────────────────┘
       │ JNI call (handle passed as jlong)
┌──────▼───────────────────────────────────────────────────┐
│                  Native Heap                              │
│                                                           │
│  ObjectManager:                                           │
│    Handle 0x0001000100000003 →                            │
│      unique_ptr<Document> ──────► Document { ... }       │
│                                        │                  │
│                                      lexbor_document_t*  │
│                                   (further native alloc)  │
└──────────────────────────────────────────────────────────┘
```

**Kotlin side**:
- A Kotlin object (e.g., `LexSoupDocument`) is GC-managed.
- It holds a `private val handle: Long` only — no pointers.
- When Kotlin calls `close()`, the JNI method is invoked which destroys the native object.
- After `close()`, the `handle` field is set to 0 in Kotlin (sentinel for "already closed").

**Native side**:
- `ObjectManager` holds `unique_ptr<T>` for every live object.
- When `destroy(handle)` is called, `unique_ptr` destructor fires, freeing all nested allocations.
- The engine (e.g., Lexbor) is responsible for its internal tree; the `Document` destructor calls `lxb_html_document_destroy()`.

---

## 2. Handle Lifecycle Diagram

```
  Kotlin: LexSoupDocument.parse(html)
           │
           │ JNI call
           ▼
  [Native] lexsoup::parseDocument(html)
           │
           ├─► MemoryManager::allocate()  ← allocate Document
           │   std::make_unique<Document>(...)
           │
           ├─► HandleManager::allocate(kTypeDocument)
           │   → Handle h = 0x000100010001...
           │
           ├─► ObjectManager::insert(h, std::move(doc_ptr))
           │
           └─► return h
           │
           │ return jlong to Kotlin
           ▼
  Kotlin holds jlong = 0x000100010001...
           │
     [object in use — queries, traversals, etc.]
           │
  Kotlin: document.close()
           │
           │ JNI call
           ▼
  [Native] ObjectManager::destroy(h)
           │
           ├─► unique_ptr<Document> destructor fires
           │   └─► lxb_html_document_destroy(doc)
           │
           ├─► HandleManager::release(h)
           │   slot.generation++
           │   slot.in_use = false
           │
           └─► h is now stale forever
           │
  Kotlin sets handle = 0  ← sentinel "closed"
           │
  Any future Kotlin call on closed document:
     checks handle == 0  → throws IllegalStateException (Kotlin side)
  Any native call with stale jlong:
     HandleManager::is_valid(h) → false
     → throws NrpHandleException
```

---

## 3. RAII Patterns

### 3.1 JNI String RAII Guard

```cpp
// core/jni_utils.h
namespace nrp {

// RAII wrapper for GetStringUTFChars / ReleaseStringUTFChars.
// Usage:
//   JniStringGuard guard(env, jstr);
//   std::string_view sv = guard.view();
class JniStringGuard {
public:
    JniStringGuard(JNIEnv* env, jstring js) : env_{env}, js_{js} {
        if (js) chars_ = env->GetStringUTFChars(js, nullptr);
    }
    ~JniStringGuard() {
        if (js_ && chars_) env_->ReleaseStringUTFChars(js_, chars_);
    }

    [[nodiscard]] std::string_view view() const {
        if (!chars_) throw NrpException{"null jstring"};
        return {chars_};
    }

    [[nodiscard]] std::string str() const { return std::string{view()}; }

    // Non-copyable, non-movable
    JniStringGuard(const JniStringGuard&) = delete;
    JniStringGuard& operator=(const JniStringGuard&) = delete;

private:
    JNIEnv*     env_;
    jstring     js_;
    const char* chars_ = nullptr;
};

} // namespace nrp
```

### 3.2 JNI Local Reference RAII Guard

```cpp
// RAII wrapper for local JNI references created during a JNI call.
class JniLocalRef {
public:
    JniLocalRef(JNIEnv* env, jobject ref) : env_{env}, ref_{ref} {}
    ~JniLocalRef() { if (ref_) env_->DeleteLocalRef(ref_); }

    jobject get()  const noexcept { return ref_; }
    operator bool() const noexcept { return ref_ != nullptr; }

    JniLocalRef(const JniLocalRef&) = delete;
    JniLocalRef& operator=(const JniLocalRef&) = delete;

private:
    JNIEnv* env_;
    jobject ref_;
};
```

### 3.3 Luau State RAII Guard

```cpp
// Used when creating a temporary Luau state for evaluation.
class LuauStateGuard {
public:
    explicit LuauStateGuard(lua_State* L) : L_{L} {}
    ~LuauStateGuard() { if (L_) lua_close(L_); }

    lua_State* get() const noexcept { return L_; }
    LuauStateGuard(const LuauStateGuard&) = delete;

private:
    lua_State* L_;
};
```

---

## 4. Allocation Strategy — SharedAllocator

### 4.1 Why a Custom Allocator?

| Concern | Approach |
|---|---|
| Leak tracking | Every `allocate()` increments `MemoryManager::alloc_count_` |
| Peak usage monitoring | `MemoryManager::peak_bytes_` tracked atomically |
| Hard limit enforcement | Configurable cap; throws `bad_alloc` if exceeded |
| Pluggable | Future: swap to jemalloc arena or mmap pool without API change |

### 4.2 Allocation Path

```
SharedAllocator<T>::allocate(n)
    │
    ▼
MemoryManager::allocate(n * sizeof(T), alignof(T))
    │
    ├─► [DEBUG] record allocation with __builtin_return_address(0)
    │
    ├─► check bytes_allocated_ + n * sizeof(T) <= hard_limit_
    │   if over: throw std::bad_alloc{}
    │
    └─► ::operator new(n * sizeof(T), std::align_val_t{align})
        returns void*
```

### 4.3 When to Use SharedAllocator

```cpp
// ✅ Use SharedAllocator for containers inside engine objects
using ManagedString  = std::basic_string<char, std::char_traits<char>,
                           nrp::SharedAllocator<char>>;
using ManagedVec     = std::vector<std::string, nrp::SharedAllocator<std::string>>;
using ManagedMap     = std::unordered_map<nrp::Handle, ObjectBox,
                           std::hash<nrp::Handle>, std::equal_to<>,
                           nrp::SharedAllocator<std::pair<const nrp::Handle, ObjectBox>>>;

// ❌ Don't use SharedAllocator for temporary on-stack strings
std::string tmp = "hello";   // fine, goes on stack/heap, not tracked
```

---

## 5. Memory Leak Prevention Checklist

### Development Time

- [ ] Every `HandleManager::allocate()` is paired with `HandleManager::release()` (via `ObjectManager::destroy()`).
- [ ] Every `ObjectManager::insert()` has a corresponding path to `ObjectManager::destroy()`.
- [ ] All JNI `GetStringUTFChars` calls use `JniStringGuard` (never raw `ReleaseStringUTFChars`).
- [ ] All JNI local refs created inside a long-running JNI call use `JniLocalRef` or are manually deleted.
- [ ] No global JNI references are created without explicit `DeleteGlobalRef` on cleanup.
- [ ] Luau `__gc` metamethods call `ObjectManager::destroy()` for every userdata object.
- [ ] Engine destructors call their native cleanup API (e.g., `lxb_html_document_destroy`).

### At Shutdown (JNI_OnUnload)

- [ ] `ObjectManager::destroy_all()` is called — destroys all remaining native objects.
- [ ] `StringManager::clear_intern_table()` is called.
- [ ] `MemoryManager::clean()` returns `true` — zero live allocations.

### Debugging Tools

```bash
# Run with AddressSanitizer (ASAN) on device:
# In CMakeLists.txt:
target_compile_options(luandro_nrp PRIVATE -fsanitize=address)
target_link_options(luandro_nrp PRIVATE -fsanitize=address)

# LeakSanitizer output will be written to logcat at process exit.
```

---

## 6. Smart Pointer Usage Guide

### 6.1 `unique_ptr<T>` — Use for exclusive ownership

```cpp
// ✅ ObjectManager owns native objects via unique_ptr.
std::unique_ptr<Document> doc = std::make_unique<Document>(html);
object_mgr_.insert(handle, std::move(doc));  // ownership transferred

// ✅ Engine-internal resources.
struct Document {
    // The Lexbor document is exclusively owned by the Document struct.
    std::unique_ptr<lxb_html_document_t, LexborDocDeleter> lxb_doc;
};

// ❌ Don't use shared_ptr for engine objects — ownership is always single.
```

### 6.2 `shared_ptr<T>` — Use only for shared ownership

```cpp
// ✅ Runtime singleton components shared between subsystems.
std::shared_ptr<MemoryManager> mem_mgr_;

// ❌ Don't use shared_ptr for objects tracked by ObjectManager.
//    ObjectManager already manages lifetime; shared_ptr adds ref-count overhead.
```

### 6.3 Raw pointers — Use only for non-owning observers

```cpp
// ✅ Non-owning pointer: returned from get<T>(), valid only during JNI call.
Document* doc = object_mgr_.get<Document>(h);  // ObjectManager still owns
doc->querySelector(sel);                        // use while handle is valid

// ❌ Don't store the raw pointer across JNI calls.
//    The handle may be destroyed between calls.
```

### 6.4 Decision Matrix

| Scenario | Pointer Type |
|---|---|
| Sole owner of a heap object | `unique_ptr<T>` |
| Shared ownership (rare) | `shared_ptr<T>` |
| Observing an existing object | Raw `T*` (document lifetime clearly) |
| JNI bridge: Kotlin-owned Luau state | `unique_ptr<lua_State, LuaCloser>` |
| C library resource | `unique_ptr<T, CustomDeleter>` |

---

## 7. String Memory Management

### 7.1 String Lifetime Rules

| String source | Lifetime | Who frees |
|---|---|---|
| `GetStringUTFChars(env, js)` | Until `ReleaseStringUTFChars` | JVM (via JniStringGuard) |
| `std::string` local variable | Stack frame | C++ destructor |
| Interned string (`StringManager::intern`) | Runtime lifetime | `StringManager::clear_intern_table()` at shutdown |
| `NewStringUTF(env, chars)` local ref | Until DeleteLocalRef or frame pop | JVM (via JniLocalRef) |
| Luau `lua_tostring` | Until next Lua stack op | Luau VM |

### 7.2 Anti-Patterns

```cpp
// ❌ Returning a pointer to a temporary string
const char* bad_get_name(Handle h) {
    std::string name = compute_name(h);   // local string
    return name.c_str();                   // DANGLING after function returns
}

// ✅ Return by value
std::string good_get_name(Handle h) {
    return compute_name(h);
}

// ❌ Storing JNI chars past the JNI call
const char* stored_chars = env->GetStringUTFChars(js, nullptr);
// ... later, after returning from JNI function:
printf("%s", stored_chars);   // INVALID — JVM has no reference anymore

// ✅ Copy immediately
std::string safe_str = nrp::JniStringGuard{env, js}.str();
```

---

## 8. JNI Reference Management

### 8.1 Reference Types

| Type | Created by | Lives in | Lifetime |
|---|---|---|---|
| **Local ref** | Most JNI calls | JNI local frame | Until `DeleteLocalRef` or JNI return |
| **Global ref** | `NewGlobalRef` | Native heap | Until `DeleteGlobalRef` |
| **Weak global ref** | `NewWeakGlobalRef` | Native heap | Collected when JVM decides |

### 8.2 Rules

1. **Never store a local ref across JNI calls.** Local refs are invalid once the JNI method returns.
2. **Use global refs only for caching `jclass` or `jmethodID`** that you cache at `JNI_OnLoad` time.
3. **Delete global refs at `JNI_OnUnload`** or when the cached object is no longer needed.
4. **If creating many local refs in a loop**, use `PushLocalFrame` / `PopLocalFrame` or call `DeleteLocalRef` each iteration.

### 8.3 Cached JNI IDs Pattern

```cpp
// jni/jni_cache.h  — populated at JNI_OnLoad
namespace nrp::jni {

struct JniCache {
    // Exception classes (global refs)
    jclass NrpException_class;
    jclass NrpHandleException_class;
    jclass NrpParseException_class;

    // Exception constructors
    jmethodID NrpException_ctor;

    // String class
    jclass String_class;
};

extern JniCache g_jni_cache;

void init_jni_cache(JNIEnv* env);   // called in JNI_OnLoad
void clear_jni_cache(JNIEnv* env);  // called in JNI_OnUnload

} // namespace nrp::jni
```

```cpp
// jni/jni_cache.cpp
void init_jni_cache(JNIEnv* env) {
    auto find = [&](const char* name) -> jclass {
        jclass local = env->FindClass(name);
        return static_cast<jclass>(env->NewGlobalRef(local));
    };
    g_jni_cache.NrpException_class       = find("dev/luandro/nrp/NrpException");
    g_jni_cache.NrpHandleException_class = find("dev/luandro/nrp/NrpHandleException");
    g_jni_cache.NrpException_ctor        =
        env->GetMethodID(g_jni_cache.NrpException_class, "<init>", "(Ljava/lang/String;)V");
}

void clear_jni_cache(JNIEnv* env) {
    env->DeleteGlobalRef(g_jni_cache.NrpException_class);
    env->DeleteGlobalRef(g_jni_cache.NrpHandleException_class);
}
```

---

*See also: [Architecture.md](Architecture.md) | [Runtime.md](Runtime.md) | [ObjectLifecycle.md](ObjectLifecycle.md)*
