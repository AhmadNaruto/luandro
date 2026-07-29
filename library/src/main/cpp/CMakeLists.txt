# CMakeLists.txt — Luandro Native Runtime Platform (NRP)
# Phase 0: Repository Foundation — skeleton only, no implementation
# Phase 0.6: Build & Toolchain Specification

cmake_minimum_required(VERSION 3.22.1)

project(luandro_nrp
        VERSION 0.1.0
        DESCRIPTION "Luandro Native Runtime Platform"
        LANGUAGES CXX)

# ============================================================
# C++ Standard — C++20 required (Phase 0.6)
# ============================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Symbol visibility — hide internal symbols (Phase 0.6)
# ============================================================
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# ============================================================
# Compiler warnings (Phase 0.6)
# ============================================================
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
)

# ============================================================
# Build type specific flags (Phase 0.6)
# ============================================================
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(NRP_DEBUG NRP_ENABLE_LOGGING NRP_ENABLE_ASSERTIONS)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3)
    add_compile_definitions(NDEBUG)
endif()

# ============================================================
# Source root paths (Phase 0 — directory layout)
# ============================================================
set(NRP_ROOT        "${CMAKE_SOURCE_DIR}/../../../../")
set(NATIVE_ROOT     "${NRP_ROOT}/native")
set(THIRDPARTY_ROOT "${NRP_ROOT}/thirdparty")

# ============================================================
# Third-party submodules (Phase 0 — placeholders for submodules)
# Added in later phases when submodules are initialized
# ============================================================
add_subdirectory(${THIRDPARTY_ROOT}/lexbor  lexbor)
add_subdirectory(${THIRDPARTY_ROOT}/quickjs quickjs)
# add_subdirectory(${THIRDPARTY_ROOT}/luau    luau)
# add_subdirectory(${THIRDPARTY_ROOT}/jsregexp jsregexp)

# ============================================================
# NRP Library
# ============================================================
add_library(luandro_nrp SHARED
    # Phase 3: Runtime Core
    "${NATIVE_ROOT}/runtime/handle_manager/handle_manager.cpp"
    "${NATIVE_ROOT}/runtime/object_manager/object_manager.cpp"
    "${NATIVE_ROOT}/runtime/memory/memory_manager.cpp"
    "${NATIVE_ROOT}/runtime/strings/string_manager.cpp"
    "${NATIVE_ROOT}/runtime/exceptions/exception_manager.cpp"
    "${NATIVE_ROOT}/runtime/converter/type_converter.cpp"
    "${NATIVE_ROOT}/runtime/runtime.cpp"

    # Phase 5: LexSoup Engine
    "${NATIVE_ROOT}/lexsoup/document.cpp"
    "${NATIVE_ROOT}/lexsoup/element.cpp"
    "${NATIVE_ROOT}/lexsoup/elements.cpp"
    "${NATIVE_ROOT}/binding/luau/lexsoup_binding.cpp"
    lexsoup_jni.cpp

    # Phase 6: Regex Engine
    "${NATIVE_ROOT}/regex/regex.cpp"
    "${NATIVE_ROOT}/regex/pattern.cpp"
    "${NATIVE_ROOT}/regex/matcher.cpp"
    "${NATIVE_ROOT}/regex/match_result.cpp"
    "${NATIVE_ROOT}/binding/luau/regex_binding.cpp"
    regex_jni.cpp

    # Phase 7: QuickJS Engine
    "${NATIVE_ROOT}/quickjs/js_runtime.cpp"
    "${NATIVE_ROOT}/quickjs/js_context.cpp"
    "${NATIVE_ROOT}/quickjs/js_value.cpp"
    "${NATIVE_ROOT}/binding/luau/quickjs_binding.cpp"
    quickjs_jni.cpp

    # Phase 0: minimal stub to verify build
    nrp_init.cpp
)

target_include_directories(luandro_nrp PRIVATE
    "${NATIVE_ROOT}"
    "${NATIVE_ROOT}/runtime"
    "${NATIVE_ROOT}/lexsoup"
    "${NATIVE_ROOT}/regex"
    "${NATIVE_ROOT}/quickjs"
    "${NATIVE_ROOT}/luau"
    "${NATIVE_ROOT}/binding"
    "${THIRDPARTY_ROOT}/luau/VM/include"
    "${THIRDPARTY_ROOT}/lexbor/source"
    "${THIRDPARTY_ROOT}/quickjs"
)

# Link Android system libraries
# NDK 23+: link by name directly, no find_library needed
target_link_libraries(luandro_nrp
    lexbor_static
    qjs
    log
    android
)
