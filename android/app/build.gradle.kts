plugins {
    id("com.android.application")
}

android {
    namespace = "com.mrquentinet.matrixcontroller"
    compileSdk = 36

    defaultConfig {
        applicationId = "com.mrquentinet.matrixcontroller"
        minSdk = 26
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"
    }

    buildTypes {
        release {
            isMinifyEnabled = true
            isShrinkResources = true
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"))
            // Sideloaded only, so the debug key is enough to install it
            signingConfig = signingConfigs.getByName("debug")
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
}
