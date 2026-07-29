// native/quickjs/js_value.cpp
// Phase 7: QuickJS Engine — JSValueWrapper implementation

#include "js_value.h"
#include "js_exception.h"
#include <runtime.h>
#include <quickjs.h>
#include <cstring>
#include <memory>

namespace nrp::js {

// ===================================================================
// Constructor / Destructor
// ===================================================================

JSValueWrapper::JSValueWrapper(JSContext* ctx, void* raw_jsval, bool is_exception)
    : ctx_(ctx), raw_(raw_jsval)
{
    if (is_exception) {
        type_ = Type::Exception;
    } else {
        compute_type();
    }
}

JSValueWrapper::~JSValueWrapper() {
    free();
}

JSValueWrapper::JSValueWrapper(JSValueWrapper&& other) noexcept
    : ctx_(other.ctx_), raw_(other.raw_), type_(other.type_),
      parent_ctx_handle_(other.parent_ctx_handle_)
{
    other.raw_ = nullptr;
    other.ctx_ = nullptr;
    other.type_ = Type::Unknown;
}

JSValueWrapper& JSValueWrapper::operator=(JSValueWrapper&& other) noexcept {
    if (this != &other) {
        free();
        ctx_  = other.ctx_;
        raw_  = other.raw_;
        type_ = other.type_;
        parent_ctx_handle_ = other.parent_ctx_handle_;
        other.raw_ = nullptr;
        other.ctx_ = nullptr;
        other.type_ = Type::Unknown;
    }
    return *this;
}

// ===================================================================
// Type detection
// ===================================================================

void JSValueWrapper::compute_type() {
    if (!raw_ || !ctx_) { type_ = Type::Unknown; return; }
    JSValue* v = static_cast<JSValue*>(raw_);
    int tag = JS_VALUE_GET_TAG(*v);
    switch (tag) {
        case JS_TAG_UNDEFINED:  type_ = Type::Undefined; break;
        case JS_TAG_NULL:       type_ = Type::Null;      break;
        case JS_TAG_BOOL:       type_ = Type::Bool;      break;
        case JS_TAG_INT:        type_ = Type::Int;        break;
        case JS_TAG_FLOAT64:    type_ = Type::Float64;   break;
        case JS_TAG_STRING:     type_ = Type::String;    break;
        case JS_TAG_OBJECT: {
            // Determine if Array, Function, or Promise
            JSValue* val = v;
            if (JS_IsArray(*val)) {
                type_ = Type::Array;
            } else if (JS_IsFunction(ctx_, *val)) {
                type_ = Type::Function;
            } else {
                // Check for Promise (has 'then' property that is a function)
                JSValue then_prop = JS_GetPropertyStr(ctx_, *val, "then");
                bool is_thenable = JS_IsFunction(ctx_, then_prop);
                JS_FreeValue(ctx_, then_prop);
                type_ = is_thenable ? Type::Promise : Type::Object;
            }
            break;
        }
        case JS_TAG_EXCEPTION:  type_ = Type::Exception; break;
        default:                type_ = Type::Unknown;   break;
    }
}

// ===================================================================
// Type predicates
// ===================================================================

bool JSValueWrapper::isUndefined() const noexcept { return type_ == Type::Undefined; }
bool JSValueWrapper::isNull()      const noexcept { return type_ == Type::Null;      }
bool JSValueWrapper::isBool()      const noexcept { return type_ == Type::Bool;      }
bool JSValueWrapper::isInt()       const noexcept { return type_ == Type::Int;       }
bool JSValueWrapper::isNumber()    const noexcept { return type_ == Type::Int || type_ == Type::Float64; }
bool JSValueWrapper::isString()    const noexcept { return type_ == Type::String;    }
bool JSValueWrapper::isObject()    const noexcept {
    return type_ == Type::Object || type_ == Type::Array || type_ == Type::Function || type_ == Type::Promise;
}
bool JSValueWrapper::isArray()     const noexcept { return type_ == Type::Array;     }
bool JSValueWrapper::isFunction()  const noexcept { return type_ == Type::Function;  }
bool JSValueWrapper::isPromise()   const noexcept { return type_ == Type::Promise;   }
bool JSValueWrapper::isException() const noexcept { return type_ == Type::Exception; }

// ===================================================================
// Value extractors
// ===================================================================

bool JSValueWrapper::toBool() const {
    if (!raw_) throw NrpException("JSValueWrapper: value is closed");
    JSValue* v = static_cast<JSValue*>(raw_);
    int b = JS_ToBool(ctx_, *v);
    if (b < 0) throw JSException("JSException: toBool failed");
    return b != 0;
}

int JSValueWrapper::toInt() const {
    if (!raw_) throw NrpException("JSValueWrapper: value is closed");
    JSValue* v = static_cast<JSValue*>(raw_);
    int32_t out = 0;
    if (JS_ToInt32(ctx_, &out, *v) < 0) {
        throw JSException("JSException: toInt conversion failed");
    }
    return static_cast<int>(out);
}

double JSValueWrapper::toDouble() const {
    if (!raw_) throw NrpException("JSValueWrapper: value is closed");
    JSValue* v = static_cast<JSValue*>(raw_);
    double out = 0.0;
    if (JS_ToFloat64(ctx_, &out, *v) < 0) {
        throw JSException("JSException: toDouble conversion failed");
    }
    return out;
}

std::string JSValueWrapper::toString() const {
    if (!raw_) throw NrpException("JSValueWrapper: value is closed");
    JSValue* v = static_cast<JSValue*>(raw_);
    size_t len = 0;
    const char* cstr = JS_ToCStringLen(ctx_, &len, *v);
    if (!cstr) throw JSException("JSException: toString conversion failed");
    std::string result(cstr, len);
    JS_FreeCString(ctx_, cstr);
    return result;
}

// ===================================================================
// Property access
// ===================================================================

Handle JSValueWrapper::getProperty(Handle ctx_handle, const std::string& name) const {
    if (!raw_) throw NrpException("JSValueWrapper: value is closed");
    if (!isObject()) throw NrpException("ClassCastException: JSValue is not an object");
    JSValue* v = static_cast<JSValue*>(raw_);
    JSValue prop = JS_GetPropertyStr(ctx_, *v, name.c_str());
    if (JS_IsException(prop)) {
        JSValue exc = JS_GetException(ctx_);
        size_t len = 0;
        const char* msg = JS_ToCStringLen(ctx_, &len, exc);
        std::string err_msg(msg ? msg : "JSException", len);
        if (msg) JS_FreeCString(ctx_, msg);
        JS_FreeValue(ctx_, exc);
        JS_FreeValue(ctx_, prop);
        throw JSException(err_msg);
    }
    auto* raw_copy = new JSValue(prop); // already a new ref
    auto wrapper = std::make_unique<JSValueWrapper>(ctx_, raw_copy);
    wrapper->set_parent_context(ctx_handle);
    Handle h = nrp::Runtime::get().handles().allocate(JSValueWrapper::type_tag);
    nrp::Runtime::get().objects().insert<JSValueWrapper>(h, std::move(wrapper));
    return h;
}

void JSValueWrapper::setProperty(const std::string& name, Handle value_handle) {
    if (!raw_) throw NrpException("JSValueWrapper: value is closed");
    if (!isObject()) throw NrpException("ClassCastException: JSValue is not an object");
    auto* val_wrapper = nrp::Runtime::get().objects().get<JSValueWrapper>(value_handle);
    if (!val_wrapper || val_wrapper->is_closed()) throw NrpException("JSException: value is closed");
    JSValue* self = static_cast<JSValue*>(raw_);
    JSValue* val  = static_cast<JSValue*>(val_wrapper->raw());
    // JS_SetPropertyStr takes ownership of val copy, so we dup
    JSValue dup = JS_DupValue(ctx_, *val);
    int ret = JS_SetPropertyStr(ctx_, *self, name.c_str(), dup);
    if (ret < 0) throw JSException("JSException: setProperty failed");
}

Handle JSValueWrapper::getPropertyAt(Handle ctx_handle, int index) const {
    if (!raw_) throw NrpException("JSValueWrapper: value is closed");
    JSValue* v = static_cast<JSValue*>(raw_);
    JSValue prop = JS_GetPropertyUint32(ctx_, *v, static_cast<uint32_t>(index));
    if (JS_IsException(prop)) {
        JSValue exc = JS_GetException(ctx_);
        size_t len = 0;
        const char* msg = JS_ToCStringLen(ctx_, &len, exc);
        std::string err_msg(msg ? msg : "JSException", len);
        if (msg) JS_FreeCString(ctx_, msg);
        JS_FreeValue(ctx_, exc);
        JS_FreeValue(ctx_, prop);
        throw JSException(err_msg);
    }
    auto* raw_copy = new JSValue(prop);
    auto wrapper = std::make_unique<JSValueWrapper>(ctx_, raw_copy);
    wrapper->set_parent_context(ctx_handle);
    Handle h = nrp::Runtime::get().handles().allocate(JSValueWrapper::type_tag);
    nrp::Runtime::get().objects().insert<JSValueWrapper>(h, std::move(wrapper));
    return h;
}

int JSValueWrapper::length() const {
    if (!raw_) throw NrpException("JSValueWrapper: value is closed");
    JSValue* v = static_cast<JSValue*>(raw_);
    JSValue len_val = JS_GetPropertyStr(ctx_, *v, "length");
    int32_t len = 0;
    if (JS_ToInt32(ctx_, &len, len_val) < 0) len = 0;
    JS_FreeValue(ctx_, len_val);
    return static_cast<int>(len);
}

// ===================================================================
// Function call
// ===================================================================

Handle JSValueWrapper::call(Handle ctx_handle, Handle this_obj_handle, const std::vector<Handle>& arg_handles) const {
    if (!raw_) throw NrpException("JSValueWrapper: value is closed");
    if (!isFunction()) throw NrpException("ClassCastException: JSValue is not a function");

    JSValue* func = static_cast<JSValue*>(raw_);

    // Resolve this object
    JSValue this_val = JS_UNDEFINED;
    if (this_obj_handle != kInvalidHandle) {
        auto* tw = nrp::Runtime::get().objects().get<JSValueWrapper>(this_obj_handle);
        if (tw && !tw->is_closed()) {
            this_val = *static_cast<JSValue*>(tw->raw());
        }
    }

    // Build args array
    std::vector<JSValue> js_args;
    js_args.reserve(arg_handles.size());
    for (Handle ah : arg_handles) {
        auto* aw = nrp::Runtime::get().objects().get<JSValueWrapper>(ah);
        if (aw && !aw->is_closed()) {
            js_args.push_back(*static_cast<JSValue*>(aw->raw()));
        } else {
            js_args.push_back(JS_UNDEFINED);
        }
    }

    JSValue result = JS_Call(ctx_, *func, this_val,
                             static_cast<int>(js_args.size()),
                             js_args.empty() ? nullptr : js_args.data());

    if (JS_IsException(result)) {
        JSValue exc = JS_GetException(ctx_);
        size_t len = 0;
        const char* msg = JS_ToCStringLen(ctx_, &len, exc);
        std::string err_msg(msg ? msg : "JSException");
        if (msg) JS_FreeCString(ctx_, msg);

        // Try to get stack trace
        JSValue stack = JS_GetPropertyStr(ctx_, exc, "stack");
        std::string stack_str;
        if (!JS_IsUndefined(stack) && !JS_IsNull(stack)) {
            size_t sl = 0;
            const char* sc = JS_ToCStringLen(ctx_, &sl, stack);
            if (sc) { stack_str = std::string(sc, sl); JS_FreeCString(ctx_, sc); }
        }
        JS_FreeValue(ctx_, stack);
        JS_FreeValue(ctx_, exc);
        JS_FreeValue(ctx_, result);
        throw JSException(err_msg, stack_str);
    }

    auto* raw_result = new JSValue(result);
    auto wrapper = std::make_unique<JSValueWrapper>(ctx_, raw_result);
    wrapper->set_parent_context(ctx_handle);
    Handle h = nrp::Runtime::get().handles().allocate(JSValueWrapper::type_tag);
    nrp::Runtime::get().objects().insert<JSValueWrapper>(h, std::move(wrapper));
    return h;
}

// ===================================================================
// Free / close
// ===================================================================

void JSValueWrapper::free() {
    if (raw_ && ctx_) {
        JSValue* v = static_cast<JSValue*>(raw_);
        JS_FreeValue(ctx_, *v);
        delete v;
        raw_ = nullptr;
        ctx_ = nullptr;
    }
}

} // namespace nrp::js
