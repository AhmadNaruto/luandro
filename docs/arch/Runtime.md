# NRP Runtime Core — Architecture

> Runtime Core is the C++20 layer that sits below the JNI bridge and above the engine integrations.  
> It provides: handle management, object lifecycle, memory, strings, exceptions, and type conversion.

---

## Table of Contents

1. [Component Overview](#1-component-overview)
2. [Component Interaction Diagram](#2-component-interaction-diagram)
3. [HandleManager](#3-handlemanager)
4. [ObjectManager](#4-objectmanager)
5. [MemoryManager / SharedAllocator](#5-memorymanager--sharedallocator)
6. [StringManager](#6-stringmanager)
7. [ExceptionManager](#7-exceptionmanager)
8. [TypeConverter](#8-typeconverter)
9. [Internal C++ API Reference](#9-internal-c-api-reference)

---

## 1. Component Overview

| Component | File(s) | Singleton? | Purpose |
|---|---|---|---|
| `HandleManager` | `handle_manager.h/.cpp` | Yes (per-runtime) | Opaque handle allocation and validation |
| `ObjectManager` | `object_manager.h/.cpp` | Yes (per-runtime) | Map handle → typed native object |
| `MemoryManager` | `memory_manager.h/.cpp` | Yes (per-runtime) | Centralized allocator, leak tracking |
| `SharedAllocator<T>` | `shared_allocator.h` | No (template) | STL-compatible allocator backed by MemoryManager |
| `StringManager` | `string_manager.h/.cpp` | Yes (per-runtime) | JNI string conversion, interning |
| `ExceptionManager` | `exception_manager.h/.cpp` | No (utilities) | C++ → Java exception translation |
| `TypeConverter` | `type_converter.h/.cpp` | No (utilities) | Type mapping across layers |

All singletons are accessed via `nrp::Runtime::get()`, which holds the single Runtime instance for the process lifetime.

---

## 2. Component Interaction Diagram

```
JNI Bridge
    │
    │  Java_nrp_lexsoup_Document_querySelector(env, obj, jhandle, jselector)
    │
    ▼
┌───────────────────────────────────────────────────────────────┐
│  JNI Entry Point                                              │
│  1. TypeConverter::fromJlong(jhandle)  → Handle               │
│  2. StringManager::fromJstring(env, jselector) → std::string  │
│  3. ObjectManager::get<Document>(handle) → Document*          │
│  4. Document::querySelector(selector) → Handle (result)       │
│  5. Return TypeConverter::toJlong(result_handle)              │
│  ─ if any step throws: ExceptionManager::throwToJava(env, e)  │
└──────────────────────────┬────────────────────────────────────┘
                           │
          ┌────────────────▼────────────────┐
          │        ObjectManager            │
          │  unordered_map<Handle, ObjBox>  │
          └────────────────┬────────────────┘
                           │ lookup
          ┌────────────────▼────────────────┐
          │        HandleManager            │
          │  dense slot array + generation  │
          └────────────────┬────────────────┘
                           │ validate
          ┌────────────────▼────────────────┐
          │        MemoryManager            │
          │  arena pool + tracked allocs    │
          └─────────────────────────────────┘
```

---

## 3. HandleManager

### 3.1 What is a Handle?

A `Handle` is a **64-bit opaque integer** composed of two parts:

```
  63            32  31            16  15             0
  ┌──────────────────┬──────────────┬────────────────┐
  │  generation (32) │  type tag(16)│  slot index(16)│
  └──────────────────┴──────────────┴────────────────┘
```

- **slot index**: Position in the dense handle slot array (max 65 535 active objects).
- **type tag**: Engine-assigned object type (Document=1, NodeList=2, Regex=3, ...).
- **generation**: Monotonically incrementing counter per slot, prevents use-after-free.

```cpp
// core/handle.h
namespace nrp {

using Handle = uint64_t;
constexpr Handle kInvalidHandle = 0;

constexpr uint16_t handle_slot(Handle h)       { return h & 0xFFFF; }
constexpr uint16_t handle_type(Handle h)       { return (h >> 16) & 0xFFFF; }
constexpr uint32_t handle_generation(Handle h) { return (h >> 32); }

constexpr Handle make_handle(uint16_t slot, uint16_t type, uint32_t gen) {
    return (static_cast<uint64_t>(gen) << 32)
         | (static_cast<uint64_t>(type) << 16)
         | static_cast<uint64_t>(slot);
}

} // namespace nrp
```

### 3.2 HandleManager API

```cpp
// core/handle_manager.h
namespace nrp {

class HandleManager {
public:
    // Allocate a new handle for an object of the given type.
    // Returns kInvalidHandle if capacity is exhausted.
    Handle allocate(uint16_t type_tag) noexcept;

    // Release a handle, incrementing its generation counter.
    // After this call, the old handle value is permanently invalid.
    void release(Handle h) noexcept;

    // Check if a handle is currently valid (slot in use, generation matches).
    [[nodiscard]] bool is_valid(Handle h) const noexcept;

    // Return the type tag encoded in a handle (does NOT validate).
    [[nodiscard]] uint16_t type_of(Handle h) const noexcept;

    // Return count of currently live handles (for leak detection).
    [[nodiscard]] size_t live_count() const noexcept;

private:
    struct Slot {
        uint32_t generation = 0;
        uint16_t type_tag   = 0;
        bool     in_use     = false;
    };

    std::array<Slot, 65536> slots_{};
    std::vector<uint16_t>   free_list_;
    mutable std::mutex      mutex_;
};

} // namespace nrp
```

### 3.3 Handle Lifecycle

```
HandleManager::allocate(type)
        │
        ▼
  Pop slot from free_list
  Increment slot.generation
  Set slot.in_use = true
        │
        ▼
  Return make_handle(slot, type, generation)
        │
  [Object in use by Kotlin / Luau]
        │
        ▼
HandleManager::release(h)
  Validate slot matches generation
  Set slot.in_use = false
  Push slot back to free_list
        │
        ▼
  Old handle h is now stale.
  Any future is_valid(h) → false
```

### 3.4 Thread Safety

`HandleManager` is thread-safe: all public methods acquire `mutex_`. This permits multiple JNI threads (e.g., Android worker threads calling into NRP) to safely share the runtime.

---

## 4. ObjectManager

### 4.1 Responsibilities

- Owns the association between a `Handle` and a typed native object.
- Calls the destructor of the native object when a handle is destroyed.
- Provides type-safe retrieval: `get<T>(handle)` throws if the type tag doesn't match `T`.

### 4.2 Object Box

Each registered object is wrapped in an `ObjectBox`, which stores:
- A `std::unique_ptr<void, Deleter>` to the heap-allocated object.
- The expected type tag (for validation in `get<T>`).
- A `std::string` debug name (only in `DEBUG` builds).

```cpp
// core/object_manager.h
namespace nrp {

class ObjectManager {
public:
    // Register a newly created object; takes ownership.
    // handle must have been allocated by HandleManager.
    template<typename T>
    void insert(Handle h, std::unique_ptr<T> obj);

    // Retrieve a typed pointer. Throws NrpException if handle invalid or type mismatch.
    template<typename T>
    [[nodiscard]] T* get(Handle h) const;

    // Destroy the object associated with handle and release the handle.
    // Safe to call multiple times (idempotent after first call).
    void destroy(Handle h) noexcept;

    // Destroy all objects (called at runtime shutdown).
    void destroy_all() noexcept;

    [[nodiscard]] size_t object_count() const noexcept;

private:
    struct ObjectBox {
        std::unique_ptr<void, void(*)(void*)> ptr;
        uint16_t type_tag;
    };

    mutable std::shared_mutex          mutex_;
    std::unordered_map<Handle, ObjectBox> objects_;
    HandleManager&                     handle_mgr_;
};

} // namespace nrp
```

### 4.3 Type Tag Registry

Each engine registers its object types with a compile-time constant:

```cpp
// engines/lexsoup/types.h
namespace nrp::lexsoup {
    constexpr uint16_t kTypeDocument = 0x0101;
    constexpr uint16_t kTypeNodeList = 0x0102;
    constexpr uint16_t kTypeNode     = 0x0103;
}

// engines/regex/types.h
namespace nrp::regex {
    constexpr uint16_t kTypeRegex    = 0x0201;
    constexpr uint16_t kTypeMatch    = 0x0202;
}
```

Type tags are 16-bit. High byte = engine ID, low byte = type within engine. This namespacing ensures no collision across engines.

---

## 5. MemoryManager / SharedAllocator

### 5.1 MemoryManager

`MemoryManager` is a thin wrapper around platform `malloc`/`free` with:
- **Allocation tracking** (DEBUG only): counts outstanding allocations for leak detection.
- **Hard limit** (configurable): throws `std::bad_alloc` if a cap is exceeded.
- **Statistics API**: reports peak usage, current usage, total allocations.

```cpp
// core/memory_manager.h
namespace nrp {

class MemoryManager {
public:
    [[nodiscard]] void* allocate(size_t bytes, size_t align = alignof(std::max_align_t));
    void                deallocate(void* ptr, size_t bytes) noexcept;

    // Statistics
    [[nodiscard]] size_t bytes_allocated() const noexcept;
    [[nodiscard]] size_t peak_bytes()      const noexcept;
    [[nodiscard]] size_t alloc_count()     const noexcept;

    // Leak check: returns true if all allocations were freed.
    [[nodiscard]] bool clean() const noexcept;

private:
    std::atomic<size_t> bytes_allocated_{0};
    std::atomic<size_t> peak_bytes_{0};
    std::atomic<size_t> alloc_count_{0};
    size_t              hard_limit_{SIZE_MAX};
};

} // namespace nrp
```

### 5.2 SharedAllocator

`SharedAllocator<T>` is an STL-compatible allocator that delegates to `MemoryManager`. Use it wherever a `std::vector`, `std::unordered_map`, or `std::string` should be tracked by the memory manager.

```cpp
// core/shared_allocator.h
namespace nrp {

template<typename T>
class SharedAllocator {
public:
    using value_type = T;
    explicit SharedAllocator(MemoryManager& mgr) noexcept : mgr_{mgr} {}

    template<typename U>
    SharedAllocator(const SharedAllocator<U>& other) noexcept : mgr_{other.mgr_} {}

    [[nodiscard]] T* allocate(std::size_t n) {
        return static_cast<T*>(mgr_.allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T* p, std::size_t n) noexcept {
        mgr_.deallocate(p, n * sizeof(T));
    }

    MemoryManager& mgr_;
};

} // namespace nrp
```

---

## 6. StringManager

### 6.1 Responsibilities

- Convert `jstring` → `std::string` (UTF-8, handles null terminator correctly).
- Convert `std::string` / `std::string_view` → `jstring`.
- Optional **string interning** for frequently repeated strings (CSS selectors, tag names).

### 6.2 API

```cpp
// core/string_manager.h
namespace nrp {

class StringManager {
public:
    // JNI → C++: returns UTF-8 std::string, throws on null.
    [[nodiscard]] std::string from_jstring(JNIEnv* env, jstring js) const;

    // C++ → JNI: returns local jstring reference.
    [[nodiscard]] jstring to_jstring(JNIEnv* env, std::string_view sv) const;

    // Intern a string: returns a stable string_view into the intern table.
    // Interned strings live for the lifetime of the StringManager.
    [[nodiscard]] std::string_view intern(std::string_view sv);

    // Clear the intern table (called at runtime shutdown).
    void clear_intern_table() noexcept;

private:
    std::unordered_set<std::string> intern_table_;
    mutable std::shared_mutex       mutex_;
};

} // namespace nrp
```

### 6.3 Conversion Pattern

```cpp
// Correct pattern inside a JNI function:
std::string StringManager::from_jstring(JNIEnv* env, jstring js) const {
    if (!js) throw NrpException{"null jstring passed to from_jstring"};

    const char* chars = env->GetStringUTFChars(js, nullptr);
    if (!chars) throw NrpException{"GetStringUTFChars returned null"};

    std::string result{chars};
    env->ReleaseStringUTFChars(js, chars);   // Always release
    return result;
}
```

**Key rule**: `GetStringUTFChars` must always be paired with `ReleaseStringUTFChars`. `StringManager` encapsulates this to prevent leaks.

---

## 7. ExceptionManager

### 7.1 NRP Exception Hierarchy

```
std::exception
    └── NrpException
            ├── NrpHandleException     (invalid handle)
            ├── NrpTypeException       (type mismatch)
            ├── NrpParseException      (parse failure)
            ├── NrpScriptException     (script runtime error)
            └── NrpMemoryException     (allocation failure)
```

```cpp
// core/exception_manager.h
namespace nrp {

class NrpException : public std::exception {
public:
    explicit NrpException(std::string msg, std::string code = "NRP_ERROR")
        : msg_{std::move(msg)}, code_{std::move(code)} {}

    const char* what() const noexcept override { return msg_.c_str(); }
    const std::string& code() const noexcept   { return code_; }

private:
    std::string msg_;
    std::string code_;
};

// ... subclass declarations ...

// Translation: call this at the bottom of every JNI catch block.
namespace ExceptionManager {
    void throw_to_java(JNIEnv* env, const NrpException& e) noexcept;
    void throw_to_java(JNIEnv* env, const std::exception& e) noexcept;
    void throw_unknown_to_java(JNIEnv* env) noexcept;
}

} // namespace nrp
```

### 7.2 Java Exception Classes

| C++ Exception | Java Class |
|---|---|
| `NrpException` | `dev.luandro.nrp.NrpException` |
| `NrpHandleException` | `dev.luandro.nrp.NrpHandleException` |
| `NrpTypeException` | `dev.luandro.nrp.NrpTypeException` |
| `NrpParseException` | `dev.luandro.nrp.NrpParseException` |
| `NrpScriptException` | `dev.luandro.nrp.NrpScriptException` |
| `NrpMemoryException` | `dev.luandro.nrp.NrpMemoryException` |
| `std::bad_alloc` | `java.lang.OutOfMemoryError` |
| any other | `dev.luandro.nrp.NrpException` ("unknown") |

### 7.3 Standard JNI Try/Catch Template

```cpp
JNIEXPORT jlong JNICALL Java_dev_luandro_nrp_lexsoup_Document_querySelector(
    JNIEnv* env, jobject, jlong jhandle, jstring jselector)
{
    try {
        auto& rt = nrp::Runtime::get();
        auto h   = nrp::TypeConverter::from_jlong(jhandle);
        auto sel = rt.strings().from_jstring(env, jselector);
        auto doc = rt.objects().get<lexsoup::Document>(h);
        auto res = doc->querySelector(sel);
        return nrp::TypeConverter::to_jlong(res);
    }
    catch (const nrp::NrpException& e) { nrp::ExceptionManager::throw_to_java(env, e); }
    catch (const std::exception&    e) { nrp::ExceptionManager::throw_to_java(env, e); }
    catch (...)                        { nrp::ExceptionManager::throw_unknown_to_java(env); }
    return 0;  // unreachable; exception is pending in JVM
}
```

---

## 8. TypeConverter

### 8.1 Type Mapping Table

| Kotlin / Java Type | JNI Type | C++ Type | Luau Type |
|---|---|---|---|
| `Long` (handle) | `jlong` | `nrp::Handle` (uint64_t) | `userdata` (lightuserdata ptr) |
| `Int` | `jint` | `int32_t` | `number` (integer) |
| `Long` | `jlong` | `int64_t` | `number` (integer) |
| `Double` | `jdouble` | `double` | `number` (float) |
| `Boolean` | `jboolean` | `bool` | `boolean` |
| `String` | `jstring` | `std::string` / `std::string_view` | `string` |
| `ByteArray` | `jbyteArray` | `std::span<const uint8_t>` | `buffer` (via Luau buffer lib) |
| `Array<T>` | `jobjectArray` | `std::vector<T>` | `table` |
| `null` | `nullptr` | `std::nullopt` / `nullptr` | `nil` |

### 8.2 TypeConverter API

```cpp
// core/type_converter.h
namespace nrp {

struct TypeConverter {
    // Handle <-> jlong
    static Handle      from_jlong(jlong v) noexcept;
    static jlong       to_jlong(Handle h)  noexcept;

    // Numeric
    static int32_t     from_jint(jint v)        noexcept;
    static jint        to_jint(int32_t v)        noexcept;
    static int64_t     from_jlong_int(jlong v)   noexcept;
    static jlong       to_jlong_int(int64_t v)   noexcept;
    static double      from_jdouble(jdouble v)   noexcept;
    static jdouble     to_jdouble(double v)       noexcept;
    static bool        from_jboolean(jboolean v) noexcept;
    static jboolean    to_jboolean(bool v)        noexcept;

    // ByteArray <-> span
    static std::vector<uint8_t> from_jbytearray(JNIEnv* env, jbyteArray arr);
    static jbyteArray           to_jbytearray(JNIEnv* env, std::span<const uint8_t> data);

    // Luau value <-> C++  (lua_State* is the Luau VM state)
    static Handle      luau_check_handle(lua_State* L, int idx, uint16_t expected_type);
    static void        luau_push_handle(lua_State* L, Handle h);
    static std::string luau_check_string(lua_State* L, int idx);
    static void        luau_push_string(lua_State* L, std::string_view sv);
    static int32_t     luau_check_integer(lua_State* L, int idx);
    static void        luau_push_integer(lua_State* L, int32_t v);
    static double      luau_check_number(lua_State* L, int idx);
    static void        luau_push_number(lua_State* L, double v);
    static bool        luau_check_boolean(lua_State* L, int idx);
    static void        luau_push_boolean(lua_State* L, bool v);
};

} // namespace nrp
```

---

## 9. Internal C++ API Reference

### 9.1 Runtime Singleton

```cpp
// core/runtime.h
namespace nrp {

class Runtime {
public:
    // Access the global runtime instance (created at JNI_OnLoad).
    static Runtime& get();

    // Initialize / destroy (called from JNI_OnLoad / JNI_OnUnload).
    static void initialize();
    static void destroy() noexcept;

    HandleManager&  handles()  noexcept;
    ObjectManager&  objects()  noexcept;
    MemoryManager&  memory()   noexcept;
    StringManager&  strings()  noexcept;

private:
    Runtime() = default;
    ~Runtime() = default;

    std::unique_ptr<HandleManager>  handle_mgr_;
    std::unique_ptr<ObjectManager>  object_mgr_;
    std::unique_ptr<MemoryManager>  memory_mgr_;
    std::unique_ptr<StringManager>  string_mgr_;

    static std::unique_ptr<Runtime> instance_;
    static std::once_flag           init_flag_;
};

} // namespace nrp
```

### 9.2 JNI OnLoad / OnUnload

```cpp
// jni/jni_init.cpp
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    nrp::Runtime::initialize();
    // Register JNI methods (if using dynamic registration)
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM* /*vm*/, void* /*reserved*/) {
    nrp::Runtime::destroy();
}
```

---

*See also: [Architecture.md](Architecture.md) | [Memory.md](Memory.md) | [JNI.md](JNI.md)*
