#include "KeyPointWrapper.h"


namespace ORB_SLAM2 {
    KeyPointWrapper::KeyPointWrapper() {}

    KeyPointWrapper::KeyPointWrapper(cv::KeyPoint keyPoint, bool person):
     mKeyPoint(keyPoint),
     mbHuman(person)
     {}
}

