LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := bootable/recovery
LOCAL_SRC_FILES := recovery_updater.c
LOCAL_MODULE := librecovery_updater_op3
LOCAL_MODULE_TAGS := eng
include $(BUILD_STATIC_LIBRARY)
