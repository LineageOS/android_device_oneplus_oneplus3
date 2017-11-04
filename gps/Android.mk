ifneq ($(BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE),)
LOCAL_PATH := $(call my-dir)

include $(call all-makefiles-under,$(LOCAL_PATH))
endif
