LOCAL_PATH:= $(call my-dir)

#############DLib模块##################

include $(CLEAR_VARS)
MAINDIR:= $(LOCAL_PATH)
include $(MAINDIR)/Thirdparty/clapack/Android.mk
LOCAL_PATH := $(MAINDIR)

include $(CLEAR_VARS)
MAINDIR:= $(LOCAL_PATH)
LOCAL_MODULE:= lapack
LOCAL_SHORT_COMMANDS := true

#should tmglib be here too? I think it's undefined
LOCAL_STATIC_LIBRARIES := clapack blas f2c

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
LOCAL_EXPORT_LDLIBS := $(LOCAL_LDLIBS)
LOCAL_PATH := $(MAINDIR)
include $(BUILD_SHARED_LIBRARY)
######################################

#############DLib模块##################
include $(CLEAR_VARS)
MAIN_DIR:=$(LOCAL_PATH)
OPENCV_LIB_TYPE:=STATIC
ifeq ("$(wildcard $(OPENCV_MK_PATH))","")  
#try to load OpenCV.mk from default install location  
include /home/nodog/Downloads/OpenCV-2.4.9-android-sdk/sdk/native/jni/OpenCV.mk
include $(OPENCV_MK_PATH)  
endif 
LOCAL_PATH:=$(MAIN_DIR)
LOCAL_MODULE:=DLib

####################源文件部分######################################
LOCAL_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/DBoW2/DLib/include/DUtils
FILE_LIST:=$(wildcard $(LOCAL_PATH)/Thirdparty/DBoW2/DLib/src/DUtils/*.cpp)
LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)

#LOCAL_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/DBoW2/DLib/include/DUtilsCV
#FILE_LIST:=$(wildcard $(LOCAL_PATH)/Thirdparty/DBoW2/DLib/src/DUtilsCV/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#
#LOCAL_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/DBoW2/DLib/include/DVision
#FILE_LIST:=$(wildcard $(LOCAL_PATH)/Thirdparty/DBoW2/DLib/src/DVision/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)

################BOOST#####################部分
BOOST_VERSION      := 1_49
PROJECT_ROOT       := $(LOCAL_PATH)
BOOST_INCLUDE_PATH := $(PROJECT_ROOT)/Thirdparty/Boost/include/boost-1_49
BOOST_LIB_PATH     := $(PROJECT_ROOT)/Thirdparty/Boost/lib
LOCAL_C_INCLUDES+= $(BOOST_INCLUDE_PATH) 
LOCAL_LDLIBS    := -llog
# The order of these libraries is often important.
LOCAL_LDLIBS += -L$(BOOST_LIB_PATH)     
LOCAL_LDLIBS    +=-lz -llog -landroid -lEGL -lGLESv1_CM
LOCAL_CPPFLAGS := -std=c++11 -pthread -frtti -fexceptions
LOCAL_CPPFLAGS += -D__cplusplus=201103L

#LOCAL_EXPORT_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/DBoW2/DLib/include/DUtils
#LOCAL_EXPORT_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/DBoW2/DLib/include/DUtilsCV
LOCAL_EXPORT_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/DBoW2/DLib/include
LOCAL_EXPORT_C_INCLUDES+=$(BOOST_INCLUDE_PATH)
LOCAL_EXPORT_C_INCLUDES+=E:/ORB_SLAM2/OpenCV-2.4.9-android-sdk/sdk/native/jni/include
LOCAL_PATH:=$(MAIN_DIR)
include $(BUILD_SHARED_LIBRARY)
###############################################################

##############DBoW2模块#########################################
include $(CLEAR_VARS)
MAIN_DIR:=$(LOCAL_PATH)

OPENCV_LIB_TYPE:=STATIC
ifeq ("$(wildcard $(OPENCV_MK_PATH))","")  
#try to load OpenCV.mk from default install location  
include /home/nodog/Downloads/OpenCV-2.4.9-android-sdk/sdk/native/jni/OpenCV.mk
else  
include $(OPENCV_MK_PATH)  
endif 
LOCAL_PATH:=$(MAIN_DIR)

LOCAL_MODULE:=DBoW2
LOCAL_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/DBoW2/include/DBoW2
FILE_LIST:=$(wildcard $(LOCAL_PATH)/Thirdparty/DBoW2/src/*.cpp)
LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_SHARED_LIBRARIES+=DLib
LOCAL_EXPORT_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/DBoW2/include
LOCAL_CPPFLAGS := -std=c++11 -pthread -frtti -fexceptions
LOCAL_CPPFLAGS += -D__cplusplus=201103L
LOCAL_PATH:=$(MAIN_DIR)
include $(BUILD_SHARED_LIBRARY)
###############################################################


###################G2O模块##################################
include $(CLEAR_VARS)
MAIN_DIR:=$(LOCAL_PATH)
LOCAL_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/g2o/g2o/core
LOCAL_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/g2o/g2o/stuff
LOCAL_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/g2o/g2o/solvers
LOCAL_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/g2o/g2o/types
FILE_LIST:=$(wildcard $(LOCAL_PATH)/Thirdparty/g2o/g2o/core/*.cpp)
LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)

FILE_LIST:=$(wildcard $(LOCAL_PATH)/Thirdparty/g2o/g2o/solvers/*.cpp)
LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)

FILE_LIST:=$(wildcard $(LOCAL_PATH)/Thirdparty/g2o/g2o/stuff/*.cpp)
LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)

FILE_LIST:=$(wildcard $(LOCAL_PATH)/Thirdparty/g2o/g2o/types/*.cpp)
LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/eigen3
LOCAL_SHARED_LIBRARIES+=lapack
LOCAL_MODULE:=g2o
LOCAL_EXPORT_LDLIBS := $(LOCAL_LDLIBS)
LOCAL_EXPORT_C_INCLUDES+=LOCAL_C_INCLUDES
LOCAL_CPPFLAGS := -std=c++11 -pthread -frtti -fexceptions
LOCAL_CPPFLAGS += -D__cplusplus=201103L
LOCAL_PATH:=$(MAIN_DIR)
include $(BUILD_SHARED_LIBRARY)
############################################################

##############Pangolin模块##################################
#include $(CLEAR_VARS)
#MAIN_DIR:=$(LOCAL_PATH)
#
#LOCAL_MODULE:=pangolin
#LOCAL_C_INCLUDES:=$(LOCAL_PATH)/Thirdparty/pangolin/include
#
#SRC_PANGOLIN_DIR:=$(LOCAL_PATH)/Thirdparty/pangolin/src
#LOCAL_SRC_FILES+=$(SRC_PANGOLIN_DIR)/console/SonsoleView.cpp
#
#FILE_LIST:=$(wildcard $(SRC_PANGOLIN_DIR)/display/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#
#FILE_LIST:=$(wildcard $(SRC_PANGOLIN_DIR)/display/device/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#
#FILE_LIST:=$(wildcard $(SRC_PANGOLIN_DIR)/display/widgets/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#
#FILE_LIST:=$(wildcard $(SRC_PANGOLIN_DIR)/gl/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#FILE_LIST:=$(wildcard $(SRC_PANGOLIN_DIR)/gl/compat/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#
#FILE_LIST:=$(wildcard $(SRC_PANGOLIN_DIR)/handler/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#
#FILE_LIST:=$(wildcard $(SRC_PANGOLIN_DIR)/hud/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#
#FILE_LIST:=$(wildcard $(SRC_PANGOLIN_DIR)/image/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#
#FILE_LIST:=$(wildcard $(SRC_PANGOLIN_DIR)/log/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#
#FILE_LIST:=$(wildcard $(SRC_PANGOLIN_DIR)/plot/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#
#FILE_LIST:=$(wildcard $(SRC_PANGOLIN_DIR)/python/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#
#FILE_LIST:=$(wildcard $(SRC_PANGOLIN_DIR)/utils/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#
#FILE_LIST:=$(wildcard $(SRC_PANGOLIN_DIR)/var/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#
#FILE_LIST:=$(wildcard $(SRC_PANGOLIN_DIR)/video/*.cpp)
#LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)
#LOCAL_EXPORT_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/pangolin/include
#
#LOCAL_CPPFLAGS := -std=c++11 -pthread -frtti -fexceptions -ftemplate-backtrace-limit=0
#LOCAL_CPPFLAGS += -D__cplusplus=201103L
#
#LOCAL_PATH:=$(MAIN_DIR)
#
#include $(BUILD_SHARED_LIBRARY)

##############ORB_SLAM2模块##################################
#clear out the variables
include $(CLEAR_VARS)
MAIN_DIR:=$(LOCAL_PATH)

#opencv stuff
OPENCV_LIB_TYPE:=STATIC
ifeq ("$(wildcard $(OPENCV_MK_PATH))","")  
#try to load OpenCV.mk from default install location  
include /home/nodog/Downloads/OpenCV-2.4.9-android-sdk/sdk/native/jni/OpenCV.mk
else  
include $(OPENCV_MK_PATH)  
endif

#name of the module
LOCAL_MODULE := ORB_SLAM2

LOCAL_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/eigen3
LOCAL_C_INCLUDES+=$(LOCAL_PATH)/ORB_SLAM2/include

FILE_LIST:=$(wildcard $(LOCAL_PATH)/ORB_SLAM2/src/*.cc)

#remove local path from each file in FILE_LIST to get relative paths, since file paths must be relative to LOCAL_PATH
LOCAL_SRC_FILES+=$(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_CPP_EXTENSION := .cc

LOCAL_SHARED_LIBRARIES+=DBoW2
LOCAL_SHARED_LIBRARIES+=DLib
LOCAL_SHARED_LIBRARIES+=g2o

#make this depend on tensorflowlite_jni!!!
LOCAL_SHARED_LIBRARIES+=Tensorflow
#LOCAL_SHARED_LIBRARIES+=Tensorflow-gpu

LOCAL_PREBUILTS+=Tensorflow
#LOCAL_PREBUILTS+=Tensorflow-gpu

#LOCAL_SHARED_LIBRARIES+=pangolin


LOCAL_LDLIBS += -llog -landroid -lEGL -lGLESv1_CM

#any module that depend on this one will inherit these includes (headers)
LOCAL_EXPORT_C_INCLUDES+=$(LOCAL_PATH)/ORB_SLAM2/include

#flags for the c++ compiler
LOCAL_CPPFLAGS := -std=c++11 -pthread -frtti -fexceptions -ftemplate-backtrace-limit=0
LOCAL_CPPFLAGS += -D__cplusplus=201103L

LOCAL_PATH:=$(MAIN_DIR)

#end the build
include $(BUILD_SHARED_LIBRARY)
############################################################

##############ORB_SLAM2 JNI FILE執行模块###############################
include $(CLEAR_VARS)
MAIN_DIR:=$(LOCAL_PATH)
OPENCV_LIB_TYPE:=STATIC
ifeq ("$(wildcard $(OPENCV_MK_PATH))","")  
#try to load OpenCV.mk from default install location  
include /home/nodog/Downloads/OpenCV-2.4.9-android-sdk/sdk/native/jni/OpenCV.mk
else  
include $(OPENCV_MK_PATH)  
endif 
LOCAL_MODULE := ORB_SLAM2_EXECUTOR
LOCAL_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/eigen3
LOCAL_C_INCLUDES+=orb_slam2_android_nativefunc_OrbNdkHelper.h

LOCAL_SRC_FILES+=orb_slam2_android_nativefunc_OrbNdkHelper.cpp

LOCAL_SHARED_LIBRARIES+=ORB_SLAM2
LOCAL_SHARED_LIBRARIES+=g2o

LOCAL_LDLIBS += -llog -landroid -lEGL -lGLESv1_CM  

LOCAL_CPPFLAGS := -std=c++11 -pthread -frtti -fexceptions -ftemplate-backtrace-limit=0
LOCAL_CPPFLAGS += -D__cplusplus=201103L
LOCAL_PATH:=$(MAIN_DIR)
include $(BUILD_SHARED_LIBRARY)
############################################################

##############TENSORFLOW - PREBUILT###############################
include $(CLEAR_VARS)
MAIN_DIR:=$(LOCAL_PATH)
OPENCV_LIB_TYPE:=STATIC
ifeq ("$(wildcard $(OPENCV_MK_PATH))","")
#try to load OpenCV.mk from default install location
include /home/nodog/Downloads/OpenCV-2.4.9-android-sdk/sdk/native/jni/OpenCV.mk
else
include $(OPENCV_MK_PATH)
endif

LOCAL_MODULE := Tensorflow
LOCAL_SRC_FILES := ../jniLibs/armeabi-v7a/libtensorflowlite_jni.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/ORB_SLAM2/include
include $(PREBUILT_SHARED_LIBRARY)


#include $(CLEAR_VARS)
#MAIN_DIR:=$(LOCAL_PATH)
#OPENCV_LIB_TYPE:=STATIC
#ifeq ("$(wildcard $(OPENCV_MK_PATH))","")
#try to load OpenCV.mk from default install location
#include /home/nodog/Downloads/OpenCV-2.4.9-android-sdk/sdk/native/jni/OpenCV.mk
#else
#include $(OPENCV_MK_PATH)
#endif
#LOCAL_MODULE := tensorflowlite_jni
#LOCAL_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/eigen3
#LOCAL_C_INCLUDES+=$(LOCAL_PATH)/ORB_SLAM2/include
#LOCAL_C_INCLUDES+=orb_slam2_android_nativefunc_OrbNdkHelper.h

#LOCAL_SRC_FILES+=orb_slam2_android_nativefunc_OrbNdkHelper.cpp
#LOCAL_SHARED_LIBRARIES+=ORB_SLAM2
#LOCAL_SHARED_LIBRARIES+=g2o

#LOCAL_LDLIBS += -llog -landroid -lEGL -lGLESv1_CM

#LOCAL_CPPFLAGS := -std=c++11 -pthread -frtti -fexceptions -ftemplate-backtrace-limit=0
#LOCAL_CPPFLAGS += -D__cplusplus=201103L
#LOCAL_PATH:=$(MAIN_DIR)
#include $(BUILD_SHARED_LIBRARY)
############################################################

##############TENSORFLOW GPU - PREBUILT###############################
include $(CLEAR_VARS)
MAIN_DIR:=$(LOCAL_PATH)
OPENCV_LIB_TYPE:=STATIC
ifeq ("$(wildcard $(OPENCV_MK_PATH))","")
#try to load OpenCV.mk from default install location
include /home/nodog/Downloads/OpenCV-2.4.9-android-sdk/sdk/native/jni/OpenCV.mk
else
include $(OPENCV_MK_PATH)
endif

LOCAL_MODULE := Tensorflow-gpu
LOCAL_SRC_FILES := ../jniLibs/armeabi-v7a/libtensorflowlite_gpu_jni.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/ORB_SLAM2/include
include $(PREBUILT_SHARED_LIBRARY)


#include $(CLEAR_VARS)
#MAIN_DIR:=$(LOCAL_PATH)
#OPENCV_LIB_TYPE:=STATIC
#ifeq ("$(wildcard $(OPENCV_MK_PATH))","")
#try to load OpenCV.mk from default install location
#include /home/nodog/Downloads/OpenCV-2.4.9-android-sdk/sdk/native/jni/OpenCV.mk
#else
#include $(OPENCV_MK_PATH)
#endif
#LOCAL_MODULE := tensorflowlite_jni
#LOCAL_C_INCLUDES+=$(LOCAL_PATH)/Thirdparty/eigen3
#LOCAL_C_INCLUDES+=$(LOCAL_PATH)/ORB_SLAM2/include
#LOCAL_C_INCLUDES+=orb_slam2_android_nativefunc_OrbNdkHelper.h

#LOCAL_SRC_FILES+=orb_slam2_android_nativefunc_OrbNdkHelper.cpp
#LOCAL_SHARED_LIBRARIES+=ORB_SLAM2
#LOCAL_SHARED_LIBRARIES+=g2o

#LOCAL_LDLIBS += -llog -landroid -lEGL -lGLESv1_CM

#LOCAL_CPPFLAGS := -std=c++11 -pthread -frtti -fexceptions -ftemplate-backtrace-limit=0
#LOCAL_CPPFLAGS += -D__cplusplus=201103L
#LOCAL_PATH:=$(MAIN_DIR)
#include $(BUILD_SHARED_LIBRARY)
############################################################
