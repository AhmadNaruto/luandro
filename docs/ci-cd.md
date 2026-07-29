# GitHub Actions CI/CD Pipeline

> **Workflow Path:** `.github/workflows/ci.yml`

---

## Overview

The Luandro Native Runtime Platform (NRP) uses GitHub Actions for continuous integration and automated release distribution.

---

## Pipeline Jobs

```
                              ┌───────────────────────────────────┐
                              │  Trigger: push / PR / tag / manual│
                              └─────────────────┬─────────────────┘
                                                │
                                                ▼
                              ┌───────────────────────────────────┐
                              │  Job 1: validate-generator        │
                              │  - Validates ASL YAML specs       │
                              │  - Checks generated code diff     │
                              └─────────────────┬─────────────────┘
                                                │
                                                ▼
                              ┌───────────────────────────────────┐
                              │  Job 2: build-and-package         │
                              │  - Sets up Java 21 & Android NDK  │
                              │  - Builds Debug & Release AARs    │
                              │  - Uploads AAR build artifacts    │
                              └─────────────────┬─────────────────┘
                                                │
                                       (If tag matches v*)
                                                │
                                                ▼
                              ┌───────────────────────────────────┐
                              │  Job 3: release                   │
                              │  - Creates GitHub Release         │
                              │  - Attaches AAR binaries          │
                              └───────────────────────────────────┘
```

---

## Jobs Breakdown

### 1. `validate-generator`
- **Environment:** `ubuntu-latest`, Python 3.11, PyYAML
- **Steps:**
  - Checks out repository.
  - Runs `python3 tools/generator/generate.py`.
  - Verifies `git diff --quiet` to ensure committed code matches ASL specifications.

### 2. `build-and-package`
- **Environment:** `ubuntu-latest`, Java 21 (Temurin), Android NDK 28
- **Steps:**
  - Clones submodules (`thirdparty/lexbor`, `thirdparty/quickjs`, `thirdparty/luau`, `thirdparty/jsregexp`).
  - Runs `./gradlew :library:assembleDebug :library:assembleRelease`.
  - Uploads `library-release.aar` and `library-debug.aar` as build artifacts.

### 3. `release`
- **Trigger:** Only executed on version tag pushes matching `v*.*.*` (e.g. `v1.0.0`).
- **Steps:**
  - Downloads built AAR artifacts.
  - Creates a GitHub Release with generated release notes and attaches binary AAR artifacts.
