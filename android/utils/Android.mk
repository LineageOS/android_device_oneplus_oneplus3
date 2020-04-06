LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := liblocbatterylistener
LOCAL_SANITIZE += $(GNSS_SANITIZE)
# activate the following line for debug purposes only, comment out for production
#LOCAL_SANITIZE_DIAG += $(GNSS_SANITIZE_DIAG)
LOCAL_VENDOR_MODULE := true

LOCAL_CFLAGS += $(GNSS_CFLAGS)

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH) \

LOCAL_SRC_FILES:= \
    battery_listener.cpp
LOCAL_SHARED_LIBRARIES := \
    liblog \
    libhidlbase \
    libhidltransport \
    libhwbinder \
    libcutils \
    libutils \
    android.hardware.health@1.0 \
    android.hardware.health@2.0 \
    android.hardware.power@1.2 \
    libbase

LOCAL_STATIC_LIBRARIES := libhealthhalutils
LOCAL_CFLAGS += -DBATTERY_LISTENER_ENABLED

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := liblocbatterylistener_headers
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

include $(BUILD_HEADER_LIBRARY)


