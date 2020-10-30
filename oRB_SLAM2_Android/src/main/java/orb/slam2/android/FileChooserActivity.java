package orb.slam2.android;

import java.io.File;
import java.security.acl.LastOwnerException;
import java.util.ArrayList;

import orb.slam2.android.FileChooserAdapter.FileInfo;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.GridView;
import android.widget.TextView;
import android.widget.Toast;


public class FileChooserActivity extends Activity {
	private GridView mGridView;
	private View mBackView;
	private View mBtExit,mBtOk;
	private TextView mTvPath ;
	
	private String mSdcardRootPath;
	private String mLastFilePath;
	
	private ArrayList<FileInfo> mFileLists;

	//our adapter
	private FileChooserAdapter mAdapter;
	
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		//make it fullscreen
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);

		//set content to "filechooser_show.xml"
		setContentView(R.layout.filechooser_show);

		//get the path to external storage directory/sd card
		//mSdcardRootPath = Environment.getExternalStorageDirectory().getAbsolutePath();
		mSdcardRootPath = "/storage/emulated/0/Android/data/orb.slam2.android/files";

		//log what was found
		Log.i("DBUG", mSdcardRootPath);

		//find the views for each of the buttons
		mBackView = findViewById(R.id.imgBackFolder);
		mBackView.setOnClickListener(mClickListener);
		mBtExit = findViewById(R.id.btExit);
		mBtExit.setOnClickListener(mClickListener);
		mBtOk= findViewById(R.id.btOK);
		mBtOk.setOnClickListener(mClickListener);
		
		mTvPath = (TextView)findViewById(R.id.tvPath);
		
		mGridView = (GridView)findViewById(R.id.gvFileChooser);
		mGridView.setEmptyView(findViewById(R.id.tvEmptyHint));
		mGridView.setOnItemClickListener(mItemClickListener);

		//set up the file explorer view using the path found above
		setGridViewAdapter(mSdcardRootPath);
	}

	//instantiate an adapter for the gridview, and load the files in this folder
	private void setGridViewAdapter(String filePath) {
		//load the files that should be here
		updateFileItems(filePath);

		mAdapter = new FileChooserAdapter(this , mFileLists);
		mGridView.setAdapter(mAdapter);
	}


	private void updateFileItems(String filePath) {
		//show pwd in upper left cnr
		mLastFilePath = filePath ;
		mTvPath.setText(mLastFilePath);

		//if the file list hasn't ever been allocated yet, do so now
		if(mFileLists == null)
			mFileLists = new ArrayList<FileInfo>();

		//clear out old junk data from the file ArrayList
		if(!mFileLists.isEmpty())
			mFileLists.clear();
		
		File[] files = folderScan(filePath);
		if (files == null)
			//this is not a directory
			return;

		//go through the list of files returned
		for (int i = 0; i < files.length; i++) {
			if(files[i].isHidden())
				//jump to next iteration of loop
				continue;

			//get the path of this file
			String fileAbsolutePath = files[i].getAbsolutePath();

			//get name of file
			String fileName = files[i].getName();

			//check if this is a directory
		    boolean isDirectory = false;

			if (files[i].isDirectory()) {
				isDirectory = true;
			}

			//instantiate a FileInfo object from this file
		    FileInfo fileInfo = new FileInfo(fileAbsolutePath, fileName, isDirectory);

			//add this file to the list of files
			mFileLists.add(fileInfo);
		}

		//When we enter for first time, the mAdapter object isn't initialized, so do it now
		if (mAdapter != null)
		    mAdapter.notifyDataSetChanged();
	}


	//get files from this directory
	private File[] folderScan(String path) {
		//converts given pathname into abstract pathname
		File file = new File(path);

		//Returns array of abstract pathnames denoting files in directory denoted by this abstract pathname. Returns null if this isn't a directory
		File[] files = file.listFiles();

		return files;
	}
	
	private View.OnClickListener mClickListener = new  OnClickListener() {
		public void onClick(View v) {
			switch (v.getId()) {
				//go back to previous screen if BACK clicked
				case R.id.imgBackFolder:
					backProcess();
					break;

				//exit file selection activity if EXIT clicked
				case R.id.btExit :
					setResult(RESULT_CANCELED);

					//end activity
					finish();
					break;

				//set the new path and exit if SELECT clicked
				case R.id.btOK:
					Intent intent = new Intent();
					intent.putExtra(DataSetModeActivity.EXTRA_FILE_CHOOSER , mLastFilePath);
					setResult(RESULT_OK, intent);

					//end activity
					finish();
					break;
			}
		}
	};
	
	private AdapterView.OnItemClickListener mItemClickListener = new OnItemClickListener() {
		public void onItemClick(AdapterView<?> adapterView, View view, int position,
				long id) {
			FileInfo fileInfo = (FileInfo)(((FileChooserAdapter)adapterView.getAdapter()).getItem(position));
			if(fileInfo.isDirectory())
				updateFileItems(fileInfo.getFilePath()) ;
			else{
				Intent intent = new Intent();
			    intent.putExtra(DataSetModeActivity.EXTRA_FILE_CHOOSER , fileInfo.getFilePath());
			    setResult(RESULT_OK , intent);
			    finish();
			}
		}
	};
    
	public boolean onKeyDown(int keyCode , KeyEvent event){
		if(event.getAction() == KeyEvent.ACTION_DOWN && event.getKeyCode()
			== KeyEvent.KEYCODE_BACK){
			backProcess();   
			return true ;
		}
		return super.onKeyDown(keyCode, event);
	}


	public void backProcess() {
		if (!mLastFilePath.equals(mSdcardRootPath)) {  
			File thisFile = new File(mLastFilePath);
			String parentFilePath = thisFile.getParent();
			updateFileItems(parentFilePath);
		}

		else {
			setResult(RESULT_CANCELED);
			finish();
		}
	}
	private void toast(CharSequence hint){
	    Toast.makeText(this, hint , Toast.LENGTH_SHORT).show();
	}
}