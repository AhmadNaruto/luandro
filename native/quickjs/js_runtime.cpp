// native/quickjs/js_runtime.cpp
// Phase 7: QuickJS Engine — Runtime implementation

#include "js_runtime.h"
#include "js_context.h"
#include "js_exception.h"
#include <runtime.h>
#include <quickjs/quickjs.h>

namespace nrp::js {

Runtime::Runtime(JSRuntime* rt)
    : rt_(rt) {}

Runtime::~Runtime() {
    if (!is_closed_) {
        close();
    }
}

Handle Runtime::newContext(Handle self_handle) {
    if (is_closed_) throw RuntimeClosedException();

    JSContext* ctx = JS_NewContext(rt_);
    if (!ctx) {
        throw NrpException("OutOfMemoryError: failed to create JS Context");
    }

    auto ctx_obj = std::make_unique<Context>(ctx, self_handle);
    Handle h = nrp::Runtime::get().handles().allocate(Context::type_tag);
    nrp::Runtime::get().objects().insert<Context>(h, std::move(ctx_obj));
    track_child(h);
    return h;
}

void Runtime::gc() {
    if (is_closed_) throw RuntimeClosedException();
    JS_RunGC(rt_);
}

void Runtime::setMemoryLimit(size_t max_bytes) {
    if (is_closed_) throw RuntimeClosedException();
    JS_SetMemoryLimit(rt_, max_bytes);
}

void Runtime::setStackSize(size_t stack_bytes) {
    if (is_closed_) throw RuntimeClosedException();
    JS_SetMaxStackSize(rt_, stack_bytes);
}

size_t Runtime::memoryUsed() const {
    if (is_closed_) throw RuntimeClosedException();
    JSMemoryUsage stats;
    JS_ComputeMemoryUsage(rt_, &stats);
    return static_cast<size_t>(stats.malloc_size);
}

void Runtime::close() {
    if (is_closed_) return;
    is_closed_ = true;

    // Destroy all child contexts
    for (Handle h : child_handles_) {
        nrp::Runtime::get().objects().destroy(h);
    }
    child_handles_.clear();

    if (rt_) {
        JS_FreeRuntime(rt_);
        rt_ = nullptr;
    }
}

void Runtime::track_child(Handle h) {
    child_handles_.insert(h);
}

void Runtime::untrack_child(Handle h) {
    child_handles_.erase(h);
}

} // namespace nrp::js
