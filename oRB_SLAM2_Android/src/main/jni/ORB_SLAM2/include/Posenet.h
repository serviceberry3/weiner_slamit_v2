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
    TfLiteModel* model;
    float x;
    float y;
};

class KeyPoint {
  BodyPart bodyPart;
  Position position;
  float score;
};

    //TfLiteInterpreterOptions* TfLiteInterpreterOptionsCreate();
    class Person {
      std::vector<KeyPoint> keyPoints;
      float score;
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
        TfLiteInterpreter* interpreter;

        int NUM_LITE_THREADS = 4;


        public:
            Posenet();
            Posenet(const char* pFilename, Device pDevice);
            void close();
            TfLiteInterpreter* getInterpreter();
            std::vector<float> initInputArray(cv::Mat incomingImg);
            float sigmoid(float x);
            std::unordered_map<int, std::vector<std::vector<std::vector<std::vector<float>>>>> initOutputMap(TfLiteInterpreter* interpreter);
            void runForMultipleInputsOutputs(std::vector<float> inputs, std::unordered_map<int,
            std::vector<std::vector<std::vector<std::vector<float>>>> > outputs);
            Person estimateSinglePose(cv::Mat img);
    };

}

#endif //POSENET_H
