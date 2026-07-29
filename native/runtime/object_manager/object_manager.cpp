// native/runtime/object_manager/object_manager.cpp
// Phase 3: Runtime Core

#include "object_manager.h"
#include <vector>

namespace nrp {

ObjectManager::ObjectManager(HandleManager& handle_mgr) : handle_mgr_(handle_mgr) {}

ObjectManager::~ObjectManager() {
    destroy_all();
}

void ObjectManager::destroy(Handle h) noexcept {
    if (h == kInvalidHandle) return;

    ObjectBox box_to_destroy;
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        auto it = objects_.find(h);
        if (it != objects_.end()) {
            box_to_destroy = std::move(it->second);
            objects_.erase(it);
        }
    }

    handle_mgr_.release(h);
}

void ObjectManager::destroy_all() noexcept {
    std::unordered_map<Handle, ObjectBox> local_objects;
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        local_objects = std::move(objects_);
        objects_.clear();
    }

    for (const auto& pair : local_objects) {
        handle_mgr_.release(pair.first);
    }
}

size_t ObjectManager::object_count() const noexcept {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return objects_.size();
}

} // namespace nrp
