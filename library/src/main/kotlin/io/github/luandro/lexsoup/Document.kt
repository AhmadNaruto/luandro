package io.github.luandro.lexsoup

class DocumentClosedException(message: String) : IllegalStateException(message)

class Document(internal val handle: Long) : AutoCloseable {

    init {
        try {
            System.loadLibrary("luandro_nrp")
        } catch (e: UnsatisfiedLinkError) {}
    }

    private var isClosed = false

    private fun checkClosed() {
        if (isClosed) {
            throw DocumentClosedException("Document is already closed")
        }
    }

    fun title(): String {
        checkClosed()
        return nativeTitle(handle)
    }

    fun body(): Element? {
        checkClosed()
        val eh = nativeBody(handle)
        return if (eh != 0L) Element(eh) else null
    }

    fun head(): Element? {
        checkClosed()
        val eh = nativeHead(handle)
        return if (eh != 0L) Element(eh) else null
    }

    fun select(cssQuery: String): Elements {
        checkClosed()
        val esh = nativeSelect(handle, cssQuery)
        if (esh == 0L) {
            throw RuntimeException("Select query failed")
        }
        return Elements(esh)
    }

    fun getElementById(id: String): Element? {
        checkClosed()
        val eh = nativeGetElementById(handle, id)
        return if (eh != 0L) Element(eh) else null
    }

    fun getElementsByTag(tag: String): Elements {
        checkClosed()
        val esh = nativeGetElementsByTag(handle, tag)
        return Elements(esh)
    }

    fun getElementsByClass(cls: String): Elements {
        checkClosed()
        val esh = nativeGetElementsByClass(handle, cls)
        return Elements(esh)
    }

    fun outerHtml(): String {
        checkClosed()
        return nativeOuterHtml(handle)
    }

    fun text(): String {
        checkClosed()
        return nativeText(handle)
    }

    override fun close() {
        if (!isClosed) {
            nativeClose(handle)
            isClosed = true
        }
    }

    private external fun nativeTitle(handle: Long): String
    private external fun nativeBody(handle: Long): Long
    private external fun nativeHead(handle: Long): Long
    private external fun nativeSelect(handle: Long, query: String): Long
    private external fun nativeGetElementById(handle: Long, id: String): Long
    private external fun nativeGetElementsByTag(handle: Long, tag: String): Long
    private external fun nativeGetElementsByClass(handle: Long, cls: String): Long
    private external fun nativeOuterHtml(handle: Long): String
    private external fun nativeText(handle: Long): String
    private external fun nativeClose(handle: Long)
}
