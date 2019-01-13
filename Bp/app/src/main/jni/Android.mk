LOCAL_PATH:=$(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE:=JniBp
LOCAL_SRC_FILES:=JniBp.c\
                 prosody_gen.c


include $(BUILD_SHARED_LIBRARY)