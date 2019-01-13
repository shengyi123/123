LOCAL_PATH:=$(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE:=JniLib
LOCAL_SRC_FILES:=JniLib.c\
                 HTS_engine1.c\
                 HTS_gstream.c\
                 HTS_label.c\
                 HTS_misc.c\
                 HTS_model.c\
                 HTS_orthogonal_expansion.c\
                 HTS_pstream.c\
                 HTS_sstream.c\
                 HTS_vocoder.c\
                 HTS_Xlabel.c\
                 HTS_audio.c\
                 oec.c
LOCAL_LDLIBS :=-llog

include $(BUILD_SHARED_LIBRARY)