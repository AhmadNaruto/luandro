// native/runtime/allocator/shared_allocator.h
// Phase 3: Runtime Core

#pragma once

#include "../memory/memory_manager.h"
#include <cstddef>

namespace nrp {

template<typename T>
class SharedAllocator {
public:
    using value_type = T;

    explicit SharedAllocator(MemoryManager& mgr) noexcept : mgr_(mgr) {}

    template<typename U>
    SharedAllocator(const SharedAllocator<U>& other) noexcept : mgr_(other.mgr_) {}

    [[nodiscard]] T* allocate(std::size_t n) {
        return static_cast<T*>(mgr_.allocate(n * sizeof(T), alignof(T)));
    }

    void deallocate(T* p, std::size_t n) noexcept {
        mgr_.deallocate(p, n * sizeof(T));
    }

    template<typename U>
    friend class SharedAllocator;

private:
    MemoryManager& mgr_;
};

template<typename T, typename U>
bool operator==(const SharedAllocator<T>& a, const SharedAllocator<U>& b) noexcept {
    return &a.mgr_ == &b.mgr_;
}

template<typename T, typename U>
bool operator!=(const SharedAllocator<T>& a, const SharedAllocator<U>& b) noexcept {
    return !(a == b);
}

} // namespace nrp
