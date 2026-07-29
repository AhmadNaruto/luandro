// native/runtime/exceptions/exception_manager.h
// Phase 3: Runtime Core

#pragma once

#include <exception>
#include <string>
#include <jni.h>

namespace nrp {

class NrpException : public std::exception {
public:
    explicit NrpException(std::string msg, std::string code = "NRP_ERROR")
        : msg_(std::move(msg)), code_(std::move(code)) {}

    const char* what() const noexcept override { return msg_.c_str(); }
    const std::string& code() const noexcept   { return code_; }

private:
    std::string msg_;
    std::string code_;
};

class NrpHandleException : public NrpException {
public:
    explicit NrpHandleException(std::string msg)
        : NrpException(std::move(msg), "NRP_HANDLE_ERROR") {}
};

class NrpTypeException : public NrpException {
public:
    explicit NrpTypeException(std::string msg)
        : NrpException(std::move(msg), "NRP_TYPE_ERROR") {}
};

class NrpParseException : public NrpException {
public:
    explicit NrpParseException(std::string msg)
        : NrpException(std::move(msg), "NRP_PARSE_ERROR") {}
};

class NrpScriptException : public NrpException {
public:
    explicit NrpScriptException(std::string msg)
        : NrpException(std::move(msg), "NRP_SCRIPT_ERROR") {}
};

class NrpMemoryException : public NrpException {
public:
    explicit NrpMemoryException(std::string msg)
        : NrpException(std::move(msg), "NRP_MEMORY_ERROR") {}
};

namespace ExceptionManager {
    void throw_to_java(JNIEnv* env, const NrpException& e) noexcept;
    void throw_to_java(JNIEnv* env, const std::exception& e) noexcept;
    void throw_unknown_to_java(JNIEnv* env) noexcept;
}

} // namespace nrp
