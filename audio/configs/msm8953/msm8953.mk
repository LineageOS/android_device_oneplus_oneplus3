##AUDIO_FEATURE_FLAGS
#BOARD_USES_GENERIC_AUDIO := true
BOARD_USES_ALSA_AUDIO := true
USE_CUSTOM_AUDIO_POLICY := 1
USE_XML_AUDIO_POLICY_CONF := 1
BOARD_SUPPORTS_SOUND_TRIGGER := true
AUDIO_USE_LL_AS_PRIMARY_OUTPUT := true
AUDIO_FEATURE_ENABLED_HIFI_AUDIO := true
AUDIO_FEATURE_ENABLED_VBAT_MONITOR := true
#AUDIO_FEATURE_ENABLED_VOICE_CONCURRENCY := true
AUDIO_FEATURE_ENABLED_AAC_ADTS_OFFLOAD := true
AUDIO_FEATURE_ENABLED_NT_PAUSE_TIMEOUT := true
AUDIO_FEATURE_ENABLED_ANC_HEADSET := true
AUDIO_FEATURE_ENABLED_COMPRESS_CAPTURE := false
AUDIO_FEATURE_ENABLED_COMPRESS_VOIP := true
AUDIO_FEATURE_ENABLED_CUSTOMSTEREO := true
AUDIO_FEATURE_ENABLED_EXTN_FORMATS := true
AUDIO_FEATURE_ENABLED_EXTN_FLAC_DECODER := true
AUDIO_FEATURE_ENABLED_EXTN_RESAMPLER := true
AUDIO_FEATURE_ENABLED_FM_POWER_OPT := true
AUDIO_FEATURE_ENABLED_FLUENCE := true
AUDIO_FEATURE_ENABLED_HDMI_SPK := true
AUDIO_FEATURE_ENABLED_HDMI_EDID := true
AUDIO_FEATURE_ENABLED_EXT_HDMI := true
AUDIO_FEATURE_ENABLED_HFP := true
AUDIO_FEATURE_ENABLED_INCALL_MUSIC := false
AUDIO_FEATURE_ENABLED_MULTI_VOICE_SESSIONS := true
AUDIO_FEATURE_ENABLED_PCM_OFFLOAD := true
AUDIO_FEATURE_ENABLED_PCM_OFFLOAD_24 := true
AUDIO_FEATURE_ENABLED_FLAC_OFFLOAD := true
AUDIO_FEATURE_ENABLED_VORBIS_OFFLOAD := true
AUDIO_FEATURE_ENABLED_WMA_OFFLOAD := true
AUDIO_FEATURE_ENABLED_ALAC_OFFLOAD := true
AUDIO_FEATURE_ENABLED_APE_OFFLOAD := true
AUDIO_FEATURE_ENABLED_PROXY_DEVICE := true
AUDIO_FEATURE_ENABLED_KPI_OPTIMIZE := true
AUDIO_FEATURE_ENABLED_SPKR_PROTECTION := true
AUDIO_FEATURE_ENABLED_SSR := true
AUDIO_FEATURE_ENABLED_ACDB_LICENSE := true
AUDIO_FEATURE_ENABLED_DTS_EAGLE := false
BOARD_USES_SRS_TRUEMEDIA := true
DTS_CODEC_M_ := true
#AUDIO_FEATURE_ENABLED_MULTIPLE_TUNNEL := true
AUDIO_FEATURE_ENABLED_DEV_ARBI := true
MM_AUDIO_ENABLED_FTM := true
MM_AUDIO_ENABLED_SAFX := true
TARGET_USES_QCOM_MM_AUDIO := true
AUDIO_FEATURE_ENABLED_HW_ACCELERATED_EFFECTS := false
#AUDIO_FEATURE_ENABLED_DS2_DOLBY_DAP := true
AUDIO_FEATURE_ENABLED_SOURCE_TRACKING := true
AUDIO_FEATURE_ENABLED_AUDIOSPHERE := true

##AUDIO_FEATURE_FLAGS

#Audio Specific device overlays
DEVICE_PACKAGE_OVERLAYS += hardware/qcom/audio/configs/common/overlay

## Audio configuration file

PRODUCT_COPY_FILES += \
hardware/qcom/audio/configs/msm8953/audio_policy.conf:system/etc/audio_policy.conf \
hardware/qcom/audio/configs/msm8953/audio_output_policy.conf:system/vendor/etc/audio_output_policy.conf \
hardware/qcom/audio/configs/msm8953/audio_effects.conf:system/vendor/etc/audio_effects.conf \
hardware/qcom/audio/configs/msm8953/mixer_paths_mtp.xml:system/etc/mixer_paths_mtp.xml \
hardware/qcom/audio/configs/msm8953/mixer_paths_qrd_skuh.xml:system/etc/mixer_paths_qrd_skuh.xml \
hardware/qcom/audio/configs/msm8953/mixer_paths_qrd_skui.xml:system/etc/mixer_paths_qrd_skui.xml \
hardware/qcom/audio/configs/msm8953/mixer_paths_qrd_skuhf.xml:system/etc/mixer_paths_qrd_skuhf.xml \
hardware/qcom/audio/configs/msm8953/mixer_paths_qrd_skum.xml:system/etc/mixer_paths_qrd_skum.xml \
hardware/qcom/audio/configs/msm8953/mixer_paths_qrd_sku3.xml:system/etc/mixer_paths_qrd_sku3.xml \
hardware/qcom/audio/configs/msm8953/mixer_paths_wcd9330.xml:system/etc/mixer_paths_wcd9330.xml \
hardware/qcom/audio/configs/msm8953/mixer_paths_wcd9306.xml:system/etc/mixer_paths_wcd9306.xml \
hardware/qcom/audio/configs/msm8953/mixer_paths_wcd9335.xml:system/etc/mixer_paths_wcd9335.xml \
hardware/qcom/audio/configs/msm8953/mixer_paths_wcd9326.xml:system/etc/mixer_paths_wcd9326.xml \
hardware/qcom/audio/configs/msm8953/mixer_paths_qrd_skun.xml:system/etc/mixer_paths_qrd_skun.xml \
hardware/qcom/audio/configs/msm8953/mixer_paths_skuk.xml:system/etc/mixer_paths_skuk.xml \
hardware/qcom/audio/configs/msm8953/mixer_paths.xml:system/etc/mixer_paths.xml \
hardware/qcom/audio/configs/msm8953/sound_trigger_mixer_paths.xml:system/etc/sound_trigger_mixer_paths.xml \
hardware/qcom/audio/configs/msm8953/sound_trigger_mixer_paths_wcd9306.xml:system/etc/sound_trigger_mixer_paths_wcd9306.xml \
hardware/qcom/audio/configs/msm8953/sound_trigger_mixer_paths_wcd9330.xml:system/etc/sound_trigger_mixer_paths_wcd9330.xml \
hardware/qcom/audio/configs/msm8953/sound_trigger_mixer_paths_wcd9335.xml:system/etc/sound_trigger_mixer_paths_wcd9335.xml \
hardware/qcom/audio/configs/msm8953/sound_trigger_platform_info.xml:system/etc/sound_trigger_platform_info.xml \
hardware/qcom/audio/configs/msm8953/audio_platform_info.xml:system/etc/audio_platform_info.xml \
hardware/qcom/audio/configs/msm8953/audio_platform_info_extcodec.xml:system/etc/audio_platform_info_extcodec.xml \
hardware/qcom/audio/configs/msm8953/aanc_tuning_mixer.txt:system/etc/aanc_tuning_mixer.txt

#XML Audio configuration files
ifeq ($(USE_XML_AUDIO_POLICY_CONF), 1)
ifeq ($(TARGET_USES_AOSP), true)
PRODUCT_COPY_FILES += \
    $(TOPDIR)hardware/qcom/audio/configs/common/audio_policy_configuration.xml:/system/etc/audio_policy_configuration.xml
else
PRODUCT_COPY_FILES += \
    $(TOPDIR)hardware/qcom/audio/configs/msm8953/audio_policy_configuration.xml:system/etc/audio_policy_configuration.xml
endif
PRODUCT_COPY_FILES += \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/a2dp_audio_policy_configuration.xml:/system/etc/a2dp_audio_policy_configuration.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:/system/etc/audio_policy_volumes.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/default_volume_tables.xml:/system/etc/default_volume_tables.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:/system/etc/r_submix_audio_policy_configuration.xml \
    $(TOPDIR)frameworks/av/services/audiopolicy/config/usb_audio_policy_configuration.xml:/system/etc/usb_audio_policy_configuration.xml
endif

# Reduce client buffer size for fast audio output tracks
PRODUCT_PROPERTY_OVERRIDES += \
af.fast_track_multiplier=1

#Low latency audio buffer size in frames
PRODUCT_PROPERTY_OVERRIDES += \
audio_hal.period_size=192

##fluencetype can be "fluence" or "fluencepro" or "none"
PRODUCT_PROPERTY_OVERRIDES += \
ro.qc.sdk.audio.fluencetype=none\
persist.audio.fluence.voicecall=true\
persist.audio.fluence.voicerec=false\
persist.audio.fluence.speaker=true

#disable tunnel encoding
PRODUCT_PROPERTY_OVERRIDES += \
tunnel.audio.encode=false

#Buffer size in kbytes for compress offload playback
PRODUCT_PROPERTY_OVERRIDES += \
audio.offload.buffer.size.kb=64

#Minimum duration for offload playback in secs
PRODUCT_PROPERTY_OVERRIDES += \
audio.offload.min.duration.secs=30

#Enable offload audio video playback by default
PRODUCT_PROPERTY_OVERRIDES += \
audio.offload.video=true

#Enable 16 bit PCM offload by default
PRODUCT_PROPERTY_OVERRIDES += \
audio.offload.pcm.16bit.enable=true

#Enable 24 bit PCM offload by default
PRODUCT_PROPERTY_OVERRIDES += \
audio.offload.pcm.24bit.enable=true

#Enable audio track offload by default
PRODUCT_PROPERTY_OVERRIDES += \
audio.offload.track.enable=true

#Enable music through deep buffer
PRODUCT_PROPERTY_OVERRIDES += \
audio.deep_buffer.media=true

#Default pcm audio sink buffer size in msec. This is used in calculating framecount
PRODUCT_PROPERTY_OVERRIDES += \
media.stagefright.audio.sink=280

#enable voice path for PCM VoIP by default
PRODUCT_PROPERTY_OVERRIDES += \
use.voice.path.for.pcm.voip=true

#Enable multi channel aac through offload
PRODUCT_PROPERTY_OVERRIDES += \
audio.offload.multiaac.enable=true

#Enable DS2, Hardbypass feature for Dolby
PRODUCT_PROPERTY_OVERRIDES += \
audio.dolby.ds2.enabled=true\
audio.dolby.ds2.hardbypass=true

#Disable Multiple offload sesison
PRODUCT_PROPERTY_OVERRIDES += \
audio.offload.multiple.enabled=false

#Disable Compress passthrough playback
PRODUCT_PROPERTY_OVERRIDES += \
audio.offload.passthrough=false

#Disable surround sound recording
PRODUCT_PROPERTY_OVERRIDES += \
ro.qc.sdk.audio.ssr=false

#enable dsp gapless mode by default
PRODUCT_PROPERTY_OVERRIDES += \
audio.offload.gapless.enabled=true

#enable pbe effects
PRODUCT_PROPERTY_OVERRIDES += \
audio.safx.pbe.enabled=true

#parser input buffer size(256kb) in byte stream mode
PRODUCT_PROPERTY_OVERRIDES += \
audio.parser.ip.buffer.size=262144

#enable downsampling for multi-channel content > 48Khz
PRODUCT_PROPERTY_OVERRIDES += \
audio.playback.mch.downsample=true

#enable software decoders for ALAC and APE.
PRODUCT_PROPERTY_OVERRIDES += \
use.qti.sw.alac.decoder=true\
use.qti.sw.ape.decoder=true

#property for AudioSphere Post processing
PRODUCT_PROPERTY_OVERRIDES += \
audio.pp.asphere.enabled=false

#Audio voice concurrency related flags
PRODUCT_PROPERTY_OVERRIDES += \
voice.playback.conc.disabled=true\
voice.record.conc.disabled=false\
voice.voip.conc.disabled=true

#Decides the audio fallback path during voice call,
#deep-buffer and fast are the two allowed fallback paths now.
PRODUCT_PROPERTY_OVERRIDES += \
voice.conc.fallbackpath=deep-buffer

#Disable speaker protection by default
PRODUCT_PROPERTY_OVERRIDES += \
persist.speaker.prot.enable=false

#Enable HW AAC Encoder by default
PRODUCT_PROPERTY_OVERRIDES += \
qcom.hw.aac.encoder=true

#flac sw decoder 24 bit decode capability
PRODUCT_PROPERTY_OVERRIDES += \
flac.sw.decoder.24bit.support=true
