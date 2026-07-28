# PROJECT
Luandro - Native Runtime Platform (NRP) for Android

Repository Name:
luandro

Package Root:
io.github.luandro

Primary Goal

Develop a modular, high-performance Native Runtime Platform (NRP) for Android ARM64.

This project is NOT a collection of JNI wrappers.

The entire business logic, parsing, execution, object lifecycle, and memory management MUST live in native code.

Kotlin is only a thin public API.

Luau is another frontend that shares the exact same native implementation.

No duplicated implementation is allowed.

==================================================
ARCHITECTURE
==================================================

                Android App
                       │
                       ▼

              Kotlin Public API

io.github.luandro.lexsoup
io.github.luandro.regex
io.github.luandro.js
io.github.luandro.luau

                       │
                Thin JNI Layer
                       │

======================================
        Native Runtime Platform
======================================

Runtime Core

- Object Manager
- Handle Manager
- Memory Manager
- String Manager
- Exception Manager
- Type Converter
- JNI Utilities
- Lua Binding Utilities
- Shared Allocator
- Common Utilities

======================================

Native Engines

LexSoup
Regex
QuickJS
Luau

======================================

Third Party

Lexbor
QuickJS-NG
Luau
jsregexp

==================================================
PROJECT STRUCTURE
==================================================

thirdparty/

    luau/
    lexbor/
    quickjs/
    jsregexp/

native/

    runtime/

        object_manager/
        handle_manager/
        memory/
        strings/
        exceptions/
        converter/
        allocator/
        utilities/

    lexsoup/

    regex/

    quickjs/

    luau/

    binding/

        kotlin/

        luau/

kotlin/

    io/github/luandro/

        lexsoup/
        regex/
        js/
        luau/

==================================================
DEPENDENCIES
==================================================

Use Git Submodules.

Required:

Luau
https://github.com/luau-lang/luau

Lexbor
https://github.com/lexbor/lexbor

QuickJS-NG
https://github.com/quickjs-ng/quickjs

jsregexp
https://github.com/kmarius/jsregexp

==================================================
CORE DESIGN PRINCIPLES
==================================================

The native layer is the single source of truth.

Business logic MUST NEVER exist in Kotlin.

Business logic MUST NEVER exist in Luau.

Every feature is implemented exactly once in native.

Both Kotlin and Luau call the same implementation.

==================================================
OBJECT MODEL
==================================================

Every native object must be managed through handles.

Example

Handle 100
→ Document

Handle 101
→ Element

Handle 102
→ Elements

Handle 200
→ Regex

Handle 300
→ JSRuntime

Handle 400
→ Luau VM

Kotlin and Luau must never own raw pointers.

==================================================
MEMORY MANAGEMENT
==================================================

Native owns all objects.

Kotlin only stores handles.

Luau only stores handles.

Objects are destroyed only by Runtime.

No memory leaks.

RAII where applicable.

==================================================
KOTLIN API
==================================================

Kotlin must look like ordinary Kotlin.

Example

val doc = LexSoup.parse(html)

val title = doc.title()

val links = doc.select("a")

Everything executes in native.

Only results are returned.

==================================================
LUAU API
==================================================

LexSoup becomes a global module.

Example

local doc = lexbor.parse(html)

local title = doc:title()

local links = doc:select("a")

The implementation MUST call the exact same native code used by Kotlin.

==================================================
LEXSOUP
==================================================

Package

io.github.luandro.lexsoup

Purpose

Provide a JSoup-compatible API backed entirely by Lexbor.

Native performs:

HTML parsing

CSS selector

DOM traversal

DOM modification

Serialization

Node management

Selector optimization

Caching

Kotlin never manipulates DOM directly.

==================================================
REGEX
==================================================

Package

io.github.luandro.regex

Powered by:

jsregexp

Must expose:

Pattern

Matcher

MatchResult

Replace

Split

Find

Matches

Global module inside Luau.

==================================================
QUICKJS
==================================================

Package

io.github.luandro.js

Powered by:

QuickJS-NG

Support

Runtime

Context

Module

Promise

JSON

Script execution

Expose as Kotlin API and Luau global.

==================================================
LUAU
==================================================

Package

io.github.luandro.luau

Provide

LuauVM

Script

Compiler

Execution

Global registration

Native function registration

Automatic binding of

LexSoup

Regex

QuickJS

==================================================
JNI DESIGN
==================================================

JNI must be extremely thin.

Responsibilities:

Convert String

Convert arrays

Convert primitives

Convert handles

Nothing else.

No business logic.

==================================================
LUA BINDING DESIGN
==================================================

Lua binding must also be thin.

Responsibilities:

Read Lua arguments.

Call Runtime.

Push results.

Nothing else.

==================================================
NO DUPLICATED LOGIC
==================================================

Example

Document.select()

Exists exactly once.

Both

Kotlin

Luau

must call

Runtime::Document::select()

==================================================
CODE STYLE
==================================================

Modern C++20

RAII

Smart pointers internally

No exceptions crossing JNI

No raw pointer exposure

Minimal allocations

Stable ABI

==================================================
PERFORMANCE
==================================================

Optimize for

Low memory

Fast startup

Minimal JNI overhead

Minimal object allocation

Native execution

Zero duplicated DOM

==================================================
DOCUMENTATION
==================================================

Generate:

Architecture document

API specification

JNI mapping

Luau mapping

Directory documentation

Object lifecycle documentation

Memory management documentation

==================================================
IMPLEMENTATION STRATEGY
==================================================

Develop incrementally.

Each stage must compile successfully before proceeding.

Suggested order:

1. Runtime Core
2. Object & Handle Manager
3. JNI Infrastructure
4. LexSoup Engine
5. Kotlin LexSoup API
6. Luau Binding for LexSoup
7. Regex Engine
8. Kotlin Regex API
9. Luau Regex Binding
10. QuickJS Engine
11. Kotlin QuickJS API
12. Luau QuickJS Binding
13. Luau VM
14. Runtime Integration
15. Documentation

Do not skip steps.

If architectural improvements are required during implementation, update the design documents first, then apply the necessary patches. Maintain a clean, modular, and extensible architecture at all times.
