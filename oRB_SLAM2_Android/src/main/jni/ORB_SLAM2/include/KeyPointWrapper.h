#ifndef WEINER_SLAMIT_V2_KEYPOINTWRAPPER_H
#define WEINER_SLAMIT_V2_KEYPOINTWRAPPER_H

#include <opencv2/opencv.hpp>

namespace ORB_SLAM2 {
    class KeyPointWrapper {
        public:
            cv::KeyPoint mKeyPoint;
            bool mbHuman;

            KeyPointWrapper();
            KeyPointWrapper(cv::KeyPoint keyPoint, bool human);
    };
}

#endif //WEINER_SLAMIT_V2_KEYPOINTWRAPPER_H
