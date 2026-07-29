// Root build.gradle.kts — Luandro Native Runtime Platform
// Phase 0: Repository Foundation
// No implementation here. Only plugin declarations.

plugins {
    alias(libs.plugins.android.application) apply false
    alias(libs.plugins.android.library) apply false
    alias(libs.plugins.kotlin.android) apply false
    alias(libs.plugins.kotlin.compose) apply false
    alias(libs.plugins.dokka) apply false
}
