# Regex — Public API Reference
## `io.github.luandro.regex`

> **Status:** Specified (Phase 2 ✅)  
> **Engine:** jsregexp (POSIX ERE + JavaScript regex extensions)  
> **Java.util.regex compatibility:** Yes (subset)

---

## Overview

The `io.github.luandro.regex` module provides a Java-style regex API backed
by the native jsregexp C library. All regex compilation and matching runs in
native code; Kotlin and Luau hold only **handles**. The API mirrors
`java.util.regex.Pattern` and `java.util.regex.Matcher`.

---

## Classes

| Class | Description |
|-------|-------------|
| `Regex` | Static entry point — convenience factory and match functions |
| `Pattern` | Compiled regex pattern (immutable, thread-unsafe native handle) |
| `Matcher` | Stateful engine for matching Pattern against an input string |
| `MatchResult` | Immutable snapshot of a single match (groups, start, end) |

---

## `Regex` — Static Entry Point

```kotlin
object Regex {
    fun compile(pattern: String): Pattern
    fun compile(pattern: String, flags: Int): Pattern
    fun matches(pattern: String, input: String): Boolean
    fun find(pattern: String, input: String): MatchResult?
    fun findAll(pattern: String, input: String): List<MatchResult>
    fun replaceAll(pattern: String, input: String, replacement: String): String
    fun replaceFirst(pattern: String, input: String, replacement: String): String
    fun split(pattern: String, input: String): Array<String>
    fun split(pattern: String, input: String, limit: Int): Array<String>

    // Flags (bitwise OR)
    const val CASE_INSENSITIVE: Int = 0x02
    const val MULTILINE: Int        = 0x08
    const val DOTALL: Int           = 0x20
    const val UNICODE_CASE: Int     = 0x40
}
```

### Luau
```lua
regex.compile(pattern)            -- Pattern userdata
regex.compile(pattern, flags)     -- Pattern userdata
regex.matches(pattern, input)     -- boolean
regex.find(pattern, input)        -- MatchResult or nil
regex.replaceAll(pattern, input, replacement) -- string
```

---

## `Pattern` — Compiled Regex

```kotlin
class Pattern : AutoCloseable {
    val pattern: String   // the source pattern string
    val flags: Int        // the compilation flags

    fun matcher(input: String): Matcher
    fun matches(input: String): Boolean
    fun split(input: String): Array<String>
    fun split(input: String, limit: Int): Array<String>
    fun asPredicate(): (String) -> Boolean

    fun close()

    companion object {
        fun compile(pattern: String): Pattern
        fun compile(pattern: String, flags: Int): Pattern
        fun quote(literal: String): String   // escape all metacharacters
    }
}
```

### Parameters & Return Values

| Method | Parameters | Returns | Throws |
|--------|-----------|---------|--------|
| `matcher(input)` | `input: String` | `Matcher` | `PatternClosedException` |
| `matches(input)` | `input: String` | `Boolean` | `PatternClosedException` |
| `split(input)` | `input: String` | `Array<String>` | `PatternClosedException` |
| `split(input, limit)` | `input: String`, `limit: Int` | `Array<String>` | `PatternClosedException` |
| `close()` | — | `void` | — |

---

## `Matcher` — Stateful Match Engine

```kotlin
class Matcher : AutoCloseable {
    val pattern: Pattern
    val input: String
    val hasMatch: Boolean

    fun matches(): Boolean
    fun find(): Boolean
    fun findFrom(startIndex: Int): Boolean
    fun lookingAt(): Boolean

    fun group(): String?
    fun groupByIndex(groupIndex: Int): String?
    fun groupCount(): Int
    fun start(): Int
    fun end(): Int

    fun reset(): Matcher
    fun resetWithInput(input: String): Matcher

    fun replaceAll(replacement: String): String
    fun replaceFirst(replacement: String): String

    fun toMatchResult(): MatchResult

    fun close()
}
```

---

## `MatchResult` — Immutable Match Snapshot

```kotlin
class MatchResult {
    val groupCount: Int
    val value: String           // group(0) — the full match
    val range: IntRange         // start..end-1

    fun group(index: Int): String?
    fun groupRange(index: Int): IntRange?
    fun start(index: Int): Int
    fun end(index: Int): Int

    val groups: List<String?>   // groups[0] = full match, [1..N] = captures
}
```

---

## Exceptions

| Exception | Parent | When |
|-----------|--------|------|
| `PatternSyntaxException` | `IllegalArgumentException` | Invalid regex pattern |
| `PatternClosedException` | `IllegalStateException` | Method on closed Pattern |
| `MatcherClosedException` | `IllegalStateException` | Method on closed Matcher |
| `IllegalStateException` | `RuntimeException` | group()/start()/end() before any match |
| `IndexOutOfBoundsException` | `RuntimeException` | Invalid group index |

---

## Lifecycle

```
Regex.compile(pattern)
    │
    ▼
Pattern (ACTIVE, immutable)
    │
    ├── matcher(input) → Matcher (stateful)
    │       │
    │       ├── find() → true/false
    │       ├── group() → String?
    │       └── toMatchResult() → MatchResult (immutable snapshot)
    │
    └── close() → DESTROYED
```

> **Rule:** `Pattern` is independent and immutable after creation. Multiple
> `Matcher` instances can be created from the same Pattern simultaneously
> (but each Matcher is NOT thread-safe).

---

## Regex Flags

| Constant | Value | Description |
|----------|-------|-------------|
| `CASE_INSENSITIVE` | `0x02` | Case-insensitive matching (`(?i)`) |
| `MULTILINE` | `0x08` | `^` and `$` match line boundaries (`(?m)`) |
| `DOTALL` | `0x20` | `.` matches newlines too (`(?s)`) |
| `UNICODE_CASE` | `0x40` | Unicode-aware case folding |

---

## Kotlin Usage Examples

```kotlin
import io.github.luandro.regex.Pattern
import io.github.luandro.regex.Regex

// Simple match
val match = Regex.matches("""\d+""", "42")  // true

// Find all
val results = Regex.findAll("""\d+""", "a1 b22 c333")
// results = [MatchResult("1"), MatchResult("22"), MatchResult("333")]

// Pattern with groups
Pattern.compile("""(\d{4})-(\d{2})-(\d{2})""").use { p ->
    val m = p.matcher("Date: 2026-07-28")
    if (m.find()) {
        println(m.group())           // "2026-07-28"
        println(m.groupByIndex(1))   // "2026"
        println(m.groupByIndex(2))   // "07"
        println(m.groupByIndex(3))   // "28"
    }
    m.close()
}

// Replace
val result = Regex.replaceAll("""\s+""", "hello   world", " ")
// result = "hello world"

// Split
val parts = Regex.split(""",\s*""", "a, b, c, d")
// parts = ["a", "b", "c", "d"]

// Case-insensitive
Pattern.compile("hello", Regex.CASE_INSENSITIVE).use { p ->
    println(p.matches("HELLO"))  // true
}
```

---

## Luau Usage Examples

```lua
-- Simple match
local matched = regex.matches([[\d+]], "42")
print(matched)  -- true

-- Find with groups
local p = regex.compile([[(\d{4})-(\d{2})-(\d{2})]])
local m = p:matcher("Date: 2026-07-28")
if m:find() then
    print(m:group())       -- 2026-07-28
    print(m:groupAt(1))    -- 2026
    print(m:groupAt(2))    -- 07
end
m:close()
p:close()

-- Replace all
local result = regex.replaceAll([[\s+]], "hello   world", " ")
print(result)  -- "hello world"

-- Find all numbers
local nums = regex.findAll([[\d+]], "a1 b22 c333")
for _, mr in ipairs(nums) do
    print(mr.value)  -- 1, 22, 333
end
```

---

## Replacement String Syntax

In `replaceAll()` / `replaceFirst()`, the replacement string supports backreferences:

| Syntax | Description |
|--------|-------------|
| `$0` | Entire match |
| `$1`, `$2`, ... | Capture group by index |
| `\$` | Literal `$` character |
