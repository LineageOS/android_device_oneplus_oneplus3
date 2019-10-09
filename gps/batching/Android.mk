ifneq ($(BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE),)
ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libbatching
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := \
    libutils \
    libcutils \
    liblog \
    libloc_core \
    libgps.utils \
    libdl

LOCAL_SRC_FILES += \
    location_batching.cpp \
    BatchingAdapter.cpp

LOCAL_HEADER_LIBRARIES := \
    libgps.utils_headers \
    libloc_core_headers \
    libloc_pla_headers \
    liblocation_api_headers

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS += $(GNSS_CFLAGS)
include $(BUILD_SHARED_LIBRARY)

endif # not BUILD_TINY_ANDROID
endif # BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE
