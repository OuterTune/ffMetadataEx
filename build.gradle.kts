plugins {
	id("com.android.library")
	kotlin("android")
}

kotlin {
	jvmToolchain(17)
}

android {
	namespace = "wah.mikooomich.ffMetadataEx"
	compileSdk = 35

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
		sourceCompatibility = JavaVersion.VERSION_17
		targetCompatibility = JavaVersion.VERSION_17
	}
	kotlinOptions {
		jvmTarget = "17"
	}
}
