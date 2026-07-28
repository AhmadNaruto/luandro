# LexSoup — Public API Reference
## `io.github.luandro.lexsoup`

> **Status:** Specified (Phase 2 ✅)  
> **Engine:** Lexbor  
> **JSoup compatibility:** Yes

---

## Overview

LexSoup is an HTML parser library that provides a JSoup-compatible API powered by
the native Lexbor engine (C library). All parsing, DOM traversal, querying, and
serialization happen in native code; Kotlin and Luau hold only **handles**.

---

## Classes

| Class | Description |
|-------|-------------|
| `LexSoup` | Static entry point — `parse()` factory |
| `Document` | Root document node |
| `Element` | Individual HTML element (tag + attributes + children) |
| `Elements` | Ordered collection of Element objects (iterable) |
| `Node` | Base type for Document and Element |

---

## `LexSoup` — Static Entry Point

```kotlin
object LexSoup {
    fun parse(html: String): Document
    fun parse(html: String, baseUri: String): Document
}
```

### Luau
```lua
lexsoup.parse(html)  -- returns Document userdata
```

---

## `Document` — Root HTML Document

```kotlin
class Document : AutoCloseable {
    fun title(): String
    fun body(): Element?
    fun head(): Element?
    fun select(cssQuery: String): Elements
    fun getElementById(id: String): Element?
    fun getElementsByTag(tag: String): Elements
    fun getElementsByClass(cls: String): Elements
    fun outerHtml(): String
    fun text(): String
    fun close()
}
```

### Parameters & Return Values

| Method | Parameters | Returns | Throws |
|--------|-----------|---------|--------|
| `title()` | — | `String` | `DocumentClosedException` |
| `body()` | — | `Element?` | `DocumentClosedException` |
| `head()` | — | `Element?` | `DocumentClosedException` |
| `select(cssQuery)` | `cssQuery: String` | `Elements` | `DocumentClosedException`, `CssSyntaxException` |
| `getElementById(id)` | `id: String` | `Element?` | `DocumentClosedException` |
| `getElementsByTag(tag)` | `tag: String` | `Elements` | `DocumentClosedException` |
| `getElementsByClass(cls)` | `cls: String` | `Elements` | `DocumentClosedException` |
| `outerHtml()` | — | `String` | `DocumentClosedException` |
| `text()` | — | `String` | `DocumentClosedException` |
| `close()` | — | `void` | — |

---

## `Element` — HTML Element

```kotlin
class Element : AutoCloseable {
    // Tag / attributes
    fun tagName(): String
    fun attr(key: String): String
    fun attr(key: String, value: String): Element
    fun hasAttr(key: String): Boolean
    fun removeAttr(key: String): Element
    fun attributes(): Map<String, String>
    fun id(): String
    fun className(): String
    fun classNames(): Set<String>
    fun hasClass(cls: String): Boolean

    // Content
    fun text(): String
    fun ownText(): String
    fun html(): String
    fun outerHtml(): String
    fun innerHTML(): String

    // DOM traversal
    fun parent(): Element?
    fun children(): Elements
    fun child(index: Int): Element
    fun childrenSize(): Int
    fun firstElementChild(): Element?
    fun lastElementChild(): Element?
    fun nextElementSibling(): Element?
    fun previousElementSibling(): Element?
    fun siblingElements(): Elements

    // Querying
    fun select(cssQuery: String): Elements
    fun selectFirst(cssQuery: String): Element?
    fun is(cssQuery: String): Boolean
    fun closest(cssQuery: String): Element?

    // DOM modification
    fun text(value: String): Element
    fun html(value: String): Element
    fun append(html: String): Element
    fun prepend(html: String): Element
    fun after(html: String): Element
    fun before(html: String): Element
    fun remove()
    fun wrap(html: String): Element
    fun unwrap(): Element?

    fun close()
}
```

---

## `Elements` — Collection of Elements

```kotlin
class Elements : Iterable<Element>, AutoCloseable {
    fun size(): Int
    fun isEmpty(): Boolean
    fun first(): Element?
    fun last(): Element?
    fun get(index: Int): Element
    fun select(cssQuery: String): Elements
    fun attr(key: String): String
    fun attr(key: String, value: String): Elements
    fun hasAttr(key: String): Boolean
    fun text(): String
    fun outerHtml(): String
    fun toList(): List<Element>
    fun close()
}
```

---

## Exceptions

| Exception | Parent | When |
|-----------|--------|------|
| `DocumentClosedException` | `IllegalStateException` | Method called on closed Document |
| `ElementClosedException` | `IllegalStateException` | Method called on closed Element |
| `CssSyntaxException` | `IllegalArgumentException` | Invalid CSS selector syntax |

---

## Lifecycle

```
LexSoup.parse(html)
    │
    ▼
Document (ACTIVE)
    │
    ├── select(query) → Elements
    │       │
    │       └── [0], [1], ... → Element
    │
    └── close() → DESTROYED (all child Elements also invalid)
```

> **Rule:** `Document` owns all `Element` and `Elements` objects. Calling
> `Document.close()` invalidates all derived elements. Never close an Element
> before the Document.

---

## Kotlin Usage Examples

```kotlin
import io.github.luandro.lexsoup.LexSoup

// Basic parsing
val doc = LexSoup.parse("<html><body><h1 class='main'>Hello</h1><p>World</p></body></html>")
println(doc.title())                    // ""
println(doc.select("h1").first()?.text())  // "Hello"
println(doc.select("h1").attr("class"))    // "main"
doc.close()

// AutoCloseable
LexSoup.parse("<p id='intro'>NRP</p>").use { doc ->
    println(doc.getElementById("intro")?.text())  // "NRP"
}

// DOM traversal
LexSoup.parse("<ul><li>A</li><li>B</li><li>C</li></ul>").use { doc ->
    doc.select("li").forEach { li ->
        println(li.text())
    }
}

// DOM modification
LexSoup.parse("<div>original</div>").use { doc ->
    doc.select("div").first()?.text("modified")
    println(doc.body()?.outerHtml())
}
```

---

## Luau Usage Examples

```lua
-- Basic parsing
local doc = lexsoup.parse("<h1>Hello</h1><p>World</p>")
print(doc:select("h1"):first():text())  -- Hello
print(doc:select("p"):text())           -- World
doc:close()

-- Iteration
local doc2 = lexsoup.parse("<ul><li>A</li><li>B</li></ul>")
local items = doc2:select("li")
for i = 1, items:size() do
    print(items:get(i - 1):text())
end
doc2:close()

-- Attributes
local doc3 = lexsoup.parse([[<a href="https://example.com" class="link">Click</a>]])
local link = doc3:select("a"):first()
print(link:attr("href"))   -- https://example.com
print(link:text())         -- Click
doc3:close()
```

---

## CSS Selector Support

LexSoup supports a JSoup-compatible subset of CSS3 selectors:

| Selector | Description |
|----------|-------------|
| `tag` | Elements by tag name |
| `#id` | Element by id attribute |
| `.class` | Elements by class name |
| `[attr]` | Elements with attribute present |
| `[attr=val]` | Elements where attr equals val |
| `[attr^=val]` | Attr starts with val |
| `[attr$=val]` | Attr ends with val |
| `[attr*=val]` | Attr contains val |
| `a b` | `b` descendant of `a` |
| `a > b` | `b` direct child of `a` |
| `a + b` | `b` immediately after `a` |
| `a, b` | Either `a` or `b` |
| `:first-child` | First child element |
| `:last-child` | Last child element |
| `:nth-child(n)` | nth child |
| `:not(selector)` | Negation |
| `:contains(text)` | Text content contains |
| `:has(selector)` | Has descendant matching |
