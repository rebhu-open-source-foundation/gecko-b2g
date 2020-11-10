LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libhwchaltest

LOCAL_SRC_FILES := \
    ../android_10/ComposerHal.cpp \
    ../android_10/HWC2.cpp

LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/../android_10

HWCHAL_SHARED_LIBRARIES := \
    android.hardware.graphics.allocator@2.0 \
    android.hardware.graphics.allocator@3.0 \
    android.hardware.graphics.composer@2.1 \
    android.hardware.graphics.composer@2.2 \
    android.hardware.graphics.composer@2.3 \
    android.hardware.configstore@1.0 \
    android.hardware.configstore-utils \
    android.hardware.power@1.0 \
    libbase \
    libbinder \
    liblog \
    libcutils \
    libdl \
    libEGL \
    libfmq \
    libGLESv1_CM \
    libGLESv2 \
    libgui \
    libhardware \
    libhidlbase \
    libhidltransport \
    libhwbinder \
    libpowermanager \
    libprotobuf-cpp-lite \
    libsuspend \
    libsync \
    libui \
    libutils \
    libnativewindow

LOCAL_SHARED_LIBRARIES := $(HWCHAL_SHARED_LIBRARIES)

LOCAL_EXPORT_SHARED_LIBRARY_HEADERS := $(HWCHAL_SHARED_LIBRARIES)

LOCAL_HEADER_LIBRARIES := \
    android.hardware.graphics.composer@2.1-command-buffer \
    android.hardware.graphics.composer@2.2-command-buffer \
    android.hardware.graphics.composer@2.3-command-buffer
    
LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES += \
    system/core/libsuspend/include \
    frameworks/native/libs/ui/include \

LOCAL_CFLAGS := -Wno-unused-parameter

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := testHwc2

LOCAL_SRC_FILES := \
    Hwc2TestBuffer.cpp \
    Hwc2TestClientTarget.cpp \
    Hwc2TestLayer.cpp \
    Hwc2TestLayers.cpp \
    Hwc2TestProperties.cpp \
    Hwc2Test.cpp

LOCAL_SHARED_LIBRARIES := \
    libhwchaltest \
    libui \
    liblog \
    libgui \
    libutils \
    libEGL \
    libGLESv1_CM \
    libGLESv2

LOCAL_HEADER_LIBRARIES := \
    android.hardware.graphics.composer@2.1-command-buffer \
    android.hardware.graphics.composer@2.2-command-buffer \
    android.hardware.graphics.composer@2.3-command-buffer

LOCAL_MODULE_TAGS := tests

LOCAL_CFLAGS := -Wno-unused-parameter -Wno-unused-result

include $(BUILD_NATIVE_TEST)
