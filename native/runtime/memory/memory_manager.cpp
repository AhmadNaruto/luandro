// native/runtime/memory/memory_manager.cpp
// Phase 3: Runtime Core

#include "memory_manager.h"
#include <cstdlib>

namespace nrp {

void* MemoryManager::allocate(size_t bytes, size_t align) {
    if (bytes_allocated_.load() + bytes > hard_limit_.load()) {
        throw std::bad_alloc();
    }

    void* ptr = nullptr;
    if (align <= alignof(std::max_align_t)) {
        ptr = std::malloc(bytes);
    } else {
        if (posix_memalign(&ptr, align, bytes) != 0) {
            ptr = nullptr;
        }
    }

    if (!ptr) {
        throw std::bad_alloc();
    }

    bytes_allocated_ += bytes;
    alloc_count_++;

    // Update peak bytes
    size_t current = bytes_allocated_.load();
    size_t peak = peak_bytes_.load();
    while (current > peak && !peak_bytes_.compare_exchange_weak(peak, current)) {
        // Retry
    }

    return ptr;
}

void MemoryManager::deallocate(void* ptr, size_t bytes) noexcept {
    if (!ptr) return;
    std::free(ptr);

    if (bytes_allocated_ >= bytes) {
        bytes_allocated_ -= bytes;
    } else {
        bytes_allocated_ = 0;
    }

    if (alloc_count_ > 0) {
        alloc_count_--;
    }
}

size_t MemoryManager::bytes_allocated() const noexcept {
    return bytes_allocated_.load();
}

size_t MemoryManager::peak_bytes() const noexcept {
    return peak_bytes_.load();
}

size_t MemoryManager::alloc_count() const noexcept {
    return alloc_count_.load();
}

void MemoryManager::set_hard_limit(size_t limit) noexcept {
    hard_limit_.store(limit);
}

bool MemoryManager::clean() const noexcept {
    return bytes_allocated_.load() == 0 && alloc_count_.load() == 0;
}

} // namespace nrp
