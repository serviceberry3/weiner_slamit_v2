/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/



#include "System.h"
#include "Converter.h"
#include <thread>
//#include <pangolin/pangolin.h>
#include <iomanip>
#include <android/log.h>
#define LOG_TAG "ORB_SLAM_SYSTEM"

#define LOG(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

namespace ORB_SLAM2
{

//implementation for singleton instance to remain alive for instance existed in JNI interface.
System* System::_instance = NULL;
System* System::get_instance() {
	return _instance;
}

System::System(const string &strVocFile, const string &strSettingsFile, const eSensor sensor, const bool bUseViewer):
//initialization list
mSensor(sensor),
mbReset(false),
mbActivateLocalizationMode(false),
mbDeactivateLocalizationMode(false)
{

	_instance = this;
	LOG("ORB_initiate (System() constructor called in System.cc)");


    if(mSensor==MONOCULAR) {
        LOG("Looking to initialize monocular system");
    }
    else if(mSensor==STEREO) {

    }
    else if(mSensor==RGBD) {

    }

    //LOG("Checking/opening calibration file for reading... %s", strSettingsFile.c_str()); //c_str() get's a null-terminated char* from a c++
                                                                                         //string (basically gets C string from C++ string)

    //LOG("Value of READ: %d", cv::FileStorage::READ);


    /*
    //// Method 1: cpp read file - ok
    ifstream myfile(strSettingsFile.c_str());
    string line;


    if (myfile.is_open())
    {
        LOG("SUCCESS OPENING FILE FOR C++ READING");

        while (getline(myfile, line))
        {
            ////It does come here
            cout << line <<'\n';
        }
        myfile.close();

    }
    else {
        LOG("FAIL OPENING FOR C++ READING");
    }
    */



    //Check settings (calibration) file. Instantiate an OpenCV FileStorage object, which is
    //XML/YAML/JSON file storage class that encapsulates all info necessary for writing or reading data to/from a file
    //cv::FileStorage fsSettings;
    //fsSettings.open(strSettingsFile.c_str(), cv::FileStorage::APPEND);
    //cv::FileStorage fsSettings(strSettingsFile.c_str(), cv::FileStorage::APPEND); //open the file for reading

    /*
    //check if the YAML file was opened successfully
    if (!fsSettings.isOpened())
    {
        //failure
        LOG("Failed to open calibration file, trying again...");
        bool tryAgain = fsSettings.open(strSettingsFile.c_str(), cv::FileStorage::APPEND);
        if (!tryAgain) {
            LOG("Failed to open calibration file second time, exiting in 10 sec...");
            sleep(10);
            exit(-1);
        }
    }
    */


    //success
    //LOG("Finished opening calibration file successfully");

    //instantiating a new ORBVocabulary
    LOG("Instantiating new ORBVocabulary");

    //instantiate a new
    mpVocabulary = new ORBVocabulary();

    //Load ORB Vocabulary
    LOG("Loading ORB vocabulary. This could take a while...");

    bool bVocLoad = mpVocabulary->loadFromTextFile(strVocFile);

    if(!bVocLoad)
    {
        LOG("FAILED TO LOAD VOCAB");
        exit(-1);
    }
    LOG("VOCAB LOADED SUCCESSFULLY!");

    //Create KeyFrame Database from the vocab file
    mpKeyFrameDatabase = new KeyFrameDatabase(*mpVocabulary);

    //Create the Map
    mpMap = new Map();

    //Create Drawers. These are used by the Viewer
    mpFrameDrawer = new FrameDrawer(mpMap);
    mpMapDrawer = new MapDrawer(mpMap, strSettingsFile);

    //Initialize the Tracking thread
    //It will live in the main thread of execution (the one that called this constructor)
    mpTracker = new Tracking(this, mpVocabulary, mpFrameDrawer, mpMapDrawer, mpMap, mpKeyFrameDatabase, strSettingsFile, mSensor);

    //Initialize the Local Mapping thread and launch
    mpLocalMapper = new LocalMapping(mpMap, mSensor == MONOCULAR);

    //launch local mapping on new thread
    mptLocalMapping = new thread(&ORB_SLAM2::LocalMapping::Run, mpLocalMapper);

    //Initialize the Loop Closing thread and launch
    mpLoopCloser = new LoopClosing(mpMap, mpKeyFrameDatabase, mpVocabulary, mSensor!=MONOCULAR);
    mptLoopClosing = new thread(&ORB_SLAM2::LoopClosing::Run, mpLoopCloser);

    //Initialize the Viewer thread and launch
    mpViewer = new Viewer(this, mpFrameDrawer,mpMapDrawer,mpTracker,strSettingsFile);

    if (bUseViewer)
        mptViewer = new thread(&Viewer::Run, mpViewer);

    mpTracker->SetViewer(mpViewer);

    //Set pointers between threads
    mpTracker->SetLocalMapper(mpLocalMapper);
    mpTracker->SetLoopClosing(mpLoopCloser);

    mpLocalMapper->SetTracker(mpTracker);
    mpLocalMapper->SetLoopCloser(mpLoopCloser);

    mpLoopCloser->SetTracker(mpTracker);
    mpLoopCloser->SetLocalMapper(mpLocalMapper);


    Reb = cv::Mat::eye(3,3,CV_32F);
    RebOld = cv::Mat::eye(3,3,CV_32F);
    Rbc = cv::Mat::eye(3,3,CV_32F);
    Rbc.at<float>(0,0) = 0;
    Rbc.at<float>(0,1) = -1;
    Rbc.at<float>(0,2) = 0;
    Rbc.at<float>(1,0) = -1;
    Rbc.at<float>(1,1) = 0;
    Rbc.at<float>(1,2) = 0;
    Rbc.at<float>(2,0) = 0;
    Rbc.at<float>(2,1) = 0;
    Rbc.at<float>(2,2) = -1;


    LOG("ORB_initiate System() constructor FINISHED!");
}


cv::Mat System::getmVelocity(cv::Mat Rebin){
    Reb.copyTo(RebOld);
    Rebin.copyTo(Reb);
    cv::Mat velocity = cv::Mat::eye(4,4,CV_32F);
    //cv::Mat rotation = velocity.colRange(0,3).rowRange(0,3);
    cv::Mat Rcc = Rbc.t()*Reb.t()*RebOld*Rbc;
    Rcc.copyTo(velocity.colRange(0,3).rowRange(0,3));
    return velocity;
}


void System::drawGL(){
	LOG("DrawGL Thread has been started!");

	mpViewer->drawGL();
}

cv::Mat System::TrackStereo(const cv::Mat &imLeft, const cv::Mat &imRight, const double &timestamp)
{
    if(mSensor!=STEREO)
    {
        cerr << "ERROR: you called TrackStereo but input sensor was not set to STEREO." << endl;
        exit(-1);
    }   

    // Check mode change
    {
        unique_lock<mutex> lock(mMutexMode);
        if(mbActivateLocalizationMode)
        {
            mpLocalMapper->RequestStop();

            // Wait until Local Mapping has effectively stopped
            while(!mpLocalMapper->isStopped())
            {
                usleep(1000);
            }

            mpTracker->InformOnlyTracking(true);
            mbActivateLocalizationMode = false;
        }
        if(mbDeactivateLocalizationMode)
        {
            mpTracker->InformOnlyTracking(false);
            mpLocalMapper->Release();
            mbDeactivateLocalizationMode = false;
        }
    }

    // Check reset
    {
        unique_lock<mutex> lock(mMutexReset);
        if(mbReset)
        {
            mpTracker->Reset();
            mbReset = false;
        }
    }

    return mpTracker->GrabImageStereo(imLeft,imRight,timestamp);
}

cv::Mat System::TrackRGBD(const cv::Mat &im, const cv::Mat &depthmap, const double &timestamp)
{
    if(mSensor!=RGBD)
    {
        cerr << "ERROR: you called TrackRGBD but input sensor was not set to RGBD." << endl;
        exit(-1);
    }    

    // Check mode change
    {
        unique_lock<mutex> lock(mMutexMode);
        if(mbActivateLocalizationMode)
        {
            mpLocalMapper->RequestStop();

            // Wait until Local Mapping has effectively stopped
            while(!mpLocalMapper->isStopped())
            {
                usleep(1000);
            }

            mpTracker->InformOnlyTracking(true);
            mbActivateLocalizationMode = false;
        }
        if(mbDeactivateLocalizationMode)
        {
            mpTracker->InformOnlyTracking(false);
            mpLocalMapper->Release();
            mbDeactivateLocalizationMode = false;
        }
    }

    // Check reset
    {
    unique_lock<mutex> lock(mMutexReset);
    if(mbReset)
    {
        mpTracker->Reset();
        mbReset = false;
    }
    }

    return mpTracker->GrabImageRGBD(im,depthmap,timestamp);
}


//actual used processing function for monocular ORB SLAM
cv::Mat System::TrackMonocular(const cv::Mat &im, const double &timestamp)
{
    if (mSensor != MONOCULAR)
    {
    	LOG("ERROR: you called TrackMonocular but input sensor was not set to Monocular." );
        exit(-1);
    }

    //Check mode change
    {
        //get the lock on mMutexMode
        unique_lock<mutex> lock(mMutexMode);

        if (mbActivateLocalizationMode)
        {
            mpLocalMapper->RequestStop();

            //Wait until Local Mapping has effectively stopped
            while (!mpLocalMapper->isStopped())
            {
                usleep(1000);
            }

            mpTracker->InformOnlyTracking(true);
            mbActivateLocalizationMode = false;
        }

        if (mbDeactivateLocalizationMode)
        {
            mpTracker->InformOnlyTracking(false);
            mpLocalMapper->Release();
            mbDeactivateLocalizationMode = false;
        }
    }

    //Check reset
    {
        //get the lock on mMutexReset
        unique_lock<mutex> lock(mMutexReset);
        if (mbReset)
        {
            mpTracker->Reset();
            mbReset = false;
        }
    }

    //return the Mat that results from running GrabImageMonocular() on the captured image frame
    return mpTracker->GrabImageMonocular(im, timestamp);
    //return mpFrameDrawer->DrawFrame();
}

float* System::posenetExternal() { //the & here just means pass by reference, so DON'T make copy of passed Mat
    return mpTracker->posenetExternalGetPts();
}

void System::ActivateLocalizationMode()
{
    unique_lock<mutex> lock(mMutexMode);
    mbActivateLocalizationMode = true;
}

void System::DeactivateLocalizationMode()
{
    unique_lock<mutex> lock(mMutexMode);
    mbDeactivateLocalizationMode = true;
}

void System::Reset()
{
    unique_lock<mutex> lock(mMutexReset);
    mbReset = true;
}

void System::Shutdown()
{
    mpLocalMapper->RequestFinish();
    mpLoopCloser->RequestFinish();
    mpViewer->RequestFinish();

    // Wait until all thread have effectively stopped
    while (!mpLocalMapper->isFinished() || !mpLoopCloser->isFinished()  ||
          !mpViewer->isFinished()      || mpLoopCloser->isRunningGBA())
    {
        usleep(5000);
    }

    //clean up posenet
    if (mpTracker) {
        mpTracker->mPosenet.close();
    }
}

void System::SaveTrajectoryTUM(const string &filename)
{
    cout << endl << "Saving camera trajectory to " << filename << " ..." << endl;

    vector<KeyFrame*> vpKFs = mpMap->GetAllKeyFrames();
    sort(vpKFs.begin(),vpKFs.end(),KeyFrame::lId);

    // Transform all keyframes so that the first keyframe is at the origin.
    // After a loop closure the first keyframe might not be at the origin.
    cv::Mat Two = vpKFs[0]->GetPoseInverse();

    ofstream f;
    f.open(filename.c_str());
    f << fixed;

    //Frame pose is stored relative to its reference keyframe (which is optimized by BA and pose graph).
    //We need to get first the keyframe pose and then concatenate the relative transformation.
    //Frames not localized (tracking failure) are not saved.

    //For each frame we have a reference keyframe (lRit), the timestamp (lT) and a flag
    //which is true when tracking failed (lbL).
    list<ORB_SLAM2::KeyFrame*>::iterator lRit = mpTracker->mlpReferences.begin();
    list<double>::iterator lT = mpTracker->mlFrameTimes.begin();
    list<bool>::iterator lbL = mpTracker->mlbLost.begin();
    for(list<cv::Mat>::iterator lit=mpTracker->mlRelativeFramePoses.begin(),
        lend=mpTracker->mlRelativeFramePoses.end();lit!=lend;lit++, lRit++, lT++, lbL++)
    {
        if(*lbL)
            continue;

        KeyFrame* pKF = *lRit;

        cv::Mat Trw = cv::Mat::eye(4,4,CV_32F);

        // If the reference keyframe was culled, traverse the spanning tree to get a suitable keyframe.
        while(pKF->isBad())
        {
            Trw = Trw*pKF->mTcp;
            pKF = pKF->GetParent();
        }

        Trw = Trw*pKF->GetPose()*Two;

        cv::Mat Tcw = (*lit)*Trw;
        cv::Mat Rwc = Tcw.rowRange(0,3).colRange(0,3).t();
        cv::Mat twc = -Rwc*Tcw.rowRange(0,3).col(3);

        vector<float> q = Converter::toQuaternion(Rwc);

        f << setprecision(6) << *lT << " " <<  setprecision(9) << twc.at<float>(0) << " " << twc.at<float>(1) << " " << twc.at<float>(2) << " " << q[0] << " " << q[1] << " " << q[2] << " " << q[3] << endl;
    }
    f.close();
    cout << endl << "trajectory saved!" << endl;
}


void System::SaveKeyFrameTrajectoryTUM(const string &filename)
{
    cout << endl << "Saving keyframe trajectory to " << filename << " ..." << endl;

    vector<KeyFrame*> vpKFs = mpMap->GetAllKeyFrames();
    sort(vpKFs.begin(),vpKFs.end(),KeyFrame::lId);

    // Transform all keyframes so that the first keyframe is at the origin.
    // After a loop closure the first keyframe might not be at the origin.
    //cv::Mat Two = vpKFs[0]->GetPoseInverse();

    ofstream f;
    f.open(filename.c_str());
    f << fixed;

    for(size_t i=0; i<vpKFs.size(); i++)
    {
        KeyFrame* pKF = vpKFs[i];

       // pKF->SetPose(pKF->GetPose()*Two);

        if(pKF->isBad())
            continue;

        cv::Mat R = pKF->GetRotation().t();
        vector<float> q = Converter::toQuaternion(R);
        cv::Mat t = pKF->GetCameraCenter();
        f << setprecision(6) << pKF->mTimeStamp << setprecision(7) << " " << t.at<float>(0) << " " << t.at<float>(1) << " " << t.at<float>(2)
          << " " << q[0] << " " << q[1] << " " << q[2] << " " << q[3] << endl;

    }

    f.close();
    cout << endl << "trajectory saved!" << endl;
}

void System::SaveTrajectoryKITTI(const string &filename)
{
    cout << endl << "Saving camera trajectory to " << filename << " ..." << endl;

    vector<KeyFrame*> vpKFs = mpMap->GetAllKeyFrames();
    sort(vpKFs.begin(),vpKFs.end(),KeyFrame::lId);

    // Transform all keyframes so that the first keyframe is at the origin.
    // After a loop closure the first keyframe might not be at the origin.
    cv::Mat Two = vpKFs[0]->GetPoseInverse();

    ofstream f;
    f.open(filename.c_str());
    f << fixed;

    // Frame pose is stored relative to its reference keyframe (which is optimized by BA and pose graph).
    // We need to get first the keyframe pose and then concatenate the relative transformation.
    // Frames not localized (tracking failure) are not saved.

    // For each frame we have a reference keyframe (lRit), the timestamp (lT) and a flag
    // which is true when tracking failed (lbL).
    list<ORB_SLAM2::KeyFrame*>::iterator lRit = mpTracker->mlpReferences.begin();
    list<double>::iterator lT = mpTracker->mlFrameTimes.begin();
    for(list<cv::Mat>::iterator lit=mpTracker->mlRelativeFramePoses.begin(), lend=mpTracker->mlRelativeFramePoses.end();lit!=lend;lit++, lRit++, lT++)
    {
        ORB_SLAM2::KeyFrame* pKF = *lRit;

        cv::Mat Trw = cv::Mat::eye(4,4,CV_32F);

        while(pKF->isBad())
        {
          //  cout << "bad parent" << endl;
            Trw = Trw*pKF->mTcp;
            pKF = pKF->GetParent();
        }

        Trw = Trw*pKF->GetPose()*Two;

        cv::Mat Tcw = (*lit)*Trw;
        cv::Mat Rwc = Tcw.rowRange(0,3).colRange(0,3).t();
        cv::Mat twc = -Rwc*Tcw.rowRange(0,3).col(3);

        f << setprecision(9) << Rwc.at<float>(0,0) << " " << Rwc.at<float>(0,1)  << " " << Rwc.at<float>(0,2) << " "  << twc.at<float>(0) << " " <<
             Rwc.at<float>(1,0) << " " << Rwc.at<float>(1,1)  << " " << Rwc.at<float>(1,2) << " "  << twc.at<float>(1) << " " <<
             Rwc.at<float>(2,0) << " " << Rwc.at<float>(2,1)  << " " << Rwc.at<float>(2,2) << " "  << twc.at<float>(2) << endl;
    }
    f.close();
    cout << endl << "trajectory saved!" << endl;
}

} //namespace ORB_SLAM
