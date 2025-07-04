import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
	id("com.android.library")
	kotlin("android")
}

kotlin {
	jvmToolchain(21)
    compilerOptions {
        jvmTarget.set(JvmTarget.JVM_21)
    }
}

android {
	namespace = "wah.mikooomich.ffMetadataEx"
	compileSdk = 36

	defaultConfig {
		minSdk = 24

		externalNativeBuild {
			cmake {
				arguments += listOf("-DCMAKE_SHARED_LINKER_FLAGS=-Wl,--build-id=none")
			}
		}
	}

	sourceSets {
		getByName("main") {
			jniLibs.srcDirs("ffmpeg-android-maker/output/lib/")
		}
	}

	externalNativeBuild {
		cmake {
			path = file("src/main/cpp/CMakeLists.txt")
			version = "3.31.6"
		}
	}

	ndkVersion = "29.0.13113456"

	compileOptions {
		sourceCompatibility = JavaVersion.VERSION_21
		targetCompatibility = JavaVersion.VERSION_21
	}
}
