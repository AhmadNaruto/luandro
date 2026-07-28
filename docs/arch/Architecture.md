# Luandro Native Runtime Platform (NRP) — Architecture

> **Platform**: Android ARM64  
> **Native Core**: C++20  
> **Scripting Frontend**: Luau  
> **Kotlin API**: Thin JNI bridge  

---

## Table of Contents

1. [High-Level Module Diagram](#1-high-level-module-diagram)
2. [Component Responsibilities](#2-component-responsibilities)
3. [Data Flow: Kotlin → JNI → Native](#3-data-flow-kotlin--jni--native)
4. [Module Dependency Graph](#4-module-dependency-graph)
5. [Directory Responsibilities](#5-directory-responsibilities)
6. [Build Architecture](#6-build-architecture)
7. [Key Design Decisions](#7-key-design-decisions)

---

## 1. High-Level Module Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Android Application                          │
├─────────────────────────────────────────────────────────────────────┤
│                    Kotlin API Layer (thin)                           │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────────────────┐  │
│  │ LexSoup  │  │  Regex   │  │ QuickJS  │  │   LuauRuntime      │  │
│  │  (HTML)  │  │          │  │  (JS)    │  │   (Lua scripting)  │  │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └─────────┬──────────┘  │
│       │              │              │                   │             │
│  Handle (jlong)  Handle (jlong) Handle (jlong)   Handle (jlong)     │
└───────┼──────────────┼──────────────┼───────────────────┼───────────┘
        │              │              │                   │
┌───────▼──────────────▼──────────────▼───────────────────▼───────────┐
│                        JNI Bridge Layer                              │
│          (string/handle/exception conversion only)                   │
│  Java_nrp_lexsoup_*   Java_nrp_regex_*   Java_nrp_quickjs_*         │
│  Java_nrp_luau_*                                                     │
└───────────────────────────────────┬──────────────────────────────────┘
                                    │  C API
┌───────────────────────────────────▼──────────────────────────────────┐
│                       NRP Runtime Core (C++20)                       │
│                                                                      │
│  ┌────────────────┐  ┌─────────────────┐  ┌──────────────────────┐  │
│  │  HandleManager │  │  ObjectManager  │  │   ExceptionManager   │  │
│  └────────┬───────┘  └────────┬────────┘  └──────────────────────┘  │
│           │                   │                                      │
│  ┌────────▼───────┐  ┌────────▼────────┐  ┌──────────────────────┐  │
│  │ MemoryManager  │  │  StringManager  │  │    TypeConverter      │  │
│  │ SharedAllocator│  └─────────────────┘  └──────────────────────┘  │
│  └────────────────┘                                                  │
└──────────┬──────────────────┬──────────────────┬─────────────────────┘
           │                  │                  │
┌──────────▼──────┐  ┌────────▼──────┐  ┌────────▼──────────────────┐
│  Engine: LexSoup│  │Engine: jsregexp│  │  Engine: QuickJS / Luau   │
│  (Lexbor HTML)  │  │  (RE2/PCRE)   │  │  (JS / Lua VM)            │
└─────────────────┘  └───────────────┘  └───────────────────────────┘
```

---

## 2. Component Responsibilities

| Component | Layer | Responsibility |
|---|---|---|
| **Kotlin API** | Application | Thin wrapper, hold `jlong` handles, expose idiomatic Kotlin API |
| **JNI Bridge** | Bridge | Convert JVM types ↔ C types, translate exceptions, call runtime C API |
| **HandleManager** | Runtime Core | Assign/validate/invalidate opaque integer handles to native objects |
| **ObjectManager** | Runtime Core | Map handles to typed native objects, lifecycle tracking |
| **MemoryManager** | Runtime Core | Centralized allocation/deallocation, arena-based, leak tracking |
| **SharedAllocator** | Runtime Core | C++ allocator adapter backed by MemoryManager |
| **StringManager** | Runtime Core | String interning, jstring↔std::string conversion cache |
| **ExceptionManager** | Runtime Core | Capture C++ exceptions, translate to structured error payloads for JNI |
| **TypeConverter** | Runtime Core | Bidirectional mapping: Kotlin type ↔ C++ type ↔ Luau type |
| **LexSoup Engine** | Engine | HTML parsing via Lexbor, DOM traversal, CSS selector querying |
| **Regex Engine** | Engine | Regex compilation/matching via jsregexp |
| **QuickJS Engine** | Engine | JavaScript evaluation via QuickJS |
| **Luau VM** | Engine | Luau script execution, binding to runtime objects |

---

## 3. Data Flow: Kotlin → JNI → Native

### 3.1 Synchronous Call Flow

```
Kotlin                  JNI Bridge              Runtime Core          Engine
  │                         │                        │                  │
  │  document.querySelector │                        │                  │
  │  ("div.title")          │                        │                  │
  │──────────────────────►  │                        │                  │
  │  (jlong handle,         │                        │                  │
  │   jstring selector)     │                        │                  │
  │                         │  1. validate jlong     │                  │
  │                         │─────────────────────►  │                  │
  │                         │  Handle h = (Handle)   │                  │
  │                         │  jlong                 │                  │
  │                         │                        │                  │
  │                         │  2. resolve handle     │                  │
  │                         │─────────────────────►  │                  │
  │                         │  Document* doc =       │                  │
  │                         │  ObjectMgr::get(h)     │                  │
  │                         │                        │                  │
  │                         │  3. convert jstring    │                  │
  │                         │  std::string sel =     │                  │
  │                         │  StringMgr::convert(j) │                  │
  │                         │                        │                  │
  │                         │  4. engine call        │                  │
  │                         │────────────────────────────────────────►  │
  │                         │                        │  lexbor_query()  │
  │                         │                        │                  │
  │                         │  5. result handle      │                  │
  │                         │◄────────────────────────────────────────  │
  │                         │  Handle result_h       │                  │
  │                         │                        │                  │
  │  return jlong           │                        │                  │
  │◄────────────────────────│                        │                  │
```

### 3.2 Exception Flow

```
Kotlin                  JNI Bridge              Runtime Core
  │                         │                        │
  │  call()                 │                        │
  │──────────────────────►  │                        │
  │                         │  try { ... }           │                  
  │                         │────────────────────────►  throws NrpException
  │                         │                        │  ◄───────────────
  │                         │  catch(NrpException&)  │
  │                         │  ExceptionMgr::         │
  │                         │  translateToJava(env,e) │
  │                         │  env->ThrowNew(...)     │
  │                         │                        │
  │  ← Java exception       │                        │
  │  caught in Kotlin       │                        │
```

---

## 4. Module Dependency Graph

```
Application (Kotlin)
        │
        ▼
   JNI Bridge
   ┌─────┴──────────────────────────────┐
   │                                    │
   ▼                                    ▼
Runtime Core ◄──────────────────── Engines
   │  ┌──────────────────────────────┐
   │  │ HandleManager                │
   │  │ ObjectManager                │
   │  │   └──► HandleManager         │
   │  │ MemoryManager / SharedAlloc  │
   │  │ StringManager                │
   │  │ ExceptionManager             │
   │  │ TypeConverter                │
   │  └──────────────────────────────┘
   │
   ▼
Platform Allocator (system malloc / jemalloc)
```

**Rules:**
- Engines depend on Runtime Core; Runtime Core does NOT depend on Engines.
- JNI Bridge depends on Runtime Core and Engines.
- Application (Kotlin) depends ONLY on JNI Bridge (via `System.loadLibrary`).
- No circular dependencies anywhere.

---

## 5. Directory Responsibilities

```
luandro/
├── app/                          Android application module
│   └── src/main/
│       ├── kotlin/               Kotlin API wrappers
│       │   └── dev/luandro/nrp/
│       │       ├── core/         Core runtime bindings
│       │       ├── lexsoup/      HTML parser API
│       │       ├── regex/        Regex API
│       │       ├── quickjs/      QuickJS API
│       │       └── luau/         Luau scripting API
│       └── cpp/                  All native code
│           ├── CMakeLists.txt    Top-level build
│           ├── core/             NRP Runtime Core
│           │   ├── handle.h      Handle type definition
│           │   ├── handle_manager.{h,cpp}
│           │   ├── object_manager.{h,cpp}
│           │   ├── memory_manager.{h,cpp}
│           │   ├── shared_allocator.h
│           │   ├── string_manager.{h,cpp}
│           │   ├── exception_manager.{h,cpp}
│           │   └── type_converter.{h,cpp}
│           ├── jni/              JNI bridge
│           │   ├── jni_lexsoup.cpp
│           │   ├── jni_regex.cpp
│           │   ├── jni_quickjs.cpp
│           │   └── jni_luau.cpp
│           ├── engines/          Engine integrations
│           │   ├── lexsoup/      Lexbor HTML engine
│           │   ├── regex/        jsregexp engine
│           │   ├── quickjs/      QuickJS engine
│           │   └── luau/         Luau VM + bindings
│           └── third_party/      Vendored C libraries
│               ├── lexbor/
│               ├── jsregexp/
│               ├── quickjs/
│               └── luau/
├── docs/                         Documentation
│   ├── arch/                     Architecture docs (this dir)
│   └── api/                      API references
└── gradle/                       Build configuration
```

---

## 6. Build Architecture

### 6.1 CMake Module Structure

```cmake
# Top-level CMakeLists.txt
cmake_minimum_required(VERSION 3.22)
project(luandro_nrp CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Third-party libraries (static)
add_subdirectory(third_party/lexbor)
add_subdirectory(third_party/jsregexp)
add_subdirectory(third_party/quickjs)
add_subdirectory(third_party/luau)

# Runtime core (static)
add_subdirectory(core)        # → nrp_core (static)

# Engine integrations (static)
add_subdirectory(engines)     # → nrp_engines (static)

# JNI shared library
add_library(luandro_nrp SHARED
    jni/jni_lexsoup.cpp
    jni/jni_regex.cpp
    jni/jni_quickjs.cpp
    jni/jni_luau.cpp
)
target_link_libraries(luandro_nrp
    nrp_core
    nrp_engines
    lexbor_static
    jsregexp
    quickjs
    luau_vm
    android
    log
)
```

### 6.2 Compiler Flags

| Flag | Reason |
|---|---|
| `-std=c++20` | Language features: concepts, ranges, designated init |
| `-O2` | Release performance |
| `-fvisibility=hidden` | Avoid symbol leakage across DSO boundary |
| `-fstack-protector-strong` | Security |
| `-DANDROID` | Platform guard |
| `-Wall -Wextra -Werror` | Strict compilation |

### 6.3 Link Strategy

All third-party engines are compiled as **static libraries** and linked into a **single shared library** (`luandro_nrp.so`). This:
- Eliminates `dlopen` overhead at runtime.
- Prevents symbol conflicts between engines.
- Simplifies packaging for the APK.

---

## 7. Key Design Decisions

### 7.1 Handles as Opaque `jlong`

**Decision**: All native objects are exposed to Kotlin only as `jlong` handle values; Kotlin never holds a raw pointer.

**Rationale**:
- Pointer invalidation after native destruction is invisible to Kotlin.
- Handles can be validated before use (generation counter or set membership).
- Enables future migration to shared memory without changing Kotlin API.

### 7.2 Native Owns All Memory

**Decision**: All object allocation and deallocation happens in native code. Kotlin never allocates native objects.

**Rationale**:
- GC-managed JVM objects and manually-managed C++ objects have incompatible lifetimes.
- Native finalize (`close()`) must be explicit; relying on GC finalizers for native cleanup is unreliable on Android.
- Explicit ownership eliminates double-free and use-after-free from the JVM side.

### 7.3 JNI Bridge is Conversion-Only

**Decision**: JNI methods contain **no business logic**. They only:
1. Convert `jlong` → `Handle`.
2. Convert `jstring` → `std::string`.
3. Call runtime C++ functions.
4. Convert result → JVM type.
5. Translate C++ exceptions to Java exceptions.

**Rationale**:
- Testability: core logic is unit-testable without JVM.
- Separation of concerns: JNI code is boilerplate-only.
- Native unit tests (GoogleTest) can test the entire stack below JNI.

### 7.4 C++20 as Baseline

**Decision**: Require C++20 features (concepts, `std::span`, designated initializers, `constexpr` improvements).

**Rationale**:
- Android NDK r25+ supports C++20 on ARM64.
- Concepts improve type safety at compile time with no runtime cost.
- `std::span` allows zero-copy buffer passing through the stack.

### 7.5 Single Shared Library

**Decision**: One `.so` file (`luandro_nrp.so`), not one per engine.

**Rationale**:
- Shared JNI state: `HandleManager` and `ObjectManager` are singletons shared across all engines.
- Simpler `System.loadLibrary` call.
- Smaller binary (no duplicated C++ stdlib code).

### 7.6 Luau as Scripting Glue

**Decision**: Luau (not Lua 5.4, not LuaJIT) is the scripting language.

**Rationale**:
- Luau has a typed dialect and a sandbox-safe VM suitable for user-provided scripts.
- Luau's native module system aligns with NRP's engine binding pattern.
- Luau's C API is compatible with Lua 5.1 embedding patterns.

---

*Last updated: 2026-07-28*  
*See also: [Runtime.md](Runtime.md) | [Memory.md](Memory.md) | [JNI.md](JNI.md) | [LuauBinding.md](LuauBinding.md)*
