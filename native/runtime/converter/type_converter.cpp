// native/runtime/converter/type_converter.cpp
// Phase 3: Runtime Core

#include "type_converter.h"
#include "../exceptions/exception_manager.h"
#include <lua.h>
#include <lualib.h>

namespace nrp {

Handle TypeConverter::from_jlong(jlong v) noexcept {
    return static_cast<Handle>(v);
}

jlong TypeConverter::to_jlong(Handle h) noexcept {
    return static_cast<jlong>(h);
}

int32_t TypeConverter::from_jint(jint v) noexcept {
    return static_cast<int32_t>(v);
}

jint TypeConverter::to_jint(int32_t v) noexcept {
    return static_cast<jint>(v);
}

int64_t TypeConverter::from_jlong_int(jlong v) noexcept {
    return static_cast<int64_t>(v);
}

jlong TypeConverter::to_jlong_int(int64_t v) noexcept {
    return static_cast<jlong>(v);
}

double TypeConverter::from_jdouble(jdouble v) noexcept {
    return static_cast<double>(v);
}

jdouble TypeConverter::to_jdouble(double v) noexcept {
    return static_cast<jdouble>(v);
}

bool TypeConverter::from_jboolean(jboolean v) noexcept {
    return v == JNI_TRUE;
}

jboolean TypeConverter::to_jboolean(bool v) noexcept {
    return v ? JNI_TRUE : JNI_FALSE;
}

std::vector<uint8_t> TypeConverter::from_jbytearray(JNIEnv* env, jbyteArray arr) {
    if (!arr) return {};
    jsize len = env->GetArrayLength(arr);
    std::vector<uint8_t> vec(static_cast<size_t>(len));
    env->GetByteArrayRegion(arr, 0, len, reinterpret_cast<jbyte*>(vec.data()));
    return vec;
}

jbyteArray TypeConverter::to_jbytearray(JNIEnv* env, std::span<const uint8_t> data) {
    jbyteArray arr = env->NewByteArray(static_cast<jsize>(data.size()));
    if (!arr) {
        throw NrpException("Failed to allocate jbyteArray");
    }
    env->SetByteArrayRegion(arr, 0, static_cast<jsize>(data.size()),
                            reinterpret_cast<const jbyte*>(data.data()));
    return arr;
}

// LongArray <-> Handle vector
std::vector<Handle> TypeConverter::from_jlongarray(JNIEnv* env, jlongArray arr) {
    if (!arr) return {};
    jsize len = env->GetArrayLength(arr);
    std::vector<Handle> vec(static_cast<size_t>(len));
    env->GetLongArrayRegion(arr, 0, len, reinterpret_cast<jlong*>(vec.data()));
    return vec;
}

jlongArray TypeConverter::to_jlongarray(JNIEnv* env, std::span<const Handle> data) {
    jlongArray arr = env->NewLongArray(static_cast<jsize>(data.size()));
    if (!arr) {
        throw NrpException("Failed to allocate jlongArray");
    }
    env->SetLongArrayRegion(arr, 0, static_cast<jsize>(data.size()),
                            reinterpret_cast<const jlong*>(data.data()));
    return arr;
}

// ObjectArray (String) <-> String vector
std::vector<std::string> TypeConverter::from_jobjectarray_string(JNIEnv* env, jobjectArray arr) {
    if (!arr) return {};
    jsize len = env->GetArrayLength(arr);
    std::vector<std::string> vec;
    vec.reserve(static_cast<size_t>(len));
    for (jsize i = 0; i < len; ++i) {
        jstring js = static_cast<jstring>(env->GetObjectArrayElement(arr, i));
        if (js) {
            const char* chars = env->GetStringUTFChars(js, nullptr);
            if (chars) {
                vec.emplace_back(chars);
                env->ReleaseStringUTFChars(js, chars);
            }
            env->DeleteLocalRef(js);
        } else {
            vec.emplace_back("");
        }
    }
    return vec;
}

jobjectArray TypeConverter::to_jobjectarray_string(JNIEnv* env, std::span<const std::string> data) {
    jclass str_class = env->FindClass("java/lang/String");
    if (!str_class) {
        throw NrpException("Failed to find java/lang/String class");
    }
    jobjectArray arr = env->NewObjectArray(static_cast<jsize>(data.size()), str_class, nullptr);
    env->DeleteLocalRef(str_class);
    if (!arr) {
        throw NrpException("Failed to allocate jobjectArray of String");
    }
    for (jsize i = 0; i < static_cast<jsize>(data.size()); ++i) {
        jstring js = env->NewStringUTF(data[i].c_str());
        env->SetObjectArrayElement(arr, i, js);
        env->DeleteLocalRef(js);
    }
    return arr;
}

namespace {
const char* get_metatable_name(uint16_t type_tag) noexcept {
    switch (type_tag) {
        case 0x0101: return "lexsoup.Document";
        case 0x0102: return "lexsoup.Element";
        case 0x0103: return "lexsoup.Elements";
        case 0x0201: return "regex.Pattern";
        case 0x0202: return "regex.Matcher";
        case 0x0301: return "js.JSContext";
        default: return nullptr;
    }
}
}

// Luau value <-> C++
Handle TypeConverter::luau_check_handle(lua_State* L, int idx, uint16_t expected_type) {
    const char* mt_name = get_metatable_name(expected_type);
    if (!mt_name) {
        throw NrpException("No registered Luau metatable for type tag: " + std::to_string(expected_type));
    }
    void* ud = luaL_checkudata(L, idx, mt_name);
    if (!ud) {
        luaL_error(L, "Expected %s", mt_name);
    }
    Handle h = *static_cast<Handle*>(ud);
    if (h == kInvalidHandle) {
        luaL_error(L, "%s is already closed", mt_name);
    }
    return h;
}

void TypeConverter::luau_push_handle(lua_State* L, Handle h) {
    uint16_t type_tag = handle_type(h);
    const char* mt_name = get_metatable_name(type_tag);
    if (!mt_name) {
        throw NrpException("No registered Luau metatable for type tag: " + std::to_string(type_tag));
    }
    void* ud = lua_newuserdata(L, sizeof(Handle));
    *static_cast<Handle*>(ud) = h;
    luaL_getmetatable(L, mt_name);
    lua_setmetatable(L, -2);
}

std::string TypeConverter::luau_check_string(lua_State* L, int idx) {
    size_t len = 0;
    const char* str = luaL_checklstring(L, idx, &len);
    return std::string(str, len);
}

void TypeConverter::luau_push_string(lua_State* L, std::string_view sv) {
    lua_pushlstring(L, sv.data(), sv.size());
}

int32_t TypeConverter::luau_check_integer(lua_State* L, int idx) {
    return static_cast<int32_t>(luaL_checkinteger(L, idx));
}

void TypeConverter::luau_push_integer(lua_State* L, int32_t v) {
    lua_pushinteger(L, v);
}

double TypeConverter::luau_check_number(lua_State* L, int idx) {
    return luaL_checknumber(L, idx);
}

void TypeConverter::luau_push_number(lua_State* L, double v) {
    lua_pushnumber(L, v);
}

bool TypeConverter::luau_check_boolean(lua_State* L, int idx) {
    luaL_checktype(L, idx, LUA_TBOOLEAN);
    return lua_toboolean(L, idx) != 0;
}

void TypeConverter::luau_push_boolean(lua_State* L, bool v) {
    lua_pushboolean(L, v ? 1 : 0);
}

} // namespace nrp
