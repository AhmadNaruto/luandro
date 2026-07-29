package io.github.luandro.regex

import java.io.Closeable
import java.util.concurrent.atomic.AtomicBoolean

class Matcher internal constructor(internal val handle: Long) : Closeable {
    private val isClosed = AtomicBoolean(false)

    val pattern: Pattern
        get() {
            checkClosed()
            return Pattern(nativeGetPattern(handle))
        }

    val input: String
        get() {
            checkClosed()
            return nativeGetInput(handle)
        }

    val hasMatch: Boolean
        get() {
            checkClosed()
            return nativeHasMatch(handle)
        }

    fun matches(): Boolean {
        checkClosed()
        return nativeMatches(handle)
    }

    fun find(): Boolean {
        checkClosed()
        return nativeFind(handle)
    }

    fun findFrom(startIndex: Int): Boolean {
        checkClosed()
        return nativeFindFrom(handle, startIndex)
    }

    fun lookingAt(): Boolean {
        checkClosed()
        return nativeLookingAt(handle)
    }

    fun group(): String? {
        checkClosed()
        return nativeGroup(handle)
    }

    fun groupByIndex(groupIndex: Int): String? {
        checkClosed()
        return nativeGroupByIndex(handle, groupIndex)
    }

    fun groupCount(): Int {
        checkClosed()
        return nativeGroupCount(handle)
    }

    fun start(): Int {
        checkClosed()
        return nativeStart(handle)
    }

    fun end(): Int {
        checkClosed()
        return nativeEnd(handle)
    }

    fun reset(): Matcher {
        checkClosed()
        nativeReset(handle)
        return this
    }

    fun resetWithInput(input: String): Matcher {
        checkClosed()
        nativeResetWithInput(handle, input)
        return this
    }

    fun replaceAll(replacement: String): String {
        checkClosed()
        return nativeReplaceAll(handle, replacement)
    }

    fun replaceFirst(replacement: String): String {
        checkClosed()
        return nativeReplaceFirst(handle, replacement)
    }

    fun toMatchResult(): MatchResult {
        checkClosed()
        val mrh = nativeToMatchResult(handle)
        return MatchResult(mrh)
    }

    override fun close() {
        if (isClosed.compareAndSet(false, true)) {
            nativeClose(handle)
        }
    }

    private fun checkClosed() {
        if (isClosed.get()) {
            throw IllegalStateException("MatcherClosedException: Matcher is closed")
        }
    }

    protected fun finalize() {
        close()
    }

    private external fun nativeGetPattern(handle: Long): Long
    private external fun nativeGetInput(handle: Long): String
    private external fun nativeHasMatch(handle: Long): Boolean
    private external fun nativeMatches(handle: Long): Boolean
    private external fun nativeFind(handle: Long): Boolean
    private external fun nativeFindFrom(handle: Long, startIndex: Int): Boolean
    private external fun nativeLookingAt(handle: Long): Boolean
    private external fun nativeGroup(handle: Long): String?
    private external fun nativeGroupByIndex(handle: Long, groupIndex: Int): String?
    private external fun nativeGroupCount(handle: Long): Int
    private external fun nativeStart(handle: Long): Int
    private external fun nativeEnd(handle: Long): Int
    private external fun nativeReset(handle: Long)
    private external fun nativeResetWithInput(handle: Long, input: String)
    private external fun nativeReplaceAll(handle: Long, replacement: String): String
    private external fun nativeReplaceFirst(handle: Long, replacement: String): String
    private external fun nativeToMatchResult(handle: Long): Long
    private external fun nativeClose(handle: Long)
}
