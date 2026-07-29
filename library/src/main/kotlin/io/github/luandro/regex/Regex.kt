package io.github.luandro.regex

object Regex {
    init {
        System.loadLibrary("luandro_nrp")
    }

    fun compile(pattern: String, flags: String = ""): Pattern {
        val h = nativeCompile(pattern, flags)
        return Pattern(h)
    }

    fun matches(pattern: String, input: String): Boolean {
        return nativeMatches(pattern, input)
    }

    fun find(pattern: String, input: String): MatchResult? {
        val h = nativeFind(pattern, input)
        return if (h != 0L) MatchResult(h) else null
    }

    fun findAll(pattern: String, input: String): List<MatchResult> {
        val handles = nativeFindAll(pattern, input)
        return handles.map { MatchResult(it) }
    }

    fun replace(pattern: String, input: String, replacement: String): String {
        return nativeReplace(pattern, input, replacement)
    }

    fun replaceAll(pattern: String, input: String, replacement: String): String {
        return nativeReplaceAll(pattern, input, replacement)
    }

    fun split(pattern: String, input: String): List<String> {
        return nativeSplit(pattern, input).toList()
    }

    @JvmStatic
    private external fun nativeCompile(pattern: String, flags: String): Long
    @JvmStatic
    private external fun nativeMatches(pattern: String, input: String): Boolean
    @JvmStatic
    private external fun nativeFind(pattern: String, input: String): Long
    @JvmStatic
    private external fun nativeFindAll(pattern: String, input: String): LongArray
    @JvmStatic
    private external fun nativeReplace(pattern: String, input: String, replacement: String): String
    @JvmStatic
    private external fun nativeReplaceAll(pattern: String, input: String, replacement: String): String
    @JvmStatic
    private external fun nativeSplit(pattern: String, input: String): Array<String>
}
