package io.github.luandro.luau

import java.io.Closeable
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Phase 8: Luau Engine — Kotlin public API.
 *
 * A Luau virtual machine that auto-registers the `lexsoup`, `regex`, and `js`
 * native modules as globals upon creation.
 *
 * **Thread safety:** NOT thread-safe. Use a single LuauVM per thread.
 * **Lifecycle:** Must be closed after use (implements [AutoCloseable] via [Closeable]).
 *
 * Usage:
 * ```kotlin
 * Luau.createVM().use { vm ->
 *     val result = vm.execute("return 1 + 2")
 *     println(result) // "3"
 * }
 * ```
 */
class LuauVM internal constructor(internal val handle: Long) : Closeable {

    private val isClosed = AtomicBoolean(false)

    // ----------------------------------------------------------------
    // Script execution
    // ----------------------------------------------------------------

    /**
     * Compiles and executes a Luau script, returning the script's return value.
     *
     * @param script     Luau source code
     * @param chunkName  Name used in error messages (defaults to `"=(chunk)"`)
     * @throws LuauCompileException  if the source has syntax errors
     * @throws LuauRuntimeException  if the script throws an unhandled error
     * @throws IllegalStateException if this VM has been closed
     */
    fun execute(script: String, chunkName: String = "=(chunk)"): LuauValue {
        checkClosed()
        return nativeExecute(handle, script, chunkName)
    }

    /**
     * Compiles Luau source code to bytecode without executing it.
     *
     * @param script     Luau source code
     * @param chunkName  Name used in error messages
     * @return compiled bytecode as a [ByteArray]
     * @throws LuauCompileException  if the source has syntax errors
     * @throws IllegalStateException if this VM has been closed
     */
    fun compile(script: String, chunkName: String = "=(chunk)"): ByteArray {
        checkClosed()
        return nativeCompile(handle, script, chunkName)
    }

    /**
     * Executes precompiled Luau bytecode.
     *
     * @param bytecode   bytecode produced by [compile]
     * @param chunkName  Name used in error messages
     * @throws LuauRuntimeException  if the script throws an unhandled error
     * @throws IllegalStateException if this VM has been closed
     */
    fun executeCompiled(bytecode: ByteArray, chunkName: String = "=(chunk)"): LuauValue {
        checkClosed()
        return nativeExecuteCompiled(handle, bytecode, chunkName)
    }

    // ----------------------------------------------------------------
    // Global state
    // ----------------------------------------------------------------

    /**
     * Sets a global variable in the Luau VM.
     *
     * @param name  global variable name
     * @param value value to set (use [LuauValue.Nil] to unset)
     * @throws IllegalStateException if this VM has been closed
     */
    fun setGlobal(name: String, value: LuauValue) {
        checkClosed()
        nativeSetGlobal(handle, name, value)
    }

    /**
     * Reads a global variable from the Luau VM.
     *
     * @param name  global variable name
     * @return the current value, or [LuauValue.Nil] if unset
     * @throws IllegalStateException if this VM has been closed
     */
    fun getGlobal(name: String): LuauValue {
        checkClosed()
        return nativeGetGlobal(handle, name)
    }

    /**
     * Removes a global variable (sets it to nil).
     *
     * @param name  global variable name
     * @throws IllegalStateException if this VM has been closed
     */
    fun removeGlobal(name: String) {
        checkClosed()
        nativeRemoveGlobal(handle, name)
    }

    // ----------------------------------------------------------------
    // Lifecycle
    // ----------------------------------------------------------------

    /**
     * Returns `true` if this VM is still open.
     */
    val isLive: Boolean get() = !isClosed.get()

    /**
     * Closes the Luau VM and releases all native resources.
     * Idempotent — safe to call multiple times.
     */
    override fun close() {
        if (isClosed.compareAndSet(false, true)) {
            nativeDestroy(handle)
        }
    }

    @Suppress("deprecation")
    protected fun finalize() { close() }

    // ----------------------------------------------------------------
    // Internals
    // ----------------------------------------------------------------

    private fun checkClosed() {
        if (isClosed.get()) throw IllegalStateException("VMClosedException: LuauVM has been closed")
    }

    private external fun nativeExecute(handle: Long, script: String, chunkName: String): LuauValue
    private external fun nativeCompile(handle: Long, script: String, chunkName: String): ByteArray
    private external fun nativeExecuteCompiled(handle: Long, bytecode: ByteArray, chunkName: String): LuauValue
    private external fun nativeSetGlobal(handle: Long, name: String, value: LuauValue)
    private external fun nativeGetGlobal(handle: Long, name: String): LuauValue
    private external fun nativeRemoveGlobal(handle: Long, name: String)
    private external fun nativeDestroy(handle: Long)

    companion object {
        @JvmStatic private external fun nativeCreate(): Long
        @JvmStatic private external fun nativeCreateWithMemoryLimit(maxBytes: Long): Long
    }
}
