# PHASE 0.6 — Build & Toolchain Specification

Goal

Define the complete build environment, toolchain, dependency policy, and CI strategy before implementing any source code.

This document becomes the official build specification for the Luandro Native Runtime Platform.

All future phases MUST follow this specification.

==================================================
TARGET PLATFORM
==================================================

Primary Platform

Android

Architecture

ARM64 only

ABI

arm64-v8a

Minimum SDK

21

Target SDK

Latest stable Android SDK

==================================================
BUILD SYSTEM
==================================================

Gradle

Use the latest stable Gradle version compatible with the selected Android Gradle Plugin.

Android Gradle Plugin

Use a single AGP version across the entire project.

Kotlin

Use one Kotlin version consistently.

Do not mix Kotlin compiler versions.

Native Build

CMake

Use modern CMake.

Enable C++20.

==================================================
NDK
==================================================

Use one NDK version for the entire project.

Do not mix toolchains.

Document the required version.

==================================================
COMPILER
==================================================

Language

C++20

Compiler Options

Enable warnings.

Treat important warnings as errors.

Enable hidden symbol visibility by default.

Optimize for performance in Release builds.

Disable RTTI only if every dependency supports it.

Disable exceptions only if every dependency supports it.

==================================================
BUILD TYPES
==================================================

Debug

Enable assertions.

Enable logging.

Enable runtime validation.

Enable sanitizers where possible.

Release

Maximum optimization.

Strip symbols.

Disable debug logging.

Enable link-time optimization if supported.

==================================================
PROJECT MODULES
==================================================

Android Library

Native Runtime

Third-party Libraries

Documentation

Examples

Tests

Benchmark

==================================================
THIRDPARTY POLICY
==================================================

Every external dependency must be added as a Git Submodule.

Do not copy third-party source code into the repository.

Required Submodules

Luau

Lexbor

QuickJS-NG

jsregexp

Only patch third-party code when absolutely necessary.

All patches must be isolated and documented.

==================================================
PATCH POLICY
==================================================

Never modify upstream code directly without documentation.

If a patch is required

Create a dedicated patch directory.

Document

Reason

Files modified

Compatibility impact

Maintenance strategy

==================================================
DEPENDENCY POLICY
==================================================

Avoid unnecessary dependencies.

Prefer existing Runtime utilities.

Every new dependency requires

Technical justification

License verification

Architecture review

Performance review

==================================================
OUTPUT ARTIFACTS
==================================================

Produce

Android AAR

Native shared library

Documentation

API Reference

Examples

==================================================
JNI POLICY
==================================================

JNI must remain minimal.

JNI responsibilities

String conversion

Primitive conversion

Handle conversion

Exception conversion

No business logic.

==================================================
SYMBOL VISIBILITY
==================================================

Hide internal native symbols.

Export only the public Runtime API.

Minimize JNI exports.

==================================================
TESTING TOOLCHAIN
==================================================

Provide

Unit Tests

Integration Tests

JNI Tests

Leak Tests

Stress Tests

Performance Benchmarks

Every module must include tests.

==================================================
BENCHMARK POLICY
==================================================

Benchmark

HTML parsing

CSS selection

DOM traversal

Regex execution

JavaScript execution

Lua execution

Memory allocation

JNI overhead

==================================================
DOCUMENTATION
==================================================

Generate

Build Guide

Developer Guide

Architecture Guide

Contribution Guide

API Reference

==================================================
CI/CD
==================================================

Create GitHub Actions workflows.

Required workflows

Build

Native Build

Unit Tests

Integration Tests

Formatting Checks

Documentation Validation

Release Packaging

Every pull request must pass all required checks.

==================================================
VERSIONING
==================================================

Use Semantic Versioning.

Major

Breaking API changes.

Minor

New features.

Patch

Bug fixes.

==================================================
SOURCE CONTROL
==================================================

Keep commits focused.

One logical change per commit.

Avoid unrelated changes.

Document architectural changes.

==================================================
PROJECT QUALITY
==================================================

Prefer maintainability over clever implementations.

Optimize only after correctness.

Avoid premature optimization.

Keep the Runtime modular.

Keep bindings thin.

Keep public APIs stable.

==================================================
SUCCESS CRITERIA
==================================================

The complete build environment is fully defined.

All developers and AI agents can reproduce identical builds.

No implementation phase may change the toolchain without updating this specification first.
