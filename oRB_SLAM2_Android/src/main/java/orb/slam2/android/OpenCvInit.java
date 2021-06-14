package orb.slam2.android;

import android.content.Context;
import android.util.Log;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;

public class OpenCvInit {
    public static final String TAG = "OpenCvInit";

    public static void tryInitDebug(Context appContext, BaseLoaderCallback loaderCallback) {
        if (!OpenCVLoader.initDebug()) {
            Log.d("OpenCV", "Internal OpenCV library not found. Using OpenCV Manager for initialization");
            OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_3_4_0, appContext, loaderCallback);
        } else {
            Log.d("OpenCV", "OpenCV library found inside package. Using it!");
            loaderCallback.onManagerConnected(LoaderCallbackInterface.SUCCESS);
        }
    }
}
