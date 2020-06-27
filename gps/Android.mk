ifneq ($(BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE),)
LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/build/target_specific_features.mk

include $(call all-makefiles-under,$(LOCAL_PATH))
endif
