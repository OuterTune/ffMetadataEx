plugins {
	alias(libs.plugins.android.application)
	alias(libs.plugins.kotlin.android)
}

android {
	namespace = "wah.mikooomich.ffMetadataEx"
	compileSdk = 35

	defaultConfig {
		applicationId = "wah.mikooomich.ffMetadataEx"
		minSdk = 24
		targetSdk = 35
		versionCode = 1
		versionName = "0.1.0"

		externalNativeBuild {
			cmake {
				arguments += listOf("-DCMAKE_SHARED_LINKER_FLAGS=-Wl,--build-id=none")
			}
		}

		testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
	}

	buildTypes {
		release {
			isMinifyEnabled = true
			isShrinkResources = true
			isCrunchPngs = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
		}
	}

	splits {
		abi {
			isEnable = true
			reset()

			include("x86", "x86_64", "armeabi-v7a", "arm64-v8a")
			isUniversalApk = true
		}
	}

	sourceSets {
		getByName("main") {
			jniLibs.srcDirs("src/main/cpp/ffmpeg-android-maker/output/lib/")
		}
	}

	externalNativeBuild {
		cmake {
			path = file("src/main/cpp/CMakeLists.txt")
			version = "3.31.6"
		}
	}

	ndkVersion = "29.0.13113456"

	applicationVariants.all {
		val variant = this
		variant.outputs
			.map { it as com.android.build.gradle.internal.api.BaseVariantOutputImpl }
			.forEach { output ->
				val outputFileName = "-${variant.versionName}-${variant.baseName}.apk"
				output.outputFileName = output.outputFileName.substringBeforeLast(".apk") + outputFileName
			}
	}

	compileOptions {
		sourceCompatibility = JavaVersion.VERSION_17
		targetCompatibility = JavaVersion.VERSION_17
	}
	kotlinOptions {
		jvmTarget = "17"
	}
}

dependencies {

	implementation(libs.androidx.core.ktx)
	implementation(libs.androidx.appcompat)
	implementation(libs.material)
	testImplementation(libs.junit)
	androidTestImplementation(libs.androidx.junit)
	androidTestImplementation(libs.androidx.espresso.core)

	implementation(libs.timber)

}