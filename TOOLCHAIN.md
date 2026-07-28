# TOOLCHAIN.md — Luandro Native Runtime Platform
# Build & Toolchain Specification
# Phase 0.6 — Build & Toolchain Specification
#
# This document is the official build specification for the Luandro NRP.
# All future phases MUST follow this specification.
# No implementation phase may change the toolchain without updating this document first.

---

## 1. Target Platform

| Property | Value |
|----------|-------|
| Platform | Android |
| Architecture | ARM64 only |
| ABI | `arm64-v8a` |
| Min SDK | 21 (Android 5.0 Lollipop) |
| Compile SDK | 36 |
| Target SDK | 36 |

> **Why ARM64 only?** Maximum performance and simplicity. No multi-ABI maintenance overhead.

---

## 2. Toolchain Versions

| Tool | Version | Notes |
|------|---------|-------|
| **Gradle** | 8.9 | via wrapper (`~/.gradle/wrapper/dists/gradle-8.9-bin/`) |
| **Android Gradle Plugin (AGP)** | 8.7.3 | Compatible with Gradle 8.9 |
| **Kotlin** | 2.0.21 | Consistent across entire project |
| **NDK** | 28.2.13676358 | Single version for entire project |
| **CMake** | 4.4.0 (system) | via `cmake.dir=/data/data/com.termux/files/usr` in `local.properties` |
| **Ninja** | system | bundled with system CMake |
| **C++ Standard** | C++20 | Required, enforced via `-std=c++20` |
| **Clang** | 21.1.8 | NDK 28.2 bundled LLVM toolchain |
| **Java** | 21 | OpenJDK 21 (Termux) |
| **Build Tools** | 36.0.0 | Android SDK Build-Tools |

### Why these versions?
- **Gradle 8.9 + AGP 8.7.3**: Gradle 9.6 removed internal APIs used by AGP 8.x, so 8.9 is the compatible choice with the current NDK setup
- **NDK 28.2**: Latest stable NDK with LLVM 21 (Clang 21.1.8)
- **CMake 4.4.0 (system)**: Android SDK's cmake 3.22.1 had module path mismatch with system cmake 4.4.0; using system cmake avoids the conflict
- **`ANDROID_USE_LEGACY_TOOLCHAIN_FILE=OFF`**: Required for cmake 4.4 + NDK 28 compatibility

---

## 3. Build System Configuration

### Gradle Invocation
```bash
# Use Gradle 8.9 (not system Gradle 9.6.1)
~/.gradle/wrapper/dists/gradle-8.9-bin/90cnw93cvbtalezasaz0blq0a/gradle-8.9/bin/gradle <task>

# Or use gradlew after setting up wrapper
./gradlew <task>
```

### Key Files
| File | Purpose |
|------|---------|
| `settings.gradle.kts` | Multi-module definition |
| `build.gradle.kts` | Root plugin declarations |
| `library/build.gradle.kts` | Android Library config (NDK, CMake, ABI) |
| `gradle/libs.versions.toml` | All dependency versions (single source of truth) |
| `gradle/wrapper/gradle-wrapper.properties` | Gradle 8.9 distribution |
| `gradle.properties` | JVM args, parallel build, AndroidX flag |
| `local.properties` | SDK path, `cmake.dir` (not committed to git) |

### CMake Invocation (what AGP generates)
```bash
cmake \
  -DCMAKE_SYSTEM_NAME=Android \
  -DCMAKE_SYSTEM_VERSION=21 \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_NDK=<ndk_path> \
  -DCMAKE_TOOLCHAIN_FILE=<ndk>/build/cmake/android.toolchain.cmake \
  -DANDROID_USE_LEGACY_TOOLCHAIN_FILE=OFF \
  -DANDROID_STL=c++_shared \
  -DCMAKE_CXX_FLAGS="-std=c++20" \
  -DCMAKE_BUILD_TYPE=<Debug|Release> \
  -GNinja
```

---

## 4. Compiler Flags

### Debug Build
```cmake
-std=c++20
-Wall -Wextra -Wpedantic
-DDEBUG
-DNRP_ENABLE_LOGGING
-DNRP_ENABLE_ASSERTIONS
-g              # debug symbols
```

### Release Build
```cmake
-std=c++20
-Wall -Wextra -Wpedantic
-O3             # maximum optimization
-DNDEBUG
-fvisibility=hidden
-fvisibility-inlines-hidden
```

### Symbol Visibility
```cmake
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
```
Only JNI-registered functions are exported. All internal symbols are hidden.

---

## 5. Module Structure

| Module | Path | Type |
|--------|------|------|
| Android Library | `library/` | `com.android.library` |
| Native Runtime | `native/runtime/` | CMake object library |
| Native Engines | `native/{lexsoup,regex,quickjs,luau}/` | CMake object library |
| Bindings | `native/binding/` | CMake compiled into main .so |
| Third Party | `thirdparty/` | Git Submodules |
| Kotlin API | `kotlin/` | Shared source set |
| Generated Code | `generated/` | Never committed, always regenerated |
| Specs (ASL) | `spec/` | YAML input for Binding Generator |

---

## 6. Third-Party Dependency Policy

| Rule | Requirement |
|------|-------------|
| **Git Submodules** | ALL external dependencies added as submodules |
| **No source copy** | Never copy third-party source into repo |
| **Single version** | One version per dependency, no mixing |
| **License check** | Verify license compatibility before adding |
| **Minimal deps** | No dependency without technical justification |

### Required Submodules (to be initialized in Phase 5+)
```bash
git submodule add https://github.com/luau-lang/luau        thirdparty/luau
git submodule add https://github.com/lexbor/lexbor          thirdparty/lexbor
git submodule add https://github.com/quickjs-ng/quickjs     thirdparty/quickjs
git submodule add https://github.com/kmarius/jsregexp       thirdparty/jsregexp
```

### Patch Policy
If a third-party patch is required:
1. Create `thirdparty/patches/<lib>/` directory
2. Document: reason, files modified, compatibility impact, maintenance strategy
3. Never modify upstream directly

---

## 7. Build Types

### Debug
- Assertions enabled (`NRP_ENABLE_ASSERTIONS`)
- Logging enabled (`NRP_ENABLE_LOGGING`)
- Runtime validation enabled
- Sanitizers enabled where possible (ASan, UBSan)
- Debug symbols included

### Release
- Maximum optimization (`-O3`)
- Strip symbols
- Debug logging disabled (`NDEBUG`)
- Link-time optimization if supported

---

## 8. Output Artifacts

| Artifact | Path | Description |
|----------|------|-------------|
| Debug AAR | `library/build/outputs/aar/library-debug.aar` | Android archive (debug) |
| Release AAR | `library/build/outputs/aar/library-release.aar` | Android archive (release) |
| Native .so | Inside AAR at `jni/arm64-v8a/libluandro_nrp.so` | Native shared library |

---

## 9. CI/CD (GitHub Actions — to be configured in Phase 9)

Required workflows:
| Workflow | Trigger |
|----------|---------|
| Build | Push / PR |
| Native Build | Push / PR |
| Unit Tests | Push / PR |
| Integration Tests | Push / PR |
| Formatting Check | Push / PR |
| Documentation Validation | Push / PR |
| Release Packaging | Tag push |

---

## 10. Versioning Policy

**Semantic Versioning** (`MAJOR.MINOR.PATCH`):

| Change | Version bump |
|--------|-------------|
| Breaking API change | MAJOR |
| New feature | MINOR |
| Bug fix | PATCH |

Current version: **0.1.0** (Phase 0 skeleton)

---

## 11. Environment Setup (Termux/Android)

```bash
# Verify environment
gradle --version        # should show 9.6.1 (system, not used directly)
java -version           # should show 21
cmake --version         # should show 4.4.0
git --version           # should show 2.55+

# Actual build command (Gradle 8.9)
GRADLE89=~/.gradle/wrapper/dists/gradle-8.9-bin/90cnw93cvbtalezasaz0blq0a/gradle-8.9/bin/gradle
$GRADLE89 :library:assembleDebug    # build debug AAR
$GRADLE89 :library:assembleRelease  # build release AAR
$GRADLE89 :library:test             # run unit tests
```

### Environment Variables
```bash
ANDROID_HOME=/data/data/com.termux/files/home/android-sdk
ANDROID_SDK_ROOT=/data/data/com.termux/files/home/android-sdk
```

### local.properties (NOT committed)
```properties
sdk.dir=/data/data/com.termux/files/home/android-sdk
cmake.dir=/data/data/com.termux/files/usr
```
