package io.github.luandro.js

/**
 * Phase 7: QuickJS Engine — JavaScript exception.
 *
 * Thrown when a JavaScript error occurs (syntax error, runtime error, etc.)
 * Carries the JS error message and optional stack trace.
 */
class JSException(
    message: String,
    val jsStack: String = ""
) : RuntimeException(message)
