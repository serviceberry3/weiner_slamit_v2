I downloaded the code linked in description. I'm fixing it for use on Google Pixel 4. It currently crashes when user tries to start up SLAM.

UPDATE(10/08/20): now running on Google Pixel 4. Main probem is that the so libraries in jniLibs were set to be built by ndk-build as 32-bit object files, whereas the Pixel 4 is expecting 64-bit files. The version of OpenCV used in this project, version 2.4.9, doesn't have 64-bit support, so we'll have to try to use 32-bit. We can trick the app into allowing 32-bit. Specific changes that need to be made:

In FILE gradle.properties ADD android.useDeprecatedNdk=true
In FILE oRB_SLAM2_Android/build.gradle WITHIN android{} WITHIN defaultConfig{} WITHIN ndk{} ADD abiFilters 'armeabi-v7a', 'x86', 'mips', 'armeabi'
In FILE oRB_SLAM2_Android/src/main/jni/Android.mk VERIFY PATH OF OpenCV.mk
In FILE oRB_SLAM2_Android/src/main/jni/Application.mk CHANGE APP_ABI := armeabi TO APP_ABI := x86_64

The app is still not running successfully. Upon tapping "Finish and start SLAM," we only see a blank screen. Working on it.
