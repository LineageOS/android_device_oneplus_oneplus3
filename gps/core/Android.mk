ifneq ($(BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE),)
ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libloc_core
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
LOCAL_MODULE_TAGS := optional

ifeq ($(TARGET_DEVICE),apq8026_lw)
LOCAL_CFLAGS += -DPDK_FEATURE_SET
else ifeq ($(BOARD_VENDOR_QCOM_LOC_PDK_FEATURE_SET),true)
LOCAL_CFLAGS += -DPDK_FEATURE_SET
endif

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libutils \
    libcutils \
    libgps.utils \
    libdl \
    liblog \
    libloc_pla

LOCAL_SRC_FILES += \
    LocApiBase.cpp \
    LocAdapterBase.cpp \
    ContextBase.cpp \
    LocDualContext.cpp \
    loc_core_log.cpp \
    data-items/DataItemsFactoryProxy.cpp \
    data-items/common/ClientIndex.cpp \
    data-items/common/DataItemIndex.cpp \
    data-items/common/IndexFactory.cpp \
    SystemStatusOsObserver.cpp \
    SystemStatus.cpp

LOCAL_CFLAGS += \
     -fno-short-enums \
     -D_ANDROID_

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/data-items \
    $(LOCAL_PATH)/data-items/common \
    $(LOCAL_PATH)/observer \

LOCAL_HEADER_LIBRARIES := \
    libgps.utils_headers \
    libloc_pla_headers \
    liblocation_api_headers

LOCAL_CFLAGS += $(GNSS_CFLAGS)

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libloc_core_headers
LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/data-items \
    $(LOCAL_PATH)/data-items/common \
    $(LOCAL_PATH)/observer
include $(BUILD_HEADER_LIBRARY)

endif # not BUILD_TINY_ANDROID
endif # BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE
