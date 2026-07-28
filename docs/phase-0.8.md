# PHASE 0.8 — API Specification Language (ASL)

Goal

Design a formal API Specification Language (ASL) that becomes the single source of truth for the entire Luandro Native Runtime Platform.

Every public API must be described using ASL before implementation.

The ASL will be consumed by the Binding Generator.

No generated code may define APIs that are not present in the ASL.

==================================================
PURPOSE
==================================================

The API Specification Language defines

- Public Kotlin API
- Native Runtime API
- Luau API
- JNI Mapping
- Type Conversion
- Documentation
- Code Generation

The ASL completely replaces handwritten interface definitions.

==================================================
DESIGN PRINCIPLES
==================================================

Human readable.

Machine readable.

Version controlled.

Deterministic.

Extensible.

Language independent.

Generator friendly.

==================================================
FILE FORMAT
==================================================

Use YAML.

One class per file.

One package per directory.

Example

spec/

    lexsoup/

        Document.yaml

        Element.yaml

        Elements.yaml

    regex/

        Pattern.yaml

        Matcher.yaml

    js/

        Runtime.yaml

        Context.yaml

    luau/

        VM.yaml

==================================================
ROOT STRUCTURE
==================================================

Each specification defines

Package

Class

Description

Constructors

Methods

Properties

Static Functions

Enums

Constants

Exceptions

Lifecycle

Ownership

Thread Safety

Visibility

Examples

==================================================
TYPE SYSTEM
==================================================

Primitive Types

Boolean

Int

Long

Double

String

Void

Native Handle

Collections

Array

List

Map

Set

Optional

Nullable

Native Objects

Document

Element

Node

Regex

Matcher

Runtime

Context

VM

Support future custom types.

==================================================
METHOD DEFINITION
==================================================

Every method defines

Name

Description

Visibility

Static

Arguments

Return Type

Nullable

Throws

Thread Safety

Ownership Rules

Example

==================================================
PROPERTY DEFINITION
==================================================

Support

Read Only

Read Write

Lazy

Computed

Native Backed

==================================================
ENUM SUPPORT
==================================================

Support strongly typed enumerations.

==================================================
EXCEPTION MODEL
==================================================

Every method may declare

Native Exception

Kotlin Exception

Lua Error

Generator automatically maps exceptions.

==================================================
OWNERSHIP MODEL
==================================================

Each object defines

Owner

Lifetime

Destroy Policy

Reference Policy

Handle Policy

==================================================
JNI MAPPING
==================================================

Every method automatically defines

JNI Signature

Argument Conversion

Return Conversion

Handle Conversion

==================================================
LUAU MAPPING
==================================================

Every method automatically defines

Lua Name

Colon Syntax

Dot Syntax

Property Access

Userdata Mapping

==================================================
KOTLIN MAPPING
==================================================

Automatically generate

Class

Constructors

Methods

Properties

Companion Object

Closeable Support

==================================================
DOCUMENTATION
==================================================

Generate

Markdown

API Reference

Examples

Package Documentation

Method Documentation

==================================================
VERSIONING
==================================================

Every specification contains

Version

Author

Status

Experimental

Stable

Deprecated

Removed

==================================================
VALIDATION
==================================================

Before generation

Validate

Duplicate names

Invalid types

Circular references

Missing descriptions

Missing examples

Invalid ownership

Invalid lifecycle

==================================================
COMPATIBILITY
==================================================

The generator must detect

Breaking changes

Signature changes

Removed methods

Type changes

==================================================
FUTURE EXTENSIONS
==================================================

Support

Async methods

Coroutines

Streams

Events

Callbacks

Generics

Annotations

Custom metadata

==================================================
SUCCESS CRITERIA
==================================================

Every public API in Luandro is fully described by ASL.

No public API may be implemented before its ASL specification exists.

The Binding Generator consumes only ASL files and produces all generated code automatically.
