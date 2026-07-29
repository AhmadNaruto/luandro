# Phase 10 — Binding Generator Report

> **Status:** ✅ DONE  
> **Date:** 2026-07-29  
> **Tool Location:** `tools/generator/generate.py`

---

## Overview

Phase 10 implements the **NRP Binding Generator**, an automated code generation system that consumes API Specification Language (ASL) YAML definitions in `spec/` and automatically generates all required cross-language bindings and documentation.

---

## Code Generation Architecture

```
                       ┌─────────────────────────┐
                       │  ASL Specifications     │
                       │  spec/**/*.yaml         │
                       └────────────┬────────────┘
                                    │
                                    ▼
                       ┌─────────────────────────┐
                       │   tools/generator/      │
                       │   generate.py           │
                       └────────────┬────────────┘
                                    │
    ┌─────────────────┬─────────────┼─────────────┬─────────────────┐
    ▼                 ▼             ▼             ▼                 ▼
Native Headers    JNI Bindings  Kotlin Wrappers Luau Bindings  Markdown Reference
generated/native/ generated/jni/ generated/kotlin/ generated/luau/ generated/docs/
```

---

## Generator Output Targets

| Target | Output Path | Description |
|--------|-------------|-------------|
| Native Headers | `generated/native/<module>/<Class>.gen.h` | Pure C++ abstract interface prototypes |
| JNI Bindings | `generated/jni/<module>/<Class>_jni.gen.cpp` | JNI export functions and exception translation stubs |
| Kotlin Wrappers | `generated/kotlin/<package>/<Class>Gen.kt` | Kotlin wrapper class with native handle lifecycle |
| Luau Bindings | `generated/luau/<module>/<Class>_binding.gen.cpp` | Luau userdata metatable registration & C-function bridges |
| Markdown Reference | `generated/docs/<module>/<Class>.md` | Human-readable API reference documentation |

---

## CLI Usage

```bash
# Run the Binding Generator from the project root
python3 tools/generator/generate.py
```

### Success Criteria Verified

- [x] All 16 ASL spec files in `spec/` parse and generate code without manual editing.
- [x] Generated files contain header warnings disallowing manual edits.
- [x] Native headers, JNI exports, Kotlin wrappers, Luau bindings, and Markdown docs generated automatically.
- [x] Adding a new API requires only creating an ASL YAML spec and executing `generate.py`.
