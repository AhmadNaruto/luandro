package io.github.luandro.js

import java.io.Closeable
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Phase 7: QuickJS Engine — JavaScript Value wrapper.
 *
 * Wraps any JavaScript value (primitive or object) from a Context.
 * Ref-counted by QuickJS internally.
 * Must not outlive the Context that created it.
 * Call free() to release early, or let the Context close handle cleanup.
 */
class JSValue internal constructor(
    internal val handle: Long,
    private val context: Context
) : Closeable {
    private val isFreed = AtomicBoolean(false)

    // ----------------------------------------------------------------
    // Type checks
    // ----------------------------------------------------------------

    fun isUndefined(): Boolean = nativeIsUndefined(handle)
    fun isNull():      Boolean = nativeIsNull(handle)
    fun isBool():      Boolean = nativeIsBool(handle)
    fun isNumber():    Boolean = nativeIsNumber(handle)
    fun isString():    Boolean = nativeIsString(handle)
    fun isObject():    Boolean = nativeIsObject(handle)
    fun isArray():     Boolean = nativeIsArray(handle)
    fun isFunction():  Boolean = nativeIsFunction(handle)

    // ----------------------------------------------------------------
    // Value extractors
    // ----------------------------------------------------------------

    fun toBool():   Boolean = nativeToBool(handle)
    fun toInt():    Int     = nativeToInt(handle)
    fun toDouble(): Double  = nativeToDouble(handle)

    override fun toString(): String = nativeToString(handle)

    // ----------------------------------------------------------------
    // Property access (objects / arrays)
    // ----------------------------------------------------------------

    /**
     * Gets a named property from this JS object.
     * @throws ClassCastException if this value is not an object
     * @throws JSException if the getter throws
     */
    fun getProperty(name: String): JSValue {
        checkFreed()
        val h = nativeGetProperty(handle, name)
        return JSValue(h, context)
    }

    /**
     * Sets a named property on this JS object.
     * @throws ClassCastException if this value is not an object
     */
    fun setProperty(name: String, value: JSValue) {
        checkFreed()
        nativeSetProperty(handle, name, value.handle)
    }

    /**
     * Gets an element from a JS array by index.
     * @throws ClassCastException if not an array or object
     */
    fun getPropertyAt(index: Int): JSValue {
        checkFreed()
        val h = nativeGetPropertyAt(handle, index)
        return JSValue(h, context)
    }

    /**
     * Returns the "length" property of an array or string.
     */
    fun length(): Int {
        checkFreed()
        return nativeLength(handle)
    }

    // ----------------------------------------------------------------
    // Function call
    // ----------------------------------------------------------------

    /**
     * Calls this JS value as a function.
     * @param thisObj value of `this` (use context.undefined() for no this)
     * @param args    function arguments
     * @throws ClassCastException if not a function
     * @throws JSException if the function throws
     */
    fun call(thisObj: JSValue? = null, vararg args: JSValue): JSValue {
        checkFreed()
        val thisHandle = thisObj?.handle ?: 0L
        val argHandles = LongArray(args.size) { args[it].handle }
        val h = nativeCall(handle, thisHandle, argHandles)
        return JSValue(h, context)
    }

    // ----------------------------------------------------------------
    // Lifecycle
    // ----------------------------------------------------------------

    /**
     * Explicitly decrements the QuickJS reference count.
     * After free(), this object must not be used.
     */
    fun free() {
        if (isFreed.compareAndSet(false, true)) {
            nativeFree(handle)
        }
    }

    override fun close() = free()

    private fun checkFreed() {
        if (isFreed.get()) throw IllegalStateException("JSValue has been freed")
    }

    protected fun finalize() {
        if (!isFreed.get()) free()
    }

    // ----------------------------------------------------------------
    // Native declarations
    // ----------------------------------------------------------------

    private external fun nativeIsUndefined(handle: Long): Boolean
    private external fun nativeIsNull(handle: Long):      Boolean
    private external fun nativeIsBool(handle: Long):      Boolean
    private external fun nativeIsNumber(handle: Long):    Boolean
    private external fun nativeIsString(handle: Long):    Boolean
    private external fun nativeIsObject(handle: Long):    Boolean
    private external fun nativeIsArray(handle: Long):     Boolean
    private external fun nativeIsFunction(handle: Long):  Boolean
    private external fun nativeToBool(handle: Long):      Boolean
    private external fun nativeToInt(handle: Long):       Int
    private external fun nativeToDouble(handle: Long):    Double
    private external fun nativeToString(handle: Long):    String
    private external fun nativeGetProperty(handle: Long, name: String): Long
    private external fun nativeSetProperty(handle: Long, name: String, valueHandle: Long)
    private external fun nativeGetPropertyAt(handle: Long, index: Int): Long
    private external fun nativeLength(handle: Long): Int
    private external fun nativeCall(handle: Long, thisHandle: Long, args: LongArray): Long
    private external fun nativeFree(handle: Long)
}
