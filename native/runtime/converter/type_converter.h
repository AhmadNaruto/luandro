// native/runtime/converter/type_converter.h
// Phase 3: Runtime Core

#pragma once

#include "../handle_manager/handle.h"
#include <jni.h>
#include <vector>
#include <span>
#include <string>
#include <string_view>

// Forward declare lua_State to keep Runtime Core independent of Luau VM headers in Phase 3
struct lua_State;

namespace nrp {

struct TypeConverter {
    // Handle <-> jlong
    static Handle      from_jlong(jlong v) noexcept;
    static jlong       to_jlong(Handle h)  noexcept;

    // Numeric
    static int32_t     from_jint(jint v)        noexcept;
    static jint        to_jint(int32_t v)        noexcept;
    static int64_t     from_jlong_int(jlong v)   noexcept;
    static jlong       to_jlong_int(int64_t v)   noexcept;
    static double      from_jdouble(jdouble v)   noexcept;
    static jdouble     to_jdouble(double v)       noexcept;
    static bool        from_jboolean(jboolean v) noexcept;
    static jboolean    to_jboolean(bool v)        noexcept;

    // ByteArray <-> span
    static std::vector<uint8_t> from_jbytearray(JNIEnv* env, jbyteArray arr);
    static jbyteArray           to_jbytearray(JNIEnv* env, std::span<const uint8_t> data);

    // LongArray <-> Handle vector
    static std::vector<Handle> from_jlongarray(JNIEnv* env, jlongArray arr);
    static jlongArray           to_jlongarray(JNIEnv* env, std::span<const Handle> data);

    // ObjectArray (String) <-> String vector
    static std::vector<std::string> from_jobjectarray_string(JNIEnv* env, jobjectArray arr);
    static jobjectArray              to_jobjectarray_string(JNIEnv* env, std::span<const std::string> data);

    // Luau value <-> C++ (stubs for Phase 3, fully implemented in Luau phase)
    static Handle      luau_check_handle(lua_State* L, int idx, uint16_t expected_type);
    static void        luau_push_handle(lua_State* L, Handle h);
    static std::string luau_check_string(lua_State* L, int idx);
    static void        luau_push_string(lua_State* L, std::string_view sv);
    static int32_t     luau_check_integer(lua_State* L, int idx);
    static void        luau_push_integer(lua_State* L, int32_t v);
    static double      luau_check_number(lua_State* L, int idx);
    static void        luau_push_number(lua_State* L, double v);
    static bool        luau_check_boolean(lua_State* L, int idx);
    static void        luau_push_boolean(lua_State* L, bool v);
};

} // namespace nrp
