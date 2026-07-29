// native/runtime/runtime.h
// Phase 3: Runtime Core

#pragma once

#include "handle_manager/handle_manager.h"
#include "object_manager/object_manager.h"
#include "memory/memory_manager.h"
#include "strings/string_manager.h"
#include <memory>
#include <mutex>

namespace nrp {

class Runtime {
public:
    // Access the global runtime instance.
    static Runtime& get();

    // Initialize / destroy the runtime singleton.
    static void initialize();
    static void destroy() noexcept;

    HandleManager&  handles()  noexcept;
    ObjectManager&  objects()  noexcept;
    MemoryManager&  memory()   noexcept;
    StringManager&  strings()  noexcept;

    ~Runtime() = default;

private:
    Runtime();

    std::unique_ptr<HandleManager>  handle_mgr_;
    std::unique_ptr<ObjectManager>  object_mgr_;
    std::unique_ptr<MemoryManager>  memory_mgr_;
    std::unique_ptr<StringManager>  string_mgr_;

    static std::unique_ptr<Runtime> instance_;
    static std::once_flag           init_flag_;
};

} // namespace nrp
