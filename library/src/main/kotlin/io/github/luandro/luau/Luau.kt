package io.github.luandro.luau

/**
 * Phase 8: Luau Engine — Top-level entry point.
 *
 * Static factory for creating [LuauVM] instances.
 * Every VM automatically registers `lexsoup`, `regex`, and `js` as Luau globals.
 *
 * Usage:
 * ```kotlin
 * // Default VM (no memory cap)
 * Luau.createVM().use { vm ->
 *     val result = vm.execute("return lexsoup.parse('<h1>Hello</h1>'):title()")
 *     println(result) // "Hello"
 * }
 *
 * // VM with 64 MB memory limit
 * Luau.createVM(maxHeapBytes = 64 * 1024 * 1024L).use { vm ->
 *     vm.execute("print('memory-limited VM')")
 * }
 * ```
 */
object Luau {

    init {
        System.loadLibrary("luandro_nrp")
    }

    /**
     * Creates a new [LuauVM] with no memory cap.
     *
     * The VM automatically exposes:
     * - `lexsoup` — HTML parser (JSoup-compatible)
     * - `regex`   — Regular expressions
     * - `js`      — QuickJS JavaScript engine
     *
     * @return a new, fully initialised [LuauVM]
     */
    fun createVM(): LuauVM {
        val h = nativeCreate()
        return LuauVM(h)
    }

    /**
     * Creates a new [LuauVM] with a maximum heap size.
     *
     * @param maxHeapBytes maximum memory the VM may allocate (in bytes, must be > 0)
     * @return a new [LuauVM] capped at [maxHeapBytes]
     * @throws IllegalArgumentException if [maxHeapBytes] ≤ 0
     */
    fun createVM(maxHeapBytes: Long): LuauVM {
        require(maxHeapBytes > 0) { "maxHeapBytes must be > 0" }
        val h = nativeCreateWithMemoryLimit(maxHeapBytes)
        return LuauVM(h)
    }

    @JvmStatic private external fun nativeCreate(): Long
    @JvmStatic private external fun nativeCreateWithMemoryLimit(maxBytes: Long): Long
}
