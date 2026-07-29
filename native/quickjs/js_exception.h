// native/quickjs/js_exception.h
// Phase 7: QuickJS Engine — Exception hierarchy

#pragma once

#include <exceptions/exception_manager.h>
#include <string>

namespace nrp::js {

/**
 * Thrown when a JavaScript error occurs (syntax error, runtime error, etc.)
 * Carries the JS error message and optional stack trace string.
 */
class JSException : public NrpException {
public:
    explicit JSException(const std::string& message,
                         const std::string& js_stack = "")
        : NrpException(message), stack_(js_stack) {}

    [[nodiscard]] const std::string& jsStack() const noexcept { return stack_; }

private:
    std::string stack_;
};

/**
 * Thrown when a method is called on a destroyed Runtime.
 */
class RuntimeClosedException : public NrpException {
public:
    RuntimeClosedException()
        : NrpException("RuntimeClosedException: Runtime has been closed") {}
};

/**
 * Thrown when a method is called on a destroyed Context.
 */
class ContextClosedException : public NrpException {
public:
    ContextClosedException()
        : NrpException("ContextClosedException: Context has been closed") {}
};

} // namespace nrp::js
