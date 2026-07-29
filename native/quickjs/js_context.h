// native/quickjs/js_context.h
// Phase 7: QuickJS Engine — Context

#pragma once

#include <handle_manager/handle.h>
#include <string>
#include <functional>
#include <unordered_set>
#include <memory>

typedef struct JSContext JSContext;
typedef struct JSRuntime JSRuntime;

namespace nrp::js {

/**
 * Module loader callback type.
 * Called by QuickJS when an `import` statement is resolved.
 * @param module_name  The bare module specifier (e.g., "./util.js")
 * @param base_name    The filename of the importing module
 * @return             The source code of the module, or empty string to signal error
 */
using ModuleLoader = std::function<std::string(const std::string& module_name, const std::string& base_name)>;

/**
 * A JavaScript execution Context.
 * Created by Runtime::newContext(). Owns its JSValues.
 * NOT thread-safe — must be used on the thread that created the Runtime.
 */
class Context {
public:
    static constexpr uint16_t type_tag = 0x0302;

    explicit Context(JSContext* ctx, Handle runtime_handle);
    ~Context();

    // Non-copyable, non-moveable (holds pointer-based C state)
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    [[nodiscard]] JSContext* raw()            const noexcept { return ctx_; }
    [[nodiscard]] Handle     runtime()        const noexcept { return runtime_handle_; }
    [[nodiscard]] bool       is_closed()      const noexcept { return is_closed_; }

    // --- Script execution ---

    /**
     * Evaluates JS code and returns a Handle to a JSValueWrapper.
     * @param code  JavaScript source
     * @param filename  virtual filename for stack traces (default: "<eval>")
     */
    Handle eval(Handle self_handle, const std::string& code, const std::string& filename = "<eval>");

    /**
     * Evaluates an ES Module and returns a Promise handle.
     */
    Handle evalModule(Handle self_handle, const std::string& code, const std::string& filename);

    // --- Global object access ---
    void   setGlobal(const std::string& name, Handle value_handle);
    Handle getGlobal(Handle self_handle, const std::string& name);

    // --- JSON ---
    Handle parseJSON(Handle self_handle, const std::string& json);
    std::string stringifyJSON(Handle value_handle, int indent = 0);

    // --- Factory helpers ---
    Handle newObject(Handle self_handle);
    Handle newArray(Handle self_handle);
    Handle newBool(Handle self_handle, bool value);
    Handle newInt(Handle self_handle, int value);
    Handle newDouble(Handle self_handle, double value);
    Handle newString(Handle self_handle, const std::string& value);
    Handle jsUndefined(Handle self_handle);
    Handle jsNull(Handle self_handle);

    // --- Promise / module event loop ---
    int executePendingJobs();

    // --- Module loader ---
    void setModuleLoader(ModuleLoader loader);
    [[nodiscard]] const ModuleLoader& moduleLoader() const noexcept { return module_loader_; }

    // --- Lifecycle ---
    void close();

    // Child tracking (JSValue handles created in this context)
    void track_child(Handle h);
    void untrack_child(Handle h);

private:
    // Wraps a raw QuickJS JSValue (allocated on heap) into a Handle
    Handle wrap_value(Handle self_handle, void* raw_jsval, bool is_exception = false);

    // Checks exception from QuickJS and throws JSException if needed
    void check_exception(void* raw_jsval, const char* ctx_name);

    JSContext* ctx_ = nullptr;
    Handle     runtime_handle_;
    bool       is_closed_ = false;
    ModuleLoader module_loader_;
    std::unordered_set<Handle> child_handles_;
};

} // namespace nrp::js
