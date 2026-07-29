// native/quickjs/js_value.h
// Phase 7: QuickJS Engine — JSValue wrapper

#pragma once

#include <handle_manager/handle.h>
#include <string>
#include <vector>

// Forward-declare QuickJS types to avoid exposing quickjs.h in headers
typedef struct JSContext JSContext;
typedef long long int64_t;
// QuickJS JSValue is a tagged union — we forward declare the opaque type
struct JSValueOpaque;

namespace nrp::js {

class Context;

/**
 * Wrapper for any JavaScript value within a Context.
 * JSValue objects are ref-counted by QuickJS internally.
 * Must not outlive the Context that created it.
 *
 * Type tags (mirrors QuickJS internal type IDs):
 *   UNDEFINED, NULL_VAL, BOOL, INT, FLOAT64, STRING, OBJECT, EXCEPTION
 */
class JSValueWrapper {
public:
    static constexpr uint16_t type_tag = 0x0301;

    // JS type enum
    enum class Type {
        Undefined,
        Null,
        Bool,
        Int,
        Float64,
        String,
        Object,
        Array,
        Function,
        Promise,
        Exception,
        Unknown
    };

    // Constructed from a raw QuickJS value (takes ownership / inc ref)
    JSValueWrapper(JSContext* ctx, void* raw_jsval, bool is_exception = false);
    ~JSValueWrapper();

    // Non-copyable (ref counting managed internally via free())
    JSValueWrapper(const JSValueWrapper&) = delete;
    JSValueWrapper& operator=(const JSValueWrapper&) = delete;

    // Moveable
    JSValueWrapper(JSValueWrapper&&) noexcept;
    JSValueWrapper& operator=(JSValueWrapper&&) noexcept;

    [[nodiscard]] Type type() const noexcept { return type_; }
    [[nodiscard]] JSContext* ctx() const noexcept { return ctx_; }
    [[nodiscard]] void* raw() const noexcept { return raw_; }
    [[nodiscard]] bool is_closed() const noexcept { return raw_ == nullptr; }

    // Type checks
    [[nodiscard]] bool isUndefined() const noexcept;
    [[nodiscard]] bool isNull()      const noexcept;
    [[nodiscard]] bool isBool()      const noexcept;
    [[nodiscard]] bool isInt()       const noexcept;
    [[nodiscard]] bool isNumber()    const noexcept;
    [[nodiscard]] bool isString()    const noexcept;
    [[nodiscard]] bool isObject()    const noexcept;
    [[nodiscard]] bool isArray()     const noexcept;
    [[nodiscard]] bool isFunction()  const noexcept;
    [[nodiscard]] bool isPromise()   const noexcept;
    [[nodiscard]] bool isException() const noexcept;

    // Value extractors
    [[nodiscard]] bool        toBool()   const;
    [[nodiscard]] int         toInt()    const;
    [[nodiscard]] double      toDouble() const;
    [[nodiscard]] std::string toString() const;

    // Property access (for objects / arrays)
    Handle getProperty(Handle ctx_handle, const std::string& name) const;
    void   setProperty(const std::string& name, Handle value_handle);
    Handle getPropertyAt(Handle ctx_handle, int index) const;
    int    length() const;

    // Function call
    Handle call(Handle ctx_handle, Handle this_obj, const std::vector<Handle>& args) const;

    // Explicitly release the QuickJS ref (called by close/GC)
    void free();

    // Close handle (alias for free)
    void close() { free(); }

    // Parent context handle (for child tracking)
    [[nodiscard]] Handle parent_context() const noexcept { return parent_ctx_handle_; }
    void set_parent_context(Handle h) noexcept { parent_ctx_handle_ = h; }

private:
    void compute_type();

    JSContext* ctx_ = nullptr;
    void*      raw_ = nullptr;   // heap-allocated copy of JSValue struct
    Type       type_ = Type::Unknown;
    Handle     parent_ctx_handle_ = kInvalidHandle;
};

} // namespace nrp::js
