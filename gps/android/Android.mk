LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.gnss@1.0-impl-qti
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
    AGnss.cpp \
    Gnss.cpp \
    GnssBatching.cpp \
    GnssGeofencing.cpp \
    GnssMeasurement.cpp \
    GnssNi.cpp \
    GnssConfiguration.cpp \
    GnssDebug.cpp \
    AGnssRil.cpp

LOCAL_SRC_FILES += \
    location_api/LocationUtil.cpp \
    location_api/GnssAPIClient.cpp \
    location_api/GeofenceAPIClient.cpp \
    location_api/BatchingAPIClient.cpp \
    location_api/MeasurementAPIClient.cpp \

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/location_api
LOCAL_HEADER_LIBRARIES := \
    libgps.utils_headers \
    libloc_core_headers \
    libloc_pla_headers \
    liblocation_api_headers

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libhidlbase \
    libhidltransport \
    libhwbinder \
    libutils \
    android.hardware.gnss@1.0 \

LOCAL_SHARED_LIBRARIES += \
    libloc_core \
    libgps.utils \
    libdl \
    libloc_pla \
    liblocation_api \

LOCAL_CFLAGS += $(GNSS_CFLAGS)
include $(BUILD_SHARED_LIBRARY)

BUILD_GNSS_HIDL_SERVICE := true
ifneq ($(BOARD_VENDOR_QCOM_LOC_PDK_FEATURE_SET), true)
ifneq ($(LW_FEATURE_SET),true)
BUILD_GNSS_HIDL_SERVICE := false
endif # LW_FEATURE_SET
endif # BOARD_VENDOR_QCOM_LOC_PDK_FEATURE_SET

ifeq ($(BUILD_GNSS_HIDL_SERVICE), true)
include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.gnss@1.0-service-qti
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_EXECUTABLES)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_INIT_RC := android.hardware.gnss@1.0-service-qti.rc
LOCAL_SRC_FILES := \
    service.cpp \

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/location_api
LOCAL_HEADER_LIBRARIES := \
    libgps.utils_headers \
    libloc_core_headers \
    libloc_pla_headers \
    liblocation_api_headers


LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libdl \
    libbase \
    libutils \

LOCAL_SHARED_LIBRARIES += \
    libhwbinder \
    libhidlbase \
    libhidltransport \
    android.hardware.gnss@1.0 \

LOCAL_CFLAGS += $(GNSS_CFLAGS)
include $(BUILD_EXECUTABLE)
endif # BUILD_GNSS_HIDL_SERVICE
