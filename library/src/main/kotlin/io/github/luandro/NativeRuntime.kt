// NativeRuntime.kt — Luandro Native Runtime Platform
// Phase 3: Runtime Core

package io.github.luandro

/**
 * Entry point for the Luandro Native Runtime Platform.
 */
object NativeRuntime {

    init {
        System.loadLibrary("luandro_nrp")
    }

    /**
     * Returns the NRP version string from the native layer.
     */
    fun version(): String = nativeVersion()

    private external fun nativeVersion(): String
}
