# PHASE 0.7 — Code Generation & Automation Strategy

Goal

Design the code generation system before implementing any Runtime module.

The generator becomes the single source responsible for producing repetitive code.

The objective is to eliminate duplicated JNI, Kotlin wrapper, and Luau binding implementations.

==================================================
CORE PRINCIPLE
==================================================

API Specifications are the single source of truth.

Generated code must never be edited manually.

Manual code and generated code must always be separated.

Every generated file must include a header indicating it is automatically generated.

==================================================
GENERATOR RESPONSIBILITIES
==================================================

The generator must be capable of generating

- Native C API
- JNI Registration
- JNI Wrapper Functions
- Kotlin Wrapper Classes
- Kotlin Native Methods
- Luau Binding Functions
- Luau Registration Tables
- API Documentation
- Markdown Reference
- Unit Test Skeletons

==================================================
INPUT
==================================================

The generator consumes API Specification files.

Example

Document

Element

Elements

Node

Regex

Matcher

QuickJS Runtime

QuickJS Context

Luau VM

etc.

No generated code may invent new APIs.

==================================================
OUTPUT
==================================================

Generated Kotlin

/generated/kotlin/

Generated JNI

/generated/jni/

Generated Lua Binding

/generated/luau/

Generated Native Headers

/generated/native/

Generated Documentation

/generated/docs/

==================================================
FILE OWNERSHIP
==================================================

Generated files

NEVER edit manually.

Manual files

May reference generated files.

Manual files

Must never duplicate generated logic.

==================================================
NAMING CONVENTIONS
==================================================

Generated files must use predictable names.

Examples

Document.gen.kt

Document.gen.cpp

Document.gen.hpp

Regex.gen.kt

Regex.gen.cpp

==================================================
KOTLIN GENERATION
==================================================

Generate

Public wrappers

Native declarations

Constructors

Object lifecycle

Closeable implementation where required

Automatic documentation

==================================================
JNI GENERATION
==================================================

Generate

Native signatures

Method registration

String conversion

Handle conversion

Primitive conversion

Exception translation

No business logic.

==================================================
LUAU GENERATION
==================================================

Generate

Global registration

Method tables

Property access

Object conversion

Automatic userdata creation

Automatic lifetime handling

==================================================
DOCUMENTATION GENERATION
==================================================

Generate

API Reference

Method Reference

Examples

Object Lifecycle

Package Overview

==================================================
TEST GENERATION
==================================================

Generate

Unit Test Skeleton

JNI Test Skeleton

Luau Test Skeleton

==================================================
CUSTOM CODE
==================================================

Every generated class must expose extension points.

Developers may implement custom behavior only inside manual files.

Generated files must remain replaceable.

==================================================
REGENERATION POLICY
==================================================

Whenever the API Specification changes

Regenerate all affected bindings.

Do not manually synchronize generated code.

==================================================
BACKWARD COMPATIBILITY
==================================================

The generator must preserve existing public APIs whenever possible.

Breaking API changes require explicit review.

==================================================
BUILD INTEGRATION
==================================================

The build system must support

Generate before compile.

Skip generation when inputs have not changed.

Detect outdated generated files.

==================================================
SUCCESS CRITERIA
==================================================

Adding a new API should require only

1. Update API Specification.

2. Run the generator.

3. Compile.

No manual JNI, Kotlin wrapper, or Luau binding implementation should be required for standard APIs.
