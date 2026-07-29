// native/runtime/object_manager/object_manager.h
// Phase 3: Runtime Core

#pragma once

#include "../handle_manager/handle.h"
#include "../handle_manager/handle_manager.h"
#include "../exceptions/exception_manager.h"
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <string>

namespace nrp {

// Primary template for TypeTag trait. Specializations or class members
// must specify 'value' (e.g., using a static constexpr uint16_t).
template <typename T>
struct TypeTag {
    static constexpr uint16_t value = T::type_tag;
};

class ObjectManager {
public:
    explicit ObjectManager(HandleManager& handle_mgr);
    ~ObjectManager();

    // Register a newly created object; takes ownership.
    // handle must have been allocated by HandleManager.
    template<typename T>
    void insert(Handle h, std::unique_ptr<T> obj) {
        std::unique_lock<std::shared_mutex> lock(mutex_);

        auto deleter = [](void* p) {
            delete static_cast<T*>(p);
        };

        ObjectBox box{
            std::unique_ptr<void, void(*)(void*)>(obj.release(), deleter),
            handle_type(h)
        };
        objects_[h] = std::move(box);
    }

    // Retrieve a typed pointer. Throws NrpException if handle invalid or type mismatch.
    template<typename T>
    [[nodiscard]] T* get(Handle h) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);

        // 1. Check validity with HandleManager
        if (!handle_mgr_.is_valid(h)) {
            throw NrpHandleException("Invalid or stale handle: " + std::to_string(h));
        }

        // 2. Look up in objects map
        auto it = objects_.find(h);
        if (it == objects_.end()) {
            throw NrpHandleException("Handle not found in ObjectManager: " + std::to_string(h));
        }

        // 3. Verify type tag
        uint16_t expected_tag = TypeTag<T>::value;
        if (it->second.type_tag != expected_tag) {
            throw NrpTypeException("Type tag mismatch: expected " + std::to_string(expected_tag) +
                                   ", got " + std::to_string(it->second.type_tag));
        }

        return static_cast<T*>(it->second.ptr.get());
    }

    // Destroy the object associated with handle and release the handle.
    // Safe to call multiple times (idempotent after first call).
    void destroy(Handle h) noexcept;

    // Destroy all objects (called at runtime shutdown).
    void destroy_all() noexcept;

    [[nodiscard]] size_t object_count() const noexcept;

private:
    struct ObjectBox {
        std::unique_ptr<void, void(*)(void*)> ptr{nullptr, [](void*){}};
        uint16_t type_tag = 0;
    };

    mutable std::shared_mutex          mutex_;
    std::unordered_map<Handle, ObjectBox> objects_;
    HandleManager&                     handle_mgr_;
};

} // namespace nrp
