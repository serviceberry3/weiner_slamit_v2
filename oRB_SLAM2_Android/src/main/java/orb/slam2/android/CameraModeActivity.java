package orb.slam2.android;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.text.TextUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

/**
 *
 * @author buptzhaofang@163.com Mar 24, 2016 1:00:27 PM
 * @author nodog Nov 2020 - noah.weiner@yale.edu
 *
 */
public class CameraModeActivity extends Activity implements OnClickListener{
    Button ChooseCalibration,finish;
    TextView CalibrationTxt, VOCPathText;

    private static final int REQUEST_CODE_2 = 2;
    private static final int REQUEST_CODE_3 = 3;

    private Intent fileChooserIntent;
    public static final String EXTRA_FILE_CHOOSER = "file_chooser";

    private String VOCPath = "/storage/emulated/0/SLAM/ORBvoc.txt";
    private String TUMPath = "/storage/emulated/0/SLAM/List3.yaml";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);

        setContentView(R.layout.activity_camera_mode);

        ChooseCalibration=(Button)findViewById(R.id.choose_calibration);
        //ChooseVOC = (Button)findViewById(R.id.choose_voc);
        finish=(Button)findViewById(R.id.finish);
        finish.setOnClickListener(this);
        ChooseCalibration.setOnClickListener(this);
        //ChooseVOC.setOnClickListener(this);
        CalibrationTxt=(TextView)findViewById(R.id.cal_path_txt);
        //VOCPathText = (TextView)findViewById(R.id.voc_path_txt);

        fileChooserIntent =  new Intent(this, FileChooserActivity.class);

        //Choose vocabulary file
        /*
        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))
            startActivityForResult(fileChooserIntent , REQUEST_CODE_3);
        else
            Toast.makeText(CameraModeActivity.this, "can't find SDcard", Toast.LENGTH_LONG).show();*/
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            //if the "Choose calibration file" button is clicked
            case R.id.choose_calibration:
                //open the file explorer after checking that SD card is where we think it is
                if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))
                    startActivityForResult(fileChooserIntent, REQUEST_CODE_2);
                else
                    Toast.makeText(CameraModeActivity.this, "Can't find SDcard", Toast.LENGTH_LONG).show();
                break;

            /*
            case R.id.choose_voc:
                if(Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED))
                    startActivityForResult(fileChooserIntent , REQUEST_CODE_3);
                else
                    Toast.makeText(CameraModeActivity.this, "can't find SDcard", Toast.LENGTH_LONG).show();
                break;*/

            //start up actual SLAM
            case R.id.finish:
                if(!TextUtils.isEmpty(TUMPath) && !TextUtils.isEmpty(VOCPath)) {
                    Bundle bundle = new Bundle();

                    //add the calibration and dataset files as strings to the intent
                    bundle.putString("voc", VOCPath );
                    bundle.putString("calibration", TUMPath );

                    //make an intent to send over to the newly spawned activity
                    Intent intent = new Intent(CameraModeActivity.this, ORBSLAMForCameraModeActivity.class);
                    intent.putExtras(bundle);

                    //start up ORBSLAMForTestActivity, not waiting for result
                    startActivity(intent);

                    //end this activity
                    finish();
                }

                else {
                    Toast.makeText(CameraModeActivity.this, "Dataset and calibration file path must both be selected!",
                            Toast.LENGTH_LONG).show();
                }
                break;
        }
    }


    //get result back from file selection activity
    public void onActivityResult(int requestCode , int resultCode , Intent data){
        if (resultCode == RESULT_CANCELED){
            Toast.makeText(CameraModeActivity.this, "no return value", Toast.LENGTH_LONG).show();
            return ;
        }
        if (resultCode == RESULT_OK && requestCode == REQUEST_CODE_2) {
            TUMPath = data.getStringExtra(EXTRA_FILE_CHOOSER);
            CalibrationTxt.setText("The calibration path is " + TUMPath);
            return;
        }

        if (resultCode == RESULT_OK && requestCode == REQUEST_CODE_3) {
            VOCPath = data.getStringExtra(EXTRA_FILE_CHOOSER);
            //VOCPathText.setText("calibration path is " + VOCPath);
            return;
        }

    }

}
