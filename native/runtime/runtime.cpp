// native/runtime/runtime.cpp
// Phase 3: Runtime Core

#include "runtime.h"

namespace nrp {

std::unique_ptr<Runtime> Runtime::instance_ = nullptr;
std::once_flag Runtime::init_flag_;

Runtime::Runtime() {
    handle_mgr_ = std::make_unique<HandleManager>();
    object_mgr_ = std::make_unique<ObjectManager>(*handle_mgr_);
    memory_mgr_ = std::make_unique<MemoryManager>();
    string_mgr_ = std::make_unique<StringManager>();
}

Runtime& Runtime::get() {
    std::call_once(init_flag_, []() {
        initialize();
    });
    return *instance_;
}

void Runtime::initialize() {
    if (!instance_) {
        // Use new directly since constructor is private
        instance_.reset(new Runtime());
    }
}

void Runtime::destroy() noexcept {
    instance_.reset();
}

HandleManager& Runtime::handles() noexcept {
    return *handle_mgr_;
}

ObjectManager& Runtime::objects() noexcept {
    return *object_mgr_;
}

MemoryManager& Runtime::memory() noexcept {
    return *memory_mgr_;
}

StringManager& Runtime::strings() noexcept {
    return *string_mgr_;
}

} // namespace nrp
