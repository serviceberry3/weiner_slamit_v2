package orb.slam2.android;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import orb.slam2.android.nativefunc.OrbNdkHelper;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewFrame;
import org.opencv.android.CameraBridgeViewBase.CvCameraViewListener2;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Mat;
import org.tensorflow.lite.examples.noah.lib.Device;
import org.tensorflow.lite.examples.noah.lib.Posenet;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.opengl.GLSurfaceView;
import android.opengl.GLSurfaceView.Renderer;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;

import java.util.Objects;

/**
 * ORB Test Activity For CameraMode
 *
 */
public class ORBSLAMForCameraModeActivity extends Activity implements Renderer,CvCameraViewListener2, View.OnClickListener, LocationListener,
        SensorEventListener {

    Button TrackOnly,calibrationStart, distanceStart;
    boolean calibrationIsStart;
    boolean distanceIsStart;
    TextView dataTextView, distTextView;
    LocationManager locationManager;
    String provider;
    float displacement;
    public static Location location;
    public static double lng;
    public static double lat;
    boolean checkPermission;
    public static SensorManager mSensorManager;
    public Sensor linearAccelerometer;
    public Sensor gravitySensor;
    public Sensor gyroscope;
    public Sensor magnetometer;
    public Sensor accelerometer;
    public static double[] acce;
    public static double[] gyro;
    public static float[] mGravity;
    public static float[] mGeomagnetic;
    public static float[] RMatrix;
    public static float[] IMatrix;
    public static float[] orientation;
    public static float[] RCaptured;
    public double timestamp;
    public double timestampOld;
    public double timeStep;
    public double[] velocity;
    //public RK4 velocityCalculator;
    public float[] caliStartPos;
    public float[] caliEndPos;
    public float[] distStartPos;
    public float[] distEndPos;
    //public float[] scale;
    public float scale;
    public float[] pos;

    private Posenet posenet;


    private static final String TAG = "CameraModeORB2";
    ImageView imgDealed;

    LinearLayout linear;

    String vocPath, calibrationPath;

    private static final int INIT_FINISHED = 0x00010001;

    private CameraBridgeViewBase mOpenCvCameraView;
    private boolean mIsJavaCamera = true;
    private MenuItem mItemSwitchCamera = null;

    private final int CONTEXT_CLIENT_VERSION = 3;

    //OpenGL SurfaceView
    private GLSurfaceView mGLSurfaceView;

    long addr;
    int w,h;
    boolean isSLAMRunning=true;


    public class velocityVerlet {
        public double acceTime;
        public double acceTimeOld;
        public double acceStep;
        public double[] acceOld;
        public int velocityVerletFlag;
        public int startFlag;

        public velocityVerlet(){
            velocityVerletFlag = 0;
            startFlag = 0;
            acceOld = new double[3];
        }
        public void calculateVelocity(){
            if(startFlag == 0) return;
            if(velocityVerletFlag == 0){
                velocityVerletFlag = 1;
                acceTimeOld =  (double)System.currentTimeMillis()/1000.0;
                for(int i = 0;i<3;i++){
                    acceOld[i] = acce[i];
                    velocity[i] = 0;
                }
            }
            else{
                acceTime = (double)System.currentTimeMillis()/1000.0;
                acceStep = acceTime - acceTimeOld;
                acceTimeOld = acceTime;
                for(int i = 0;i<3;i++){
                    velocity[i] = velocity[i] + (acce[i]+acceOld[i])*acceStep/2.0;
                    acceOld[i] = acce[i];
                }
            }
        }
    }

    public class RK4 {
        public double acceStep;
        public double[] acce0;
        public double[] acce1;
        public int RK4Flag;
        public int startFlag;

        public RK4(){
            RK4Flag = 0;
            startFlag = 0;
            acce0 = new double[3];
            acce1 = new double[3];
            acceStep = 0.01;
        }
        public void calculateVelocity(){
            if(startFlag == 0) return;
            if(RK4Flag == 0){
                RK4Flag = 1;
                for(int i = 0;i<3;i++){
                    acce0[i] = acce[i];
                    velocity[i] = 0;
                }
            }
            else if(RK4Flag == 1){
                RK4Flag = 2;
                for(int i = 0;i<3;i++){
                    acce1[i] = acce[i];
                }
            }
            else if(RK4Flag == 2){
                RK4Flag = 1;
                for(int i = 0;i<3;i++){
                    velocity[i] = velocity[i] + acceStep*2/6*(acce0[i]+4*acce1[i]+acce[i]);
                    acce0[i] = acce[i];
                }
            }
        }
    }


    //callback fxn to execute when OpenCV is loaded
    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                {
                    Log.i(TAG, "OpenCV loaded successfully");
                    mOpenCvCameraView.enableView();
                } break;
                default:
                {
                    super.onManagerConnected(status);
                } break;
            }
        }
    };

    //ERRORING - FIXED 10/08
    static {
        //System.loadLibrary("tensorflowlite_jni");
        //System.loadLibrary("tensorflowlite_gpu_jni");
        System.loadLibrary("ORB_SLAM2_EXECUTOR");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "Success");
        //window settings: no title bar, fullscreen, keep screen on
        requestWindowFeature(Window.FEATURE_NO_TITLE);// 隐藏标题

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);// 设置全屏
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        //instantiate new Posenet session
        posenet = new Posenet(this, "posenet_model.tflite", Device.GPU);

        //set the view
        setContentView(R.layout.activity_test_orb);

        //declarations/initializations
        calibrationIsStart = false;
        displacement = 0;
        distanceIsStart = false;
        pos = new float[3];
        //scale = new float[3];
        //scale[0] = 1;
        //scale[1] = 1;
        //scale[2] = 1;
        scale = 1;
        caliStartPos = new float[3];
        caliEndPos = new float[3];
        distStartPos = new float[3];
        distEndPos = new float[3];
        //velocityCalculator = new RK4();
        acce = new double[3];
        velocity = new double[3];
        gyro = new double[3];
        RMatrix = new float[9];
        IMatrix = new float[9];
        orientation = new float[3];
        RCaptured = RMatrix;


        dataTextView = (TextView) findViewById(R.id.dataTextView);
        distTextView = (TextView) findViewById(R.id.distTextView);
        TrackOnly = (Button) findViewById(R.id.track_only);
        //dataCollection = (Button) findViewById(R.id.data_collection);
        calibrationStart = (Button) findViewById(R.id.calibrationStart);
        distanceStart = (Button) findViewById(R.id.distanceStart);
        //calibrationEnd = (Button) findViewById(R.id.calibrationEnd);
        TrackOnly.setOnClickListener(this);
        //dataCollection.setOnClickListener(this);
        calibrationStart.setOnClickListener(this);
        distanceStart.setOnClickListener(this);
        //calibrationEnd.setOnClickListener(this);

        imgDealed = (ImageView) findViewById(R.id.img_dealed);

        //mIsJavaCamera is bool describing whether or not we're using JavaCameraView. Which we always are, it seems.
        if (mIsJavaCamera)
            mOpenCvCameraView = (CameraBridgeViewBase) findViewById(R.id.tutorial1_activity_java_surface_view);
        else
            mOpenCvCameraView = (CameraBridgeViewBase) findViewById(R.id.tutorial1_activity_native_surface_view);

        //check the the view was found by ID and all is well
        if (mOpenCvCameraView == null) {
            Log.e(TAG, "mOpenCvCameraView came up null");
        } else {
            Log.d(TAG, "mOpenCvCameraView non-null, OK");
        }

        //make our OpenCvCameraView visible and set the listener for the camera
        mOpenCvCameraView.setVisibility(SurfaceView.VISIBLE);
        mOpenCvCameraView.setCvCameraViewListener(this);

        //instantiate new GLSurfaceView
        mGLSurfaceView = new GLSurfaceView(this);
        linear = (LinearLayout) findViewById(R.id.surfaceLinear);

        //mGLSurfaceView.setEGLContextClientVersion(CONTEXT_CLIENT_VERSION);

        //set the renderer for the GLSurfaceView
        mGLSurfaceView.setRenderer(this);


        linear.addView(mGLSurfaceView, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.MATCH_PARENT));

        //get the file paths: dataset path and calibration file path
        /*
        vocPath = getIntent().getStringExtra("voc");
        calibrationPath = getIntent().getStringExtra("calibration");

        vocPath = "/storage/emulated/0/Android/data/orb.slam2.android/files/ORBvoc.txt";
        calibrationPath = "/storage/emulated/0/Android/data/orb.slam2.android/files/List3.yaml";

        vocPath = "/data/media/0/Android/data/orb.slam2.android/files/ORBvoc.txt";
        calibrationPath = "/data/media/0/Android/data/orb.slam2.android/files/List3.yaml";
         */

        //USED FOR AOSP DEV
        vocPath = "/system/files/SLAM/ORBvoc.txt";
        calibrationPath = "/system/files/SLAM/List3.yaml";

        //vocPath = "/sdcard/SLAM/ORBvoc.txt";
       //calibrationPath = "/sdcard/SLAM/List3.yaml";

        //make sure both dataset and calibration paths were set by user
        if (TextUtils.isEmpty(vocPath) || TextUtils.isEmpty(calibrationPath)) {
            Toast.makeText(this, "One of file paths is NULL!", Toast.LENGTH_LONG).show();
            Log.e(TAG, "One of paths is NULL!");
            finish();
        }

        else {
            Log.i(TAG, "Vocab path is " + vocPath);
            Log.i(TAG, "Calib path is " + calibrationPath);

            Log.i(TAG, "Both paths found, OK");
            Toast.makeText(ORBSLAMForCameraModeActivity.this, "Init has been started!", Toast.LENGTH_LONG).show();


            //start initialization on a new thread (in background)
            new Thread(new Runnable() {
                @Override
                public void run() {
                    Log.i(TAG, "Trying to run initSystemWithParameters on auxiliary thread now...");

                    //run native code fxn to initialize the SLAM system using the calibration parameters given
                    OrbNdkHelper.initSystemWithParameters(vocPath, calibrationPath);

                    Log.i(TAG, "Init has finished successfully!");

                    myHandler.sendEmptyMessage(INIT_FINISHED);
                }
            }).start();
        }

//        ////GPS
//        checkPermission = true;
//        locationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
//        provider = locationManager.getBestProvider(new Criteria(), false);
//        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
//            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED && ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
//                requestPermissions(new String[]{
//                        Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION
//                }, 10);
//                return;
//            }
//        }
//        location = locationManager.getLastKnownLocation(provider);
//        if (location != null) {
//            Toast.makeText(getApplicationContext(), "Location achieved", Toast.LENGTH_SHORT).show();
//        } else {
//            Toast.makeText(getApplicationContext(), "Location not achieved", Toast.LENGTH_SHORT).show();
//        }


        //Motion Sensor
        mSensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        linearAccelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION);
        gravitySensor = mSensorManager.getDefaultSensor(Sensor.TYPE_GRAVITY);
        gyroscope = mSensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
        magnetometer = mSensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
        accelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);

        dataTextView.setText("No Data, running initialization...");
    }


    @Override
    public void onClick(View v) {

        switch(v.getId()) {
            case R.id.track_only:
                OrbNdkHelper.trackOnly();
                Toast.makeText(ORBSLAMForCameraModeActivity.this, "Track Only", Toast.LENGTH_LONG).show();
                break;
//            case R.id.data_collection:
//                Double lat = location.getLatitude();
//                Double lng = location.getLongitude();
//                dataTextView.setText("Lat: " + String.valueOf(lat) + "\nLng: " + String.valueOf(lng) + "\n" + "accex:"
//                        + String.valueOf(acce[0])+"\naccey:"+String.valueOf(acce[1])+"\naccez:"+String.valueOf(acce[2]));

//                break;
            case R.id.calibrationStart:
                if (!calibrationIsStart) {
                    calibrationIsStart = true;
                    calibrationStart.setText("End Calibration");
                    for (int i = 0; i < 3; i++) {
                        caliStartPos[i] = pos[i];
                    }
                    Toast.makeText(this, "Calibration Start", Toast.LENGTH_LONG)
                            .show();
                }
                else if (calibrationIsStart) {
                    calibrationIsStart = false;
                    calibrationStart.setText("Start Calibration");
                    for (int i = 0; i < 3; i++) {
                        caliEndPos[i] = pos[i];
//                    float displacement = (float) Math.sqrt(Math.pow(caliEndPos[i]-caliStartPos[i],2));
//                    scale[i] = (float) 0.5/displacement*scale[i];
                    }
                    displacement = (float) Math.sqrt(Math.pow(caliEndPos[0] - caliStartPos[0], 2) + Math.pow(caliEndPos[1] - caliStartPos[1], 2) + Math.pow(caliEndPos[2] - caliStartPos[2], 2));
                    scale = (float) (2.0 / displacement) * scale;
                    Toast.makeText(this, "Calibration End", Toast.LENGTH_LONG)
                            .show();
                }
                break;
            case R.id.distanceStart:
                if (!distanceIsStart) {
                    distanceIsStart = true;
                    distanceStart.setText("End Distance");
                    for (int i = 0; i < 3; i++) {
                        distStartPos[i] = pos[i];
                    }
                    Toast.makeText(this, "Distance Calculation Start", Toast.LENGTH_LONG)
                            .show();
                }
                else if (distanceIsStart) {
                    distanceIsStart = false;
                    distanceStart.setText("Start Distance");
                    for (int i = 0; i < 3; i++) {
                        distEndPos[i] = pos[i];
                    }
                    displacement = (float) Math.sqrt(Math.pow(distEndPos[0] - distStartPos[0], 2) + Math.pow(distEndPos[1] - distStartPos[1], 2) + Math.pow(distEndPos[2] - distStartPos[2], 2));
                    distTextView.setText("Moving Distance: " + String.valueOf(displacement));

                    Toast.makeText(this, "Distance Calculation End", Toast.LENGTH_LONG)
                            .show();
                }
                break;

        }
    }


    Handler myHandler = new Handler(Looper.getMainLooper()) {
        //process an incoming message (an object containing a bundle of data) to the handler
        public void handleMessage(Message msg) {
            /*
            switch (msg.what) {
                case INIT_FINISHED:
                    Toast.makeText(ORBSLAMForCameraModeActivity.this, "init has been finished!", Toast.LENGTH_LONG).show();
                    new Thread(new Runnable() {

                       @Override
                       public void run() {
                           while(isSLAMRunning) {
                               timestamp = (double)System.currentTimeMillis()/1000.0;

                               int[] resultInt = OrbNdkHelper.startCurrentORBForCamera(timestamp, addr, w, h);
                               resultImg = Bitmap.createBitmap(w, h, Bitmap.Config.RGB_565);
                               resultImg.setPixels(resultInt, 0, w, 0, 0, w, h);
                               runOnUiThread(new Runnable() {
                                   @Override
                                   public void run() {
                                       imgDealed.setImageBitmap(resultImg);
                                   }
                               });
                           }
                       }
                   }).start();
                   break;
            }
            super.handleMessage(msg);
            */

            //check to see if we're getting a message saying that initialization has finished
            if (msg.what == INIT_FINISHED) {
                Toast.makeText(ORBSLAMForCameraModeActivity.this, "Init has been finished successfully!", Toast.LENGTH_LONG).show();
                Log.i(TAG, "Handler received message, starting up the ORB SLAM System using OpenGL...");

                //velocityCalculator.startFlag = 1;

                //start up a new background thread
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        //do this infinitely as long as SLAM is running
                        while (isSLAMRunning) {
                            timestampOld = timestamp;

                            timestamp = (double) System.currentTimeMillis() / 1000.0;

                            timeStep = timestamp - timestampOld;

                            //array of nine floats, rotation matrix R transforming a vector from the device coordinate
                            //system to the world's coordinate system
                            RCaptured = RMatrix;

                            //resultfloat = OrbNdkHelper.startCurrentORBForCamera2(timestamp, addr, w, h, RCaptured);

                            //start up the ORB, passing native address of incoming camera frame, getting back a 4x4 camera pose matrix
                            resultfloat = OrbNdkHelper.startCurrentORBForCamera(timestamp, addr, w, h);

                            /*
                            resultImg = Bitmap.createBitmap(w, h, Bitmap.Config.RGB_565);
                            resultImg.setPixels(resultInt, 0, w, 0, 0, w, h);
                            */

                            //run this part on the main thread so we can edit the text without getting error
                            runOnUiThread(new Runnable() {
                                @Override
                                public void run() {

                                     //imgDealed.setImageBitmap(resultImg);
                                    //dataTextView.setText("W: " + String.valueOf(w) + "\nH: " + String.valueOf(h)+ "\nTime: " + String.valueOf(timeStep));
                                    // dataTextView.setText("Vx: " + String.valueOf(velocity[0]) + "\nVy: " + String.valueOf(velocity[1])+ "\nVz: " + String.valueOf(velocity[2]));
                                    // dataTextView.setText("AcceSetp:"+String.valueOf(velocityCalculator.acceStep)+"\nAx: " + String.valueOf(acce[0]) + "\nAy: " + String.valueOf(acce[1])+ "\nAz: " + String.valueOf(acce[2]));
                                    // if(resultfloat.length==16)
                                    //dataTextView.setText("X: " + String.valueOf(-resultfloat[3]) + "\nY: " + String.valueOf(-resultfloat[7]) + "\n" + "Z:"+ String.valueOf(-resultfloat[11]));

                                    //make sure the camera pose calculation succeeded
                                    if (resultfloat.length == 16) {
                                        //doing something here with the first 3 elements of fourth column of returned matrix (translation vector)
                                        float[] OCc = {-resultfloat[3], -resultfloat[7], -resultfloat[11]}; //the translation vector

                                        //reorder to get proper x, y, z order
                                        float[] OCb = {-OCc[1], -OCc[0], -OCc[2]};

                                        //array of three floats. Multiplying the 3x3 rotation matrix (computed from the sensors) by the 3x1 translation matrix
                                        //(computed from ORBslam)
                                        float[] OCe = {(RCaptured[0] * OCb[0]) + (RCaptured[1] * OCb[1]) + (RCaptured[2] * OCb[2]),
                                                (RCaptured[3] * OCb[0]) + (RCaptured[4] * OCb[1]) + (RCaptured[5] * OCb[2]),
                                                (RCaptured[6] * OCb[0]) + (RCaptured[7] * OCb[1]) + (RCaptured[8] * OCb[2])
                                        };

                                        //set first three of pos array to corresponding entry from OCe mult by scale (1 in this case)
                                        for (int i = 0; i < 3; i++) {
                                            //pos[i] = scale[i]*OCe[i];
                                            pos[i] = scale * OCe[i];
                                        }

                                        //display the camera position and scale
                                        dataTextView.setText("Seconds taken to run: " + timeStep +
                                                "\nPosition (rel to origin): " +
                                                "\nX: " + pos[0] +
                                                "\nY: " + pos[1] +
                                                "\n" + "Z: " + pos[2] +
                                                "\n" + "Scale: " + scale);
                                    }

                                    else {
                                        dataTextView.setText("ERROR: lost camera pose, please backtrack");
                                    }

                                    //check if user has clicked "Start Distance"
                                    if (distanceIsStart) {
                                        distTextView.setText("Moving Distance: " + displacement);
                                    }


                                    /*
                                    dataTextView.setText("X: " + String.valueOf(resultfloat.length));
                                    if(resultfloat.length==6)
                                        dataTextView.setText("X: " + String.valueOf(resultfloat[0]) + "\nY: " +
                                                String.valueOf(resultfloat[1]) + "\n" + "Z:" +
                                                String.valueOf(resultfloat[2])+"Roll: " + String.valueOf(resultfloat[3])
                                                + "\nPitch: " + String.valueOf(resultfloat[4]) + "\n" + "Yaw:"
                                                + String.valueOf(resultfloat[5]));
                                    */
                                }
                            });
                        }
                    }
                }).start();
            }
            super.handleMessage(msg);
        }
    };

    private Bitmap tmp, resultImg;
    private float[] resultfloat;
    //private double timestamp;

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        //OrbNdkHelper.readShaderFile(mAssetMgr);

        //initialize the OpenGL ES framework
        Log.i(TAG, "Initializing the GL ES framework/starting drawing...");
        OrbNdkHelper.glesInit();
    }


    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        OrbNdkHelper.glesResize(width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        Log.i(TAG, "Drawing next GL frame...");
        OrbNdkHelper.glesRender();
    }

    @Override
    protected void onResume() {
        Log.i(TAG, "onResume Called");

        super.onResume();


        mGLSurfaceView.onResume();

        OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_2_4_9, this, mLoaderCallback);
        //mSensorManager.registerListener(this, linearAccelerometer, 200000);

        //register the listeners for all of the sensors we're using
        mSensorManager.registerListener(this, linearAccelerometer, SensorManager.SENSOR_DELAY_FASTEST);
        mSensorManager.registerListener(this, gravitySensor, 50000);
        mSensorManager.registerListener(this, gyroscope, 100000);
        mSensorManager.registerListener(this, accelerometer, SensorManager.SENSOR_DELAY_FASTEST);
        mSensorManager.registerListener(this, magnetometer, SensorManager.SENSOR_DELAY_FASTEST);


        //if (checkPermission) {
        //      locationManager.requestLocationUpdates(provider, 100, 0, this);
        //}
    }



    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onPause() {
        super.onPause();
        mGLSurfaceView.onPause();

        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
        mSensorManager.unregisterListener(this);
//        if (checkPermission) {
//            locationManager.removeUpdates(this);
//        }
    }

    public void onDestroy() {
        super.onDestroy();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        Log.i(TAG, "called onCreateOptionsMenu");
        mItemSwitchCamera = menu.add("stop SLAM");
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        String toastMesage = new String();
        Log.i(TAG, "called onOptionsItemSelected; selected item: " + item);

        if (item == mItemSwitchCamera) {
            isSLAMRunning = false;
//	            mOpenCvCameraView.setVisibility(SurfaceView.GONE);
//	            mIsJavaCamera = !mIsJavaCamera;
//
//	            if (mIsJavaCamera) {
//	                mOpenCvCameraView = (CameraBridgeViewBase) findViewById(R.id.tutorial1_activity_java_surface_view);
//	                toastMesage = "Java Camera";
//	            } else {
//	                mOpenCvCameraView = (CameraBridgeViewBase) findViewById(R.id.tutorial1_activity_native_surface_view);
//	                toastMesage = "Native Camera";
//	            }
//
//	            mOpenCvCameraView.setVisibility(SurfaceView.VISIBLE);
//	            mOpenCvCameraView.setCvCameraViewListener(this);
//	            mOpenCvCameraView.enableView();
//	            Toast toast = Toast.makeText(this, toastMesage, Toast.LENGTH_LONG);
//	            toast.show();
        }

        return true;
    }

    public void onCameraViewStarted(int width, int height) {
    }

    public void onCameraViewStopped() {
    }

    //callback for when a camera frame is ready
    public Mat onCameraFrame(CvCameraViewFrame inputFrame) {
        //get the frame as a mat in RGBA format
        Mat im = inputFrame.rgba();

        //lock on the incoming frame Mat
        synchronized (im) {
            //get native pointer to the image mat so we can pass to native code
            addr = im.getNativeObjAddr();
        }

        //get width and height of image
        w = im.cols();
        h = im.rows();

        //I want to run posenet now and draw it onto the img preview


        //whatever gets returned here is what's displayed
        return inputFrame.rgba();
    }


    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        switch (requestCode) {
            case 10:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    checkPermission = true;
                }

                else {
                    checkPermission = false;
                }
        }
    }

    @Override
    public void onLocationChanged(Location location) {
//
//        long time = System.currentTimeMillis();
//            lat = location.getLatitude();
//            lng = location.getLongitude();
//        dataTextView.setText("Lat: " + String.valueOf(lat) + "\nLng: " + String.valueOf(lng) + "\n");
    }

    @Override
    public void onStatusChanged(String provider, int status, Bundle extras) {

    }

    @Override
    public void onProviderEnabled(String provider) {

    }

    @Override
    public void onProviderDisabled(String provider) {

    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        long time = System.currentTimeMillis();
        Sensor sensor = event.sensor;

        //the acceleration of the device excluding gravity
        if (sensor.getType() == Sensor.TYPE_LINEAR_ACCELERATION) {
            acce[0] = event.values[0];
            acce[1] = event.values[1];
            acce[2] = event.values[2];
            //velocityCalculator.calculateVelocity();

            //new WriteData().writeToFile("Acceleration.txt", String.valueOf(time));
            //new WriteData().writeToFile("Acceleration.txt", "X: " + String.valueOf(event.values[0]) +"\nY: " + String.valueOf(event.values[1]) + "\nZ: " + String.valueOf(event.values[2]));
            //acceTextView.setText("X: " + String.valueOf(event.values[0]) +"\nY: " + String.valueOf(event.values[1]) + "\nZ: " + String.valueOf(event.values[2]));
        }

        /*
        else if (sensor.getType() == Sensor.TYPE_GRAVITY) {
           new WriteData().writeToFile("Gravity.txt", String.valueOf(time));
           new WriteData().writeToFile("Gravity.txt", "X: " + String.valueOf(event.values[0]) + "\nY: " + String.valueOf(event.values[1]) + "\nZ: " + String.valueOf(event.values[2]));

        }*/

        //if this is a gyroscope sensor event
        else if (sensor.getType() == Sensor.TYPE_GYROSCOPE) {
            //angular speeds around x, y, and z axes (in rad/sec)
            gyro[0] = event.values[0];
            gyro[1] = event.values[1];
            gyro[2] = event.values[2];
            //gyroTextView.setText("X: " + String.valueOf(event.values[0]) + "\nY: " + String.valueOf(event.values[1]) + "\nZ: " + String.valueOf(event.values[2]));
        }

        //if this is an accelerometer sensor event
        else if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER)
            //we assume the device isn't accelerating, so these three values represent the gravity vector
            mGravity = event.values;

        //if this is a magnetometer sensor event
        else if (event.sensor.getType() == Sensor.TYPE_MAGNETIC_FIELD)
            //All values are in micro-Tesla (uT) and measure the ambient magnetic field in the X, Y and Z axis.
            mGeomagnetic = event.values;

        //accelerometer data was read in, so was magnetometer data
        if (mGravity != null && mGeomagnetic != null) {

            //mGravity: an array of 3 floats containing the gravity vector expressed in the device's coordinate
            //mGeomagnetic: an array of 3 floats containing the geomagnetic vector expressed in the device's coordinate
            //Computes the inclination matrix I as well as the rotation matrix R transforming a vector from the device coordinate
            //system to the world's coordinate system

            //R is the identity matrix when the device is aligned with the world's coordinate system, that is, when the
            //device's X axis points toward East, the Y axis points to the North Pole and the device is facing the sky

            //I is a rotation matrix transforming the geomagnetic vector into the same coordinate space as gravity (the
            //world's coordinate space). I is a simple rotation around the X axis. The inclination angle in radians can be
            //computed with getInclination(float[]).
            boolean success = SensorManager.getRotationMatrix(RMatrix, IMatrix, mGravity, mGeomagnetic); //Rmatrix: how much device is rotating
                                                                                                         //from X-East, Y-North, Z-UP

            //make sure getRotationMatrix was successful
            if (success) {
                //Computes the device's orientation based on the rotation matrix.

                //When it returns, the array values are as follows:

                //orientation[0]: Azimuth, angle of rotation about the -z axis. This value represents the angle between the device's
                //y axis and the magnetic north pole. When facing north, this angle is 0, when facing south, this angle is π.
                //Likewise, when facing east, this angle is π/2, and when facing west, this angle is -π/2. The range of values is -π to π.


                //orientation[1]: Pitch, angle of rotation about the x axis. This value represents the angle between a plane parallel to the
                //device's screen and a plane parallel to the ground. Assuming that the bottom edge of the device faces the user and
                //that the screen is face-up, tilting the top edge of the device toward the ground creates a positive pitch angle. The
                //range of values is -π to π.


                //orientation[2]: Roll, angle of rotation about the y axis. This value represents the angle between a plane perpendicular to
                //the device's screen and a plane perpendicular to the ground. Assuming that the bottom edge of the device faces the
                //user and that the screen is face-up, tilting the left edge of the device toward the ground creates a positive roll angle.
                //The range of values is -π/2 to π/2.

                //**HINT: applying these three rotations in the azimuth, pitch, roll order transforms an identity matrix to the
                //rotation matrix passed into this method.
                SensorManager.getOrientation(RMatrix, orientation);

                Log.i(TAG, String.format("RMatrix: %f  %f  %f\n%f  %f  %f\n%f  %f  %f", RMatrix[0], RMatrix[1],
                        RMatrix[2], RMatrix[3], RMatrix[4], RMatrix[5], RMatrix[6], RMatrix[7], RMatrix[8]));
            }
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {

    }

    public void velocityVerlet(){

    }


}
