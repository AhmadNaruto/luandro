package io.github.luandro.lexsoup

class ElementClosedException(message: String) : IllegalStateException(message)

class Element(internal val handle: Long) : AutoCloseable {

    init {
        try {
            System.loadLibrary("luandro_nrp")
        } catch (e: UnsatisfiedLinkError) {}
    }

    private var isClosed = false

    private fun checkClosed() {
        if (isClosed) {
            throw ElementClosedException("Element is already closed")
        }
    }

    fun tagName(): String {
        checkClosed()
        return nativeTagName(handle)
    }

    fun attr(key: String): String {
        checkClosed()
        return nativeAttrGet(handle, key)
    }

    fun attr(key: String, value: String): Element {
        checkClosed()
        nativeAttrSet(handle, key, value)
        return this
    }

    fun hasAttr(key: String): Boolean {
        checkClosed()
        return nativeHasAttr(handle, key)
    }

    fun removeAttr(key: String): Element {
        checkClosed()
        nativeRemoveAttr(handle, key)
        return this
    }

    fun attributes(): Map<String, String> {
        checkClosed()
        val keys = nativeAttrKeys(handle)
        val map = HashMap<String, String>()
        for (k in keys) {
            map[k] = attr(k)
        }
        return map
    }

    fun id(): String {
        checkClosed()
        return attr("id")
    }

    fun className(): String {
        checkClosed()
        return attr("class")
    }

    fun classNames(): Set<String> {
        checkClosed()
        val cn = className().trim()
        if (cn.isEmpty()) return emptySet()
        return cn.split(Regex("\\s+")).toSet()
    }

    fun hasClass(cls: String): Boolean {
        checkClosed()
        return classNames().contains(cls)
    }

    fun text(): String {
        checkClosed()
        return nativeText(handle)
    }

    fun ownText(): String {
        checkClosed()
        return nativeOwnText(handle)
    }

    fun html(): String {
        checkClosed()
        return nativeHtml(handle)
    }

    fun outerHtml(): String {
        checkClosed()
        return nativeOuterHtml(handle)
    }

    fun innerHTML(): String {
        return html()
    }

    fun parent(): Element? {
        checkClosed()
        val ph = nativeParent(handle)
        return if (ph != 0L) Element(ph) else null
    }

    fun children(): Elements {
        checkClosed()
        val esh = nativeChildren(handle)
        return Elements(esh)
    }

    fun child(index: Int): Element {
        checkClosed()
        val ch = nativeChild(handle, index)
        if (ch == 0L) {
            throw IndexOutOfBoundsException("Child index out of bounds: $index")
        }
        return Element(ch)
    }

    fun childrenSize(): Int {
        checkClosed()
        return nativeChildrenSize(handle)
    }

    fun firstElementChild(): Element? {
        checkClosed()
        val ch = nativeFirstElementChild(handle)
        return if (ch != 0L) Element(ch) else null
    }

    fun lastElementChild(): Element? {
        checkClosed()
        val ch = nativeLastElementChild(handle)
        return if (ch != 0L) Element(ch) else null
    }

    fun nextElementSibling(): Element? {
        checkClosed()
        val sh = nativeNextElementSibling(handle)
        return if (sh != 0L) Element(sh) else null
    }

    fun previousElementSibling(): Element? {
        checkClosed()
        val sh = nativePreviousElementSibling(handle)
        return if (sh != 0L) Element(sh) else null
    }

    fun siblingElements(): Elements {
        checkClosed()
        val esh = nativeSiblingElements(handle)
        return Elements(esh)
    }

    fun select(cssQuery: String): Elements {
        checkClosed()
        val esh = nativeSelect(handle, cssQuery)
        return Elements(esh)
    }

    fun selectFirst(cssQuery: String): Element? {
        checkClosed()
        val eh = nativeSelectFirst(handle, cssQuery)
        return if (eh != 0L) Element(eh) else null
    }

    fun `is`(cssQuery: String): Boolean {
        checkClosed()
        return nativeIs(handle, cssQuery)
    }

    fun closest(cssQuery: String): Element? {
        checkClosed()
        val eh = nativeClosest(handle, cssQuery)
        return if (eh != 0L) Element(eh) else null
    }

    fun text(value: String): Element {
        checkClosed()
        nativeSetText(handle, value)
        return this
    }

    fun html(value: String): Element {
        checkClosed()
        nativeSetHtml(handle, value)
        return this
    }

    fun append(html: String): Element {
        checkClosed()
        nativeAppend(handle, html)
        return this
    }

    fun prepend(html: String): Element {
        checkClosed()
        nativePrepend(handle, html)
        return this
    }

    fun after(html: String): Element {
        checkClosed()
        nativeAfter(handle, html)
        return this
    }

    fun before(html: String): Element {
        checkClosed()
        nativeBefore(handle, html)
        return this
    }

    fun remove() {
        checkClosed()
        nativeRemove(handle)
    }

    fun wrap(html: String): Element {
        checkClosed()
        nativeWrap(handle, html)
        return this
    }

    fun unwrap(): Element? {
        checkClosed()
        val ph = nativeUnwrap(handle)
        return if (ph != 0L) Element(ph) else null
    }

    override fun close() {
        if (!isClosed) {
            nativeClose(handle)
            isClosed = true
        }
    }

    private external fun nativeTagName(handle: Long): String
    private external fun nativeAttrGet(handle: Long, key: String): String
    private external fun nativeAttrSet(handle: Long, key: String, value: String)
    private external fun nativeHasAttr(handle: Long, key: String): Boolean
    private external fun nativeRemoveAttr(handle: Long, key: String)
    private external fun nativeAttrKeys(handle: Long): Array<String>
    private external fun nativeText(handle: Long): String
    private external fun nativeOwnText(handle: Long): String
    private external fun nativeHtml(handle: Long): String
    private external fun nativeOuterHtml(handle: Long): String
    private external fun nativeParent(handle: Long): Long
    private external fun nativeChildren(handle: Long): Long
    private external fun nativeChild(handle: Long, index: Int): Long
    private external fun nativeChildrenSize(handle: Long): Int
    private external fun nativeFirstElementChild(handle: Long): Long
    private external fun nativeLastElementChild(handle: Long): Long
    private external fun nativeNextElementSibling(handle: Long): Long
    private external fun nativePreviousElementSibling(handle: Long): Long
    private external fun nativeSiblingElements(handle: Long): Long
    private external fun nativeSelect(handle: Long, query: String): Long
    private external fun nativeSelectFirst(handle: Long, query: String): Long
    private external fun nativeIs(handle: Long, query: String): Boolean
    private external fun nativeClosest(handle: Long, query: String): Long
    private external fun nativeSetText(handle: Long, value: String)
    private external fun nativeSetHtml(handle: Long, value: String)
    private external fun nativeAppend(handle: Long, html: String)
    private external fun nativePrepend(handle: Long, html: String)
    private external fun nativeAfter(handle: Long, html: String)
    private external fun nativeBefore(handle: Long, html: String)
    private external fun nativeRemove(handle: Long)
    private external fun nativeWrap(handle: Long, html: String)
    private external fun nativeUnwrap(handle: Long): Long
    private external fun nativeClose(handle: Long)
}
