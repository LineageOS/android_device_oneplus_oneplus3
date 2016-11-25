ifneq ($(filter arm aarch64 arm64, $(TARGET_ARCH)),)

AENC_QCELP13_PATH:= $(call my-dir)

include $(AENC_QCELP13_PATH)/qdsp6/Android.mk

endif
