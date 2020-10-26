#ifndef POSENET_H
#define POSENET_H

#include <opencv2/core/core.hpp>
#include <vector>
#include <unordered_map>

#include "c_api.h"
#include "delegate.h"


namespace ORB_SLAM2 {

    //TfLiteInterpreterOptions* TfLiteInterpreterOptionsCreate();

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
    };

}

#endif //POSENET_H
