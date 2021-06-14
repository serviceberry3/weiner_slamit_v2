package orb.slam2.android;

import android.content.Context;
import android.util.Log;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.LoaderCallbackInterface;

public class OpenCvLoaderCallback extends BaseLoaderCallback {
    public final String TAG = "OpenCvLoaderCallback";

    public OpenCvLoaderCallback(Context AppContext) {
        super(AppContext);
    }

    @Override
    public void onManagerConnected(int status) {
        super.onManagerConnected(status);

        Log.i(TAG, "BaseLoaderCallback called!");

        if (status == LoaderCallbackInterface.SUCCESS) {//instantiate everything we need from OpenCV
            //everything succeeded
            Log.i(TAG, "OpenCV loaded successfully, everything created");
        }

        else {
            super.onManagerConnected(status);
        }
    }
}
