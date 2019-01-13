LOCAL_PATH := $(call my-dir)
 
include $(CLEAR_VARS)
 
LOCAL_MODULE := JniAdd
LOCAL_SRC_FILES =: JniAdd.cpp shengyi.cpp jhlai.cpp
LOCAL_LDLIBS :=-llog


include $(BUILD_SHARED_LIBRARY)
