# Phase 9 — Integration Report

> **Status:** ✅ DONE  
> **Date:** 2026-07-29  
> **Package Root:** `io.github.luandro`

---

## Overview

Phase 9 integrates all five engines of the Native Runtime Platform (NRP) into a
single unified native library (`libluandro_nrp.so`).

### Engines Integrated

| Engine    | Package                        | Native Library    |
|-----------|-------------------------------|-------------------|
| Runtime   | `io.github.luandro`            | Luandro Runtime   |
| LexSoup   | `io.github.luandro.lexsoup`    | Lexbor            |
| Regex     | `io.github.luandro.regex`      | jsregexp          |
| QuickJS   | `io.github.luandro.js`         | QuickJS-NG        |
| Luau      | `io.github.luandro.luau`       | Luau VM           |

---

## Architecture Validation

### Single Source of Truth ✅

All engines share:
- One `Runtime` singleton (`nrp::Runtime::get()`)
- One `ObjectManager` / `HandleManager` pool
- One `MemoryManager` / `SharedAllocator`
- One `ExceptionManager`

### Zero Logic Duplication ✅

- Kotlin is a thin JNI wrapper — zero business logic
- Luau is a thin Luau binding — zero duplicated implementation
- JNI layer only converts types (string, primitives, handles, exceptions)

### Handle Safety ✅

- Raw pointers NEVER cross language boundaries
- All objects exposed as opaque `Long` handles (`jlong` on JVM side)
- `HandleManager` validates every handle before dereference

---

## Test Suite

### Test Categories

| Category           | File                          | # Tests |
|--------------------|-------------------------------|---------|
| LexSoup Unit       | `LexSoupTest.kt`              | ~20     |
| Regex Unit         | `RegexTest.kt`                | ~20     |
| QuickJS Unit       | `QuickJSTest.kt`              | ~18     |
| Luau Unit          | `LuauTest.kt`                 | ~20     |
| Integration        | `IntegrationTest.kt`          | ~12     |
| Performance        | `PerformanceTest.kt`          | ~8      |
| Stress / Leak      | `StressTest.kt`               | ~15     |

### Running Tests

```bash
# All instrumented tests (requires connected Android device / emulator)
./gradlew :library:connectedAndroidTest

# Specific test class
./gradlew :library:connectedAndroidTest \
  -Pandroid.testInstrumentationRunnerArguments.class=io.github.luandro.IntegrationTest

# With logcat performance output
adb logcat -s "[PERF]" &
./gradlew :library:connectedAndroidTest
```

---

## Integration Test Scenarios

### Cross-Engine Scenarios Validated

| Scenario | Engines | Description |
|----------|---------|-------------|
| Link extraction | LexSoup + Regex | Parse HTML, filter links by URL pattern |
| Price extraction | LexSoup + Regex | Parse HTML, extract decimal from text |
| Luau → LexSoup | Luau + LexSoup | Luau script calls `lexsoup.parse()` via auto-registered global |
| Luau → Regex | Luau + Regex | Luau script calls `regex.compile()` via auto-registered global |
| Luau → QuickJS | Luau + QuickJS | Luau script executes JavaScript via auto-registered `js` global |
| QuickJS → Regex | QuickJS + Regex | JS eval result validated by Regex |
| QuickJS → LexSoup | QuickJS + LexSoup | JS generates HTML string, parsed by LexSoup |
| All engines simultaneous | All | All 4 engines used in a single test, no interference |
| Multiple VM isolation | Luau | Two LuauVMs have independent global state |
| Multiple Runtime isolation | QuickJS | Two JS Runtimes have independent globals |

---

## Performance Targets

| Engine   | Operation          | Repetitions | Max Time |
|----------|--------------------|-------------|----------|
| LexSoup  | parse small HTML   | 1 000       | 500 ms   |
| LexSoup  | parse large HTML   | 1 × 2000 el | 500 ms   |
| Regex    | find()             | 10 000      | 500 ms   |
| Regex    | replaceAll()       | 10 000      | 500 ms   |
| Luau     | execute()          | 1 000       | 1 000 ms |
| Luau     | executeCompiled()  | 100         | 1 000 ms |
| QuickJS  | eval()             | 1 000       | 1 000 ms |

---

## Stress / Leak Test Scenarios

| Engine   | Scenario                           | Iterations |
|----------|------------------------------------|------------|
| LexSoup  | create+destroy documents           | 5 000      |
| LexSoup  | select+close elements              | 5 000      |
| Regex    | compile+match+destroy              | 5 000      |
| Regex    | Matcher.find loop                  | 5 000      |
| Luau     | create VM + execute + destroy      | 1 000      |
| Luau     | single VM, repeated execute        | 5 000      |
| Luau     | setGlobal + getGlobal + remove     | 1 000      |
| QuickJS  | create context + eval + destroy    | 1 000      |
| QuickJS  | heavy object creation in JS heap   | 2 000      |
| All      | interleaved cross-engine ops       | 500        |
| All      | double-close safety (idempotent)   | —          |

---

## Success Criteria

- [x] All modules use a single shared `nrp::Runtime`
- [x] All modules compile into a single `libluandro_nrp.so`
- [x] Luau VM auto-registers `lexsoup`, `regex`, and `js` as globals
- [x] Cross-engine tests pass without interference
- [x] Double-close is safe on all engine objects
- [x] Performance targets met on ARM64 device
- [x] No memory leaks under stress (5 000+ cycles per engine)
