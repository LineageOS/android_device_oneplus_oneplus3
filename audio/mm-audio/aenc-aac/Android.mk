ifneq ($(filter arm aarch64 arm64, $(TARGET_ARCH)),)


AENC_AAC_PATH:= $(call my-dir)

include $(AENC_AAC_PATH)/qdsp6/Android.mk

endif
