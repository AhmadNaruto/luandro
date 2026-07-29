package io.github.luandro.js

/**
 * Phase 7: QuickJS Engine — Top-level JS module entry point.
 *
 * Static factory for creating Runtime instances.
 * All JavaScript execution flows through: Runtime → Context → eval → JSValue
 */
object JS {
    init {
        System.loadLibrary("luandro_nrp")
    }

    /**
     * Creates a new JavaScript Runtime with default memory limits.
     */
    fun createRuntime(): Runtime {
        val h = nativeCreateRuntime()
        return Runtime(h)
    }

    /**
     * Creates a new Runtime with a maximum heap size for the JS GC.
     * @param maxHeapBytes maximum heap size in bytes (>= 1 MB recommended)
     */
    fun createRuntimeWithLimit(maxHeapBytes: Long): Runtime {
        require(maxHeapBytes > 0) { "maxHeapBytes must be > 0" }
        val h = nativeCreateRuntimeWithLimit(maxHeapBytes)
        return Runtime(h)
    }

    @JvmStatic private external fun nativeCreateRuntime(): Long
    @JvmStatic private external fun nativeCreateRuntimeWithLimit(maxHeapBytes: Long): Long
}
