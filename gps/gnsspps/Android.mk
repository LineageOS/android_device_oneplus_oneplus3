LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libgnsspps

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libcutils \
    libgps.utils \
    liblog

LOCAL_SRC_FILES += \
   gnsspps.c

LOCAL_CFLAGS += \
    -fno-short-enums \
    -D_ANDROID_

LOCAL_COPY_HEADERS_TO:= libgnsspps/

LOCAL_COPY_HEADERS:= \
    gnsspps.h

## Includes
LOCAL_C_INCLUDES := \
    $(TARGET_OUT_HEADERS)/gps.utils \
    $(TARGET_OUT_HEADERS)/libloc_pla

include $(BUILD_SHARED_LIBRARY)
