ifneq ($(USE_LEGACY_AUDIO_POLICY), 1)
ifeq ($(USE_CUSTOM_AUDIO_POLICY), 1)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := AudioPolicyManager.cpp

LOCAL_C_INCLUDES := $(TOPDIR)frameworks/av/services \
                    $(TOPDIR)frameworks/av/services/audioflinger \
                    $(call include-path-for, audio-effects) \
                    $(call include-path-for, audio-utils) \
                    $(TOPDIR)frameworks/av/services/audiopolicy/common/include \
                    $(TOPDIR)frameworks/av/services/audiopolicy/engine/interface \
                    $(TOPDIR)frameworks/av/services/audiopolicy \
                    $(TOPDIR)frameworks/av/services/audiopolicy/common/managerdefinitions/include \
                    $(call include-path-for, avextension)


LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    liblog \
    libsoundtrigger \
    libaudiopolicymanagerdefault \
    libserviceutility

LOCAL_STATIC_LIBRARIES := \
    libmedia_helper \

LOCAL_CFLAGS += -Wall -Werror
LOCAL_CLANG_CFLAGS += -Wno-overloaded-virtual -Wno-unused-variable -Wno-unused-private-field 

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_VOICE_CONCURRENCY)),true)
LOCAL_CFLAGS += -DVOICE_CONCURRENCY
endif

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_RECORD_PLAY_CONCURRENCY)),true)
LOCAL_CFLAGS += -DRECORD_PLAY_CONCURRENCY
endif

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_PCM_OFFLOAD)),true)
    LOCAL_CFLAGS += -DPCM_OFFLOAD_ENABLED
endif

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_PCM_OFFLOAD_24)),true)
       LOCAL_CFLAGS += -DPCM_OFFLOAD_ENABLED_24
endif

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_EXTN_FORMATS)),true)
    LOCAL_CFLAGS += -DAUDIO_EXTN_FORMATS_ENABLED
endif

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_AAC_ADTS_OFFLOAD)),true)
    LOCAL_CFLAGS += -DAAC_ADTS_OFFLOAD_ENABLED
endif

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_HDMI_SPK)),true)
    LOCAL_CFLAGS += -DAUDIO_EXTN_HDMI_SPK_ENABLED
endif

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_PROXY_DEVICE)),true)
    LOCAL_CFLAGS += -DAUDIO_EXTN_AFE_PROXY_ENABLED
endif

ifeq ($(strip $(AUDIO_FEATURE_ENABLED_FM_POWER_OPT)),true)
LOCAL_CFLAGS += -DFM_POWER_OPT
endif

ifeq ($(USE_XML_AUDIO_POLICY_CONF), 1)
LOCAL_CFLAGS += -DUSE_XML_AUDIO_POLICY_CONF
endif

LOCAL_MODULE := libaudiopolicymanager

include $(BUILD_SHARED_LIBRARY)

endif
endif
