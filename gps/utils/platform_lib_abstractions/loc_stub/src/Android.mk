ifneq ($(BUILD_TINY_ANDROID),true)
#Compile this library only for builds with the latest modem image

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

## Libs
LOCAL_SHARED_LIBRARIES := \
    libutils \
    libcutils \
    liblog

LOCAL_SRC_FILES += \
        loc_stub_android_runtime.cpp \
        loc_stub_gettid.cpp \
        loc_stub_property_service.cpp \
        loc_stub_sched_policy.cpp \
        loc_stub_time.cpp

LOCAL_CFLAGS += \
     -fno-short-enums \
     -D_ANDROID_ \
     -std=c++11


LOCAL_LDFLAGS += -Wl,--export-dynamic

## Includes
LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/../include \


LOCAL_COPY_HEADERS_TO:= libloc_stub/
LOCAL_COPY_HEADERS:= \
        ../include/loc_stub_android_runtime.h \
        ../include/loc_stub_gettid.h \
        ../include/loc_stub_property_service.h \
        ../include/loc_stub_sched_policy.h \
        ../include/loc_stub_time.h

LOCAL_MODULE := libloc_stub

LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
endif # not BUILD_TINY_ANDROID
