package io.github.luandro.lexsoup

object LexSoup {
    fun parse(html: String): Document {
        val handle = nativeParse(html)
        if (handle == 0L) {
            throw RuntimeException("Failed to parse HTML document")
        }
        return Document(handle)
    }

    fun parse(html: String, baseUri: String): Document {
        return parse(html)
    }

    @JvmStatic
    private external fun nativeParse(html: String): Long
}
