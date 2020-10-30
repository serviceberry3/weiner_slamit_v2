#ifndef POSENET_H
#define POSENET_H

#include <opencv2/core/core.hpp>
#include <vector>
#include <unordered_map>
#include <time.h>

#include "c_api.h"
#include "delegate.h"


namespace ORB_SLAM2 {

    enum class BodyPart {
       NOSE,
       LEFT_EYE,
       RIGHT_EYE,
       LEFT_EAR,
       RIGHT_EAR,
       LEFT_SHOULDER,
       RIGHT_SHOULDER,
       LEFT_ELBOW,
       RIGHT_ELBOW,
       LEFT_WRIST,
       RIGHT_WRIST,
       LEFT_HIP,
       RIGHT_HIP,
       LEFT_KNEE,
       RIGHT_KNEE,
       LEFT_ANKLE,
       RIGHT_ANKLE
    };

    class Position {
        public:
            float x;
            float y;
    };

    class KeyPoint {
        public:
          BodyPart bodyPart;
          Position position;
          float score;

          //convenient but unnecessary functions for retrieving the score of a given KeyPoint instance
          float getScore();
    };

    class Person {
        public:
          std::vector<KeyPoint> keyPoints;
          float score;

          //convenient but unnecessary functions for retrieving the KeyPoints and score for a Person object
          std::vector<KeyPoint> getKeyPoints();
          float getScore();
    };

    enum class Device {
        CPU,
        NNAPI,
        GPU
    };

    class Posenet {
        const char* filename;
        Device device;
        long lastInferenceTimeNanos = -1;

        //the model
        TfLiteModel* model;

        //the options for the interpreter (like settings)
        TfLiteInterpreterOptions* options;

        //interpreter for tflite model
        TfLiteInterpreter* interpreter = NULL;

        //number of threads to run on
        int NUM_LITE_THREADS = 4;

        //helper functions for running a cv::Mat through the TfLite Posenet model
        public:
            Posenet();
            Posenet(const char* pFilename, Device pDevice);
            void close();
            TfLiteInterpreter* getInterpreter();
            std::vector<float> initInputArray(const cv::Mat &incomingImg);
            float sigmoid(float x);
            std::unordered_map<int, std::vector<std::vector<std::vector<std::vector<float>>>> > initOutputMap();
            void runForMultipleInputsOutputs(std::vector<float> &inputs, std::unordered_map<int,
            std::vector<std::vector<std::vector<std::vector<float>>>> > &outputs);

            //"main" function for human pose estimation using the model
            Person estimateSinglePose(const cv::Mat &img, TfLiteInterpreter* pInterpreter);
            void readFlatIntoMultiDimensionalArray(float* data, std::vector<std::vector<std::vector<std::vector<float>>>> &map);
    };
}

#endif //POSENET_H
