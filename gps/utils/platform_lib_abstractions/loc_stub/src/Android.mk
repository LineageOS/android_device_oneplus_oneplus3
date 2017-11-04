ifneq ($(BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE),)
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

LOCAL_MODULE := libloc_stub
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false
LOCAL_CFLAGS += $(GNSS_CFLAGS)
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libloc_stub_headers
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include
include $(BUILD_HEADER_LIBRARY)

endif # not BUILD_TINY_ANDROID
endif # BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE
