# PHASE 0.5 — Project Standards & Development Guidelines

Goal

Define the project-wide development standards before implementing any architecture or source code.

This document becomes the authoritative reference for all future development.

Every subsequent phase MUST follow these standards.

No implementation may violate these rules unless the documentation is updated first.

==================================================
GENERAL PRINCIPLES
==================================================

The project is a Native Runtime Platform (NRP).

Business logic MUST exist only in native code.

Kotlin is a thin public API.

Luau is another frontend that shares the exact same native implementation.

No duplicated business logic is allowed.

Native is always the single source of truth.

==================================================
CODING STYLE
==================================================

C++

- Use C++20.
- Prefer RAII.
- Prefer smart pointers internally.
- Avoid raw pointer ownership.
- Keep classes small and focused.
- Follow SOLID principles where appropriate.
- Prefer composition over inheritance.
- Use constexpr where applicable.
- Avoid macros except for platform compatibility.

Kotlin

- Follow Kotlin coding conventions.
- Keep Kotlin as a thin wrapper.
- No business logic.
- No DOM manipulation.
- No parsing logic.
- Public API should feel idiomatic to Kotlin developers.

JNI

- JNI must only perform:
  - String conversion
  - Primitive conversion
  - Array conversion
  - Handle conversion
  - Exception translation

No business logic is allowed inside JNI.

Luau Binding

- Only convert Lua values.
- Forward calls to the Runtime.
- Push results back to Lua.
- No duplicated implementation.

==================================================
PROJECT STRUCTURE
==================================================

Each module must have a single responsibility.

Runtime components must remain independent from engine implementations.

Directory responsibilities must never overlap.

All shared functionality belongs in Runtime.

==================================================
OBJECT OWNERSHIP
==================================================

Native owns every object.

Kotlin never owns native objects.

Luau never owns native objects.

Every object must be represented by a Runtime Handle.

Raw pointers must never cross language boundaries.

==================================================
HANDLE MANAGEMENT
==================================================

All handles are managed by Handle Manager.

Handles must be validated.

Destroyed handles become invalid.

Handle reuse must be safe.

==================================================
MEMORY MANAGEMENT
==================================================

Memory allocation must be centralized.

Avoid unnecessary heap allocations.

Avoid object duplication.

Avoid copying DOM structures.

Every allocation must have a clear owner.

Memory leaks are unacceptable.

==================================================
ERROR HANDLING
==================================================

C++

Use exceptions internally only when appropriate.

Exceptions must never cross the JNI boundary.

JNI converts native exceptions into Java exceptions.

Luau converts native exceptions into Lua errors.

All public APIs should expose meaningful error messages.

==================================================
THREAD SAFETY
==================================================

Document every thread-safe component.

Avoid global mutable state.

Runtime managers must clearly define synchronization behavior.

==================================================
API DESIGN
==================================================

Public APIs must remain stable.

Avoid breaking API compatibility.

Every new API must first be added to the API Specification.

Implementation follows specification.

Never design APIs during implementation.

==================================================
PERFORMANCE
==================================================

Optimize for

- Low memory usage
- Fast startup
- Low JNI overhead
- Minimal allocations
- Zero duplicated parsing
- Native execution

Avoid unnecessary object creation.

Avoid unnecessary String conversions.

==================================================
TESTING
==================================================

Every module must include tests.

Required test types

- Unit Tests
- Integration Tests
- JNI Tests
- Memory Leak Tests
- Stress Tests
- Performance Benchmarks

New features require corresponding tests.

==================================================
DOCUMENTATION
==================================================

Every public class requires documentation.

Every Runtime component requires architecture documentation.

Every module requires

- Overview
- Responsibilities
- Lifecycle
- Usage Examples

==================================================
LOGGING
==================================================

Provide configurable logging.

Support

- Debug
- Info
- Warning
- Error

Logging must be removable or disabled in release builds.

==================================================
CODE REVIEW CHECKLIST
==================================================

Before accepting any implementation verify

- Architecture is respected.
- No duplicated logic exists.
- JNI contains no business logic.
- Kotlin remains thin.
- Luau remains thin.
- Runtime owns all objects.
- Memory ownership is correct.
- Tests are included.
- Documentation is updated.

==================================================
IMPLEMENTATION POLICY
==================================================

Implementation must proceed incrementally.

Every phase must compile successfully before continuing.

No phase may introduce unfinished APIs.

No placeholder implementations unless explicitly documented.

If architectural improvements are discovered during implementation

1. Update the documentation.
2. Review the impact.
3. Apply the necessary patches.
4. Continue implementation.

Architecture always takes precedence over implementation speed.

==================================================
SUCCESS CRITERIA
==================================================

All future development follows this document consistently.

This document becomes the official engineering standard for the Luandro Native Runtime Platform.
