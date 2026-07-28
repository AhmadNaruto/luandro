# Luau Binding Layer Architecture — Luandro NRP

> **Document:** `docs/arch/LuauBinding.md`
> **Status:** Living document — update when new Luau modules are added.
> **Scope:** All C functions registering native objects and modules into the `LuauVM`.

---

## Table of Contents

1. [Philosophy](#1-philosophy)
2. [Luau Userdata Pattern](#2-luau-userdata-pattern)
3. [Metatable Design](#3-metatable-design)
4. [Module Registration Pattern](#4-module-registration-pattern)
5. [Handle Storage in Userdata](#5-handle-storage-in-userdata)
6. [__gc and the Destruction Bridge](#6-__gc-and-the-destruction-bridge)
7. [Error Handling](#7-error-handling)
8. [Auto-Registration in LuauVM](#8-auto-registration-in-luauvm)
9. [Type Checking](#9-type-checking)
10. [Naming Convention](#10-naming-convention)
11. [Method Table Pattern](#11-method-table-pattern)
12. [Example: Document Userdata Wrapper](#12-example-document-userdata-wrapper)
13. [Example: lexsoup Module Registration](#13-example-lexsoup-module-registration)
14. [Return Value Conventions](#14-return-value-conventions)
15. [Coroutine Safety](#15-coroutine-safety)
16. [Memory Model](#16-memory-model)
17. [Quick Reference](#17-quick-reference)

---

## 1. Philosophy

### The Thin Layer Mandate

> **The Luau binding layer does exactly two things: type conversion and dispatch.**
> It is equally thin as the JNI layer. No business logic lives here.

The binding layer is the bridge between Luau's dynamic type system and the C++ core:

```
 Luau Script              Binding (thin)                C++ Core
 ───────────              ──────────────                ────────
 local doc =             check args              ──►   ObjectManager::create
   lexsoup.parse(html)   allocate userdata              <Document>(html)
                         store handle          ◄──    return handle_t
                         return userdata
                    ◄──
 doc:select("a")         checkudata             ──►   Document::select(css)
                         resolve handle
                         convert result         ◄──   return vector<handle_t>
                    ◄──  push table
```

### Design Rules

| Rule | Rationale |
|---|---|
| No business logic in binding functions | Keeps C++ core testable in isolation |
| Every `lua_CFunction` wraps all C++ calls in `try/catch` | Prevents C++ exceptions from corrupting Lua state |
| Type check via `luaL_checkudata` at every method entry | Prevents type confusion attacks |
| `handle_t` is the only field in userdata | Minimal footprint; all state in `ObjectManager` |
| Metatables registered once at module init, never again | `lua_newuserdata` must find them by name |

---

## 2. Luau Userdata Pattern

### What Is a Luau Userdata?

A Luau **full userdata** is a block of raw memory allocated by the Lua runtime, optionally associated with a **metatable**. The Luau GC owns and manages this memory block.

We use userdata as a typed wrapper around a single `handle_t`:

```
Lua heap
┌───────────────────────────────────────┐
│  userdata (full)                      │
│  ┌──────────────────────────────────┐ │
│  │  handle_t handle  (8 bytes)      │ │
│  └──────────────────────────────────┘ │
│  metatable: "lexsoup.Document"        │
└───────────────────────────────────────┘
          │
          │ ObjectManager::resolve<Document>(handle)
          │
          ▼
C++ heap: Document object (owns DOM tree, etc.)
```

### Userdata Struct Convention

Every native type gets a minimal C struct:

```c
// nrp/luau/lexsoup_binding.h

typedef struct {
    handle_t handle;
} LuaDocument;

typedef struct {
    handle_t handle;
} LuaElement;

typedef struct {
    handle_t handle;
} LuaPattern;

typedef struct {
    handle_t handle;
} LuaMatcher;

typedef struct {
    handle_t handle;
} LuaJSContext;
```

All structs are identical — the type safety comes from the **metatable name**, not the struct layout.

---

## 3. Metatable Design

### Registration (once, at module init)

```c
// Create and register a metatable with the given name
static void register_metatable(lua_State* L,
                                const char* mt_name,
                                const luaL_Reg* meta_funcs,
                                const luaL_Reg* index_funcs) {
    // Create metatable and store in registry: registry[mt_name] = {}
    luaL_newmetatable(L, mt_name);

    // Set metamethods (__gc, __tostring, __len, __newindex)
    luaL_setfuncs(L, meta_funcs, 0);

    // Create __index table and populate with methods
    lua_newtable(L);
    luaL_setfuncs(L, index_funcs, 0);
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);  // pop metatable
}
```

### Full Metatable Structure for Document

```c
// Metamethods table (special Lua operations)
static const luaL_Reg document_meta[] = {
    { "__gc",       document_gc       },   // GC hook → destroy native
    { "__tostring", document_tostring },   // tostring(doc) → human readable
    { "__len",      document_len      },   // #doc → child count
    { "__newindex", document_newindex },   // doc.x = y → error (immutable)
    { "__index",    NULL              },   // set below to method table
    { NULL, NULL }
};

// Method table (callable via doc:method())
static const luaL_Reg document_methods[] = {
    { "select",      document_select      },
    { "selectFirst", document_selectFirst },
    { "text",        document_text        },
    { "html",        document_html        },
    { "close",       document_close       },   // explicit early close
    { NULL, NULL }
};
```

### Metamethod Responsibilities

| Metamethod | Trigger | Behavior |
|---|---|---|
| `__gc` | Lua GC collects userdata | Destroy native handle; idempotent |
| `__index` | `obj.field` or `obj:method()` | Dispatch to method table |
| `__newindex` | `obj.field = value` | Raise error (objects are immutable from Lua) |
| `__tostring` | `tostring(obj)` | Return `"lexsoup.Document(handle=42)"` |
| `__len` | `#obj` | Return meaningful integer (child count, etc.) |

### __newindex as Read-Only Guard

```c
static int document_newindex(lua_State* L) {
    return luaL_error(L, "lexsoup.Document: fields are read-only");
}
```

This prevents accidental `doc.select = something` from corrupting the dispatch table.

---

## 4. Module Registration Pattern

### Pattern: lua_newtable + lua_setglobal

Each module is registered as a global table in the Lua state:

```
Global: lexsoup = {
    parse   = <C function>,
    version = "1.0.0",
}

Global: regex = {
    compile = <C function>,
    escape  = <C function>,
}

Global: js = {
    newContext = <C function>,
}
```

### Implementation

```c
// nrp/luau/lexsoup_binding.cpp

static const luaL_Reg lexsoup_module_funcs[] = {
    { "parse",   lexsoup_parse   },   // lexsoup.parse(html) → Document
    { "version", lexsoup_version },   // lexsoup.version() → string
    { NULL, NULL }
};

int luaopen_lexsoup(lua_State* L) {
    // 1. Register metatables for all types in this module
    register_metatable(L, "lexsoup.Document", document_meta, document_methods);
    register_metatable(L, "lexsoup.Element",  element_meta,  element_methods);

    // 2. Create module table
    lua_newtable(L);
    luaL_setfuncs(L, lexsoup_module_funcs, 0);

    // 3. Set version string
    lua_pushstring(L, LEXSOUP_VERSION);
    lua_setfield(L, -2, "_VERSION");

    // 4. Set as global "lexsoup"
    lua_setglobal(L, "lexsoup");

    return 0;
}
```

---

## 5. Handle Storage in Userdata

### typedef

```c
// nrp/core/handle.h (shared with Luau binding)
typedef uint64_t handle_t;
#define INVALID_HANDLE ((handle_t)0)
```

### Allocation

```c
// Helper: allocate userdata with a given metatable
static void* push_userdata(lua_State* L, size_t size, const char* mt_name) {
    void* ud = lua_newuserdata(L, size);
    luaL_getmetatable(L, mt_name);
    lua_setmetatable(L, -2);
    return ud;
}

// Usage inside lexsoup_parse:
LuaDocument* ud = (LuaDocument*)push_userdata(L, sizeof(LuaDocument),
                                               "lexsoup.Document");
ud->handle = h;
```

### Access in Methods

```c
// At the start of every method, retrieve and validate the userdata:
static LuaDocument* check_document(lua_State* L, int idx) {
    LuaDocument* ud = (LuaDocument*)luaL_checkudata(L, idx, "lexsoup.Document");
    if (ud->handle == INVALID_HANDLE) {
        luaL_error(L, "lexsoup.Document: object already closed");
    }
    return ud;
}
```

---

## 6. __gc and the Destruction Bridge

### The Bridge

```
Lua GC decides to collect userdata
        │
        ▼
  __gc metamethod called
        │
        ▼
  document_gc(lua_State* L)
        │
        ├── luaL_checkudata → get LuaDocument*
        ├── check handle != INVALID_HANDLE
        ├── ObjectManager::instance().destroy(ud->handle)
        └── ud->handle = INVALID_HANDLE  (idempotency)
```

### Implementation

```c
static int document_gc(lua_State* L) {
    // luaL_checkudata is safe here: Lua guarantees __gc receives
    // the userdata as argument 1.
    LuaDocument* ud = (LuaDocument*)luaL_checkudata(L, 1, "lexsoup.Document");

    if (ud->handle != INVALID_HANDLE) {
        // Wrap in try/catch — destructors must not throw into Lua
        try {
            ObjectManager::instance().destroy(ud->handle);
        } catch (...) {
            // Log but do not propagate — GC must not be interrupted
            // (Luau does not support longjmp from __gc)
        }
        ud->handle = INVALID_HANDLE;
    }
    return 0;
}
```

### Why __gc Must Not Error

In Luau, raising a Lua error inside `__gc` via `lua_error` or `luaL_error` results in **undefined behavior** (the error is silently swallowed or causes a panic). Always catch all C++ exceptions in `__gc` and log them without re-throwing.

### Explicit close() in Lua

Users may want deterministic early release:

```c
static int document_close(lua_State* L) {
    LuaDocument* ud = (LuaDocument*)luaL_checkudata(L, 1, "lexsoup.Document");
    if (ud->handle != INVALID_HANDLE) {
        ObjectManager::instance().destroy(ud->handle);
        ud->handle = INVALID_HANDLE;
    }
    return 0;
}
```

```lua
-- Lua usage
local doc = lexsoup.parse(html)
doc:close()      -- deterministic release; __gc becomes a no-op
```

---

## 7. Error Handling

### Standard Pattern

Every `lua_CFunction` that calls into C++ must wrap the call:

```c
static int lexsoup_parse(lua_State* L) {
    const char* html = luaL_checkstring(L, 1);

    handle_t h;
    try {
        h = ObjectManager::instance().create<Document>(html);
    } catch (const std::bad_alloc&) {
        return luaL_error(L, "lexsoup.parse: out of memory");
    } catch (const std::exception& e) {
        return luaL_error(L, "lexsoup.parse: %s", e.what());
    } catch (...) {
        return luaL_error(L, "lexsoup.parse: unknown native error");
    }

    if (h == INVALID_HANDLE) {
        return luaL_error(L, "lexsoup.parse: failed to parse HTML");
    }

    LuaDocument* ud = (LuaDocument*)push_userdata(L, sizeof(LuaDocument),
                                                   "lexsoup.Document");
    ud->handle = h;
    return 1;
}
```

### `luaL_error` vs `lua_error`

| Function | When to Use |
|---|---|
| `luaL_error(L, fmt, ...)` | Formatted error message from a binding function |
| `lua_error(L)` | Re-raise an error already on the stack |
| `luaL_argerror(L, n, msg)` | Wrong argument type/value at position `n` |
| `luaL_typerror(L, n, tname)` | Wrong type at position `n` (legacy; prefer `luaL_argerror`) |

### Invalid Handle Error

```c
static int document_select(lua_State* L) {
    LuaDocument* ud = check_document(L, 1);   // errors if handle invalid
    const char*  css = luaL_checkstring(L, 2);

    std::vector<handle_t> results;
    try {
        Document* doc = ObjectManager::instance().resolve<Document>(ud->handle);
        if (!doc) return luaL_error(L, "lexsoup.Document:select: object closed");
        results = doc->select(css);
    } catch (const std::invalid_argument& e) {
        return luaL_error(L, "lexsoup.Document:select: invalid selector: %s", e.what());
    } catch (const std::exception& e) {
        return luaL_error(L, "lexsoup.Document:select: %s", e.what());
    }

    // Push result as Lua table of Element userdata
    lua_newtable(L);
    for (size_t i = 0; i < results.size(); ++i) {
        LuaElement* eu = (LuaElement*)push_userdata(L, sizeof(LuaElement),
                                                     "lexsoup.Element");
        eu->handle = results[i];
        lua_rawseti(L, -2, (int)(i + 1));
    }
    return 1;
}
```

---

## 8. Auto-Registration in LuauVM

### LuauVM Initialization

The `LuauVM` class registers all known native modules automatically on construction:

```cpp
// nrp/luau/luau_vm.cpp

LuauVM::LuauVM() {
    L_ = luaL_newstate();
    luaL_openlibs(L_);  // standard Lua libraries

    // Auto-register NRP native modules
    luaopen_lexsoup(L_);   // registers global "lexsoup"
    luaopen_regex(L_);     // registers global "regex"
    luaopen_js(L_);        // registers global "js"
}
```

### Module Availability

After `LuauVM` construction, Lua scripts may immediately use:

```lua
-- All available without require()
local doc     = lexsoup.parse(html)
local pattern = regex.compile("[a-z]+")
local ctx     = js.newContext()
```

No `require()` call is needed — modules are globals, not packages. This is intentional: NRP Lua scripts are sandboxed micro-scripts, not full Lua programs.

### Adding a New Module

1. Implement `luaopen_<module>(lua_State*)` in `nrp/luau/<module>_binding.cpp`.
2. Add the call to `LuauVM::LuauVM()`.
3. Add the global name to the Naming Convention table below.

---

## 9. Type Checking

### luaL_checkudata

`luaL_checkudata(L, arg, tname)` checks that:
1. The value at stack position `arg` is a full userdata.
2. Its metatable matches the one registered under `tname`.

If either check fails, it raises a Lua error automatically.

```c
// Safe: errors if not a lexsoup.Document
LuaDocument* ud = (LuaDocument*)luaL_checkudata(L, 1, "lexsoup.Document");

// Safe: errors if not a lexsoup.Element
LuaElement* eu = (LuaElement*)luaL_checkudata(L, 1, "lexsoup.Element");
```

### testudata for Optional Checks

```c
// Returns NULL if type mismatch (does not error)
LuaDocument* ud = (LuaDocument*)luaL_testudata(L, 1, "lexsoup.Document");
if (!ud) {
    // not a Document — handle polymorphism or give friendly error
}
```

### Stack Position Conventions

| Position | Contents |
|---|---|
| `1` | `self` (the userdata) for method calls `obj:method()` |
| `2..n` | Method arguments in order |
| Return | Values pushed onto stack; count returned from `lua_CFunction` |

---

## 10. Naming Convention

### Lua Globals

| Module | Global Name | Registered By |
|---|---|---|
| HTML parsing | `lexsoup` | `luaopen_lexsoup()` |
| Regular expressions | `regex` | `luaopen_regex()` |
| JavaScript engine | `js` | `luaopen_js()` |

### Metatable Names (Registry Keys)

| Type | Metatable Name |
|---|---|
| lexsoup Document | `"lexsoup.Document"` |
| lexsoup Element | `"lexsoup.Element"` |
| regex Pattern | `"regex.Pattern"` |
| regex Matcher | `"regex.Matcher"` |
| js JSContext | `"js.JSContext"` |

### C Function Names

Internal C functions follow the convention:

```
<module>_<Type>_<action>   → method bound in metatable
<module>_<action>          → module-level function
```

Examples:

```
lexsoup_parse              → lexsoup.parse(html)
document_select            → bound as Document:select(css)
document_gc                → __gc for Document
regex_compile              → regex.compile(pattern, flags)
pattern_match              → bound as Pattern:match(input)
```

---

## 11. Method Table Pattern

### Static Array of lua_CFunction

```c
// nrp/luau/regex_binding.cpp

static const luaL_Reg pattern_methods[] = {
    { "match",      pattern_match      },  // Pattern:match(str) → boolean, captures...
    { "findAll",    pattern_findAll    },  // Pattern:findAll(str) → table of Matchers
    { "split",      pattern_split      },  // Pattern:split(str) → table of strings
    { "source",     pattern_source     },  // Pattern:source() → string
    { "close",      pattern_close      },  // Pattern:close() → void (early GC)
    { NULL, NULL }
};

static const luaL_Reg pattern_meta[] = {
    { "__gc",       pattern_gc         },
    { "__tostring", pattern_tostring   },
    { "__newindex", pattern_newindex   },
    { NULL, NULL }
};
```

### __index as Method Dispatch

The `__index` field in the metatable is set to the methods table. When Lua evaluates `pattern:match(...)`, it:

1. Looks up `match` in the `pattern` userdata — not found (userdata has no string fields).
2. Falls to `__index` metamethod.
3. `__index` is a table — looks up `"match"` in the methods table.
4. Finds the C function and calls it.

```
pattern:match(str)
  → __index["match"] → pattern_match(L)
  → C++ Pattern::match(str)
  → push results
```

---

## 12. Example: Document Userdata Wrapper

Full implementation of the `lexsoup.Document` binding:

```c
// nrp/luau/lexsoup_binding.cpp

// ── Helpers ────────────────────────────────────────────────────────────────

static LuaDocument* check_document(lua_State* L, int idx) {
    LuaDocument* ud = (LuaDocument*)luaL_checkudata(L, idx, "lexsoup.Document");
    if (ud->handle == INVALID_HANDLE)
        luaL_error(L, "lexsoup.Document: object already closed");
    return ud;
}

// ── Metamethods ─────────────────────────────────────────────────────────────

static int document_gc(lua_State* L) {
    LuaDocument* ud = (LuaDocument*)luaL_checkudata(L, 1, "lexsoup.Document");
    if (ud->handle != INVALID_HANDLE) {
        try { ObjectManager::instance().destroy(ud->handle); } catch (...) {}
        ud->handle = INVALID_HANDLE;
    }
    return 0;
}

static int document_tostring(lua_State* L) {
    LuaDocument* ud = (LuaDocument*)luaL_checkudata(L, 1, "lexsoup.Document");
    lua_pushfstring(L, "lexsoup.Document(handle=%I)", (lua_Integer)ud->handle);
    return 1;
}

static int document_len(lua_State* L) {
    LuaDocument* ud = check_document(L, 1);
    Document* doc   = ObjectManager::instance().resolve<Document>(ud->handle);
    if (!doc) return luaL_error(L, "lexsoup.Document: handle invalid");
    lua_pushinteger(L, (lua_Integer)doc->childCount());
    return 1;
}

static int document_newindex(lua_State* L) {
    return luaL_error(L, "lexsoup.Document: fields are read-only");
}

// ── Methods ─────────────────────────────────────────────────────────────────

static int document_select(lua_State* L) {
    LuaDocument* ud  = check_document(L, 1);
    const char*  css = luaL_checkstring(L, 2);

    Document* doc = ObjectManager::instance().resolve<Document>(ud->handle);
    if (!doc) return luaL_error(L, "lexsoup.Document:select: object closed");

    std::vector<handle_t> results;
    try {
        results = doc->select(css);
    } catch (const std::exception& e) {
        return luaL_error(L, "lexsoup.Document:select: %s", e.what());
    }

    lua_newtable(L);
    for (size_t i = 0; i < results.size(); i++) {
        LuaElement* eu = (LuaElement*)lua_newuserdata(L, sizeof(LuaElement));
        eu->handle = results[i];
        luaL_getmetatable(L, "lexsoup.Element");
        lua_setmetatable(L, -2);
        lua_rawseti(L, -2, (int)(i + 1));
    }
    return 1;
}

static int document_close(lua_State* L) {
    LuaDocument* ud = (LuaDocument*)luaL_checkudata(L, 1, "lexsoup.Document");
    if (ud->handle != INVALID_HANDLE) {
        try { ObjectManager::instance().destroy(ud->handle); } catch (...) {}
        ud->handle = INVALID_HANDLE;
    }
    return 0;
}

// ── Tables ──────────────────────────────────────────────────────────────────

static const luaL_Reg document_meta_funcs[] = {
    { "__gc",       document_gc       },
    { "__tostring", document_tostring },
    { "__len",      document_len      },
    { "__newindex", document_newindex },
    { NULL, NULL }
};

static const luaL_Reg document_index_funcs[] = {
    { "select",  document_select  },
    { "close",   document_close   },
    { NULL, NULL }
};
```

### Lua Usage

```lua
local html = [[<html><body><p class="intro">Hello</p></body></html>]]

local doc = lexsoup.parse(html)

-- Method call via __index dispatch
local paragraphs = doc:select("p.intro")
for i, el in ipairs(paragraphs) do
    print(i, el:text())
end

-- Explicit release (optional — __gc handles it otherwise)
doc:close()
```

---

## 13. Example: lexsoup Module Registration

```c
// ── Module-level functions ───────────────────────────────────────────────────

static int lexsoup_parse(lua_State* L) {
    const char* html = luaL_checkstring(L, 1);

    handle_t h;
    try {
        h = ObjectManager::instance().create<Document>(std::string(html));
    } catch (const std::bad_alloc&) {
        return luaL_error(L, "lexsoup.parse: out of memory");
    } catch (const std::exception& e) {
        return luaL_error(L, "lexsoup.parse: %s", e.what());
    }

    if (h == INVALID_HANDLE)
        return luaL_error(L, "lexsoup.parse: failed to create Document");

    LuaDocument* ud = (LuaDocument*)lua_newuserdata(L, sizeof(LuaDocument));
    ud->handle = h;
    luaL_getmetatable(L, "lexsoup.Document");
    lua_setmetatable(L, -2);
    return 1;
}

static int lexsoup_version(lua_State* L) {
    lua_pushstring(L, LEXSOUP_VERSION_STRING);
    return 1;
}

static const luaL_Reg lexsoup_funcs[] = {
    { "parse",   lexsoup_parse   },
    { "version", lexsoup_version },
    { NULL, NULL }
};

// ── Registration entry point ─────────────────────────────────────────────────

int luaopen_lexsoup(lua_State* L) {
    // Register metatables (stored in Lua registry)
    luaL_newmetatable(L, "lexsoup.Document");
    luaL_setfuncs(L, document_meta_funcs, 0);
    lua_newtable(L);
    luaL_setfuncs(L, document_index_funcs, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    luaL_newmetatable(L, "lexsoup.Element");
    luaL_setfuncs(L, element_meta_funcs, 0);
    lua_newtable(L);
    luaL_setfuncs(L, element_index_funcs, 0);
    lua_setfield(L, -2, "__index");
    lua_pop(L, 1);

    // Create and populate module table
    lua_newtable(L);
    luaL_setfuncs(L, lexsoup_funcs, 0);

    lua_pushstring(L, LEXSOUP_VERSION_STRING);
    lua_setfield(L, -2, "_VERSION");

    // Publish as global
    lua_pushvalue(L, -1);
    lua_setglobal(L, "lexsoup");

    return 1;  // leave module table on stack (in case used as require() result)
}
```

---

## 14. Return Value Conventions

| Scenario | Stack State | Return Count |
|---|---|---|
| Single value (handle/userdata) | Push 1 value | `return 1` |
| Boolean result | `lua_pushboolean(L, b)` | `return 1` |
| String result | `lua_pushstring(L, s)` | `return 1` |
| Integer result | `lua_pushinteger(L, n)` | `return 1` |
| Table of results | `lua_newtable` + populate | `return 1` |
| Multiple returns | Push each in order | `return N` |
| Void / no result | Push nothing | `return 0` |
| Error | `return luaL_error(L, ...)` | Never returns |
| Nil (not found) | `lua_pushnil(L)` | `return 1` |

### Multiple Return Example

```c
// regex Pattern:match(str) → matched(bool), capture1, capture2, ...
static int pattern_match(lua_State* L) {
    LuaPattern* ud  = check_pattern(L, 1);
    const char* str = luaL_checkstring(L, 2);

    MatchResult result = resolve_and_match(ud->handle, str);

    if (!result.matched) {
        lua_pushboolean(L, 0);
        return 1;
    }

    lua_pushboolean(L, 1);
    for (const std::string& cap : result.captures) {
        lua_pushstring(L, cap.c_str());
    }
    return (int)(1 + result.captures.size());  // bool + N captures
}
```

---

## 15. Coroutine Safety

### Problem

Luau supports coroutines (`coroutine.yield` / `coroutine.resume`). A binding function that calls `lua_yield()` or is resumed from a different OS thread introduces concurrency concerns.

### NRP Policy

> **NRP Luau binding functions are not coroutine-suspending.**
> All native calls are synchronous and blocking from Lua's perspective.

Rationale: native C++ functions cannot safely call `lua_yield()` mid-execution. The Lua stack would be left in an inconsistent state if the coroutine is abandoned.

### Safe Patterns

```c
// CORRECT ✓ — fully synchronous, no yield
static int document_select(lua_State* L) {
    // ... resolve, call C++, push results, return
    return 1;
}

// WRONG ✗ — yielding inside a C binding
static int bad_async_fetch(lua_State* L) {
    lua_yield(L, 0);   // unsafe in this architecture
    return 0;
}
```

### Coroutines and Handle Lifetime

If a Lua coroutine holds a reference to a userdata and the coroutine is abandoned (never resumed), the userdata will be collected by the GC eventually, triggering `__gc`. This is correct and safe — the GC handles cleanup regardless of coroutine state.

---

## 16. Memory Model

```
 Luau GC                      C++ ObjectManager
 ───────────────────          ──────────────────────────────
 Allocates userdata           Allocates native objects
 (LuaDocument, 8 bytes)       (Document, Element, Pattern, ...)
        │                              │
        │  handle_t (64-bit integer)   │
        └──────────────────────────────►
        │                              │
        │                              │ owns (unique_ptr<T>)
        │                              │
 GC collects userdata                 │
        │                              │
        ▼                              │
   __gc fires                          │
        │                              │
        └── destroy(handle) ──────────►│
                                       │
                              unique_ptr destructor
                              → ~Document() runs
                              → OS resources freed
```

### Who Owns What

| Resource | Owner |
|---|---|
| `LuaDocument` struct (8 bytes) | Luau GC |
| `Document` object (DOM tree, etc.) | `ObjectManager` (`unique_ptr<Document>`) |
| `handle_t` value | Copied into userdata (it's just an integer) |

The Luau GC controls **when** `__gc` fires. The `ObjectManager` controls **what** is destroyed. These are decoupled — the GC only destroys the 8-byte shell; the actual native resource is destroyed by the `ObjectManager` in response to `__gc`'s `destroy()` call.

---

## 17. Quick Reference

### Checklist for a New Native Type

- [ ] Define `LuaXxx` struct with single `handle_t handle` field
- [ ] Implement `__gc` with try/catch, idempotency guard
- [ ] Implement `__tostring` returning `"module.Type(handle=N)"`
- [ ] Implement `__newindex` raising "fields are read-only"
- [ ] Implement `__len` if object has meaningful size
- [ ] Implement `check_xxx(L, idx)` helper using `luaL_checkudata`
- [ ] Populate `static const luaL_Reg xxx_meta[]` and `xxx_methods[]`
- [ ] Register metatable via `luaL_newmetatable` in `luaopen_<module>`
- [ ] Add module-level constructor (e.g., `lexsoup.parse`)
- [ ] Add to `LuauVM` constructor (`luaopen_<module>(L_)`)

### Stack Discipline Rules

| Rule | Description |
|---|---|
| Balance the stack | Every `lua_push*` must be matched by a pop or a return |
| Check before push | Verify argument count with `lua_gettop(L)` when variadic |
| Use `lua_rawseti` in loops | Faster than `lua_settable` for sequential array building |
| Delete locals in JNI-equivalent | Luau manages its stack automatically; no explicit "local ref" release |

---

*Last updated: 2026-07 · Maintainer: NRP Core Team*
