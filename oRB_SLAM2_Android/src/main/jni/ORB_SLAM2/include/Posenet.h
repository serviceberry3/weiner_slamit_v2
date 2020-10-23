#ifndef WEINER_SLAMIT_V2_POSENET_H
#define WEINER_SLAMIT_V2_POSENET_H

extern "C" {
#include "c_api.h"
#include "delegate.h" }

namespace ORB_SLAM2 {

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
            Posenet(const char* pFilename, Device pDevice);
            void close();
            TfLiteInterpreter* getInterpreter();
            std::vector<float> initInputArray(cv::Mat incomingImg);

    };

}

#endif //WEINER_SLAMIT_V2_POSENET_H
