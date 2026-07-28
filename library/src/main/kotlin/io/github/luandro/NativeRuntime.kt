// NativeRuntime.kt — Luandro Native Runtime Platform
// Phase 0: Minimal Kotlin stub to verify build.
// This is NOT the final API. Real API is defined in Phase 2.

package io.github.luandro

/**
 * Entry point for the Luandro Native Runtime Platform.
 *
 * Phase 0: Skeleton only — verifies the build system and JNI loading.
 * Real implementation begins in Phase 3 (Runtime Core).
 */
object NativeRuntime {

    init {
        System.loadLibrary("luandro_nrp")
    }

    /**
     * Returns the NRP version string.
     * Phase 0: returns a hardcoded value until Phase 3.
     */
    fun version(): String = "0.1.0-phase0"
}
