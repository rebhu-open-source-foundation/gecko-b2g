LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := gfxdbg
LOCAL_SRC_FILES := gfxdbg.cpp

LOCAL_SHARED_LIBRARIES := libbinder libutils

include $(BUILD_EXECUTABLE)


