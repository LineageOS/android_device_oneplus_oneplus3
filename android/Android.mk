LOCAL_PATH := $(call my-dir)
ifneq ($(BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE),)
include $(CLEAR_VARS)
DIR_LIST := $(LOCAL_PATH)
include $(DIR_LIST)/utils/Android.mk
ifeq ($(GNSS_HIDL_VERSION),2.0)
include $(DIR_LIST)/2.0/Android.mk
else
ifeq ($(GNSS_HIDL_VERSION),1.1)
include $(DIR_LIST)/1.1/Android.mk
else
include $(DIR_LIST)/1.0/Android.mk
endif #GNSS HIDL 1.1
endif #GNSS HIDL 2.0
endif #BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE
