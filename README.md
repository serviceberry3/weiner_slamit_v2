I downloaded the code linked in description. I'm fixing it for use on Google Pixel 4. It currently crashes when user tries to start up SLAM.

UPDATE(10/08/20): now running on Google Pixel 4. Main probem is that the so libraries in jniLibs were set to be built by ndk-build as 32-bit object files, whereas the Pixel 4 is expecting 64-bit files. The version of OpenCV used in this project, version 2.4.9, doesn't have 64-bit support, so we'll have to try to use 32-bit. We can trick the app into allowing 32-bit. Specific changes that need to be made:

In FILE gradle.properties ADD android.useDeprecatedNdk=true  
In FILE oRB_SLAM2_Android/build.gradle WITHIN android{} WITHIN defaultConfig{} WITHIN ndk{} ADD abiFilters 'armeabi-v7a', 'x86', 'mips', 'armeabi'  
In FILE oRB_SLAM2_Android/src/main/jni/Android.mk VERIFY PATH OF OpenCV.mk  
In FILE oRB_SLAM2_Android/src/main/jni/Application.mk VERIFY THAT APP_ABI := armeabi (32-bit), NOT APP_ABI := x86_64 (64-bit) for future builds.    
  
The app is still not running successfully. Upon tapping "Finish and start SLAM," we only see a blank screen, and the process restarts (the kernel reforks it) infinitely. Working on it.

UPDATE(10/09/20): we see the SLAM screen, but the cv::FileStorage constructor isn't working. It won't open the .yaml calibration file for some reason. If we try using ifstream instead to open it, that works. I'm working to resolve this issue.

UPDATE(10/12/20): hardcoded calibration data, everything is working now. The only problem is that the SLAM tracking stops frequently and the Tracking state changes to LOST. This seems to be due to the fact that the call to DetectRelocalizationCandidates() around line 1561 is repeatedly returning no KeyFrame candidates, causing the Relocalization attempts to fail. I'm working to resolve this issue.

UPDATE(10/29/20): I've added TensorFlow Posenet human pose estimation to the app. Looking at Posenet.kt from the Posenet Android example project on github, I rewrote the Posenet tools library in C++ and tried drawing the points on the camera capture preview box to make sure everything was working. It looked good. Now I'm going to add the Posenet points to the ORB-SLAM2 MapDrawer and draw them in a different color so that we can map a human's pose in 3D space.
