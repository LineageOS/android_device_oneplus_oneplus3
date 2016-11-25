
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_PROXY_DEVICE)),true)
    LOCAL_CFLAGS += -DAFE_PROXY_ENABLED
endif

LOCAL_SRC_FILES:= \
	bundle.c \
	equalizer.c \
	bass_boost.c \
	virtualizer.c \
	reverb.c \
	effect_api.c \
	effect_util.c

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_HW_ACCELERATED_EFFECTS)),true)
    LOCAL_CFLAGS += -DHW_ACCELERATED_EFFECTS
    LOCAL_SRC_FILES += hw_accelerator.c
endif

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_AUDIOSPHERE)),true)
    LOCAL_CFLAGS += -DAUDIOSPHERE_ENABLED
    LOCAL_SRC_FILES += asphere.c
endif

LOCAL_CFLAGS+= -O2 -fvisibility=hidden

ifneq ($(strip $(AUDIO_FEATURE_DISABLED_DTS_EAGLE)),true)
    LOCAL_CFLAGS += -DDTS_EAGLE
endif

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog \
	libtinyalsa \
	libdl

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_RELATIVE_PATH := soundfx
LOCAL_MODULE:= libqcompostprocbundle

LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr

LOCAL_C_INCLUDES := \
	external/tinyalsa/include \
        $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include \
	$(call include-path-for, audio-effects)

include $(BUILD_SHARED_LIBRARY)


ifeq ($(strip $(AUDIO_FEATURE_ENABLED_HW_ACCELERATED_EFFECTS)),true)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := EffectsHwAcc.cpp

LOCAL_C_INCLUDES := \
    $(call include-path-for, audio-effects)

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libeffects

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS += -O2 -fvisibility=hidden

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_DTS_EAGLE)), true)
LOCAL_CFLAGS += -DHW_ACC_HPX
endif

LOCAL_MODULE:= libhwacceffectswrapper

include $(BUILD_STATIC_LIBRARY)
endif


################################################################################

ifneq ($(filter msm8992 msm8994 msm8996 msmcobalt msmfalcon,$(TARGET_BOARD_PLATFORM)),)

include $(CLEAR_VARS)

LOCAL_CFLAGS := -DLIB_AUDIO_HAL="/system/lib/hw/audio.primary."$(TARGET_BOARD_PLATFORM)".so"

LOCAL_SRC_FILES:= \
        volume_listener.c

LOCAL_CFLAGS+= -O2 -fvisibility=hidden

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog \
        libdl

LOCAL_MODULE_RELATIVE_PATH := soundfx
LOCAL_MODULE:= libvolumelistener

LOCAL_C_INCLUDES := \
        $(call include-path-for, audio-effects)

include $(BUILD_SHARED_LIBRARY)

endif
