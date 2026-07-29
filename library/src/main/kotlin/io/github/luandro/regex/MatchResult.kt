package io.github.luandro.regex

import java.io.Closeable
import java.util.concurrent.atomic.AtomicBoolean

class MatchResult internal constructor(internal val handle: Long) : Closeable {
    private val isClosed = AtomicBoolean(false)

    val value: String
        get() {
            checkClosed()
            return nativeValue(handle)
        }

    val range: IntRange
        get() {
            checkClosed()
            return IntRange(nativeStart(handle), nativeEnd(handle) - 1)
        }

    val groupCount: Int
        get() {
            checkClosed()
            return nativeGroupCount(handle)
        }

    fun groupValue(index: Int): String? {
        checkClosed()
        return nativeGroupValue(handle, index)
    }

    override fun close() {
        if (isClosed.compareAndSet(false, true)) {
            nativeClose(handle)
        }
    }

    private fun checkClosed() {
        if (isClosed.get()) {
            throw IllegalStateException("MatchResult is closed")
        }
    }

    protected fun finalize() {
        close()
    }

    private external fun nativeValue(handle: Long): String
    private external fun nativeStart(handle: Long): Int
    private external fun nativeEnd(handle: Long): Int
    private external fun nativeGroupCount(handle: Long): Int
    private external fun nativeGroupValue(handle: Long, index: Int): String?
    private external fun nativeClose(handle: Long)
}
