ifneq ($(filter arm aarch64 arm64, $(TARGET_ARCH)),)


AENC_AMR_PATH:= $(call my-dir)

include $(AENC_AMR_PATH)/qdsp6/Android.mk

endif
