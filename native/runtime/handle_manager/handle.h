// native/runtime/handle_manager/handle.h
// Phase 3: Runtime Core

#pragma once

#include <cstdint>

namespace nrp {

using Handle = uint64_t;
constexpr Handle kInvalidHandle = 0;

constexpr uint16_t handle_slot(Handle h)       { return h & 0xFFFF; }
constexpr uint16_t handle_type(Handle h)       { return (h >> 16) & 0xFFFF; }
constexpr uint32_t handle_generation(Handle h) { return (h >> 32); }

constexpr Handle make_handle(uint16_t slot, uint16_t type, uint32_t gen) {
    return (static_cast<uint64_t>(gen) << 32)
         | (static_cast<uint64_t>(type) << 16)
         | static_cast<uint64_t>(slot);
}

} // namespace nrp
