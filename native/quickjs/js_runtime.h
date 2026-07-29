// native/quickjs/js_runtime.h
// Phase 7: QuickJS Engine — Runtime

#pragma once

#include <handle_manager/handle.h>
#include <cstdint>
#include <cstddef>
#include <unordered_set>

typedef struct JSRuntime JSRuntime;

namespace nrp::js {

/**
 * Top-level container for the QuickJS JavaScript engine.
 * Manages memory, GC, and the lifecycle of all Context objects.
 * NOT thread-safe.
 */
class Runtime {
public:
    static constexpr uint16_t type_tag = 0x0303;

    explicit Runtime(JSRuntime* rt);
    ~Runtime();

    // Non-copyable, non-moveable
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    [[nodiscard]] JSRuntime* raw()       const noexcept { return rt_; }
    [[nodiscard]] bool       is_closed() const noexcept { return is_closed_; }

    // Factory: allocates and stores a new Context, returns its Handle
    Handle newContext(Handle self_handle);

    // GC
    void gc();

    // Memory limit
    void setMemoryLimit(size_t max_bytes);
    void setStackSize(size_t stack_bytes);
    size_t memoryUsed() const;
    bool isLive() const noexcept { return !is_closed_; }

    // Lifecycle
    void close();

    // Child tracking
    void track_child(Handle h);
    void untrack_child(Handle h);

private:
    JSRuntime* rt_ = nullptr;
    bool       is_closed_ = false;
    std::unordered_set<Handle> child_handles_;
};

} // namespace nrp::js
