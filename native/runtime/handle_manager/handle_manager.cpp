// native/runtime/handle_manager/handle_manager.cpp
// Phase 3: Runtime Core

#include "handle_manager.h"

namespace nrp {

HandleManager::HandleManager() {
    free_list_.reserve(65535);
    for (uint32_t i = 65535; i >= 1; --i) {
        free_list_.push_back(static_cast<uint16_t>(i));
    }
}

Handle HandleManager::allocate(uint16_t type_tag) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (free_list_.empty()) {
        return kInvalidHandle;
    }
    uint16_t slot_idx = free_list_.back();
    free_list_.pop_back();

    Slot& slot = slots_[slot_idx];
    slot.generation++;
    if (slot.generation == 0) {
        slot.generation = 1;
    }
    slot.type_tag = type_tag;
    slot.in_use = true;
    live_count_++;

    return make_handle(slot_idx, type_tag, slot.generation);
}

void HandleManager::release(Handle h) noexcept {
    if (h == kInvalidHandle) return;
    std::lock_guard<std::mutex> lock(mutex_);
    uint16_t slot_idx = handle_slot(h);
    uint32_t gen = handle_generation(h);

    Slot& slot = slots_[slot_idx];
    if (slot.in_use && slot.generation == gen) {
        slot.in_use = false;
        slot.type_tag = 0;
        free_list_.push_back(slot_idx);
        if (live_count_ > 0) {
            live_count_--;
        }
    }
}

bool HandleManager::is_valid(Handle h) const noexcept {
    if (h == kInvalidHandle) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    uint16_t slot_idx = handle_slot(h);
    uint32_t gen = handle_generation(h);

    const Slot& slot = slots_[slot_idx];
    return slot.in_use && slot.generation == gen;
}

uint16_t HandleManager::type_of(Handle h) const noexcept {
    return handle_type(h);
}

size_t HandleManager::live_count() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return live_count_;
}

} // namespace nrp
