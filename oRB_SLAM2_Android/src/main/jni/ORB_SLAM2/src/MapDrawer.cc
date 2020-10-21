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

#include "MapDrawer.h"
#include "MapPoint.h"
#include "KeyFrame.h"
#include <GLES/gl.h>
#include <GLES/glext.h>
// #include <GLES/glu.h>
#include <mutex>
#include <android/log.h>
#define LOG_TAG "ORB_SLAM_SYSTEM_MAPDRAWER"

#define LOG(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG, __VA_ARGS__)

namespace ORB_SLAM2 {

MapDrawer::MapDrawer(Map* pMap, const string &strSettingPath) :
    mpMap(pMap) {

	//LOG("MapDrawer instantiating FileStorage obj/opening the calib file...");
	//cv::FileStorage fSettings(strSettingPath, cv::FileStorage::READ);

/*
	//check if the YAML file was opened successfully
    if (!fSettings.isOpened())
    {
        //failure
        LOG("MapDrawer failed to open calibration file, trying again...");
        bool tryAgain = fSettings.open(strSettingPath, cv::FileStorage::APPEND);
        if (!tryAgain) {
            LOG("MapDrawer to open calibration file second time, exiting in 10 sec...");
            sleep(10);
            exit(-1);
        }
    }
    LOG("MapDrawer successfully opened the calibration file.");



	mKeyFrameSize = fSettings["Viewer.KeyFrameSize"];
	mKeyFrameLineWidth = fSettings["Viewer.KeyFrameLineWidth"];
	mGraphLineWidth = fSettings["Viewer.GraphLineWidth"];
	mPointSize = fSettings["Viewer.PointSize"];
	mCameraSize = fSettings["Viewer.CameraSize"];
	mCameraLineWidth = fSettings["Viewer.CameraLineWidth"];
	*/

	//HARDCODE
	mKeyFrameSize = 0.05;
	mKeyFrameLineWidth = 1;
    mGraphLineWidth = 0.9;
    mPointSize = 2;
    mCameraSize = 0.08;
    mCameraLineWidth = 3;
}

void MapDrawer::DrawMapPoints() {
	const vector<MapPoint*> &vpMPs = mpMap->GetAllMapPoints();
	const vector<MapPoint*> &vpRefMPs = mpMap->GetReferenceMapPoints();

	set<MapPoint*> spRefMPs(vpRefMPs.begin(), vpRefMPs.end());

	if (vpMPs.empty()) {
	    LOG("DrawMapPoints(): vpMPs came up EMPTY, returning...");
		return;
	}

	// 清除屏幕及深度缓存
	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// 重置当前的模型观察矩阵
	// glMatrixMode (GL_MODELVIEW);
	// glLoadIdentity();
	// glScalef(2.0f,2.0f,2.0f);
	// glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	// glLineWidth(mKeyFrameLineWidth);


	glPointSize(mPointSize);
	glEnable (GL_COLOR_MATERIAL);
	glEnableClientState (GL_VERTEX_ARRAY);

	//glColor4f(0.0f, 0.0f, 0.0f, 1.0f); // black
	glColor4f(0.0f, 0.0f, 1.0f, 1.0f); // blue

	for (int i = 0, iend = vpMPs.size(); i < iend; i++) {
		if (vpMPs[i]->isBad() || spRefMPs.count(vpMPs[i]))
			continue;
		cv::Mat pos = vpMPs[i]->GetWorldPos();

		GLfloat vertexArray[3] = { pos.at<float>(0), pos.at<float>(1), pos.at<float>(2) };
		glVertexPointer(3, GL_FLOAT, 0, vertexArray);
		glDrawArrays(GL_POINTS, 0, 1);
	}

	//flush gl drawing command buffer
	glFlush();

	//set color for drawing the points (RED)
	glColor4f(1.0f, 0.0f, 0.0f, 1.0f);


	for (set<MapPoint*>::iterator sit = spRefMPs.begin(), send = spRefMPs.end(); sit != send; sit++) {
		if ((*sit)->isBad())
			continue;
		cv::Mat pos = (*sit)->GetWorldPos();
		GLfloat vertexArray[] = { pos.at<float>(0), pos.at<float>(1), pos.at<float>(2) };
		glVertexPointer(3, GL_FLOAT, 0, vertexArray);

		//draw the points
		glDrawArrays(GL_POINTS, 0, 1);
	}

	//flush gl drawing command buffer
	glFlush();
}

void MapDrawer::DrawKeyFrames(const bool bDrawKF, const bool bDrawGraph) {
	const float &w = mKeyFrameSize;
	const float h = w * 0.75;
	const float z = w * 0.6;

	//LOG("debug: ----->>>> MapDrawer::DrawKeyFrames -- START --------------");

	const vector<KeyFrame*> vpKFs = mpMap->GetAllKeyFrames();

	glEnable (GL_COLOR_MATERIAL);

	glEnableClientState (GL_VERTEX_ARRAY);

	//LOG("debug: ----->>>> MapDrawer::DrawKeyFrames -- KF1 --------------");


	if (bDrawKF) {
		//LOG("debug: ----->>>> MapDrawer::DrawKeyFrames -- KF2 -- bDrawKF OPTION TRUE --------------");

		for (size_t i = 0; i < vpKFs.size(); i++) {
			KeyFrame* pKF = vpKFs[i];
			cv::Mat Twc = pKF->GetPoseInverse().t();
			glPushMatrix();
			glMultMatrixf(Twc.ptr < GLfloat > (0));
			glLineWidth(mKeyFrameLineWidth);
			glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
			GLfloat vertexArray[] = { 0, 0, 0, w, h, z, 0, 0, 0, w, -h, z, 0, 0,
					0, -w, -h, z, 0, 0, 0, -w, h, z, w, h, z, w, -h, z, -w, h,
					z, -w, -h, z, -w, h, z, w, h, z, -w, -h, z, w, -h, z };
			glVertexPointer(3, GL_FLOAT, 0, vertexArray);
			glDrawArrays(GL_LINES, 0, 16);
			glPopMatrix();
		}
	}
	glFlush();
	if (bDrawGraph) {
		//LOG("debug: ----->>>> MapDrawer::DrawKeyFrames -- KF3 bDrawGraph TRUE --------------");

		glLineWidth(mGraphLineWidth);
		glColor4f(0.0f, 1.0f, 0.0f, 0.6f);
		for (size_t i = 0; i < vpKFs.size(); i++) {
			// Covisibility Graph
			const vector<KeyFrame*> vCovKFs = vpKFs[i]->GetCovisiblesByWeight(
					100);
			cv::Mat Ow = vpKFs[i]->GetCameraCenter();
			if (!vCovKFs.empty()) {
				for (vector<KeyFrame*>::const_iterator vit = vCovKFs.begin(),
						vend = vCovKFs.end(); vit != vend; vit++) {
					if ((*vit)->mnId < vpKFs[i]->mnId)
						continue;
					cv::Mat Ow2 = (*vit)->GetCameraCenter();
					GLfloat vertexArray[] = { Ow.at<float>(0), Ow.at<float>(1),
							Ow.at<float>(2), Ow2.at<float>(0), Ow2.at<float>(1),
							Ow2.at<float>(2) };
					glVertexPointer(3, GL_FLOAT, 0, vertexArray);
					glDrawArrays(GL_LINES, 0, 2);
				}
			}

			// Spanning tree
			KeyFrame* pParent = vpKFs[i]->GetParent();
			if (pParent) {
				//LOG("debug: ----->>>> MapDrawer::DrawKeyFrames -- KF4 --------------");

				cv::Mat Owp = pParent->GetCameraCenter();
				GLfloat vertexArray[] = { Ow.at<float>(0), Ow.at<float>(1),
						Ow.at<float>(2), Owp.at<float>(0), Owp.at<float>(1),
						Owp.at<float>(2) };
				glVertexPointer(3, GL_FLOAT, 0, vertexArray);
				glDrawArrays(GL_LINES, 0, 2);
			}

			// Loops
			set<KeyFrame*> sLoopKFs = vpKFs[i]->GetLoopEdges();
			for (set<KeyFrame*>::iterator sit = sLoopKFs.begin(), send =
					sLoopKFs.end(); sit != send; sit++) {
				if ((*sit)->mnId < vpKFs[i]->mnId)
					continue;
				cv::Mat Owl = (*sit)->GetCameraCenter();
				GLfloat vertexArray[] = { Ow.at<float>(0), Ow.at<float>(1),
						Ow.at<float>(2), Owl.at<float>(0), Owl.at<float>(1),
						Owl.at<float>(2) };
				glVertexPointer(3, GL_FLOAT, 0, vertexArray);
				glDrawArrays(GL_LINES, 0, 2);
			}
		}

	}
	//LOG("debug: ----->>>> MapDrawer::DrawKeyFrames -- KF5 --------------");

	glFlush();
	//LOG("debug: ----->>>> MapDrawer::DrawKeyFrames -- END --------------");

}

//draw the current camera on the screen (the little box with x through it)
void MapDrawer::DrawCurrentCamera(const cv::Mat &M) {
    LOG("DrawCurrentCamera() called");

	const float &w = mCameraSize;
	const float h = w * 0.75;
	const float z = w * 0.6;

    //clear out what was there previously
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //scale by x2 along x, y, and z axes
	glScalef(2.0f, 2.0f, 2.0f);

    LOG("Clearing color to black...");
	//glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // white
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // black

	//generate 4x4 identity matrix
	cv::Mat temp = cv::Mat::eye(4, 4, CV_32F);

    //enable GL drawing capabilities
	glEnable (GL_COLOR_MATERIAL);

    LOG("Enabling vertex arrays...");
	//the vertex array is enabled for writing and now used during rendering when glDrawArrays, or glDrawElements is called.
	glEnableClientState (GL_VERTEX_ARRAY);


	if (!mCameraPose.empty()) {
	    LOG("mCameraPose came up non-empty, creating rotation and translation matrices");
		cv::Mat Rwc(3,3,CV_32F);
		cv::Mat twc(3,1,CV_32F);
		cv::Mat viewPos(3,1,CV_32F);

		//lookAt matrix generation
		viewPos.at<float>(0)=0.0;
		viewPos.at<float>(1)=0.0;
		viewPos.at<float>(2)=0.3;

		//start it as a 4x4 identity matrix
		cv::Mat lookAtMat = cv::Mat::eye(4, 4, CV_32F);

		//a couple of 3x1 matrices (actually vectors)
		cv::Mat zVec(3, 1, CV_32F);
		cv::Mat sVec(3, 1, CV_32F);
		cv::Mat uVec(3, 1, CV_32F);
		
		{
			unique_lock<mutex> lock(mMutexCamera);

            //rotation and translation
			Rwc = mCameraPose.rowRange(0,3).colRange(0,3).t();
			twc = -Rwc*mCameraPose.rowRange(0,3).col(3);

			//lookAt matrix
			zVec = -Rwc * viewPos;
			normalize(zVec.col(0),zVec.col(0),1,cv::NORM_L2);
			uVec.at<float>(0)=0.0;
			uVec.at<float>(1)=1.0;
			uVec.at<float>(2)=0.0;
			sVec = zVec.cross(uVec);
			normalize(sVec.col(0),sVec.col(0),1,cv::NORM_L2);
			uVec = sVec.cross(zVec);
			normalize(uVec.col(0),uVec.col(0),1,cv::NORM_L2);
			//uVec = sVec.cross(zVec);

			viewPos = Rwc * viewPos + twc;
		}

        //make a copy of the camera pose matrix
		temp = mCameraPose.clone();

        //fill the temp matrix with values
		temp.at<float>(0,3) = 0.0;
		temp.at<float>(1,3) = 0.0;
		temp.at<float>(2,3) = 0.0;
		temp.at<float>(3,0) = twc.at<float>(0);
		temp.at<float>(3,1) = twc.at<float>(1);
		temp.at<float>(3,2) = twc.at<float>(2);
		temp.at<float>(3,3) = 1.0;

        //fill the lookat matrix with preset values
		lookAtMat.at<float>(0,0) = sVec.at<float>(0);
		lookAtMat.at<float>(1,0) = sVec.at<float>(1);
		lookAtMat.at<float>(2,0) = sVec.at<float>(2);
		lookAtMat.at<float>(3,0) = 0.0;
		lookAtMat.at<float>(0,1) = uVec.at<float>(0);
		lookAtMat.at<float>(1,1) = uVec.at<float>(1);
		lookAtMat.at<float>(2,1) = uVec.at<float>(2);
		lookAtMat.at<float>(3,1) = 0.0;
		lookAtMat.at<float>(0,2) = -zVec.at<float>(0);
		lookAtMat.at<float>(1,2) = -zVec.at<float>(1);
		lookAtMat.at<float>(2,2) = -zVec.at<float>(2);
		lookAtMat.at<float>(3,2) = 0.0;
		lookAtMat.at<float>(0,3) = 0.0;
		lookAtMat.at<float>(1,3) = 0.0;
		lookAtMat.at<float>(2,3) = 0.0;
		lookAtMat.at<float>(3,3) = 1.0;

		glMultMatrixf(lookAtMat.ptr<GLfloat>(0));
		glTranslatef(-viewPos.at<float>(0),-viewPos.at<float>(1),-viewPos.at<float>(2));
		//gluLookAt(viewPos.at<float>(0),viewPos.at<float>(1),viewPos.at<float>(2),twc.at<float>(0),twc.at<float>(1),twc.at<float>(2),0.0,1.0,0.0);
	}

	else {
	    LOG("mCameraPose came up EMPTY");
	}

    //glPushMatrix pushes current matrix stack down by one, duplicating current matrix. After a glPushMatrix call, the matrix on top
    //of the stack is identical to the one below it.

    //(Initially, each of the stacks contains one matrix, an identity matrix.)
	glPushMatrix();

    //multiplies current matrix with the one specified (param), and replaces the current matrix with the product
    //Current matrix is determined by current matrix mode (see glMatrixMode). It is either projection matrix, modelview matrix,
    //or the texture matrix.
	glMultMatrixf(temp.ptr<GLfloat>(0));

    //set line width of the camera drawing
	glLineWidth(mCameraLineWidth);

    //set the color of the camera drawing (now GREEN)
	glColor4f(0.0f, 1.0f, 0.0f, 1.0f);


	GLfloat vertexArray[] = { 0, 0, 0, w, h, z, 0, 0, 0, w, -h, z, 0, 0, 0, -w,
			-h, z, 0, 0, 0, -w, h, z, w, h, z, w, -h, z, -w, h, z, -w, -h, z,
			-w, h, z, w, h, z, -w, -h, z, w, -h, z, };

	glVertexPointer(3, GL_FLOAT, 0, vertexArray);

	//draw the camera on the screen
	glDrawArrays(GL_LINES, 0, 16);

	//glPopMatrix pops the current matrix stack, removing current mat and replacing it with the one below it on the stack.
	glPopMatrix();

	//Different GL implementations buffer commands in several different locations, including network buffers and graphics accelerator itself.
	//glFlush empties all of these buffers, causing all issued commands to be executed as quickly as they are accepted by actual rendering
	//engine. Though this execution may not be completed in any particular time period, it does complete in finite time.
    //Because any GL program might be executed over a network, or on an accelerator that buffers commands, all programs should call
    //glFlush whenever they count on having all of their previously issued commands completed. For example, call before waiting for user
    //input that depends on the generated image.
	glFlush();


    //DrawKeyFrames(false,true);

    //draw the 3D map/point cloud on the screen (red)
    DrawMapPoints();
	//glDisableClientState (GL_VERTEX_ARRAY);
	//glDisable (GL_COLOR_MATERIAL);
}

void MapDrawer::SetCurrentCameraPose(const cv::Mat &Tcw) {
    LOG("SetCurrentCameraPose() called");
	unique_lock < mutex > lock(mMutexCamera);
	mCameraPose = Tcw.clone();
}

cv::Mat MapDrawer::GetCurrentOpenGLCameraMatrix() {
    //generate a new 4x4 identity matrix
	cv::Mat M = cv::Mat::eye(4, 4, CV_32F);
	// cv::Mat M(4, 4, CV_32F);
	// if (!mCameraPose.empty()) {
	// 	cv::Mat Rwc(3, 3, CV_32F);
	// 	cv::Mat twc(3, 1, CV_32F);
	// 	{
	// 		unique_lock < mutex > lock(mMutexCamera);
	// 		Rwc = mCameraPose.rowRange(0, 3).colRange(0, 3).t();
	// 		twc = -Rwc * mCameraPose.rowRange(0, 3).col(3);
	// 	}
	// 	M.at<float>(0, 0) = Rwc.at<float>(0, 0);
	// 	M.at<float>(1, 0) = Rwc.at<float>(1, 0);
	// 	M.at<float>(2, 0) = Rwc.at<float>(2, 0);
	// 	M.at<float>(3, 0) = 0;
	// 	M.at<float>(0, 1) = Rwc.at<float>(0, 1);
	// 	M.at<float>(1, 1) = Rwc.at<float>(1, 1);
	// 	M.at<float>(2, 1) = Rwc.at<float>(2, 1);
	// 	M.at<float>(3, 1) = 0;
	// 	M.at<float>(0, 2) = Rwc.at<float>(0, 2);
	// 	M.at<float>(1, 2) = Rwc.at<float>(1, 2);
	// 	M.at<float>(2, 2) = Rwc.at<float>(2, 2);
	// 	M.at<float>(3, 2) = 0;
	// 	M.at<float>(0, 3) = twc.at<float>(0);
	// 	M.at<float>(1, 3) = twc.at<float>(1);
	// 	M.at<float>(2, 3) = twc.at<float>(2);
	// 	M.at<float>(3, 3) = 1.0;
	// }
	return M;
}

} //namespace ORB_SLAM
