// native/quickjs/js_context.cpp
// Phase 7: QuickJS Engine — Context implementation

#include "js_context.h"
#include "js_value.h"
#include "js_exception.h"
#include <runtime.h>
#include <quickjs.h>
#include <cstring>
#include <memory>

namespace nrp::js {

// ===================================================================
// Module loader glue (QuickJS C callbacks)
// ===================================================================

// Per-context module loader state stored in JSRuntime opaque
struct ModuleLoaderState {
    ModuleLoader loader;
};

// QuickJS normalize module name callback
static char* qjs_module_name_normalize(JSContext* ctx, const char* base_name,
                                        const char* name, void* opaque) {
    // Simple: just return the name as-is (caller provides fully qualified paths)
    size_t len = strlen(name);
    char* out = static_cast<char*>(js_malloc(ctx, len + 1));
    if (out) memcpy(out, name, len + 1);
    return out;
}

// QuickJS load module callback
static JSModuleDef* qjs_module_loader(JSContext* ctx, const char* module_name, void* opaque) {
    auto* state = static_cast<ModuleLoaderState*>(opaque);
    if (!state || !state->loader) return nullptr;

    std::string source;
    try {
        source = state->loader(module_name, "");
    } catch (...) {
        JS_ThrowReferenceError(ctx, "Module not found: %s", module_name);
        return nullptr;
    }
    if (source.empty()) {
        JS_ThrowReferenceError(ctx, "Module not found or empty: %s", module_name);
        return nullptr;
    }

    // Compile the module
    JSValue func = JS_Eval(ctx, source.c_str(), source.size(), module_name,
                           JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(func)) return nullptr;

    // Extract the JSModuleDef from the compiled function value
    JSModuleDef* m = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(func));
    JS_FreeValue(ctx, func);
    return m;
}

// ===================================================================
// Constructor / Destructor
// ===================================================================

Context::Context(JSContext* ctx, Handle runtime_handle)
    : ctx_(ctx), runtime_handle_(runtime_handle) {}

Context::~Context() {
    if (!is_closed_) {
        close();
    }
}

// ===================================================================
// Internal helpers
// ===================================================================

Handle Context::wrap_value(Handle self_handle, void* raw_jsval, bool is_exception) {
    auto wrapper = std::make_unique<JSValueWrapper>(ctx_, raw_jsval, is_exception);
    wrapper->set_parent_context(self_handle);
    Handle h = nrp::Runtime::get().handles().allocate(JSValueWrapper::type_tag);
    nrp::Runtime::get().objects().insert<JSValueWrapper>(h, std::move(wrapper));
    track_child(h);
    return h;
}

void Context::check_exception(void* raw_jsval, const char* ctx_name) {
    JSValue* v = static_cast<JSValue*>(raw_jsval);
    if (!JS_IsException(*v)) return;

    JSValue exc = JS_GetException(ctx_);
    size_t len = 0;
    const char* msg = JS_ToCStringLen(ctx_, &len, exc);
    std::string err_msg(msg ? msg : "JSException: unknown error", msg ? len : 26);
    if (msg) JS_FreeCString(ctx_, msg);

    // Try to get stack trace
    std::string stack_str;
    JSValue stack = JS_GetPropertyStr(ctx_, exc, "stack");
    if (!JS_IsUndefined(stack) && !JS_IsNull(stack)) {
        size_t sl = 0;
        const char* sc = JS_ToCStringLen(ctx_, &sl, stack);
        if (sc) { stack_str = std::string(sc, sl); JS_FreeCString(ctx_, sc); }
    }
    JS_FreeValue(ctx_, stack);
    JS_FreeValue(ctx_, exc);
    JS_FreeValue(ctx_, *v);
    delete v;
    throw JSException(err_msg, stack_str);
}

// ===================================================================
// Script execution
// ===================================================================

Handle Context::eval(Handle self_handle, const std::string& code, const std::string& filename) {
    if (is_closed_) throw ContextClosedException();
    JSValue result = JS_Eval(ctx_, code.c_str(), code.size(),
                             filename.c_str(), JS_EVAL_TYPE_GLOBAL);
    auto* raw = new JSValue(result);
    check_exception(raw, "eval");
    return wrap_value(self_handle, raw);
}

Handle Context::evalModule(Handle self_handle, const std::string& code, const std::string& filename) {
    if (is_closed_) throw ContextClosedException();
    JSValue result = JS_Eval(ctx_, code.c_str(), code.size(),
                             filename.c_str(),
                             JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    if (JS_IsException(result)) {
        auto* raw = new JSValue(result);
        check_exception(raw, "evalModule compile");
    }
    // Evaluate the compiled module
    JSValue eval_res = JS_EvalFunction(ctx_, result);
    auto* raw = new JSValue(eval_res);
    check_exception(raw, "evalModule eval");
    return wrap_value(self_handle, raw);
}

// ===================================================================
// Global object access
// ===================================================================

void Context::setGlobal(const std::string& name, Handle value_handle) {
    if (is_closed_) throw ContextClosedException();
    auto* val = nrp::Runtime::get().objects().get<JSValueWrapper>(value_handle);
    if (!val || val->is_closed()) throw NrpException("JSException: JSValue is closed");
    JSValue global = JS_GetGlobalObject(ctx_);
    JSValue dup = JS_DupValue(ctx_, *static_cast<JSValue*>(val->raw()));
    int ret = JS_SetPropertyStr(ctx_, global, name.c_str(), dup);
    JS_FreeValue(ctx_, global);
    if (ret < 0) throw JSException("JSException: setGlobal failed for: " + name);
}

Handle Context::getGlobal(Handle self_handle, const std::string& name) {
    if (is_closed_) throw ContextClosedException();
    JSValue global = JS_GetGlobalObject(ctx_);
    JSValue prop = JS_GetPropertyStr(ctx_, global, name.c_str());
    JS_FreeValue(ctx_, global);
    auto* raw = new JSValue(prop);
    if (JS_IsException(prop)) check_exception(raw, "getGlobal");
    return wrap_value(self_handle, raw);
}

// ===================================================================
// JSON
// ===================================================================

Handle Context::parseJSON(Handle self_handle, const std::string& json) {
    if (is_closed_) throw ContextClosedException();
    JSValue result = JS_ParseJSON(ctx_, json.c_str(), json.size(), "<json>");
    auto* raw = new JSValue(result);
    check_exception(raw, "parseJSON");
    return wrap_value(self_handle, raw);
}

std::string Context::stringifyJSON(Handle value_handle, int indent) {
    if (is_closed_) throw ContextClosedException();
    auto* val = nrp::Runtime::get().objects().get<JSValueWrapper>(value_handle);
    if (!val || val->is_closed()) throw NrpException("JSException: JSValue is closed");
    JSValue* v = static_cast<JSValue*>(val->raw());

    // JSON.stringify(v, null, indent)
    JSValue json_global = JS_GetGlobalObject(ctx_);
    JSValue json_obj    = JS_GetPropertyStr(ctx_, json_global, "JSON");
    JSValue stringify   = JS_GetPropertyStr(ctx_, json_obj, "stringify");
    JS_FreeValue(ctx_, json_global);
    JS_FreeValue(ctx_, json_obj);

    JSValue indent_val = JS_NewInt32(ctx_, indent);
    JSValue args[3] = { *v, JS_NULL, indent_val };
    JSValue result = JS_Call(ctx_, stringify, JS_UNDEFINED, 3, args);
    JS_FreeValue(ctx_, indent_val);
    JS_FreeValue(ctx_, stringify);

    if (JS_IsException(result)) {
        auto* raw = new JSValue(result);
        check_exception(raw, "stringifyJSON");
    }
    size_t len = 0;
    const char* cstr = JS_ToCStringLen(ctx_, &len, result);
    std::string out(cstr ? cstr : "null", cstr ? len : 4);
    if (cstr) JS_FreeCString(ctx_, cstr);
    JS_FreeValue(ctx_, result);
    return out;
}

// ===================================================================
// Factory helpers
// ===================================================================

Handle Context::newObject(Handle self_handle) {
    if (is_closed_) throw ContextClosedException();
    JSValue obj = JS_NewObject(ctx_);
    return wrap_value(self_handle, new JSValue(obj));
}

Handle Context::newArray(Handle self_handle) {
    if (is_closed_) throw ContextClosedException();
    JSValue arr = JS_NewArray(ctx_);
    return wrap_value(self_handle, new JSValue(arr));
}

Handle Context::newBool(Handle self_handle, bool value) {
    if (is_closed_) throw ContextClosedException();
    JSValue v = JS_NewBool(ctx_, value ? 1 : 0);
    return wrap_value(self_handle, new JSValue(v));
}

Handle Context::newInt(Handle self_handle, int value) {
    if (is_closed_) throw ContextClosedException();
    JSValue v = JS_NewInt32(ctx_, static_cast<int32_t>(value));
    return wrap_value(self_handle, new JSValue(v));
}

Handle Context::newDouble(Handle self_handle, double value) {
    if (is_closed_) throw ContextClosedException();
    JSValue v = JS_NewFloat64(ctx_, value);
    return wrap_value(self_handle, new JSValue(v));
}

Handle Context::newString(Handle self_handle, const std::string& value) {
    if (is_closed_) throw ContextClosedException();
    JSValue v = JS_NewStringLen(ctx_, value.c_str(), value.size());
    return wrap_value(self_handle, new JSValue(v));
}

Handle Context::jsUndefined(Handle self_handle) {
    if (is_closed_) throw ContextClosedException();
    JSValue v = JS_UNDEFINED;
    return wrap_value(self_handle, new JSValue(v));
}

Handle Context::jsNull(Handle self_handle) {
    if (is_closed_) throw ContextClosedException();
    JSValue v = JS_NULL;
    return wrap_value(self_handle, new JSValue(v));
}

// ===================================================================
// Promise / event loop
// ===================================================================

int Context::executePendingJobs() {
    if (is_closed_) throw ContextClosedException();
    JSContext* ctx2 = nullptr;
    int total = 0;
    int ret;
    while ((ret = JS_ExecutePendingJob(JS_GetRuntime(ctx_), &ctx2)) > 0) {
        total += ret;
    }
    if (ret < 0) {
        JSValue exc = JS_GetException(ctx2 ? ctx2 : ctx_);
        size_t len = 0;
        const char* msg = JS_ToCStringLen(ctx_, &len, exc);
        std::string err_msg(msg ? msg : "JSException in pending job", msg ? len : 25);
        if (msg) JS_FreeCString(ctx_, msg);
        JS_FreeValue(ctx_, exc);
        throw JSException(err_msg);
    }
    return total;
}

// ===================================================================
// Module loader
// ===================================================================

void Context::setModuleLoader(ModuleLoader loader) {
    if (is_closed_) throw ContextClosedException();
    module_loader_ = std::move(loader);

    // Attach to the QuickJS runtime
    auto* state = new ModuleLoaderState{ module_loader_ };
    JS_SetModuleLoaderFunc(JS_GetRuntime(ctx_),
                           qjs_module_name_normalize,
                           qjs_module_loader,
                           state);
}

// ===================================================================
// Lifecycle
// ===================================================================

void Context::close() {
    if (is_closed_) return;
    is_closed_ = true;

    // Destroy all child JSValue handles
    for (Handle h : child_handles_) {
        nrp::Runtime::get().objects().destroy(h);
    }
    child_handles_.clear();

    if (ctx_) {
        JS_FreeContext(ctx_);
        ctx_ = nullptr;
    }

    // Notify parent runtime
    if (runtime_handle_ != kInvalidHandle) {
        auto* rt = nrp::Runtime::get().objects().get<nrp::js::Runtime>(runtime_handle_);
        if (rt) rt->untrack_child(/* self */ kInvalidHandle); // handled by parent
    }
}

void Context::track_child(Handle h) {
    child_handles_.insert(h);
}

void Context::untrack_child(Handle h) {
    child_handles_.erase(h);
}

} // namespace nrp::js
