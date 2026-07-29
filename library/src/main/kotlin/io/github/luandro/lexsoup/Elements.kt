package io.github.luandro.lexsoup

class ElementsClosedException(message: String) : IllegalStateException(message)

class Elements(internal val handle: Long) : Iterable<Element>, AutoCloseable {

    init {
        try {
            System.loadLibrary("luandro_nrp")
        } catch (e: UnsatisfiedLinkError) {}
    }

    private var isClosed = false

    private fun checkClosed() {
        if (isClosed) {
            throw ElementsClosedException("Elements is already closed")
        }
    }

    fun size(): Int {
        checkClosed()
        return nativeSize(handle)
    }

    fun isEmpty(): Boolean {
        checkClosed()
        return size() == 0
    }

    fun first(): Element? {
        checkClosed()
        val eh = nativeFirst(handle)
        return if (eh != 0L) Element(eh) else null
    }

    fun last(): Element? {
        checkClosed()
        val eh = nativeLast(handle)
        return if (eh != 0L) Element(eh) else null
    }

    fun get(index: Int): Element {
        checkClosed()
        val eh = nativeGet(handle, index)
        if (eh == 0L) {
            throw IndexOutOfBoundsException("Elements index out of bounds: $index")
        }
        return Element(eh)
    }

    fun select(cssQuery: String): Elements {
        checkClosed()
        val esh = nativeSelect(handle, cssQuery)
        return Elements(esh)
    }

    fun attr(key: String): String {
        checkClosed()
        return nativeAttrGet(handle, key)
    }

    fun attr(key: String, value: String): Elements {
        checkClosed()
        nativeAttrSet(handle, key, value)
        return this
    }

    fun hasAttr(key: String): Boolean {
        checkClosed()
        return nativeHasAttr(handle, key)
    }

    fun text(): String {
        checkClosed()
        return nativeText(handle)
    }

    fun outerHtml(): String {
        checkClosed()
        return nativeOuterHtml(handle)
    }

    fun toList(): List<Element> {
        checkClosed()
        val s = size()
        val list = ArrayList<Element>(s)
        for (i in 0 until s) {
            list.add(get(i))
        }
        return list
    }

    override fun iterator(): Iterator<Element> {
        return toList().iterator()
    }

    override fun close() {
        if (!isClosed) {
            nativeClose(handle)
            isClosed = true
        }
    }

    private external fun nativeSize(handle: Long): Int
    private external fun nativeFirst(handle: Long): Long
    private external fun nativeLast(handle: Long): Long
    private external fun nativeGet(handle: Long, index: Int): Long
    private external fun nativeSelect(handle: Long, query: String): Long
    private external fun nativeAttrGet(handle: Long, key: String): String
    private external fun nativeAttrSet(handle: Long, key: String, value: String)
    private external fun nativeHasAttr(handle: Long, key: String): Boolean
    private external fun nativeText(handle: Long): String
    private external fun nativeOuterHtml(handle: Long): String
    private external fun nativeClose(handle: Long)
}
