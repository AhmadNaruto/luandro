// native/runtime/memory/memory_manager.h
// Phase 3: Runtime Core

#pragma once

#include <atomic>
#include <cstddef>
#include <new>

namespace nrp {

class MemoryManager {
public:
    MemoryManager() = default;
    ~MemoryManager() = default;

    [[nodiscard]] void* allocate(size_t bytes, size_t align = alignof(std::max_align_t));
    void                deallocate(void* ptr, size_t bytes) noexcept;

    // Statistics
    [[nodiscard]] size_t bytes_allocated() const noexcept;
    [[nodiscard]] size_t peak_bytes()      const noexcept;
    [[nodiscard]] size_t alloc_count()     const noexcept;

    // Set memory hard limit
    void set_hard_limit(size_t limit) noexcept;

    // Leak check: returns true if all allocations were freed.
    [[nodiscard]] bool clean() const noexcept;

private:
    std::atomic<size_t> bytes_allocated_{0};
    std::atomic<size_t> peak_bytes_{0};
    std::atomic<size_t> alloc_count_{0};
    std::atomic<size_t> hard_limit_{static_cast<size_t>(-1)};
};

} // namespace nrp
