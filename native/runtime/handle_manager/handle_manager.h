// native/runtime/handle_manager/handle_manager.h
// Phase 3: Runtime Core

#pragma once

#include "handle.h"
#include <array>
#include <vector>
#include <mutex>

namespace nrp {

class HandleManager {
public:
    HandleManager();
    ~HandleManager() = default;

    // Allocate a new handle for an object of the given type.
    // Returns kInvalidHandle if capacity is exhausted.
    Handle allocate(uint16_t type_tag) noexcept;

    // Release a handle, incrementing its generation counter.
    // After this call, the old handle value is permanently invalid.
    void release(Handle h) noexcept;

    // Check if a handle is currently valid (slot in use, generation matches).
    [[nodiscard]] bool is_valid(Handle h) const noexcept;

    // Return the type tag encoded in a handle (does NOT validate).
    [[nodiscard]] uint16_t type_of(Handle h) const noexcept;

    // Return count of currently live handles (for leak detection).
    [[nodiscard]] size_t live_count() const noexcept;

private:
    struct Slot {
        uint32_t generation = 0;
        uint16_t type_tag   = 0;
        bool     in_use     = false;
    };

    std::array<Slot, 65536> slots_{};
    std::vector<uint16_t>   free_list_;
    size_t                  live_count_ = 0;
    mutable std::mutex      mutex_;
};

} // namespace nrp
