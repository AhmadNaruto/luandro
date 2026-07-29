package io.github.luandro.js

import java.io.Closeable
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Phase 7: QuickJS Engine — JavaScript Runtime.
 *
 * Top-level container for the QuickJS engine. Manages GC, memory, and
 * the lifecycle of all Context instances. NOT thread-safe.
 * Must be closed after use (implements AutoCloseable via Closeable).
 */
class Runtime internal constructor(internal val handle: Long) : Closeable {
    private val isClosed = AtomicBoolean(false)

    /**
     * Creates a new JavaScript execution Context within this Runtime.
     */
    fun newContext(): Context {
        checkClosed()
        val h = nativeNewContext(handle)
        return Context(h, this)
    }

    /**
     * Runs the JavaScript garbage collector manually.
     */
    fun gc() {
        checkClosed()
        nativeGc(handle)
    }

    /**
     * Updates the maximum heap size for the JavaScript GC.
     */
    fun setMemoryLimit(maxBytes: Long) {
        checkClosed()
        nativeSetMemoryLimit(handle, maxBytes)
    }

    /**
     * Sets the maximum native stack size for the JS engine.
     */
    fun setStackSize(stackBytes: Long) {
        checkClosed()
        nativeSetStackSize(handle, stackBytes)
    }

    /**
     * Returns the current JS heap memory usage in bytes.
     */
    fun memoryUsed(): Long {
        checkClosed()
        return nativeMemoryUsed(handle)
    }

    /**
     * Returns true if this Runtime has not been closed.
     */
    val isLive: Boolean get() = nativeIsLive(handle)

    override fun close() {
        if (isClosed.compareAndSet(false, true)) {
            nativeDestroy(handle)
        }
    }

    private fun checkClosed() {
        if (isClosed.get()) throw IllegalStateException("RuntimeClosedException: Runtime is closed")
    }

    protected fun finalize() { close() }

    private external fun nativeNewContext(handle: Long): Long
    private external fun nativeGc(handle: Long)
    private external fun nativeSetMemoryLimit(handle: Long, maxBytes: Long)
    private external fun nativeSetStackSize(handle: Long, stackBytes: Long)
    private external fun nativeMemoryUsed(handle: Long): Long
    private external fun nativeIsLive(handle: Long): Boolean
    private external fun nativeDestroy(handle: Long)
}
