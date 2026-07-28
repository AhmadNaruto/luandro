# ASL.md — Luandro Native Runtime Platform
# API Specification Language (ASL) Reference
# Phase 0.8 — API Specification Language
#
# ASL is the single source of truth for ALL public APIs in the Luandro NRP.
# No public API may be implemented before its ASL specification exists.
# The Binding Generator consumes only ASL files.

---

## 1. Purpose

The API Specification Language (ASL) formally defines:
- Public Kotlin API
- Native Runtime C++ API
- Luau global module API
- JNI method mapping
- Type conversions
- Object lifecycle
- Documentation

---

## 2. File Format

- **Format**: YAML
- **Location**: `spec/<package>/<ClassName>.yaml`
- **One class per file**

```
spec/
  lexsoup/
    Document.yaml
    Element.yaml
    Elements.yaml
    Node.yaml
  regex/
    Pattern.yaml
    Matcher.yaml
    MatchResult.yaml
  js/
    Runtime.yaml
    Context.yaml
    Module.yaml
  luau/
    VM.yaml
    Script.yaml
```

---

## 3. Root Structure

```yaml
# spec/lexsoup/Document.yaml
version: "1.0"
status: stable          # experimental | stable | deprecated | removed
author: luandro-nrp

package: io.github.luandro.lexsoup
class: Document
description: |
  Represents a parsed HTML document backed by Lexbor.
  All DOM operations execute entirely in native code.

# Lifecycle
lifecycle:
  created_by: LexSoup.parse(html)
  destroyed_by: Document.close() or GC
  owner: NativeRuntime
  handle_type: Long

# Thread safety
thread_safety: not_thread_safe

# Closeable
closeable: true
```

---

## 4. Type System

### Primitives
| ASL Type | Kotlin | JNI | C++ | Luau |
|----------|--------|-----|-----|------|
| `Boolean` | `Boolean` | `jboolean` | `bool` | `boolean` |
| `Int` | `Int` | `jint` | `int32_t` | `number` |
| `Long` | `Long` | `jlong` | `int64_t` | `number` |
| `Double` | `Double` | `jdouble` | `double` | `number` |
| `String` | `String` | `jstring` | `std::string` | `string` |
| `Void` | `Unit` | `void` | `void` | *(no return)* |
| `Handle` | `Long` | `jlong` | `nrp::Handle` | `userdata` |

### Collections
| ASL Type | Kotlin | C++ |
|----------|--------|-----|
| `Array<T>` | `Array<T>` | `std::vector<T>` |
| `List<T>` | `List<T>` | `std::vector<T>` |
| `Map<K,V>` | `Map<K,V>` | `std::unordered_map<K,V>` |
| `Optional<T>` | `T?` | `std::optional<T>` |
| `Nullable<T>` | `T?` | `T*` (nullable) |

### Native Objects
| ASL Type | Handle Class |
|----------|-------------|
| `Document` | `nrp::Document` |
| `Element` | `nrp::Element` |
| `Node` | `nrp::Node` |
| `Pattern` | `nrp::regex::Pattern` |
| `Matcher` | `nrp::regex::Matcher` |
| `JSRuntime` | `nrp::js::Runtime` |
| `JSContext` | `nrp::js::Context` |
| `LuauVM` | `nrp::luau::VM` |

---

## 5. Method Definition

```yaml
methods:
  - name: title
    description: Returns the document title (<title> element text content)
    visibility: public
    static: false
    args: []
    return:
      type: String
      nullable: false
    throws:
      - type: NRPException
        description: If the document has been destroyed
    thread_safety: not_thread_safe
    example: |
      val title = doc.title()  // "Hello World"

  - name: select
    description: Selects elements matching the given CSS selector
    visibility: public
    static: false
    args:
      - name: cssQuery
        type: String
        nullable: false
        description: CSS selector string (e.g. "div.content > p")
    return:
      type: Handle
      handle_class: Elements
      nullable: false
    throws:
      - type: InvalidSelectorException
        description: If the CSS selector is malformed
    thread_safety: not_thread_safe
    example: |
      val links = doc.select("a[href]")
```

---

## 6. Static Functions (Factory Methods)

```yaml
static_functions:
  - name: parse
    description: Parses an HTML string and returns a new Document
    args:
      - name: html
        type: String
        nullable: false
        description: HTML string to parse
    return:
      type: Handle
      handle_class: Document
      nullable: false
    throws:
      - type: ParseException
        description: If HTML cannot be parsed
    example: |
      val doc = LexSoup.parse("<html><body><p>Hello</p></body></html>")
```

---

## 7. Properties

```yaml
properties:
  - name: childCount
    description: Number of direct child nodes
    type: Int
    access: read_only
    nullable: false
    native_backed: true  # computed from native, no Kotlin field

  - name: tagName
    description: Element tag name (e.g. "div", "p", "a")
    type: String
    access: read_only
    nullable: false
    native_backed: true
```

---

## 8. Enums

```yaml
enums:
  - name: NodeType
    description: Type of a DOM node
    values:
      - name: ELEMENT
        value: 1
        description: Element node
      - name: TEXT
        value: 3
        description: Text node
      - name: COMMENT
        value: 8
        description: Comment node
```

---

## 9. Exception Model

```yaml
exceptions:
  - name: NRPException
    kotlin_class: io.github.luandro.NRPException
    extends: RuntimeException
    description: Base exception for all NRP runtime errors

  - name: ParseException
    kotlin_class: io.github.luandro.lexsoup.ParseException
    extends: NRPException
    description: Thrown when HTML or CSS parsing fails

  - name: InvalidHandleException
    kotlin_class: io.github.luandro.InvalidHandleException
    extends: NRPException
    description: Thrown when an invalid/destroyed handle is used
```

---

## 10. Full Example — Document.yaml

```yaml
version: "1.0"
status: stable
author: luandro-nrp

package: io.github.luandro.lexsoup
class: Document
description: |
  Represents a parsed HTML document backed by Lexbor.
  Provides a JSoup-compatible API for DOM traversal and manipulation.
  All operations execute entirely in native code.

lifecycle:
  created_by: LexSoup.parse(html)
  destroyed_by: Document.close()
  owner: NativeRuntime
  handle_type: Long

thread_safety: not_thread_safe
closeable: true

static_functions:
  - name: parse
    description: Parse an HTML string into a Document
    args:
      - name: html
        type: String
        nullable: false
    return:
      type: Handle
      handle_class: Document
      nullable: false
    throws:
      - type: ParseException

methods:
  - name: title
    description: Get the document title
    args: []
    return:
      type: String
      nullable: false

  - name: select
    description: Select elements by CSS selector
    args:
      - name: cssQuery
        type: String
        nullable: false
    return:
      type: Handle
      handle_class: Elements
      nullable: false
    throws:
      - type: InvalidSelectorException

  - name: body
    description: Get the body element
    args: []
    return:
      type: Handle
      handle_class: Element
      nullable: true

  - name: head
    description: Get the head element
    args: []
    return:
      type: Handle
      handle_class: Element
      nullable: true

  - name: outerHtml
    description: Serialize document to HTML string
    args: []
    return:
      type: String
      nullable: false

luau_mapping:
  module_name: lexbor
  object_name: document
  constructor: lexbor.parse(html)
  method_syntax: colon  # doc:title(), doc:select("a")
```

---

## 11. Validation Rules

Before code generation, the validator checks:

| Check | Description |
|-------|-------------|
| Duplicate names | No two methods with same name and signature |
| Invalid types | All types exist in the type system |
| Missing descriptions | All public APIs have descriptions |
| Missing examples | All methods have at least one example |
| Invalid ownership | Lifecycle and ownership are consistent |
| Circular references | No circular type dependencies |
| Breaking changes | Warns when removing/changing existing APIs |

---

## 12. Versioning

Each spec file contains:
```yaml
version: "1.0"
status: stable     # experimental | stable | deprecated | removed
```

| Status | Meaning |
|--------|---------|
| `experimental` | API may change without notice |
| `stable` | API is stable and follows semver |
| `deprecated` | API will be removed in next major version |
| `removed` | API no longer exists (kept for changelog) |
