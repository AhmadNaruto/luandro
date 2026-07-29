package io.github.luandro.luau

/** Thrown when Luau source has syntax errors. */
class LuauCompileException(message: String) : RuntimeException(message)

/** Thrown when a Luau script throws an unhandled runtime error. */
class LuauRuntimeException(message: String) : RuntimeException(message)
