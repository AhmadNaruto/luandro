package io.github.luandro.js

import java.io.Closeable
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Phase 7: QuickJS Engine — JavaScript Context.
 *
 * An isolated JavaScript execution environment within a Runtime.
 * Owns its global object, built-ins, and module registry.
 * NOT thread-safe. Must be closed after use.
 */
class Context internal constructor(
    internal val handle: Long,
    private val runtime: Runtime
) : Closeable {
    private val isClosed = AtomicBoolean(false)

    // ----------------------------------------------------------------
    // Script execution
    // ----------------------------------------------------------------

    /**
     * Evaluates a JavaScript expression or script and returns the result.
     * @throws JSException on JS syntax or runtime errors
     */
    fun eval(code: String, filename: String = "<eval>"): JSValue {
        checkClosed()
        val h = nativeEval(handle, code)
        return JSValue(h, this)
    }

    /**
     * Evaluates an ES Module and returns a Promise JSValue.
     * @throws JSException on syntax or runtime errors
     */
    fun evalModule(code: String, filename: String): JSValue {
        checkClosed()
        val h = nativeEvalModule(handle, code, filename)
        return JSValue(h, this)
    }

    // ----------------------------------------------------------------
    // Global object access
    // ----------------------------------------------------------------

    /**
     * Sets a named property on the JavaScript global object.
     * @throws JSException if setting the property fails
     */
    fun setGlobal(name: String, value: JSValue) {
        checkClosed()
        nativeSetGlobal(handle, name, value.handle)
    }

    /**
     * Gets a named property from the JavaScript global object.
     * Returns JSValue.undefined() if not set.
     */
    fun getGlobal(name: String): JSValue {
        checkClosed()
        val h = nativeGetGlobal(handle, name)
        return JSValue(h, this)
    }

    // ----------------------------------------------------------------
    // JSON
    // ----------------------------------------------------------------

    /**
     * Parses a JSON string and returns the corresponding JSValue.
     * @throws JSException if the JSON is malformed
     */
    fun parseJSON(json: String): JSValue {
        checkClosed()
        val h = nativeParseJSON(handle, json)
        return JSValue(h, this)
    }

    /**
     * Converts a JSValue to its JSON string representation.
     * @param indent number of spaces for indentation (0 = compact)
     * @throws JSException if the value contains circular references
     */
    fun stringifyJSON(value: JSValue, indent: Int = 0): String {
        checkClosed()
        return nativeStringifyJSON(handle, value.handle, indent)
    }

    // ----------------------------------------------------------------
    // Value factory helpers
    // ----------------------------------------------------------------

    /** Creates a new empty JavaScript object ({}) */
    fun newObject(): JSValue {
        checkClosed()
        return JSValue(nativeNewObject(handle), this)
    }

    /** Creates a new empty JavaScript array ([]) */
    fun newArray(): JSValue {
        checkClosed()
        return JSValue(nativeNewArray(handle), this)
    }

    /** Creates a JS boolean value */
    fun newBool(value: Boolean): JSValue {
        checkClosed()
        return JSValue(nativeNewBool(handle, value), this)
    }

    /** Creates a JS integer value */
    fun newInt(value: Int): JSValue {
        checkClosed()
        return JSValue(nativeNewInt(handle, value), this)
    }

    /** Creates a JS double value */
    fun newDouble(value: Double): JSValue {
        checkClosed()
        return JSValue(nativeNewDouble(handle, value), this)
    }

    /** Creates a JS string value */
    fun newString(value: String): JSValue {
        checkClosed()
        return JSValue(nativeNewString(handle, value), this)
    }

    /** Returns the JS undefined singleton */
    fun undefined(): JSValue {
        checkClosed()
        return JSValue(nativeUndefined(handle), this)
    }

    /** Returns the JS null singleton */
    fun jsNull(): JSValue {
        checkClosed()
        return JSValue(nativeNull(handle), this)
    }

    // ----------------------------------------------------------------
    // Promise / event loop
    // ----------------------------------------------------------------

    /**
     * Runs all pending microtasks (Promise callbacks).
     * Must be called after eval/evalModule to settle Promises.
     * @return number of jobs executed (-1 on error)
     * @throws JSException if a job throws an unhandled exception
     */
    fun executePendingJobs(): Int {
        checkClosed()
        return nativeExecutePendingJobs(handle)
    }

    // ----------------------------------------------------------------
    // Module loading
    // ----------------------------------------------------------------

    /**
     * Registers a callback that resolves ES module import paths and returns source.
     */
    fun setModuleLoader(loader: ModuleLoader) {
        checkClosed()
        nativeSetModuleLoader(handle, loader)
    }

    // ----------------------------------------------------------------
    // Lifecycle
    // ----------------------------------------------------------------

    override fun close() {
        if (isClosed.compareAndSet(false, true)) {
            nativeDestroy(handle)
        }
    }

    private fun checkClosed() {
        if (isClosed.get()) throw IllegalStateException("ContextClosedException: Context is closed")
    }

    protected fun finalize() { close() }

    // ----------------------------------------------------------------
    // Native declarations
    // ----------------------------------------------------------------

    private external fun nativeEval(handle: Long, code: String): Long
    private external fun nativeEvalModule(handle: Long, code: String, filename: String): Long
    private external fun nativeSetGlobal(handle: Long, name: String, valueHandle: Long)
    private external fun nativeGetGlobal(handle: Long, name: String): Long
    private external fun nativeParseJSON(handle: Long, json: String): Long
    private external fun nativeStringifyJSON(handle: Long, valueHandle: Long, indent: Int): String
    private external fun nativeNewObject(handle: Long): Long
    private external fun nativeNewArray(handle: Long): Long
    private external fun nativeNewBool(handle: Long, value: Boolean): Long
    private external fun nativeNewInt(handle: Long, value: Int): Long
    private external fun nativeNewDouble(handle: Long, value: Double): Long
    private external fun nativeNewString(handle: Long, value: String): Long
    private external fun nativeUndefined(handle: Long): Long
    private external fun nativeNull(handle: Long): Long
    private external fun nativeExecutePendingJobs(handle: Long): Int
    private external fun nativeSetModuleLoader(handle: Long, loader: ModuleLoader)
    private external fun nativeDestroy(handle: Long)
}
