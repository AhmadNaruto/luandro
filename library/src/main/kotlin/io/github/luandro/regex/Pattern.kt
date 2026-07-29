package io.github.luandro.regex

import java.io.Closeable
import java.util.concurrent.atomic.AtomicBoolean

class Pattern internal constructor(internal val handle: Long) : Closeable {
    private val isClosed = AtomicBoolean(false)

    val pattern: String
        get() {
            checkClosed()
            return nativePattern(handle)
        }

    val flags: String
        get() {
            checkClosed()
            return nativeFlags(handle)
        }

    fun matcher(input: String): Matcher {
        checkClosed()
        val mh = nativeMatcher(handle, input)
        return Matcher(mh)
    }

    fun matches(input: String): Boolean {
        checkClosed()
        return nativeMatches(handle, input)
    }

    fun find(input: String): MatchResult? {
        checkClosed()
        val mrh = nativeFind(handle, input)
        return if (mrh != 0L) MatchResult(mrh) else null
    }

    fun findAll(input: String): List<MatchResult> {
        checkClosed()
        val handles = nativeFindAll(handle, input)
        return handles.map { MatchResult(it) }
    }

    fun replace(input: String, replacement: String): String {
        checkClosed()
        return nativeReplace(handle, input, replacement)
    }

    fun replaceAll(input: String, replacement: String): String {
        checkClosed()
        return nativeReplaceAll(handle, input, replacement)
    }

    fun split(input: String): List<String> {
        checkClosed()
        return nativeSplit(handle, input).toList()
    }

    override fun close() {
        if (isClosed.compareAndSet(false, true)) {
            nativeClose(handle)
        }
    }

    private fun checkClosed() {
        if (isClosed.get()) {
            throw IllegalStateException("Pattern is closed")
        }
    }

    protected fun finalize() {
        close()
    }

    private external fun nativePattern(handle: Long): String
    private external fun nativeFlags(handle: Long): String
    private external fun nativeMatcher(handle: Long, input: String): Long
    private external fun nativeMatches(handle: Long, input: String): Boolean
    private external fun nativeFind(handle: Long, input: String): Long
    private external fun nativeFindAll(handle: Long, input: String): LongArray
    private external fun nativeReplace(handle: Long, input: String, replacement: String): String
    private external fun nativeReplaceAll(handle: Long, input: String, replacement: String): String
    private external fun nativeSplit(handle: Long, input: String): Array<String>
    private external fun nativeClose(handle: Long)
}
