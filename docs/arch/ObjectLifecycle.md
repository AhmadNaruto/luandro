# Object Lifecycle — Luandro NRP Architecture

> **Document:** `docs/arch/ObjectLifecycle.md`
> **Status:** Living document — update when lifecycle contracts change.
> **Scope:** All native objects managed by the `ObjectManager` subsystem across the C++/Kotlin/Luau boundary.

---

## Table of Contents

1. [Philosophy](#1-philosophy)
2. [Ownership Semantics](#2-ownership-semantics)
3. [Handle → Object Resolution Flow](#3-handle--object-resolution-flow)
4. [Lifecycle State Machine](#4-lifecycle-state-machine)
5. [Destruction Guarantees](#5-destruction-guarantees)
6. [Kotlin-Side Lifecycle](#6-kotlin-side-lifecycle)
7. [Luau-Side Lifecycle](#7-luau-side-lifecycle)
8. [Thread Safety](#8-thread-safety)
9. [Error Handling & Use-After-Free Prevention](#9-error-handling--use-after-free-prevention)
10. [Per-Type Lifecycle Examples](#10-per-type-lifecycle-examples)
11. [Ownership Transfer Rules](#11-ownership-transfer-rules)
12. [Reference Table](#12-reference-table)

---

## 1. Philosophy

The Luandro NRP follows a single, non-negotiable lifecycle contract:

```
CREATE → USE → DESTROY
```

Every native object has a **definite, observable end of life**. There is no implicit garbage collection at the C++ layer. Memory ownership is always explicit and traceable.

### Core Tenets

| Tenet | Description |
|---|---|
| **Determinism** | Destruction happens at a predictable point, not at GC whim |
| **Single Ownership** | Exactly one owner holds authority to destroy an object |
| **Opaque Handles** | Foreign runtimes (Kotlin, Luau) never hold raw pointers |
| **Fail-Fast on Misuse** | Invalid handle access throws immediately, never silently corrupts |
| **RAII at Every Layer** | C++ destructors are the final backstop; nothing leaks past them |

The motivation: native objects may own OS-level resources (file descriptors, sockets, compiled regex patterns, JS heap references). Delayed or ambiguous destruction causes resource exhaustion — not merely memory leaks.

---

## 2. Ownership Semantics

### The Golden Rule

> **The C++ layer is the sole owner of every native object.**
> Kotlin and Luau hold *handles* — opaque integer tokens — never raw pointers.

```
 C++ Layer (owns)                Foreign Layer (borrows)
 ┌──────────────────┐            ┌──────────────────────┐
 │  ObjectManager   │            │  Kotlin  │   Luau    │
 │  ┌────────────┐  │  handle_t  │  (jlong) │ (userdata)│
 │  │ Document*  │◄─┼────────────┤    42    │    42     │
 │  │ Pattern*   │  │            │          │           │
 │  │ JSContext* │  │            └──────────┴───────────┘
 │  └────────────┘  │
 └──────────────────┘
```

### What a Handle Is

```cpp
// nrp/core/handle.h
typedef uint64_t handle_t;

constexpr handle_t INVALID_HANDLE = 0;
```

A handle is a 64-bit unsigned integer. The lower bits encode an index into the `ObjectManager`'s internal table; the upper bits encode a **generation counter** that detects stale references.

### What a Handle Is NOT

- It is **not** a pointer. Never cast it to a pointer.
- It is **not** persistent across process restarts.
- It is **not** shareable across processes.
- It is **not** a reference-counted smart pointer — there is no implicit retain/release.

---

## 3. Handle → Object Resolution Flow

Every JNI function and every Luau C-function that receives a handle must resolve it before use. Resolution is the only legitimate path to a raw pointer.

```
Foreign Runtime
      │
      │  handle_t h = 42
      │
      ▼
 ObjectManager::resolve<T>(h)
      │
      ├── [invalid generation?] ──► return nullptr / throw InvalidHandleException
      │
      ├── [wrong type?] ──────────► return nullptr / throw TypeError
      │
      └── [valid] ─────────────────► T* ptr  (usable within this call frame only)
```

### C++ Resolver

```cpp
// nrp/core/object_manager.h

template<typename T>
T* ObjectManager::resolve(handle_t handle) {
    if (handle == INVALID_HANDLE) return nullptr;

    uint32_t index      = static_cast<uint32_t>(handle & 0xFFFFFFFF);
    uint32_t generation = static_cast<uint32_t>(handle >> 32);

    std::shared_lock lock(table_mutex_);

    if (index >= entries_.size()) return nullptr;
    Entry& e = entries_[index];

    if (e.generation != generation) return nullptr;   // stale handle
    if (e.type_id != typeid(T).hash_code()) return nullptr; // type mismatch

    return static_cast<T*>(e.ptr.get());
}
```

### Critical Rule: Pointer Lifetime

The raw pointer returned by `resolve<T>()` is valid **only within the current call frame** while the `ObjectManager` lock is not yet released. It must **never** be stored beyond the call boundary.

```cpp
// CORRECT ✓
void doSomething(handle_t h) {
    Document* doc = ObjectManager::instance().resolve<Document>(h);
    if (!doc) throw InvalidHandleException(h);
    doc->parse();  // used within same frame
}

// WRONG ✗ — storing the pointer
Document* g_doc = nullptr;
void badCache(handle_t h) {
    g_doc = ObjectManager::instance().resolve<Document>(h);  // DANGER
}
```

---

## 4. Lifecycle State Machine

```
                          ┌───────────────────────────────────────┐
                          │           LIFECYCLE STATES            │
                          └───────────────────────────────────────┘

                             ObjectManager::create<T>(...)
                                          │
                                          ▼
                              ┌───────────────────────┐
                              │        CREATED        │
                              │  handle issued        │
                              │  object constructed   │
                              └───────────┬───────────┘
                                          │
                                 first resolve() OK
                                          │
                                          ▼
                              ┌───────────────────────┐
                    ┌────────►│        ACTIVE         │◄────────┐
                    │         │  methods callable     │         │
                    │         │  resolve() returns ptr│         │
                    │         └───────────┬───────────┘         │
                    │                     │                      │
                    │           (re-use; multiple calls)         │
                    └─────────────────────┘                      │
                                          │                      │
                                 destroy() called                │
                                          │                      │
                                          ▼                      │
                              ┌───────────────────────┐         │
                              │        CLOSING        │         │
                              │  generation bumped    │         │
                              │  resolve() → nullptr  │         │
                              │  destructor running   │         │
                              └───────────┬───────────┘         │
                                          │                      │
                                 destructor complete             │
                                          │                      │
                                          ▼                      │
                              ┌───────────────────────┐         │
                              │       DESTROYED       │         │
                              │  slot may be reused   │         │
                              │  handle permanently   │         │
                              │  invalid              │         │
                              └───────────────────────┘         │
                                                                 │
                          (slot reused → new CREATED state) ────┘
```

### State Descriptions

| State | `resolve()` result | `destroy()` allowed |
|---|---|---|
| `CREATED` | Returns valid pointer | Yes |
| `ACTIVE` | Returns valid pointer | Yes |
| `CLOSING` | Returns `nullptr` | No (in progress) |
| `DESTROYED` | Returns `nullptr` | No (slot may be recycled) |

---

## 5. Destruction Guarantees

### RAII Chain

Destruction always follows a strict top-down RAII chain:

```
ObjectManager::destroy(handle)
    │
    ├── 1. Lock table (write lock)
    ├── 2. Bump generation counter (invalidates handle atomically)
    ├── 3. Release write lock
    ├── 4. Move unique_ptr<T> out of table (into local scope)
    └── 5. unique_ptr destructor fires → ~T() called
              │
              ├── ~Document()   → frees DOM tree
              ├── ~Pattern()    → frees compiled regex
              ├── ~JSContext()  → tears down JS heap
              └── ...any OS resources released here
```

Generation bump happens **before** the destructor runs. This ensures that concurrent threads that attempt to resolve the same handle during destruction get `nullptr` safely.

### Destructor Requirements

Every native object's destructor **must**:

1. Release all owned OS/native resources.
2. Not throw exceptions.
3. Not call back into Kotlin or Luau.
4. Be idempotent if re-invoked (defensive guard flag).

```cpp
// nrp/lexsoup/document.cpp
Document::~Document() noexcept {
    if (destroyed_) return;  // idempotency guard
    destroyed_ = true;
    // free DOM tree, release parser state, etc.
}
```

---

## 6. Kotlin-Side Lifecycle

### Constructor → Handle Acquisition

```kotlin
// io/github/luandro/lexsoup/Document.kt
class Document private constructor(private val handle: Long) : AutoCloseable {

    companion object {
        @JvmStatic
        fun parse(html: String): Document {
            val h = nativeParse(html)
            if (h == 0L) throw RuntimeException("Failed to parse HTML")
            return Document(h)
        }

        @JvmStatic private external fun nativeParse(html: String): Long
    }

    fun select(selector: String): List<Element> {
        checkOpen()
        return nativeSelect(handle, selector)
    }

    override fun close() {
        if (handle != 0L) {
            nativeDestroy(handle)
            // Note: handle field is val; JVM GC will collect the wrapper object.
            // The native object is already freed.
        }
    }

    protected fun finalize() {
        // Safety net: if close() was not called explicitly.
        close()
    }

    private fun checkOpen() {
        if (handle == 0L) throw IllegalStateException("Document already closed")
    }

    private external fun nativeSelect(handle: Long, selector: String): List<Element>
    private external fun nativeDestroy(handle: Long)
}
```

### Usage Pattern

```kotlin
// Recommended: use try-with-resources / use {}
Document.parse("<html>...</html>").use { doc ->
    val links = doc.select("a[href]")
    links.forEach { println(it.attr("href")) }
} // close() called automatically here
```

### Finalize as Safety Net

`finalize()` is a **last resort**. It runs only if the caller forgot `close()`. Do not rely on it for timely resource release, as JVM GC timing is non-deterministic. Always prefer explicit `close()`.

---

## 7. Luau-Side Lifecycle

### Userdata as Handle Container

In Luau, every native object is represented as a **full userdata** carrying exactly one `handle_t` value.

```c
// nrp/luau/lexsoup_binding.cpp

// Userdata layout for a Document
typedef struct {
    handle_t handle;   // 8 bytes — the only field
} LuaDocument;

static int lexsoup_parse(lua_State* L) {
    const char* html = luaL_checkstring(L, 1);

    handle_t h = lexsoup_parse_impl(html);
    if (h == INVALID_HANDLE) {
        return luaL_error(L, "lexsoup.parse: failed to parse HTML");
    }

    // Allocate userdata
    LuaDocument* ud = (LuaDocument*)lua_newuserdata(L, sizeof(LuaDocument));
    ud->handle = h;

    // Attach metatable (registered once at module init)
    luaL_getmetatable(L, "lexsoup.Document");
    lua_setmetatable(L, -2);

    return 1;  // push userdata onto stack
}
```

### `__gc` Metamethod — Destruction Bridge

```c
static int document_gc(lua_State* L) {
    LuaDocument* ud = (LuaDocument*)luaL_checkudata(L, 1, "lexsoup.Document");

    if (ud->handle != INVALID_HANDLE) {
        ObjectManager::instance().destroy(ud->handle);
        ud->handle = INVALID_HANDLE;  // mark as freed (idempotency)
    }
    return 0;
}
```

`__gc` fires when the Lua GC collects the userdata. This is the bridge: Lua controls *when* the wrapper is collected; `__gc` ensures the native object is destroyed at that point.

### Full Metatable Design

```c
static const luaL_Reg document_meta[] = {
    { "__gc",       document_gc       },
    { "__tostring", document_tostring },
    { "__len",      document_len      },   // number of root children
    { "__index",    document_index    },   // method dispatch
    { "__newindex", document_newindex },   // property set (or error)
    { NULL, NULL }
};
```

| Metamethod | Purpose |
|---|---|
| `__gc` | Triggers C++ destruction when Lua GC collects userdata |
| `__index` | Dispatches method calls (`doc:select(...)`) |
| `__newindex` | Controls property assignment; usually raises an error |
| `__tostring` | Human-readable representation for debugging |
| `__len` | `#doc` returns meaningful length (e.g., child count) |

---

## 8. Thread Safety

### ObjectManager Locking

The `ObjectManager` uses a **readers-writer lock** (`std::shared_mutex`):

- `resolve<T>()` acquires a **shared (read) lock** — multiple concurrent resolves are allowed.
- `create<T>()` acquires an **exclusive (write) lock** — creation serializes.
- `destroy()` acquires an **exclusive (write) lock** for the generation bump only, then releases before running the destructor.

```
Thread A (resolve)   Thread B (resolve)   Thread C (destroy)
  shared_lock ✓         shared_lock ✓        exclusive_lock
  resolve ok            resolve ok           [waits for A, B to finish]
  use ptr               use ptr              [bumps generation]
  unlock                unlock               [exclusive released]
                                             [destructor runs lock-free]
```

### Destroy During Concurrent Use

If Thread A is using a resolved pointer while Thread C destroys the same handle:

1. Thread C bumps the generation counter (exclusive lock).
2. Thread A already holds the resolved pointer — it was obtained before destruction.
3. Thread A must complete its operation and drop the pointer.
4. Thread C's destructor runs after the exclusive lock is released.

**This is safe** because Thread A's operation holds only a raw pointer to a live object during the exclusive lock acquisition by Thread C. The destruction of the underlying object happens only after Thread C gets the exclusive lock, by which time Thread A's shared lock is released.

> **Rule:** Never hold a resolved pointer across a thread yield or blocking call.

### Kotlin Coroutines

When Kotlin coroutines suspend and resume, they may execute on different threads. Handle this by:
1. Passing the `handle_t` (Long) across suspension points — not resolved pointers.
2. Re-resolving the handle on each JNI call.
3. Checking `IllegalStateException` from `checkOpen()` after any suspension.

---

## 9. Error Handling & Use-After-Free Prevention

### Generation Counter — The Core Defense

The generation counter is the primary defense against use-after-free:

```
handle = (generation << 32) | index

After destroy():
  entries_[index].generation++   ← old handle's generation now mismatches
```

Any attempt to resolve a stale handle returns `nullptr` without undefined behavior.

### Error Taxonomy

| Scenario | C++ Behavior | Kotlin Behavior | Luau Behavior |
|---|---|---|---|
| `INVALID_HANDLE` (0) passed | `resolve()` → `nullptr` | `IllegalArgumentException` | `luaL_error()` |
| Stale handle (already destroyed) | `resolve()` → `nullptr` | `IllegalStateException` | `luaL_error()` |
| Wrong type for handle | `resolve()` → `nullptr` | `IllegalArgumentException` | `luaL_error()` |
| Double destroy | Second destroy is no-op | No exception | No error |

### JNI Error Path

```cpp
// nrp/jni/lexsoup_jni.cpp
extern "C" JNIEXPORT void JNICALL
Java_io_github_luandro_lexsoup_Document_nativeDestroy(JNIEnv* env, jclass, jlong handle) {
    if (handle == 0) return;  // already invalid, nothing to do
    bool ok = ObjectManager::instance().destroy(static_cast<handle_t>(handle));
    if (!ok) {
        // Already destroyed — log but do NOT throw; finalize() may call this twice
        LOGW("nativeDestroy: handle %lld already destroyed or invalid", handle);
    }
}
```

### Double-Close Safety

Both `close()` in Kotlin and `__gc` in Luau may attempt to destroy the same handle (e.g., if `close()` is called explicitly and GC also fires). The `ObjectManager::destroy()` is idempotent:

```cpp
bool ObjectManager::destroy(handle_t handle) {
    // ... generation bump atomically marks it invalid
    // returns false if already invalid — safe to call twice
}
```

---

## 10. Per-Type Lifecycle Examples

### 10.1 Document (lexsoup)

```
create:  ObjectManager::create<Document>(html_string)
active:  doc->parse(), doc->select(selector)
destroy: ObjectManager::destroy(handle) → ~Document() frees DOM tree
```

Typical lifetime: milliseconds to seconds (per-request in server use, per-page in mobile).

### 10.2 Element (lexsoup)

Elements are **not** independently managed. An `Element*` is borrowed from its parent `Document`. Handles to elements are only valid while the parent `Document` is alive.

```
// Element handles encode parent Document handle in high bits
handle_t elem_handle = (doc_handle & DOC_MASK) | (elem_index & ELEM_MASK);
```

Destroying the `Document` invalidates all child element handles.

### 10.3 Pattern (regex)

```
create:  ObjectManager::create<Pattern>(pattern_string, flags)
active:  pattern->match(input), pattern->findAll(input)
destroy: ObjectManager::destroy(handle) → ~Pattern() frees compiled NFA/DFA
```

Patterns are typically long-lived — compiled once, used many times.

### 10.4 Matcher (regex)

A `Matcher` is created from a `Pattern` and an input string. It holds an iterator over match results.

```
create:  ObjectManager::create<Matcher>(pattern_handle, input_string)
active:  matcher->next(), matcher->group(n)
destroy: ObjectManager::destroy(handle) → ~Matcher() frees iterator state
```

Matchers are short-lived (one matching session). Destroying the parent `Pattern` does not destroy existing `Matcher`s — they hold a copy of the compiled automaton by shared reference inside C++ (not via handles).

### 10.5 JSContext (js)

```
create:  ObjectManager::create<JSContext>()
active:  ctx->eval(source), ctx->callFunction(name, args)
destroy: ObjectManager::destroy(handle) → ~JSContext() tears down JS heap, GC, etc.
```

`JSContext` is heavyweight. Typically one per isolate/worker lifetime.

---

## 11. Ownership Transfer Rules

### Rule 1: Unique Ownership

Every native object has **exactly one owner** at any given time. There is no shared ownership (no `shared_ptr` semantics at the handle level).

```
ObjectManager: owner
  → Kotlin: borrower (via handle)
  → Luau:   borrower (via handle in userdata)
```

### Rule 2: No Handle Sharing Between Runtimes

A handle obtained in Kotlin must not be passed into Luau, and vice versa. Each runtime has its own handle namespace for type safety.

> **Rationale:** Kotlin handles go through JNI type conversion; Luau handles are stored in typed userdata. Cross-runtime handle passing bypasses type checks.

### Rule 3: No Transfer of Ownership

There is no "give me ownership of this object" API. Kotlin cannot transfer a native object to Luau or vice versa. If both runtimes need access to the same underlying data, both get handles issued by the same `ObjectManager` call.

### Rule 4: Handle Validity Scope

| Scope | Guarantee |
|---|---|
| Within a single JNI call | Handle guaranteed valid if `resolve()` succeeded |
| Across JNI calls | Must re-validate on each call |
| Across Kotlin `suspend` points | Handle may still be valid; must re-check |
| After `close()` / `__gc` | Handle is permanently invalid |

---

## 12. Reference Table

| Type | Module | Owns Resources | Child Handles Exist? | Typical Lifetime |
|---|---|---|---|---|
| `Document` | lexsoup | DOM tree | Yes (Elements) | Per parse task |
| `Element` | lexsoup | None (borrowed) | No | Bound to Document |
| `Pattern` | regex | Compiled automaton | No | Application lifetime |
| `Matcher` | regex | Iterator state | No | Per-match session |
| `JSContext` | js | JS heap, GC | No | Worker lifetime |

---

*Last updated: 2026-07 · Maintainer: NRP Core Team*
