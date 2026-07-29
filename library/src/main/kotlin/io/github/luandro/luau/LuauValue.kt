package io.github.luandro.luau

/**
 * Phase 8: Luau Engine — Tagged value wrapper.
 *
 * Represents any value that can be passed to or returned from a LuauVM.
 * Mirrors the native LuauValue tagged union.
 */
sealed class LuauValue {

    object Nil : LuauValue() {
        override fun isNil()    = true
        override fun toString() = "nil"
    }

    data class Bool(val value: Boolean) : LuauValue() {
        override fun isBool()      = true
        override fun toBoolean()   = value
        override fun toString()    = value.toString()
    }

    data class Number(val value: Double) : LuauValue() {
        override fun isNumber()    = true
        override fun toDouble()    = value
        override fun toInt()       = value.toInt()
        override fun toString(): String {
            return if (value == value.toLong().toDouble()) value.toLong().toString()
            else value.toString()
        }
    }

    data class Str(val value: String) : LuauValue() {
        override fun isString()    = true
        override fun toString()    = value
    }

    // ---- Type checks (open, overridden per subclass) ----
    open fun isNil():      Boolean = false
    open fun isBool():     Boolean = false
    open fun isNumber():   Boolean = false
    open fun isString():   Boolean = false

    // ---- Value extractors ----
    open fun toBoolean(): Boolean = when (this) {
        is Nil  -> false
        is Bool -> value
        else    -> true
    }
    open fun toDouble(): Double = when (this) {
        is Number -> value
        is Str    -> value.toDoubleOrNull() ?: 0.0
        else      -> 0.0
    }
    open fun toInt(): Int = toDouble().toInt()

    companion object {
        /** Returns the Nil singleton. */
        @JvmStatic fun nil(): LuauValue = Nil

        /** Wraps a Boolean. */
        @JvmStatic fun of(value: Boolean): LuauValue = Bool(value)

        /** Wraps a Double. */
        @JvmStatic fun of(value: Double): LuauValue = Number(value)

        /** Wraps an Int as a Number. */
        @JvmStatic fun of(value: Int): LuauValue = Number(value.toDouble())

        /** Wraps a Long as a Number (may lose precision > 2^53). */
        @JvmStatic fun of(value: Long): LuauValue = Number(value.toDouble())

        /** Wraps a String. */
        @JvmStatic fun of(value: String): LuauValue = Str(value)
    }
}
