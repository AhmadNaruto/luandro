# PHASE 11 — Migration to Generated Bindings & Full Automation

Goal

Complete the transition from manually written bindings to a fully generated binding system.

After this phase, every public API must be generated from the API Specification Language (ASL).

Manual JNI, Kotlin wrappers, and Luau bindings are no longer permitted for standard APIs.

==================================================
OBJECTIVES
==================================================

Replace manually written bindings with generated bindings.

Ensure every generated binding is functionally identical to the previous implementation.

Reduce duplicated code.

Guarantee long-term consistency between

- Native Runtime
- Kotlin API
- Luau API

==================================================
MIGRATION TASKS
==================================================

Identify all manually written

- JNI wrappers
- Kotlin wrappers
- Luau bindings
- Native C API wrappers

Convert them into ASL specifications.

Regenerate all bindings using the Binding Generator.

Remove obsolete manual implementations.

==================================================
VALIDATION
==================================================

Verify that generated bindings produce identical behavior.

Run

- Unit Tests
- Integration Tests
- JNI Tests
- Luau Tests
- Performance Benchmarks

Generated bindings must not introduce regressions.

==================================================
PROJECT POLICY
==================================================

After this phase

Every new public API must begin with an ASL specification.

Developers must not manually implement

- JNI methods
- Kotlin wrappers
- Luau bindings
- Native C API wrappers

All generated files must be reproducible.

==================================================
BUILD SYSTEM
==================================================

Integrate the Binding Generator into the build process.

Generation must occur automatically before compilation when specifications change.

Support incremental regeneration.

==================================================
DOCUMENTATION
==================================================

Update

Developer Guide

Contribution Guide

Architecture Guide

Generator Guide

ASL Guide

Migration Guide

==================================================
QUALITY ASSURANCE
==================================================

Verify

No duplicated bindings remain.

No manually maintained generated files remain.

Generated code follows project coding standards.

All generated APIs are synchronized with the ASL.

==================================================
SUCCESS CRITERIA
==================================================

The Binding Generator becomes the official source for all bindings.

ASL becomes the single source of truth.

Future API development requires only

1. Update the ASL specification.
2. Run the Binding Generator.
3. Compile.
4. Execute tests.

No manual binding implementation is required for standard APIs.
