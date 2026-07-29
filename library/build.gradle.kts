// library/build.gradle.kts — Luandro NRP Android Library Module
// Phase 0: Repository Foundation

plugins {
    alias(libs.plugins.android.library)
    alias(libs.plugins.kotlin.android)
    alias(libs.plugins.dokka)
}

android {
    namespace = "io.github.luandro"
    compileSdk = libs.versions.compileSdk.get().toInt()
    buildToolsVersion = libs.versions.buildTools.get()

    defaultConfig {
        minSdk = libs.versions.minSdk.get().toInt()

        // NDK — single version for entire project (Phase 0.6)
        ndkVersion = libs.versions.ndk.get()

        // ABI: ARM64 only (Phase 0.6)
        ndk {
            abiFilters += "arm64-v8a"
        }

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++20"
                arguments += listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DANDROID_ARM_NEON=TRUE",
                    "-DANDROID_USE_LEGACY_TOOLCHAIN_FILE=OFF"
                )
            }
        }

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
        consumerProguardFiles("consumer-rules.pro")
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildTypes {
        debug {
            // Phase 0.6: Debug — assertions, logging, validation enabled
            externalNativeBuild {
                cmake {
                    arguments += "-DCMAKE_BUILD_TYPE=Debug"
                    cppFlags += listOf("-DDEBUG", "-DNRP_ENABLE_LOGGING", "-DNRP_ENABLE_ASSERTIONS")
                }
            }
        }
        release {
            // Phase 0.6: Release — maximum optimization, strip symbols
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
            externalNativeBuild {
                cmake {
                    arguments += "-DCMAKE_BUILD_TYPE=Release"
                    cppFlags += listOf("-O3", "-DNDEBUG")
                }
            }
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }

    kotlinOptions {
        jvmTarget = "21"
    }

    // Source sets pointing to the modular native layout and generated code
    sourceSets {
        getByName("main") {
            kotlin.srcDirs(
                "src/main/kotlin",
                "../kotlin",
                "../generated/kotlin"
            )
        }
    }
}

// Phase 10 & 11: Automated ASL Binding Generation task
val generateBindings = tasks.register<Exec>("generateBindings") {
    description = "Generates native headers, JNI bindings, Kotlin wrappers, and Luau bindings from ASL specs."
    group = "build setup"
    workingDir = rootDir
    commandLine("python3", "tools/generator/generate.py")
}

tasks.matching { it.name.startsWith("pre") && it.name.endsWith("Build") }.configureEach {
    dependsOn(generateBindings)
}

dependencies {
    testImplementation(libs.junit)
    androidTestImplementation(libs.junit.ext)
}
