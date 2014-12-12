LOCAL_PATH := $(call my-dir)/..

include $(CLEAR_VARS)

LOCAL_MODULE := ZgeSL
LOCAL_LDLIBS += -lOpenSLES -llog -landroid
LOCAL_CFLAGS := -Wall
LOCAL_CFLAGS += -O2 -std=c++11
#LOCAL_CFLAGS += -Os
#LOCAL_CFLAGS += -DLOGGING

LOCAL_SRC_FILES := src/ZgeSL.cpp

include $(BUILD_SHARED_LIBRARY)