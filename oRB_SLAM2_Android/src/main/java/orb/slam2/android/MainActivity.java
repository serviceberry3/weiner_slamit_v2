package orb.slam2.android;


import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.MatOfPoint2f;
import org.opencv.core.MatOfPoint3f;

public class MainActivity extends Activity implements OnClickListener{
	Button datasetMode, testMode;

	private final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
          WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setContentView(R.layout.activity_main);

        datasetMode = (Button)findViewById(R.id.dataset_mode);
		//cameraMode=(Button)findViewById(R.id.camera_mode);

		testMode = (Button)findViewById(R.id.test_mode);

        datasetMode.setOnClickListener(this);
		//cameraMode.setOnClickListener(this);

		testMode.setOnClickListener(this);
    }

	@Override
	public void onClick(View v) {
		switch(v.getId()){
		case R.id.dataset_mode:
			startActivity(new Intent(MainActivity.this, DataSetModeActivity.class));
			break;
		/*
		case R.id.camera_mode:
			Toast.makeText(MainActivity.this, "on the way...", Toast.LENGTH_LONG).show();
			startActivity(new Intent(MainActivity.this,CameraModeActivity.class));
			break;*/
		case R.id.test_mode:
			//Toast.makeText(MainActivity.this, "on the way...", Toast.LENGTH_LONG).show();
			startActivity(new Intent(MainActivity.this, CameraModeActivity.class));
			break;
		}
	}

	//use this OpenCV loader callback to instantiate Mat objects, otherwise we'll get an error about Mat not being found
	public BaseLoaderCallback mLoaderCallback = new OpenCvLoaderCallback(this);

	@Override
	protected void onResume() {
		super.onResume();

		OpenCvInit.tryInitDebug(this, mLoaderCallback);
	}
}
