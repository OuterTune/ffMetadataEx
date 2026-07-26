plugins {
    alias(libs.plugins.android.library)
}

android {
    namespace = "wah.mikooomich.ffmetadataex"
    compileSdk = 37

    defaultConfig {
        minSdk = 24

//        testInstrumentationRunner = "androidx.test.runner.AndroidJUnier"tRunn
        externalNativeBuild {
            cmake {
                arguments += listOf("-DCMAKE_SHARED_LINKER_FLAGS=-Wl,--build-id=none")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
        debug {
            isMinifyEnabled = false
        }
    }

    // TODO: why did i need this again?
//    sourceSets {
//        getByName("main") {
//            jniLibs.srcDirs("ffmpeg-android-maker/output/lib/")
//        }
//    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "4.1.2"
        }
    }

    ndkVersion = "29.0.14206865"

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }
}

dependencies {
    implementation(libs.androidx.core.ktx)
    implementation(libs.androidx.appcompat)
    implementation(libs.material)
    testImplementation(libs.junit)
    androidTestImplementation(libs.androidx.junit)
    androidTestImplementation(libs.androidx.espresso.core)

    implementation(libs.media3)
}