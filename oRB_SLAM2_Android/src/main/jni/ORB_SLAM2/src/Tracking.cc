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


#include "Tracking.h"

#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>

#include "ORBmatcher.h"
#include "FrameDrawer.h"
#include "Converter.h"
#include "Map.h"
#include "Initializer.h"

#include "Optimizer.h"
#include "PnPsolver.h"

#include <iostream>

#include <mutex>
#include <android/log.h>
#define LOG_TAG "ORB_SLAM_TRACKING"

#define LOG(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

using namespace std;

namespace ORB_SLAM2
{

    Tracking::Tracking(System *pSys, ORBVocabulary* pVoc, FrameDrawer *pFrameDrawer, MapDrawer *pMapDrawer, Map *pMap,
    KeyFrameDatabase* pKFDB, const string &strSettingPath, const int sensor):
        mState(NO_IMAGES_YET),
        mSensor(sensor),
        mbOnlyTracking(false),
        mbVO(false),
        mpORBVocabulary(pVoc),
        mpKeyFrameDB(pKFDB),
        mpInitializer(static_cast<Initializer*>(NULL)),
        mpSystem(pSys),
        mpFrameDrawer(pFrameDrawer),
        mpMapDrawer(pMapDrawer),
        mpMap(pMap),
        mnLastRelocFrameId(0)
    {
        // Load camera parameters from settings file

        //cv::FileStorage fSettings(strSettingPath, cv::FileStorage::READ);

        /*
        float fx = fSettings["Camera.fx"];
        float fy = fSettings["Camera.fy"];
        float cx = fSettings["Camera.cx"];
        float cy = fSettings["Camera.cy"];
        */


        //HARDCODE
        float fx =526.69;
        float fy =540.36;
        float cx =313.07;
        float cy =238.39;


        cv::Mat K = cv::Mat::eye(3,3,CV_32F);
        K.at<float>(0,0) = fx;
        K.at<float>(1,1) = fy;
        K.at<float>(0,2) = cx;
        K.at<float>(1,2) = cy;
        K.copyTo(mK);

        cv::Mat DistCoef(4,1,CV_32F);

        /*
        DistCoef.at<float>(0) = fSettings["Camera.k1"];
        DistCoef.at<float>(1) = fSettings["Camera.k2"];
        DistCoef.at<float>(2) = fSettings["Camera.p1"];
        DistCoef.at<float>(3) = fSettings["Camera.p2"];
        const float k3 = fSettings["Camera.k3"];
        */

        //HARDCODE
        DistCoef.at<float>(0) = 0.262383;
        DistCoef.at<float>(1) = -0.953104;
        DistCoef.at<float>(2) = -0.005358;
        DistCoef.at<float>(3) = 0.002628;
        const float k3 = 1.163314;

        if (k3!=0)
        {
            DistCoef.resize(5);
            DistCoef.at<float>(4) = k3;
        }
        DistCoef.copyTo(mDistCoef);


        //initialize the posenet keypoints array
        currKeyPoints = std::vector<float>(34, -1);


        //mbf = fSettings["Camera.bf"];

        //float fps = fSettings["Camera.fps"];

        float fps = 30;

        //default to 30 FPS
        if (fps == 0)
            fps = 30;

        // Max/Min Frames to insert keyframes and to check relocalisation
        mMinFrames = 0;
        mMaxFrames = fps;


        //int nRGB = fSettings["Camera.RGB"];
        int nRGB = 1;

        mbRGB = nRGB;

        // Load ORB parameters
        /*
        int nFeatures = fSettings["ORBextractor.nFeatures"];
        float fScaleFactor = fSettings["ORBextractor.scaleFactor"];
        int nLevels = fSettings["ORBextractor.nLevels"];
        int fIniThFAST = fSettings["ORBextractor.iniThFAST"];
        int fMinThFAST = fSettings["ORBextractor.minThFAST"];
        */

        //HARDCODE
        int nFeatures = 1000;
        float fScaleFactor = 1.2;
        int nLevels = 8;
        int fIniThFAST = 20;
        int fMinThFAST = 7;


        mpORBextractorLeft = new ORBextractor(nFeatures,fScaleFactor,nLevels,fIniThFAST,fMinThFAST);

        if(sensor==System::STEREO)
            mpORBextractorRight = new ORBextractor(nFeatures,fScaleFactor,nLevels,fIniThFAST,fMinThFAST);

        if(sensor==System::MONOCULAR)
            mpIniORBextractor = new ORBextractor(2*nFeatures,fScaleFactor,nLevels,fIniThFAST,fMinThFAST);

        /*
        //ignored
        if(sensor==System::STEREO || sensor==System::RGBD)
        {
            mThDepth = mbf*(float)fSettings["ThDepth"]/fx;
        }

        //ignored
        if(sensor==System::RGBD)
        {
            mDepthMapFactor = fSettings["DepthMapFactor"];
            if(mDepthMapFactor==0)
                mDepthMapFactor=1;
            else
                mDepthMapFactor = 1.0f/mDepthMapFactor;
        }
        */

        //create posenet object using our model file, specifying to run it using a GPU delegate (coming soon)
        LOG("Tracking(): creating posenet object");
        mPosenet = Posenet("/system/files/posenet_model.tflite", Device::GPU);

        LOG("Tracking(): posenet created, now creating interpreter...");
        mInterpreter = mPosenet.getInterpreter();

        LOG("Tracking(): finished loading interpreter and camera parameters");
    }

    void Tracking::SetLocalMapper(LocalMapping *pLocalMapper)
    {
        mpLocalMapper=pLocalMapper;
    }

    void Tracking::SetLoopClosing(LoopClosing *pLoopClosing)
    {
        mpLoopClosing=pLoopClosing;
    }

    void Tracking::SetViewer(Viewer *pViewer)
    {
        mpViewer=pViewer;
    }


    cv::Mat Tracking::GrabImageStereo(const cv::Mat &imRectLeft, const cv::Mat &imRectRight, const double &timestamp)
    {
        mImGray = imRectLeft;
        cv::Mat imGrayRight = imRectRight;

        if(mImGray.channels()==3)
        {
            if(mbRGB)
            {
                cvtColor(mImGray,mImGray,CV_RGB2GRAY);
                cvtColor(imGrayRight,imGrayRight,CV_RGB2GRAY);
            }
            else
            {
                cvtColor(mImGray,mImGray,CV_BGR2GRAY);
                cvtColor(imGrayRight,imGrayRight,CV_BGR2GRAY);
            }
        }
        else if(mImGray.channels()==4)
        {
            if(mbRGB)
            {
                cvtColor(mImGray,mImGray,CV_RGBA2GRAY);
                cvtColor(imGrayRight,imGrayRight,CV_RGBA2GRAY);
            }
            else
            {
                cvtColor(mImGray,mImGray,CV_BGRA2GRAY);
                cvtColor(imGrayRight,imGrayRight,CV_BGRA2GRAY);
            }
        }

        mCurrentFrame = Frame(mImGray,imGrayRight,timestamp,mpORBextractorLeft,mpORBextractorRight,mpORBVocabulary,mK,mDistCoef,mbf,mThDepth);

        Track();

        return mCurrentFrame.mTcw.clone();
    }

    cv::Mat Tracking::GrabImageRGBD(const cv::Mat &imRGB,const cv::Mat &imD, const double &timestamp)
    {
        mImGray = imRGB;
        cv::Mat imDepth = imD;

        if (mImGray.channels()==3)
        {
            if(mbRGB)
                cvtColor(mImGray, mImGray, CV_RGB2GRAY);
            else
                cvtColor(mImGray, mImGray, CV_BGR2GRAY);
        }
        else if (mImGray.channels() == 4)
        {
            if (mbRGB)
                cvtColor(mImGray, mImGray, CV_RGBA2GRAY);
            else
                cvtColor(mImGray, mImGray, CV_BGRA2GRAY);
        }

        if (mDepthMapFactor != 1 || imDepth.type() != CV_32F);
        imDepth.convertTo(imDepth, CV_32F, mDepthMapFactor);

        mCurrentFrame = Frame(mImGray, imDepth, timestamp, mpORBextractorLeft, mpORBVocabulary, mK, mDistCoef, mbf, mThDepth);

        Track();

        return mCurrentFrame.mTcw.clone();
    }


    cv::Mat Tracking::GrabImageMonocular(const cv::Mat &im, const double &timestamp)
    {
        LOG("GrabImageMonocular(): called!");

        //shortcut to the passed incoming camera frame
        mImGray = im;

        //do some color conversion
        if (mImGray.channels() == 3)
        {
            LOG("GrabImageMonocular(): image came in from camera with three channels");
            if (mbRGB) {
                LOG("GrabImageMonocular(): converting from RGB 2 GRAY");
                cvtColor(mImGray, mImGray, CV_RGB2GRAY);
            }
            else {
            LOG("GrabImageMonocular(): converting from BGR 2 GRAY");
                cvtColor(mImGray, mImGray, CV_BGR2GRAY);
            }
        }
        else if (mImGray.channels() == 4)
        {
            LOG("GrabImageMonocular(): image came in from camera with four channels");
            if (mbRGB) {
                LOG("GrabImageMonocular(): converting from RGBA 2 GRAY"); //**THIS IS THE ONE WE DO
                //save copy of original mat (but converted to three channels) first
                cvtColor(mImGray, mImRgb, CV_RGBA2RGB);
                //mImRgb = mImGray;

                //convert mImGray to grayscale (one channel)
                cvtColor(mImGray, mImGray, CV_RGBA2GRAY);
            }
            else {
                LOG("GrabImageMonocular(): converting from BGRA 2 GRAY");
                cvtColor(mImGray, mImGray, CV_BGRA2GRAY);
            }
        }

        {//critical section, locking mMutex ('''lock''' is the name of the unique_lock obj here)
            unique_lock<mutex> lock(mMutex); //when the unique_lock is constructed it will lock the passed mutex, and when
                                             //gets deconstructed
                                             //(end of brackets) or exception is thrown (also gets deconstructed), it will unlock it

            //now we have the lock on currKeyPoints

            //reset keypoints array to all -1 (clear it)
            std::fill(currKeyPoints.begin(), currKeyPoints.end(), -1);
        }//unlock mMutex


        //construct a Frame using the passed incoming camera capture, which also does the ORB feature extraction
        if (mState == NOT_INITIALIZED || mState == NO_IMAGES_YET) {
            LOG("Setting mCurrentFrame to new Frame obj using IniORBextractor...");
            mCurrentFrame = Frame(mImGray, mImRgb, timestamp, mpIniORBextractor, mpORBVocabulary,
            mK, mDistCoef, mbf, mThDepth, mPosenet, mInterpreter, currKeyPoints, mMutex);
        }
        else {
            LOG("Setting mCurrentFrame to new Frame obj using ORBextractorLeft...");
            mCurrentFrame = Frame(mImGray, mImRgb, timestamp, mpORBextractorLeft, mpORBVocabulary,
            mK, mDistCoef, mbf, mThDepth, mPosenet, mInterpreter, currKeyPoints, mMutex);
        }
        //posenet keypoints are now in currKeyPoints


        //add each posenet keypoint to the map
        for (int i = 0; i < currKeyPoints.size(); i+=2) {
            if (currKeyPoints[i] != -1) {

            }
        }

        //LOG("GrabImageMonocular(): now calling Tracking::Track()...");

        //run the tracking algorithms to get the camera pose
        Track();

        //LOG("GrabImageMonocular(): Track() finished!");

        //LOG("mTcw now has %d rows, %d cols", mCurrentFrame.mTcw.rows, mCurrentFrame.mTcw.cols);

        return mCurrentFrame.mTcw.clone(); //mTcw is a matrix containing the camera pose for that frame
    }

    //read currKeyPoints and return float* to the data there (this fxn is in case Java code wants to extract the pts for drawing, etc)
    float* Tracking::posenetExternalGetPts() {
        float* ret;

        {//critical section, locking mMutex ('''lock''' is the name of the unique_lock obj here)
            unique_lock<mutex> lock(mMutex); //when the unique_lock is constructed it will lock the passed mutex, and when gets deconstructed
                                             //(end of brackets) or exception is thrown (also gets deconstructed), it will unlock it


            //now we have the lock on currKeyPoints
            ret = (float*)(currKeyPoints.data());

        } //unlock mMutex

        //return ptr to first float in currKeyPoints
        return ret;
    }

    //run actual tracking algorithms and do state checking, relocalization, and MapDrawer interaction
    void Tracking::Track()
    {
        if (mState == NO_IMAGES_YET)
        {
            mState = NOT_INITIALIZED;
        }

        mLastProcessedState=mState;

        //Get Map Mutex->Map cannot be changed
        unique_lock<mutex> lock(mpMap->mMutexMapUpdate);

        //the monocular tracking system still hasn't been initialized
        if (mState == NOT_INITIALIZED)
        {
            LOG("Track(): state of the Tracker is NOT_INITIALIZED!");
            if (mSensor==System::STEREO || mSensor==System::RGBD) {
                StereoInitialization();
            }

            //this is what we want
            else {
               LOG("Track(): running MonocularInitialization()...");
               MonocularInitialization();
               LOG("Track(): MonocularInitialization()... finished!");
            }

            mpFrameDrawer->Update(this);

            if (mState != OK) {
                LOG("Track(): mState is NOT OK, returning...");
                return;
            }
        }
        else
        {
            LOG("Track(): System is initialized");
            //System is initialized. Track Frame.
            bool bOK;

            //Initial camera pose estimation using motion model or relocalization (if tracking is lost)
            if (!mbOnlyTracking) //"Track only" button is not selected
            {
                //Local Mapping is activated. This is the normal behaviour, unless
                //you explicitly activate the "only tracking" mode.

                if (mState == OK)
                {
                    //Local Mapping might have changed some MapPoints tracked in last frame
                    CheckReplacedInLastFrame();

                    if (mVelocity.empty() || mCurrentFrame.mnId < mnLastRelocFrameId + 2)
                    {
                        LOG("Tracking(): running TrackReferenceKeyFrame()");
                        //will be false if not enough matches found
                        bOK = TrackReferenceKeyFrame();
                    }

                    //we always do this
                    else
                    {
                        LOG("Tracking(): running TrackWithMotionModel()");
                        bOK = TrackWithMotionModel();

                        if (!bOK) {
                            LOG("Tracking(): TrackWithMotionModel false, now running TrackReferenceKeyFrame");
                            bOK = TrackReferenceKeyFrame(); //will probably also return false
                        }
                    }
                }

                //otherwise state is NOT OK, we need to run relocalization
                else
                {
                    LOG("Track(): local mapping, state is not okay, running relocalization...");
                    bOK = Relocalization();
                }
            }
            else
            {
                //Only Tracking: Local Mapping is deactivated
                if (mState == LOST) //if camera pose is lost then try to relocalize it
                {
                    bOK = Relocalization();
                }
                else
                {
                    if (!mbVO)
                    {
                        //In last frame we tracked enough MapPoints in the map
                        if(!mVelocity.empty())
                        {
                            bOK = TrackWithMotionModel();
                        }
                        else
                        {
                            bOK = TrackReferenceKeyFrame();
                        }
                    }
                    else
                    {
                        //In last frame we tracked mainly "visual odometry" points.

                        //We compute two camera poses, one from motion model and one doing relocalization.
                        //If relocalization is sucessfull we choose that solution, otherwise we retain
                        //the "visual odometry" solution.

                        bool bOKMM = false;
                        bool bOKReloc = false;
                        vector<MapPoint*> vpMPsMM;
                        vector<bool> vbOutMM;
                        cv::Mat TcwMM;

                        if (!mVelocity.empty())
                        {
                            bOKMM = TrackWithMotionModel();
                            vpMPsMM = mCurrentFrame.mvpMapPoints;
                            vbOutMM = mCurrentFrame.mvbOutlier;
                            TcwMM = mCurrentFrame.mTcw.clone();
                        }
                        bOKReloc = Relocalization();

                        //in order for SetPose to be run, these two conditions must be met
                        if (bOKMM && !bOKReloc)
                        {
                            LOG("Track(): Running SetPose() on the current frame...");
                            mCurrentFrame.SetPose(TcwMM);
                            mCurrentFrame.mvpMapPoints = vpMPsMM;
                            mCurrentFrame.mvbOutlier = vbOutMM;

                            if(mbVO)
                            {
                                for(int i =0; i<mCurrentFrame.N; i++)
                                {
                                    if(mCurrentFrame.mvpMapPoints[i] && !mCurrentFrame.mvbOutlier[i])
                                    {
                                        mCurrentFrame.mvpMapPoints[i]->IncreaseFound();
                                    }
                                }
                            }
                        }
                        else if (bOKReloc)
                        {
                            LOG("Track(): bOKReloc set");
                            mbVO = false;
                        }

                        bOK = bOKReloc || bOKMM;
                    }
                }
            }

            //set the current frame's reference keyframe to
            mCurrentFrame.mpReferenceKF = mpReferenceKF;

            //If we have an initial estimation of the camera pose and matching. Track the local map.
            if (!mbOnlyTracking) //if "track only" button is not selected -- NORMAL BEHAVIOUR
            {
                if (bOK)
                    //TrackLocalMap() updates mvpLocalMapPoints, sets the reference point for our local Map instance,
                    //adds all KeyFrames that observe a MapPoint to mvpLocalKeyFrames, adds all MapPointMatches to mvpLocalMapPoints,
                    //updates MapPoint stats, and determines if tracking was successful
                    bOK = TrackLocalMap();
                else
                    LOG("Track(): bOK is false (Relocalization failed or first fail of TrackWMotionModel and TrackRefKeyFrame), NOT running TrackLocalMap()!");
            }

            //if "track only" IS SELECTED
            else
            {
                //If mbVO is true, there are few matches to MapPoints in the map. We cannot retrieve
                //a local map and therefore we do not perform TrackLocalMap(). Once the system relocalizes
                //the camera we will use the local map again.
                if (bOK && !mbVO) {
                    LOG("Track(): bOK true and mbVO false, run TrackLocalMap()...");
                    bOK = TrackLocalMap();
                }
                else
                    LOG("Track(): mbVO came back true, there are too few matches to MapPoints in the map, can't retrieve local map.");
            }

            if (bOK)
                mState = OK;


            //I think this means the camera position was lost
            else {
                LOG("Track(): bOK false, setting mState to LOST...");
                mState = LOST;
            }

            //Data fusion here?

            //Update drawer
            mpFrameDrawer->Update(this);

            //If tracking was good, check if we insert a keyframe
            if (bOK)
            {
                //Update motion model
                if (!mLastFrame.mTcw.empty())
                {
                    //the last frame's camera pose matrix is non-empty

                    cv::Mat LastTwc = cv::Mat::eye(4, 4, CV_32F);

                    //get world to camera rotation matrix from last frame
                    mLastFrame.GetRotationInverse().copyTo(LastTwc.rowRange(0,3).colRange(0,3));
                    mLastFrame.GetCameraCenter().copyTo(LastTwc.rowRange(0,3).col(3));

                    //set mVelocity
                    mVelocity = mCurrentFrame.mTcw * LastTwc;
                }
                else
                    //otherwise make a new mVelocity mat
                    mVelocity = cv::Mat();

                LOG("Track(): tracking was good, calling SetCurrentCameraPose on mpMapDrawer...");
                mpMapDrawer->SetCurrentCameraPose(mCurrentFrame.mTcw);

                //Clean temporal point matches
                for (int i = 0; i < mCurrentFrame.N; i++)
                {
                    MapPoint* pMP = mCurrentFrame.mvpMapPoints[i];
                    if (pMP)
                        if (pMP->Observations()<1)
                        {
                            //nullify this point
                            mCurrentFrame.mvbOutlier[i] = false;
                            mCurrentFrame.mvpMapPoints[i] = static_cast<MapPoint*>(NULL);
                        }
                }


                //Delete temporal MapPoints
                for (list<MapPoint*>::iterator lit = mlpTemporalPoints.begin(), lend = mlpTemporalPoints.end(); lit!=lend; lit++)
                {
                    MapPoint* pMP = *lit;
                    delete pMP;
                }
                mlpTemporalPoints.clear();

                // Check if we need to insert a new keyframe to the database
                if (NeedNewKeyFrame()) {
                    LOG("Track(): new keyframe needed, creating one now...");
                    CreateNewKeyFrame();
                 }


                // We allow points with high innovation (considered outliers by the Huber Function)
                // pass to the new keyframe, so that bundle adjustment will finally decide
                // if they are outliers or not. We don't want next frame to estimate its position
                // with those points so we discard them in the frame.
                for (int i = 0; i < mCurrentFrame.N; i++)
                {
                    if(mCurrentFrame.mvpMapPoints[i] && mCurrentFrame.mvbOutlier[i])
                        mCurrentFrame.mvpMapPoints[i] = static_cast<MapPoint*>(NULL);
                }
            }

            //Reset if the camera get lost soon after initialization
            if (mState==LOST)
            {
                LOG("Track(): mState is LOST");
                if (mpMap->KeyFramesInMap() <= 5)
                {
                    LOG("Track(): camera got lost soon after initialization, calling Reset() on System...");

                    mpSystem->Reset();
                    return;
                }
            }

            if (!mCurrentFrame.mpReferenceKF)
                mCurrentFrame.mpReferenceKF = mpReferenceKF;

            //set the last frame to the current frame
            mLastFrame = Frame(mCurrentFrame);
        }


        //Store frame pose information to retrieve the complete camera trajectory afterwards.
        if (!mCurrentFrame.mTcw.empty())
        {

            cv::Mat Tcr = mCurrentFrame.mTcw * mCurrentFrame.mpReferenceKF->GetPoseInverse();


            mlRelativeFramePoses.push_back(Tcr);


            mlpReferences.push_back(mpReferenceKF);

            //record the timestamp
            mlFrameTimes.push_back(mCurrentFrame.mTimeStamp);

            //check if the state is lost and keep track
            mlbLost.push_back(mState==LOST);
        }
        else
        {
            //This can happen if tracking is lost
            mlRelativeFramePoses.push_back(mlRelativeFramePoses.back());
            mlpReferences.push_back(mlpReferences.back());
            mlFrameTimes.push_back(mlFrameTimes.back());

            LOG("Tracking(): mCurrentFrame.mTcw came up empty, adding true val to mlbLost if mState==LOST");
            mlbLost.push_back(mState==LOST);
        }

    }

    //UNUSED FXN FOR STEREO SETUP
    void Tracking::StereoInitialization()
    {
        if (mCurrentFrame.N > 500)
        {
            // Set Frame pose to the origin
            mCurrentFrame.SetPose(cv::Mat::eye(4, 4, CV_32F));

            // Create KeyFrame
            KeyFrame* pKFini = new KeyFrame(mCurrentFrame,mpMap,mpKeyFrameDB);

            // Insert KeyFrame in the map
            mpMap->AddKeyFrame(pKFini);

            // Create MapPoints and asscoiate to KeyFrame
            for(int i=0; i<mCurrentFrame.N;i++)
            {
                float z = mCurrentFrame.mvDepth[i];
                if(z>0)
                {
                    cv::Mat x3D = mCurrentFrame.UnprojectStereo(i);
                    MapPoint* pNewMP = new MapPoint(x3D,pKFini,mpMap);
                    pNewMP->AddObservation(pKFini,i);
                    pKFini->AddMapPoint(pNewMP,i);
                    pNewMP->ComputeDistinctiveDescriptors();
                    pNewMP->UpdateNormalAndDepth();
                    mpMap->AddMapPoint(pNewMP);

                    mCurrentFrame.mvpMapPoints[i]=pNewMP;
                }
            }

            mpLocalMapper->InsertKeyFrame(pKFini);

            mLastFrame = Frame(mCurrentFrame);
            mnLastKeyFrameId=mCurrentFrame.mnId;
            mpLastKeyFrame = pKFini;

            mvpLocalKeyFrames.push_back(pKFini);
            mvpLocalMapPoints=mpMap->GetAllMapPoints();
            mpReferenceKF = pKFini;
            mCurrentFrame.mpReferenceKF = pKFini;

            mpMap->SetReferenceMapPoints(mvpLocalMapPoints);

            mpMap->mvpKeyFrameOrigins.push_back(pKFini);

            mpMapDrawer->SetCurrentCameraPose(mCurrentFrame.mTcw);

            mState=OK;
        }
    }

    void Tracking::MonocularInitialization()
    {
        if (!mpInitializer)
        {
            LOG("MonocularInitialization(): No Initializer object found");

            //Set Reference Frame
            if(mCurrentFrame.mvKeys.size() > 100)
            {
                mInitialFrame = Frame(mCurrentFrame);
                mLastFrame = Frame(mCurrentFrame);
                mvbPrevMatched.resize(mCurrentFrame.mvKeysUn.size());
                for(size_t i=0; i<mCurrentFrame.mvKeysUn.size(); i++)
                    mvbPrevMatched[i]=mCurrentFrame.mvKeysUn[i].pt;

                //should be false, but just in case we delete old Initializer
                if (mpInitializer)
                    delete mpInitializer;

                //Instantiate a new Initializer
                LOG("MonocularInitialization(): making new Initializer");
                mpInitializer =  new Initializer(mCurrentFrame, 1.0, 200);

                fill(mvIniMatches.begin(), mvIniMatches.end(), -1);

                return;
            }
        }

        //otherwise we have an Initializer object, so use it
        else
        {
            LOG("MonocularInitialization(): Initializer found");


            //Try to initialize
            if ((int) mCurrentFrame.mvKeys.size() <= 100)
            {
                LOG("MonocularInitialization(): Less than 100 keypoints, deleting initializer...");
                //delete the old initializer and set it to null
                delete mpInitializer;
                mpInitializer = static_cast<Initializer*>(NULL);


                fill(mvIniMatches.begin(), mvIniMatches.end(),-1);
                return;
            }

            // Find correspondences
            ORBmatcher matcher(0.9, true);
            int nmatches = matcher.SearchForInitialization(mInitialFrame,mCurrentFrame,mvbPrevMatched,mvIniMatches,100);

            // Check if there are enough correspondences
            if (nmatches<100)
            {
                delete mpInitializer;
                mpInitializer = static_cast<Initializer*>(NULL);
                return;
            }

            cv::Mat Rcw; //Current Camera Rotation
            cv::Mat tcw; //Current Camera Translation

            vector<bool> vbTriangulated; // Triangulated Correspondences (mvIniMatches)


            if (mpInitializer->Initialize(mCurrentFrame, mvIniMatches, Rcw, tcw, mvIniP3D, vbTriangulated))
            {
                LOG("mpInitializer->Initialize successful");

                for (size_t i=0, iend=mvIniMatches.size(); i<iend;i++)
                {
                    if(mvIniMatches[i]>=0 && !vbTriangulated[i])
                    {
                        mvIniMatches[i]=-1;
                        nmatches--;
                    }
                }

                //Set Frame Poses: set initial frame's Tcw to 4x4 identity matrix
                mInitialFrame.SetPose(cv::Mat::eye(4, 4, CV_32F));

                //initialize second frame's Tcw to 4x4 identity matrix so we can start recursion
                cv::Mat Tcw = cv::Mat::eye(4,4,CV_32F);

                //copy the rotation matrix into first 3x3 of Tcw
                Rcw.copyTo(Tcw.rowRange(0,3).colRange(0,3));

                //copy translation matrix into last 3x1 of Tcw
                tcw.copyTo(Tcw.rowRange(0,3).col(3));

                //set current frame's Tcw to the one we just made
                mCurrentFrame.SetPose(Tcw);

                LOG("CreateInitialMapMonocular");

                CreateInitialMapMonocular();
            }
        }
    }

    //INIT FUNCTION THAT WE USE FOR MONOCULAR
    void Tracking::CreateInitialMapMonocular()
    {
        //Create new KeyFrames
        KeyFrame* pKFini = new KeyFrame(mInitialFrame, mpMap, mpKeyFrameDB);
        KeyFrame* pKFcur = new KeyFrame(mCurrentFrame, mpMap, mpKeyFrameDB);

        //Run computeBoW on both of the created initialization keyframes
        pKFini->ComputeBoW();
        pKFcur->ComputeBoW();

        //Insert KFs in the map to get it started
        mpMap->AddKeyFrame(pKFini);
        mpMap->AddKeyFrame(pKFcur);

        //Create MapPoints and associate them to the two created keyframes
        for (size_t i = 0; i < mvIniMatches.size(); i++)
        {
            if (mvIniMatches[i] < 0)
                continue;

            //Create MapPoint.
            cv::Mat worldPos(mvIniP3D[i]);

            MapPoint* pMP = new MapPoint(worldPos, pKFcur, mpMap);

            pKFini->AddMapPoint(pMP,i);
            pKFcur->AddMapPoint(pMP,mvIniMatches[i]);

            pMP->AddObservation(pKFini, i);
            pMP->AddObservation(pKFcur, mvIniMatches[i]);

            pMP->ComputeDistinctiveDescriptors();
            pMP->UpdateNormalAndDepth();

            //Fill Current Frame structure
            mCurrentFrame.mvpMapPoints[mvIniMatches[i]] = pMP;
            mCurrentFrame.mvbOutlier[mvIniMatches[i]] = false;

            //Add the newly created MapPoint to the Map
            mpMap->AddMapPoint(pMP);
        }

        //Update Connections
        pKFini->UpdateConnections();
        pKFcur->UpdateConnections();

        //Bundle Adjustment
        LOG("New Map created with points");

        Optimizer::GlobalBundleAdjustemnt(mpMap,20);

        //Set median depth to 1
        float medianDepth = pKFini->ComputeSceneMedianDepth(2);
        float invMedianDepth = 1.0f / medianDepth;

        if (medianDepth<0 || pKFcur->TrackedMapPoints(1)<100)
        {
            LOG("Wrong initialization, resetting...");
            Reset();
            return;
        }

        //Scale initial baseline
        cv::Mat Tc2w = pKFcur->GetPose();
        Tc2w.col(3).rowRange(0,3) = Tc2w.col(3).rowRange(0,3)*invMedianDepth;
        pKFcur->SetPose(Tc2w);

        // Scale points
        vector<MapPoint*> vpAllMapPoints = pKFini->GetMapPointMatches();
        for(size_t iMP = 0; iMP < vpAllMapPoints.size(); iMP++)
        {
            if (vpAllMapPoints[iMP])
            {
                MapPoint* pMP = vpAllMapPoints[iMP];
                pMP->SetWorldPos(pMP->GetWorldPos()*invMedianDepth);
            }
        }

        mpLocalMapper->InsertKeyFrame(pKFini);
        mpLocalMapper->InsertKeyFrame(pKFcur);

        mCurrentFrame.SetPose(pKFcur->GetPose());
        mnLastKeyFrameId=mCurrentFrame.mnId;
        mpLastKeyFrame = pKFcur;

        mvpLocalKeyFrames.push_back(pKFcur);
        mvpLocalKeyFrames.push_back(pKFini);
        mvpLocalMapPoints=mpMap->GetAllMapPoints();

        //set both this Tracking's reference KeyFrame and the current Frame's reference KF to pKFcur
        mpReferenceKF = pKFcur;
        mCurrentFrame.mpReferenceKF = pKFcur;

        //set last frame and current frame to the same thing (since we're initializing)
        mLastFrame = Frame(mCurrentFrame);

        mpMap->SetReferenceMapPoints(mvpLocalMapPoints);

        mpMapDrawer->SetCurrentCameraPose(pKFcur->GetPose());

        mpMap->mvpKeyFrameOrigins.push_back(pKFini);
        LOG("CreateInitialMapMonocular(): setting mState = OK");

        //set the state to OK
        mState = OK;
    }

    void Tracking::CheckReplacedInLastFrame()
    {
        for(int i =0; i<mLastFrame.N; i++)
        {
            MapPoint* pMP = mLastFrame.mvpMapPoints[i];

            if (pMP)
            {
                MapPoint* pRep = pMP->GetReplaced();
                if (pRep)
                {
                    mLastFrame.mvpMapPoints[i] = pRep;
                }
            }
        }
    }


    bool Tracking::TrackReferenceKeyFrame()
    {
        //Compute Bag of Words vector
        mCurrentFrame.ComputeBoW();

        //We perform first an ORB matching with the reference keyframe
        //If enough matches are found we setup a PnP solver
        ORBmatcher matcher(0.7, true);
        vector<MapPoint*> vpMapPointMatches;

        int nmatches = matcher.SearchByBoW(mpReferenceKF, mCurrentFrame, vpMapPointMatches);

        if (nmatches < 15) {
            LOG("TrackReferenceKeyFrame(): nmatches came up <15, returning false to set bOK in Tracking() to false, also trigger state=LOST");
            return false;
        }

        mCurrentFrame.mvpMapPoints = vpMapPointMatches;

        //set the mTcw for current frame (this won't happen if nmatches was < 15, causing "This can happen if tracking is lost" to trigger and set
        //state to LOST
        mCurrentFrame.SetPose(mLastFrame.mTcw);

        Optimizer::PoseOptimization(&mCurrentFrame);

        // Discard outliers
        int nmatchesMap = 0;
        for(int i =0; i<mCurrentFrame.N; i++)
        {
            if(mCurrentFrame.mvpMapPoints[i])
            {
                if(mCurrentFrame.mvbOutlier[i])
                {
                    MapPoint* pMP = mCurrentFrame.mvpMapPoints[i];

                    mCurrentFrame.mvpMapPoints[i]=static_cast<MapPoint*>(NULL);
                    mCurrentFrame.mvbOutlier[i]=false;
                    pMP->mbTrackInView = false;
                    pMP->mnLastFrameSeen = mCurrentFrame.mnId;
                    nmatches--;
                }
                else if(mCurrentFrame.mvpMapPoints[i]->Observations()>0)
                    nmatchesMap++;
            }
        }

        return nmatchesMap>=10;
    }

    void Tracking::UpdateLastFrame()
    {
        // Update pose according to reference keyframe
        KeyFrame* pRef = mLastFrame.mpReferenceKF;
        cv::Mat Tlr = mlRelativeFramePoses.back();

        mLastFrame.SetPose(Tlr*pRef->GetPose());

        if(mnLastKeyFrameId==mLastFrame.mnId || mSensor==System::MONOCULAR)
            return;

        //Create "visual odometry" MapPoints
        //We sort points according to their measured depth by the stereo/RGB-D sensor
        vector<pair<float,int> > vDepthIdx;
        vDepthIdx.reserve(mLastFrame.N);
        for(int i=0; i<mLastFrame.N;i++)
        {
            float z = mLastFrame.mvDepth[i];
            if(z>0)
            {
                vDepthIdx.push_back(make_pair(z,i));
            }
        }

        if(vDepthIdx.empty())
            return;

        sort(vDepthIdx.begin(),vDepthIdx.end());

        // We insert all close points (depth < mThDepth)
        // If less than 100 close points, we insert the 100 closest ones.
        int nPoints = 0;
        for(size_t j=0; j<vDepthIdx.size();j++)
        {
            int i = vDepthIdx[j].second;

            bool bCreateNew = false;

            MapPoint* pMP = mLastFrame.mvpMapPoints[i];
            if(!pMP)
                bCreateNew = true;
            else if(pMP->Observations()<1)
            {
                bCreateNew = true;
            }

            if(bCreateNew)
            {
                cv::Mat x3D = mLastFrame.UnprojectStereo(i);

                //create new MapPoint using
                MapPoint* pNewMP = new MapPoint(x3D, mpMap, &mLastFrame, i);

                mLastFrame.mvpMapPoints[i]=pNewMP;

                mlpTemporalPoints.push_back(pNewMP);
                nPoints++;
            }
            else
            {
                nPoints++;
            }

            if(vDepthIdx[j].first>mThDepth && nPoints>100)
                break;
        }
    }

    bool Tracking::TrackWithMotionModel()
    {
        ORBmatcher matcher(0.9,true);

        //Update last frame pose according to its reference keyframe
        //Create "visual odometry" points
        UpdateLastFrame();

        //**set the mTcw of this frame
        mCurrentFrame.SetPose(mVelocity * mLastFrame.mTcw);

        fill(mCurrentFrame.mvpMapPoints.begin(),mCurrentFrame.mvpMapPoints.end(),static_cast<MapPoint*>(NULL));

        // Project points seen in previous frame
        int th;
        if(mSensor!=System::STEREO)
            th=15;
        else
            th=7;

        //run BRIEF descriptor matching for keyfeatures (FAST corners) from this frame nad from last frame
        int nmatches = matcher.SearchByProjection(mCurrentFrame, mLastFrame, th, mSensor==System::MONOCULAR);

        //If few matches, uses a wider window search
        if (nmatches < 20)
        {
            fill(mCurrentFrame.mvpMapPoints.begin(),mCurrentFrame.mvpMapPoints.end(), static_cast<MapPoint*>(NULL));
            nmatches = matcher.SearchByProjection(mCurrentFrame, mLastFrame, 2*th, mSensor==System::MONOCULAR);
        }

        //if still too few matches, return false to set bOK to false in Tracking()
        if (nmatches < 20) {
            LOG("TrackWithMotionModel(): too few matches found by ORBmatcher between this and last frame, returning false to set bOK to false");
            return false;
        }

        //Optimize frame pose with all matches
        Optimizer::PoseOptimization(&mCurrentFrame);

        //Discard outliers
        int nmatchesMap = 0;
        for (int i = 0; i < mCurrentFrame.N; i++)
        {
            if(mCurrentFrame.mvpMapPoints[i])
            {
                if(mCurrentFrame.mvbOutlier[i])
                {
                    MapPoint* pMP = mCurrentFrame.mvpMapPoints[i];

                    mCurrentFrame.mvpMapPoints[i]=static_cast<MapPoint*>(NULL);
                    mCurrentFrame.mvbOutlier[i]=false;
                    pMP->mbTrackInView = false;
                    pMP->mnLastFrameSeen = mCurrentFrame.mnId;
                    nmatches--;
                }
                else if(mCurrentFrame.mvpMapPoints[i]->Observations()>0)
                    nmatchesMap++;
            }
        }

        if(mbOnlyTracking)
        {
            mbVO = nmatchesMap<10;
            return nmatches>20;
        }

        return nmatchesMap >= 10;
    }

    bool Tracking::TrackLocalMap()
    {
        //We have an estimation of the camera pose and some map points tracked in the frame.
        //We retrieve the local map and try to find matches to points in the local map.

        UpdateLocalMap();

        SearchLocalPoints();

        //Optimize Pose
        Optimizer::PoseOptimization(&mCurrentFrame);
        mnMatchesInliers = 0;

        //Update MapPoints Statistics
        for (int i=0; i<mCurrentFrame.N; i++)
        {
            if(mCurrentFrame.mvpMapPoints[i])
            {
                if(!mCurrentFrame.mvbOutlier[i])
                {
                    mCurrentFrame.mvpMapPoints[i]->IncreaseFound();
                    if(!mbOnlyTracking)
                    {
                        if(mCurrentFrame.mvpMapPoints[i]->Observations()>0)
                            mnMatchesInliers++;
                    }
                    else
                        mnMatchesInliers++;
                }
                else if(mSensor==System::STEREO)
                    mCurrentFrame.mvpMapPoints[i] = static_cast<MapPoint*>(NULL);

            }
        }

        //Decide if the tracking was succesful
        //More restrictive if there was a relocalization recently
        if (mCurrentFrame.mnId < mnLastRelocFrameId + mMaxFrames && mnMatchesInliers < 50)
            return false;

        if (mnMatchesInliers < 30)
            return false;
        else
            return true;
    }


    bool Tracking::NeedNewKeyFrame()
    {
        if(mbOnlyTracking)
            return false;

        // If Local Mapping is freezed by a Loop Closure do not insert keyframes
        if(mpLocalMapper->isStopped() || mpLocalMapper->stopRequested())
            return false;

        const int nKFs = mpMap->KeyFramesInMap();

        // Do not insert keyframes if not enough frames have passed from last relocalisation
        if(mCurrentFrame.mnId<mnLastRelocFrameId+mMaxFrames && nKFs>mMaxFrames)
            return false;

        // Tracked MapPoints in the reference keyframe
        int nMinObs = 3;
        if(nKFs<=2)
            nMinObs=2;
        int nRefMatches = mpReferenceKF->TrackedMapPoints(nMinObs);

        // Local Mapping accept keyframes?
        bool bLocalMappingIdle = mpLocalMapper->AcceptKeyFrames();

        // Stereo & RGB-D: Ratio of close "matches to map"/"total matches"
        // "total matches = matches to map + visual odometry matches"
        // Visual odometry matches will become MapPoints if we insert a keyframe.
        // This ratio measures how many MapPoints we could create if we insert a keyframe.
        int nMap = 0;
        int nTotal= 0;

        //IGNORE - FOR NON-MONOCULAR
        if (mSensor!=System::MONOCULAR)
        {
            for(int i =0; i<mCurrentFrame.N; i++)
            {
                if(mCurrentFrame.mvDepth[i]>0 && mCurrentFrame.mvDepth[i]<mThDepth)
                {
                    nTotal++;
                    if(mCurrentFrame.mvpMapPoints[i])
                        if(mCurrentFrame.mvpMapPoints[i]->Observations()>0)
                            nMap++;
                }
            }
        }
        else
        {
            //There are no visual odometry matches in the monocular case
            nMap = 1;
            nTotal = 1;
        }

        const float ratioMap = (float)nMap/fmax(1.0f,nTotal);

        //Thresholds
        float thRefRatio = 0.75f;
        if(nKFs<2)
            thRefRatio = 0.4f;

        if(mSensor==System::MONOCULAR)
            thRefRatio = 0.9f;

        float thMapRatio = 0.35f;
        if(mnMatchesInliers>300)
            thMapRatio = 0.20f;

        // Condition 1a: More than "MaxFrames" have passed from last keyframe insertion
        const bool c1a = mCurrentFrame.mnId>=mnLastKeyFrameId+mMaxFrames;
        // Condition 1b: More than "MinFrames" have passed and Local Mapping is idle
        const bool c1b = (mCurrentFrame.mnId>=mnLastKeyFrameId+mMinFrames && bLocalMappingIdle);
        //Condition 1c: tracking is weak
        const bool c1c =  mSensor!=System::MONOCULAR && (mnMatchesInliers<nRefMatches*0.25 || ratioMap<0.3f) ;
        // Condition 2: Few tracked points compared to reference keyframe. Lots of visual odometry compared to map matches.
        const bool c2 = ((mnMatchesInliers<nRefMatches*thRefRatio|| ratioMap<thMapRatio) && mnMatchesInliers>15);

        if ((c1a||c1b||c1c)&&c2)
        {
            // If the mapping accepts keyframes, insert keyframe.
            // Otherwise send a signal to interrupt BA
            if(bLocalMappingIdle)
            {
                return true;
            }
            else
            {
                mpLocalMapper->InterruptBA();
                if(mSensor!=System::MONOCULAR)
                {
                    if(mpLocalMapper->KeyframesInQueue()<3)
                        return true;
                    else
                        return false;
                }
                else
                    return false;
            }
        }
        else
            return false;
    }

    //make a new KeyFrame because the scene has changed
    void Tracking::CreateNewKeyFrame()
    {
        if (!mpLocalMapper->SetNotStop(true))
            return;

        //create a new KeyFrame out of the current frame, our local Map instance, and our KeyFrameDatabase instance
        KeyFrame* pKF = new KeyFrame(mCurrentFrame, mpMap, mpKeyFrameDB);

        //set our current reference KeyFrame to the newly created KeyFrame
        mpReferenceKF = pKF;

        //set the current Frame's reference KeyFrame to the newly created KeyFrame
        mCurrentFrame.mpReferenceKF = pKF;

        //IGNORE
        if (mSensor!=System::MONOCULAR)
        {
            mCurrentFrame.UpdatePoseMatrices();

            //We sort points by the measured depth by the stereo/RGBD sensor.
            //We create all those MapPoints whose depth < mThDepth.
            //If there are less than 100 close points we create the 100 closest.
            vector<pair<float,int> > vDepthIdx;
            vDepthIdx.reserve(mCurrentFrame.N);

            for (int i=0; i<mCurrentFrame.N; i++)
            {
                float z = mCurrentFrame.mvDepth[i];
                if (z > 0)
                {
                    vDepthIdx.push_back(make_pair(z,i));
                }
            }

            if(!vDepthIdx.empty())
            {
                sort(vDepthIdx.begin(),vDepthIdx.end());

                int nPoints = 0;

                for (size_t j=0; j<vDepthIdx.size();j++)
                {
                    int i = vDepthIdx[j].second;

                    bool bCreateNew = false;

                    MapPoint* pMP = mCurrentFrame.mvpMapPoints[i];

                    //if this MapPoint is NULL, we need to create a new one
                    if (!pMP)
                        bCreateNew = true;

                    //if this MapPoint's observations is <1, we also need to create a new one
                    else if(pMP->Observations() < 1)
                    {
                        bCreateNew = true;

                        //nullify this MapPoint within the Frame, because it's TRASH
                        mCurrentFrame.mvpMapPoints[i] = static_cast<MapPoint*>(NULL);
                    }

                    if (bCreateNew)
                    {
                        cv::Mat x3D = mCurrentFrame.UnprojectStereo(i);
                        MapPoint* pNewMP = new MapPoint(x3D, pKF, mpMap);
                        pNewMP->AddObservation(pKF,i);

                        //add this new MapPoint to the KeyFrame we're creating
                        pKF->AddMapPoint(pNewMP, i);
                        pNewMP->ComputeDistinctiveDescriptors();
                        pNewMP->UpdateNormalAndDepth();
                        mpMap->AddMapPoint(pNewMP);

                        mCurrentFrame.mvpMapPoints[i]=pNewMP;
                        nPoints++;
                    }
                    else
                    {
                        nPoints++;
                    }

                    if (vDepthIdx[j].first>mThDepth && nPoints>100)
                        break;
                }
            }
        }

        //insert the KeyFrame into our LocalMapping instance (NOTE**: pKF.mGrid holds KeyPoints found by FAST)
        mpLocalMapper->InsertKeyFrame(pKF);

        mpLocalMapper->SetNotStop(false);

        //save this KeyFrame as our last KeyFrame
        mnLastKeyFrameId = mCurrentFrame.mnId;
        mpLastKeyFrame = pKF;
    }

    void Tracking::SearchLocalPoints()
    {
        //Do not search map points already matched
        for(vector<MapPoint*>::iterator vit=mCurrentFrame.mvpMapPoints.begin(), vend=mCurrentFrame.mvpMapPoints.end(); vit!=vend; vit++)
        {
            MapPoint* pMP = *vit;
            if(pMP)
            {
                if(pMP->isBad())
                {
                    *vit = static_cast<MapPoint*>(NULL);
                }
                else
                {
                    pMP->IncreaseVisible();
                    pMP->mnLastFrameSeen = mCurrentFrame.mnId;
                    pMP->mbTrackInView = false;
                }
            }
        }

        int nToMatch=0;

        //Project points in frame and check its visibility
        for(vector<MapPoint*>::iterator vit=mvpLocalMapPoints.begin(), vend=mvpLocalMapPoints.end(); vit!=vend; vit++)
        {
            MapPoint* pMP = *vit;
            if (pMP->mnLastFrameSeen == mCurrentFrame.mnId)
                continue;
            if (pMP->isBad())
                //skip this MapPoint if it's bad
                continue;
            //Project (this fills MapPoint variables for matching)
            if (mCurrentFrame.isInFrustum(pMP,0.5))
            {
                pMP->IncreaseVisible();
                nToMatch++;
            }
        }

        if (nToMatch>0)
        {
            ORBmatcher matcher(0.8);
            int th = 1;

            //if we're using RGBD camera
            if (mSensor==System::RGBD)
                th = 3;

            //If the camera has been relocalized recently, perform a coarser search
            if(mCurrentFrame.mnId<mnLastRelocFrameId+2)
                th = 5;

            matcher.SearchByProjection(mCurrentFrame, mvpLocalMapPoints, th);
        }
    }

    //Update the map, setting the Map's reference map points to our current
    void Tracking::UpdateLocalMap()
    {
        //This is for visualization
        mpMap->SetReferenceMapPoints(mvpLocalMapPoints);

        //Update
        UpdateLocalKeyFrames();
        UpdateLocalPoints();
    }

    void Tracking::UpdateLocalPoints()
    {
        mvpLocalMapPoints.clear();

        //iterate over all KeyFrames in mvpLocalKeyFrames
        for (vector<KeyFrame*>::const_iterator itKF=mvpLocalKeyFrames.begin(), itEndKF=mvpLocalKeyFrames.end(); itKF!=itEndKF; itKF++)
        {
            //get this KeyFrame
            KeyFrame* pKF = *itKF;

            //get MapPointMatches from this KeyFrame
            const vector<MapPoint*> vpMPs = pKF->GetMapPointMatches();

            for (vector<MapPoint*>::const_iterator itMP=vpMPs.begin(), itEndMP=vpMPs.end(); itMP!=itEndMP; itMP++)
            {
                MapPoint* pMP = *itMP;
                if (!pMP)
                    continue;
                if (pMP->mnTrackReferenceForFrame==mCurrentFrame.mnId)
                    continue;


                if (!pMP->isBad())
                {
                    //as long as this MapPoint is good, add it to mvpLocalMapPoints
                    mvpLocalMapPoints.push_back(pMP);

                    //record this Frame's ID in a property of this MapPoint, for future reference
                    pMP->mnTrackReferenceForFrame = mCurrentFrame.mnId;
                }
            }
        }
    }


    void Tracking::UpdateLocalKeyFrames()
    {
        //Each map point vote for the keyframes in which it has been observed
        map<KeyFrame*, int> keyframeCounter;

        //iterate over all the MapPoints (associated with keypoints) from the current frame
        for (int i=0; i<mCurrentFrame.N; i++)
        {
            //null check this MapPoint
            if (mCurrentFrame.mvpMapPoints[i])
            {
                MapPoint* pMP = mCurrentFrame.mvpMapPoints[i];

                //get observations from this MapPoint and...
                if (!pMP->isBad())
                {
                    const map<KeyFrame*,size_t> observations = pMP->GetObservations();
                    for(map<KeyFrame*,size_t>::const_iterator it=observations.begin(), itend=observations.end(); it!=itend; it++)
                        keyframeCounter[it->first]++;
                }
                //throw out this MapPoint if it's bad
                else
                {
                    mCurrentFrame.mvpMapPoints[i]=NULL;
                }
            }
        }

        if (keyframeCounter.empty())
            return;

        int max=0;
        KeyFrame* pKFmax= static_cast<KeyFrame*>(NULL);

        mvpLocalKeyFrames.clear();
        mvpLocalKeyFrames.reserve(3*keyframeCounter.size());

        //All keyframes that observe a map point are included in the local map. Also check which keyframe shares most points
        for(map<KeyFrame*,int>::const_iterator it=keyframeCounter.begin(), itEnd=keyframeCounter.end(); it!=itEnd; it++)
        {
            KeyFrame* pKF = it->first;

            if(pKF->isBad())
                continue;

            if(it->second>max)
            {
                max=it->second;
                pKFmax=pKF;
            }

            mvpLocalKeyFrames.push_back(it->first);
            pKF->mnTrackReferenceForFrame = mCurrentFrame.mnId;
        }


        // Include also some not-already-included keyframes that are neighbors to already-included keyframes
        for(vector<KeyFrame*>::const_iterator itKF=mvpLocalKeyFrames.begin(), itEndKF=mvpLocalKeyFrames.end(); itKF!=itEndKF; itKF++)
        {
            // Limit the number of keyframes
            if(mvpLocalKeyFrames.size()>80)
                break;

            KeyFrame* pKF = *itKF;

            const vector<KeyFrame*> vNeighs = pKF->GetBestCovisibilityKeyFrames(10);

            for(vector<KeyFrame*>::const_iterator itNeighKF=vNeighs.begin(), itEndNeighKF=vNeighs.end(); itNeighKF!=itEndNeighKF; itNeighKF++)
            {
                KeyFrame* pNeighKF = *itNeighKF;
                if(!pNeighKF->isBad())
                {
                    if(pNeighKF->mnTrackReferenceForFrame!=mCurrentFrame.mnId)
                    {
                        mvpLocalKeyFrames.push_back(pNeighKF);
                        pNeighKF->mnTrackReferenceForFrame=mCurrentFrame.mnId;
                        break;
                    }
                }
            }

            const set<KeyFrame*> spChilds = pKF->GetChilds();
            for(set<KeyFrame*>::const_iterator sit=spChilds.begin(), send=spChilds.end(); sit!=send; sit++)
            {
                KeyFrame* pChildKF = *sit;
                if(!pChildKF->isBad())
                {
                    if(pChildKF->mnTrackReferenceForFrame!=mCurrentFrame.mnId)
                    {
                        mvpLocalKeyFrames.push_back(pChildKF);
                        pChildKF->mnTrackReferenceForFrame=mCurrentFrame.mnId;
                        break;
                    }
                }
            }

            KeyFrame* pParent = pKF->GetParent();
            if(pParent)
            {
                if(pParent->mnTrackReferenceForFrame!=mCurrentFrame.mnId)
                {
                    mvpLocalKeyFrames.push_back(pParent);
                    pParent->mnTrackReferenceForFrame=mCurrentFrame.mnId;
                    break;
                }
            }

        }

        if(pKFmax)
        {
            mpReferenceKF = pKFmax;
            mCurrentFrame.mpReferenceKF = mpReferenceKF;
        }
    }

    bool Tracking::Relocalization()
    {
        LOG("Relocalization(): CALLED");

        //Compute Bag of Words Vector
        mCurrentFrame.ComputeBoW();

        //Relocalization is performed when tracking is lost
        //Track Lost: Query KeyFrame Database for keyframe candidates for relocalization
        vector<KeyFrame*> vpCandidateKFs = mpKeyFrameDB->DetectRelocalizationCandidates(&mCurrentFrame);

        if (vpCandidateKFs.empty()) {
            LOG("Relocalization(): no relocalization candidates found from KeyFrame database, return FALSE");
            return false;
        }

        //get the number of keyframe candidates we have for relocalization
        const int nKFs = vpCandidateKFs.size();

        //We perform first an ORB matching with each candidate
        //If enough matches are found we setup a PnP solver
        ORBmatcher matcher(0.75, true); //just instantiate the matcher

        //create array of pointers to PnPsolver objects
        vector<PnPsolver*> vpPnPsolvers;

        vpPnPsolvers.resize(nKFs);

        //create array of ptrs to MapPoints
        vector<vector<MapPoint*> > vvpMapPointMatches;
        vvpMapPointMatches.resize(nKFs);

        //create an array of bools
        vector<bool> vbDiscarded;
        vbDiscarded.resize(nKFs);

        int nCandidates = 0;

        for (int i = 0; i < nKFs; i++)
        {
            //get this keyframe
            KeyFrame* pKF = vpCandidateKFs[i];

            //if the keyframe is bad, discard it
            if(pKF->isBad())
                vbDiscarded[i] = true;

            else
            {
                //use brief descriptors to search for matching pts between KeyFrame found and current frame
                int nmatches = matcher.SearchByBoW(pKF, mCurrentFrame, vvpMapPointMatches[i]);

                //not enough matches for this keyframe and the current frame, so discard it and move to next keyframe
                if (nmatches < 15)
                {
                    vbDiscarded[i] = true;
                    continue;
                }

                //enough matches were found, set up a PnP solver
                else
                {
                    LOG("Relocalization(): enough matches found between current frame and a KeyFrame from the database, setting up PnP solver");

                    //instantiate new PnPsolver using the matching points found
                    PnPsolver* pSolver = new PnPsolver(mCurrentFrame, vvpMapPointMatches[i]);
                    pSolver->SetRansacParameters(0.99, 10, 300, 4, 0.5, 5.991);

                    //add this solver to the list
                    vpPnPsolvers[i] = pSolver;

                    //up the number of valid KeyFrame candidates
                    nCandidates++;
                }
            }
        }

        //Alternatively perform some iterations of P4P RANSAC
        //Until we found a camera pose supported by enough inliers
        LOG("Relocalization(): performing iterations of P4P RANSAC until find cam pose supported by enough inliers");

        bool bMatch = false;
        ORBmatcher matcher2 (0.9, true);

        while (nCandidates>0 && !bMatch)
        {
            //iterate over all KeyFrames found in database
            for (int i=0; i<nKFs; i++)
            {
                //ignore this one if it was discarded and no PnPsolver was ever set up for it
                if(vbDiscarded[i])
                    continue;

                //Perform 5 Ransac Iterations
                vector<bool> vbInliers;
                int nInliers;
                bool bNoMore;

                //get the PnPsolver we set up for this KeyFrame
                PnPsolver* pSolver = vpPnPsolvers[i];

                //try to calculate the pose based on the current frame and KeyFrame from the database
                cv::Mat Tcw = pSolver->iterate(5, bNoMore, vbInliers, nInliers);

                //If Ransac reachs max. iterations discard keyframe, because pose calculation failed
                if (bNoMore)
                {
                    LOG("Relocalization(): pose calculation failed using this KeyFrame");
                    vbDiscarded[i] = true;

                    //decrement number of candidate KeyFrames
                    nCandidates--;
                }

                //If a Camera Pose is computed, optimize it
                if (!Tcw.empty()) //if the camera pose is not empty
                {
                    LOG("Relocalization(): pose calculation succeeded using this KeyFrame");

                    //set it as the actual camear pose
                    Tcw.copyTo(mCurrentFrame.mTcw);

                    set<MapPoint*> sFound;

                    const int np = vbInliers.size();

                    for (int j = 0; j < np; j++)
                    {
                        if(vbInliers[j])
                        {
                            mCurrentFrame.mvpMapPoints[j] = vvpMapPointMatches[i][j];
                            sFound.insert(vvpMapPointMatches[i][j]);
                        }
                        else
                            mCurrentFrame.mvpMapPoints[j]=NULL;
                    }

                    int nGood = Optimizer::PoseOptimization(&mCurrentFrame);

                    if (nGood<10)
                        continue;

                    for (int io = 0; io < mCurrentFrame.N; io++)
                        if(mCurrentFrame.mvbOutlier[io])
                            mCurrentFrame.mvpMapPoints[io] = static_cast<MapPoint*>(NULL);

                    //If few inliers, search by projection in a coarse window and optimize again
                    if (nGood < 50)
                    {
                        LOG("Relocalization(): not enough inliers...");
                        int nadditional = matcher2.SearchByProjection(mCurrentFrame,vpCandidateKFs[i],sFound,10,100);

                        if(nadditional+nGood>=50)
                        {
                            nGood = Optimizer::PoseOptimization(&mCurrentFrame);

                            //If many inliers but still not enough, search by projection again in a narrower window
                            //the camera has been already optimized with many points
                            if(nGood > 30 && nGood < 50)
                            {
                                sFound.clear();

                                for (int ip = 0; ip < mCurrentFrame.N; ip++)
                                    if (mCurrentFrame.mvpMapPoints[ip])
                                        sFound.insert(mCurrentFrame.mvpMapPoints[ip]);

                                nadditional = matcher2.SearchByProjection(mCurrentFrame,vpCandidateKFs[i],sFound,3,64);

                                //Final optimization
                                if (nGood + nadditional >= 50)
                                {
                                    nGood = Optimizer::PoseOptimization(&mCurrentFrame);

                                    for (int io = 0; io < mCurrentFrame.N; io++)
                                        if(mCurrentFrame.mvbOutlier[io])
                                            mCurrentFrame.mvpMapPoints[io]=NULL;
                                }
                            }
                        }
                    }


                    //If the pose is supported by enough inliers stop ransacs and continue
                    if (nGood >= 50)
                    {
                        LOG("Relocalization(): Pose supported by enough inliers (pose found), can resume tracking");
                        bMatch = true;
                        break;
                    }
                }
            }
        }

        //keyframe matching failed
        if (!bMatch)
        {
            LOG("Relocalization(): camera pose still not recovered, return FALSE");
            return false;
        }
        else
        {
            mnLastRelocFrameId = mCurrentFrame.mnId;
            return true;
        }

    }

    void Tracking::Reset()
    {
        mpViewer->RequestStop();
        LOG("Reset(): (CALLED) System Resetting...");
        //while(!mpViewer->isStopped())
            //usleep(3000);

        //Reset Local Mapping
        mpLocalMapper->RequestReset();

        //Reset Loop Closing
        mpLoopClosing->RequestReset();

        //Clear BoW Database
        mpKeyFrameDB->clear();

        //Clear Map (this erase MapPoints and KeyFrames)
        mpMap->clear();

        KeyFrame::nNextId = 0;
        Frame::nNextId = 0;
        mState = NO_IMAGES_YET;

        if(mpInitializer)
        {
            delete mpInitializer;
            mpInitializer = static_cast<Initializer*>(NULL);
        }

        mlRelativeFramePoses.clear();
        mlpReferences.clear();
        mlFrameTimes.clear();
        mlbLost.clear();

        mpViewer->Release();
        LOG("mpViewer->Release();");
    }

    void Tracking::ChangeCalibration(const string &strSettingPath)
    {
        cv::FileStorage fSettings(strSettingPath, cv::FileStorage::READ);
        float fx = fSettings["Camera.fx"];
        float fy = fSettings["Camera.fy"];
        float cx = fSettings["Camera.cx"];
        float cy = fSettings["Camera.cy"];

        cv::Mat K = cv::Mat::eye(3,3,CV_32F);
        K.at<float>(0,0) = fx;
        K.at<float>(1,1) = fy;
        K.at<float>(0,2) = cx;
        K.at<float>(1,2) = cy;
        K.copyTo(mK);

        cv::Mat DistCoef(4,1,CV_32F);
        DistCoef.at<float>(0) = fSettings["Camera.k1"];
        DistCoef.at<float>(1) = fSettings["Camera.k2"];
        DistCoef.at<float>(2) = fSettings["Camera.p1"];
        DistCoef.at<float>(3) = fSettings["Camera.p2"];
        const float k3 = fSettings["Camera.k3"];
        if(k3!=0)
        {
            DistCoef.resize(5);
            DistCoef.at<float>(4) = k3;
        }
        DistCoef.copyTo(mDistCoef);

        mbf = fSettings["Camera.bf"];

        Frame::mbInitialComputations = true;
    }

    void Tracking::InformOnlyTracking(const bool &flag)
    {
        mbOnlyTracking = flag;
    }

} //namespace ORB_SLAM
