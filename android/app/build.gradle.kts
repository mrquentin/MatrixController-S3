plugins {
    id("com.android.application")
}

// Releases pass the version semantic-release picked (-PappVersion=1.2.3); local builds are 0.0.0-dev.
// versionCode has to grow with every release for Android to install it over the previous one.
val appVersion = (findProperty("appVersion") as String?) ?: "0.0.0-dev"
val appVersionCode = Regex("""^(\d+)\.(\d+)\.(\d+)""").find(appVersion)
    ?.destructured
    ?.let { (major, minor, patch) -> major.toInt() * 1_000_000 + minor.toInt() * 1_000 + patch.toInt() }
    ?.coerceAtLeast(1)
    ?: 1

// Android only updates an app with one signed by the same key, so releases are signed with a fixed
// key from the environment (GitHub secrets). Without it, the machine's debug key is used.
val releaseKeystore = System.getenv("ANDROID_KEYSTORE_FILE")?.let(::file)?.takeIf { it.exists() }

android {
    namespace = "com.mrquentinet.matrixcontroller"
    compileSdk = 36

    defaultConfig {
        applicationId = "com.mrquentinet.matrixcontroller"
        minSdk = 26
        targetSdk = 36
        versionCode = appVersionCode
        versionName = appVersion
    }

    signingConfigs {
        if (releaseKeystore != null) {
            create("release") {
                storeFile = releaseKeystore
                storePassword = System.getenv("ANDROID_KEYSTORE_PASSWORD")
                keyAlias = System.getenv("ANDROID_KEY_ALIAS")
                keyPassword = System.getenv("ANDROID_KEY_PASSWORD")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"))
            // Sideloaded only, so no Play signing: a fixed release key, or the debug key locally
            signingConfig = signingConfigs.findByName("release") ?: signingConfigs.getByName("debug")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}
