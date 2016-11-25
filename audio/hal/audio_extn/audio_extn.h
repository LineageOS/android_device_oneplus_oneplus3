/*
 * Copyright (c) 2013-2016, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 *
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file was modified by DTS, Inc. The portions of the
 * code modified by DTS, Inc are copyrighted and
 * licensed separately, as follows:
 *
 * (C) 2014 DTS, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AUDIO_EXTN_H
#define AUDIO_EXTN_H

#include <cutils/str_parms.h>

#ifndef AFE_PROXY_ENABLED
#define AUDIO_DEVICE_OUT_PROXY 0x40000
#endif

#ifndef INCALL_MUSIC_ENABLED
#define AUDIO_OUTPUT_FLAG_INCALL_MUSIC 0x8000
#endif

#ifndef AUDIO_DEVICE_OUT_FM_TX
#define AUDIO_DEVICE_OUT_FM_TX 0x8000000
#endif

#ifndef FLAC_OFFLOAD_ENABLED
#define AUDIO_FORMAT_FLAC 0x1B000000UL
#endif

#ifndef WMA_OFFLOAD_ENABLED
#define AUDIO_FORMAT_WMA 0x12000000UL
#define AUDIO_FORMAT_WMA_PRO 0x13000000UL
#endif

#ifndef ALAC_OFFLOAD_ENABLED
#define AUDIO_FORMAT_ALAC 0x1C000000UL
#endif

#ifndef APE_OFFLOAD_ENABLED
#define AUDIO_FORMAT_APE 0x1D000000UL
#endif

#ifndef AAC_ADTS_OFFLOAD_ENABLED
#define AUDIO_FORMAT_AAC_ADTS 0x1E000000UL
#define AUDIO_FORMAT_AAC_ADTS_LC   (AUDIO_FORMAT_AAC_ADTS |\
                                      AUDIO_FORMAT_AAC_SUB_LC)
#define AUDIO_FORMAT_AAC_ADTS_HE_V1 (AUDIO_FORMAT_AAC_ADTS |\
                                      AUDIO_FORMAT_AAC_SUB_HE_V1)
#define AUDIO_FORMAT_AAC_ADTS_HE_V2  (AUDIO_FORMAT_AAC_ADTS |\
                                      AUDIO_FORMAT_AAC_SUB_HE_V2)
#endif

#ifndef COMPRESS_METADATA_NEEDED
#define audio_extn_parse_compress_metadata(out, parms) (0)
#else
int audio_extn_parse_compress_metadata(struct stream_out *out,
                                       struct str_parms *parms);
#endif

#ifdef AUDIO_EXTN_FORMATS_ENABLED
#define AUDIO_OUTPUT_BIT_WIDTH ((config->offload_info.bit_width == 32) ? 24\
                                   :config->offload_info.bit_width)
#else
#define AUDIO_OUTPUT_BIT_WIDTH (CODEC_BACKEND_DEFAULT_BIT_WIDTH)
#endif

#define MAX_LENGTH_MIXER_CONTROL_IN_INT                  (128)

void audio_extn_set_parameters(struct audio_device *adev,
                               struct str_parms *parms);

void audio_extn_get_parameters(const struct audio_device *adev,
                               struct str_parms *query,
                               struct str_parms *reply);

#ifndef ANC_HEADSET_ENABLED
#define audio_extn_get_anc_enabled()                     (0)
#define audio_extn_should_use_fb_anc()                   (0)
#define audio_extn_should_use_handset_anc(in_channels)   (0)
#else
bool audio_extn_get_anc_enabled(void);
bool audio_extn_should_use_fb_anc(void);
bool audio_extn_should_use_handset_anc(int in_channels);
#endif

#ifndef VBAT_MONITOR_ENABLED
#define audio_extn_is_vbat_enabled()                     (0)
#define audio_extn_can_use_vbat()                        (0)
#else
bool audio_extn_is_vbat_enabled(void);
bool audio_extn_can_use_vbat(void);
#endif

#ifndef HIFI_AUDIO_ENABLED
#define audio_extn_is_hifi_audio_enabled()               (0)
#define audio_extn_is_hifi_audio_supported()             (0)
#else
bool audio_extn_is_hifi_audio_enabled(void);
bool audio_extn_is_hifi_audio_supported(void);
#endif

#ifndef FLUENCE_ENABLED
#define audio_extn_set_fluence_parameters(adev, parms) (0)
#define audio_extn_get_fluence_parameters(adev, query, reply) (0)
#else
void audio_extn_set_fluence_parameters(struct audio_device *adev,
                                           struct str_parms *parms);
int audio_extn_get_fluence_parameters(const struct audio_device *adev,
                  struct str_parms *query, struct str_parms *reply);
#endif

#ifndef AFE_PROXY_ENABLED
#define audio_extn_set_afe_proxy_channel_mixer(adev,channel_count)     (0)
#define audio_extn_read_afe_proxy_channel_masks(out)                   (0)
#define audio_extn_get_afe_proxy_channel_count()                       (0)
#else
int32_t audio_extn_set_afe_proxy_channel_mixer(struct audio_device *adev,
                                                    int channel_count);
int32_t audio_extn_read_afe_proxy_channel_masks(struct stream_out *out);
int32_t audio_extn_get_afe_proxy_channel_count();

#endif

#ifndef USB_HEADSET_ENABLED
#define audio_extn_usb_init(adev)                                      (0)
#define audio_extn_usb_deinit()                                        (0)
#define audio_extn_usb_add_device(device, card)                        (0)
#define audio_extn_usb_remove_device(device, card)                     (0)
#define audio_extn_usb_is_config_supported(bit_width, sample_rate, ch) (0)
#define audio_extn_usb_enable_sidetone(device, enable)                 (0)
#define audio_extn_usb_set_sidetone_gain(parms, value, len)            (0)
#else
void audio_extn_usb_init(void *adev);
void audio_extn_usb_deinit();
void audio_extn_usb_add_device(audio_devices_t device, int card);
void audio_extn_usb_remove_device(audio_devices_t device, int card);
bool audio_extn_usb_is_config_supported(unsigned int *bit_width,
                                        unsigned int *sample_rate,
                                        unsigned int ch);
int audio_extn_usb_enable_sidetone(int device, bool enable);
int audio_extn_usb_set_sidetone_gain(struct str_parms *parms,
                                     char *value, int len);
#endif

#ifndef SPLIT_A2DP_ENABLED
#define audio_extn_a2dp_init(adev)                       (0)
#define audio_extn_a2dp_start_playback()                 (0)
#define audio_extn_a2dp_stop_playback()                  (0)
#define audio_extn_a2dp_set_parameters(parms)            (0)
#define audio_extn_a2dp_is_force_device_switch()         (0)
#else
void audio_extn_a2dp_init(void *adev);
int audio_extn_a2dp_start_playback();
void audio_extn_a2dp_stop_playback();
void audio_extn_a2dp_set_parameters(struct str_parms *parms);
bool audio_extn_a2dp_is_force_device_switch();
#endif

#ifndef SSR_ENABLED
#define audio_extn_ssr_check_and_set_usecase(in)      (0)
#define audio_extn_ssr_init(in, num_out_chan)         (0)
#define audio_extn_ssr_deinit()                       (0)
#define audio_extn_ssr_update_enabled()               (0)
#define audio_extn_ssr_get_enabled()                  (0)
#define audio_extn_ssr_read(stream, buffer, bytes)    (0)
#define audio_extn_ssr_set_parameters(adev, parms)    (0)
#define audio_extn_ssr_get_parameters(adev, parms, reply) (0)
#define audio_extn_ssr_get_stream()                   (0)
#else
int audio_extn_ssr_check_and_set_usecase(struct stream_in *in);
int32_t audio_extn_ssr_init(struct stream_in *in,
                            int num_out_chan);
int32_t audio_extn_ssr_deinit();
void audio_extn_ssr_update_enabled();
bool audio_extn_ssr_get_enabled();
int32_t audio_extn_ssr_read(struct audio_stream_in *stream,
                       void *buffer, size_t bytes);
void audio_extn_ssr_set_parameters(struct audio_device *adev,
                                   struct str_parms *parms);
void audio_extn_ssr_get_parameters(const struct audio_device *adev,
                                   struct str_parms *query,
                                   struct str_parms *reply);
struct stream_in *audio_extn_ssr_get_stream();
#endif

#ifndef HW_VARIANTS_ENABLED
#define hw_info_init(snd_card_name)                      (0)
#define hw_info_deinit(hw_info)                          (0)
#define hw_info_append_hw_type(hw_info,\
        snd_device, device_name)                         (0)
#define hw_info_enable_wsa_combo_usecase_support(hw_info)   (0)

#else
void *hw_info_init(const char *snd_card_name);
void hw_info_deinit(void *hw_info);
void hw_info_append_hw_type(void *hw_info, snd_device_t snd_device,
                             char *device_name);
void hw_info_enable_wsa_combo_usecase_support(void *hw_info);

#endif

#ifndef AUDIO_LISTEN_ENABLED
#define audio_extn_listen_init(adev, snd_card)                  (0)
#define audio_extn_listen_deinit(adev)                          (0)
#define audio_extn_listen_update_device_status(snd_dev, event)  (0)
#define audio_extn_listen_update_stream_status(uc_info, event)  (0)
#define audio_extn_listen_set_parameters(adev, parms)           (0)
#else
enum listen_event_type {
    LISTEN_EVENT_SND_DEVICE_FREE,
    LISTEN_EVENT_SND_DEVICE_BUSY,
    LISTEN_EVENT_STREAM_FREE,
    LISTEN_EVENT_STREAM_BUSY
};
typedef enum listen_event_type listen_event_type_t;

int audio_extn_listen_init(struct audio_device *adev, unsigned int snd_card);
void audio_extn_listen_deinit(struct audio_device *adev);
void audio_extn_listen_update_device_status(snd_device_t snd_device,
                                     listen_event_type_t event);
void audio_extn_listen_update_stream_status(struct audio_usecase *uc_info,
                                     listen_event_type_t event);
void audio_extn_listen_set_parameters(struct audio_device *adev,
                                      struct str_parms *parms);
#endif /* AUDIO_LISTEN_ENABLED */

#ifndef SOUND_TRIGGER_ENABLED
#define audio_extn_sound_trigger_init(adev)                            (0)
#define audio_extn_sound_trigger_deinit(adev)                          (0)
#define audio_extn_sound_trigger_update_device_status(snd_dev, event)  (0)
#define audio_extn_sound_trigger_update_stream_status(uc_info, event)  (0)
#define audio_extn_sound_trigger_set_parameters(adev, parms)           (0)
#define audio_extn_sound_trigger_check_and_get_session(in)             (0)
#define audio_extn_sound_trigger_stop_lab(in)                          (0)
#define audio_extn_sound_trigger_read(in, buffer, bytes)               (0)
#else

enum st_event_type {
    ST_EVENT_SND_DEVICE_FREE,
    ST_EVENT_SND_DEVICE_BUSY,
    ST_EVENT_STREAM_FREE,
    ST_EVENT_STREAM_BUSY
};
typedef enum st_event_type st_event_type_t;

int audio_extn_sound_trigger_init(struct audio_device *adev);
void audio_extn_sound_trigger_deinit(struct audio_device *adev);
void audio_extn_sound_trigger_update_device_status(snd_device_t snd_device,
                                     st_event_type_t event);
void audio_extn_sound_trigger_update_stream_status(struct audio_usecase *uc_info,
                                     st_event_type_t event);
void audio_extn_sound_trigger_set_parameters(struct audio_device *adev,
                                             struct str_parms *parms);
void audio_extn_sound_trigger_check_and_get_session(struct stream_in *in);
void audio_extn_sound_trigger_stop_lab(struct stream_in *in);
int audio_extn_sound_trigger_read(struct stream_in *in, void *buffer,
                                  size_t bytes);
#endif

#ifndef AUXPCM_BT_ENABLED
#define audio_extn_read_xml(adev, mixer_card, MIXER_XML_PATH, \
                            MIXER_XML_PATH_AUXPCM)               (-ENOSYS)
#else
int32_t audio_extn_read_xml(struct audio_device *adev, uint32_t mixer_card,
                            const char* mixer_xml_path,
                            const char* mixer_xml_path_auxpcm);
#endif /* AUXPCM_BT_ENABLED */
#ifndef SPKR_PROT_ENABLED
#define audio_extn_spkr_prot_init(adev)       (0)
#define audio_extn_spkr_prot_start_processing(snd_device)    (-EINVAL)
#define audio_extn_spkr_prot_calib_cancel(adev) (0)
#define audio_extn_spkr_prot_stop_processing(snd_device)     (0)
#define audio_extn_spkr_prot_is_enabled() (false)
#define audio_extn_spkr_prot_set_parameters(parms, value, len)   (0)
#define audio_extn_fbsp_set_parameters(parms)   (0)
#define audio_extn_fbsp_get_parameters(query, reply)   (0)
#else
void audio_extn_spkr_prot_init(void *adev);
int audio_extn_spkr_prot_start_processing(snd_device_t snd_device);
void audio_extn_spkr_prot_stop_processing(snd_device_t snd_device);
bool audio_extn_spkr_prot_is_enabled();
void audio_extn_spkr_prot_calib_cancel(void *adev);
void audio_extn_spkr_prot_set_parameters(struct str_parms *parms,
                                         char *value, int len);
int audio_extn_fbsp_set_parameters(struct str_parms *parms);
int audio_extn_fbsp_get_parameters(struct str_parms *query,
                                   struct str_parms *reply);
#endif

#ifndef COMPRESS_CAPTURE_ENABLED
#define audio_extn_compr_cap_init(in)                     (0)
#define audio_extn_compr_cap_enabled()                    (0)
#define audio_extn_compr_cap_format_supported(format)     (0)
#define audio_extn_compr_cap_usecase_supported(usecase)   (0)
#define audio_extn_compr_cap_get_buffer_size(format)      (0)
#define audio_extn_compr_cap_read(in, buffer, bytes)      (0)
#define audio_extn_compr_cap_deinit()                     (0)
#else
void audio_extn_compr_cap_init(struct stream_in *in);
bool audio_extn_compr_cap_enabled();
bool audio_extn_compr_cap_format_supported(audio_format_t format);
bool audio_extn_compr_cap_usecase_supported(audio_usecase_t usecase);
size_t audio_extn_compr_cap_get_buffer_size(audio_format_t format);
size_t audio_extn_compr_cap_read(struct stream_in *in,
                                        void *buffer, size_t bytes);
void audio_extn_compr_cap_deinit();
#endif

#ifndef DTS_EAGLE
#define audio_extn_dts_eagle_set_parameters(adev, parms)     (0)
#define audio_extn_dts_eagle_get_parameters(adev, query, reply) (0)
#define audio_extn_dts_eagle_fade(adev, fade_in, out) (0)
#define audio_extn_dts_eagle_send_lic()               (0)
#define audio_extn_dts_create_state_notifier_node(stream_out) (0)
#define audio_extn_dts_notify_playback_state(stream_out, has_video, sample_rate, \
                                    channels, is_playing) (0)
#define audio_extn_dts_remove_state_notifier_node(stream_out) (0)
#define audio_extn_check_and_set_dts_hpx_state(adev)       (0)
#else
void audio_extn_dts_eagle_set_parameters(struct audio_device *adev,
                                         struct str_parms *parms);
int audio_extn_dts_eagle_get_parameters(const struct audio_device *adev,
                  struct str_parms *query, struct str_parms *reply);
int audio_extn_dts_eagle_fade(const struct audio_device *adev, bool fade_in, const struct stream_out *out);
void audio_extn_dts_eagle_send_lic();
void audio_extn_dts_create_state_notifier_node(int stream_out);
void audio_extn_dts_notify_playback_state(int stream_out, int has_video, int sample_rate,
                                  int channels, int is_playing);
void audio_extn_dts_remove_state_notifier_node(int stream_out);
void audio_extn_check_and_set_dts_hpx_state(const struct audio_device *adev);
#endif

#if defined(DS1_DOLBY_DDP_ENABLED) || defined(DS1_DOLBY_DAP_ENABLED)
void audio_extn_dolby_set_dmid(struct audio_device *adev);
#else
#define audio_extn_dolby_set_dmid(adev)                 (0)
#define AUDIO_CHANNEL_OUT_PENTA (AUDIO_CHANNEL_OUT_QUAD | AUDIO_CHANNEL_OUT_FRONT_CENTER)
#define AUDIO_CHANNEL_OUT_SURROUND (AUDIO_CHANNEL_OUT_FRONT_LEFT | AUDIO_CHANNEL_OUT_FRONT_RIGHT | \
                                    AUDIO_CHANNEL_OUT_FRONT_CENTER | AUDIO_CHANNEL_OUT_BACK_CENTER)
#endif

#if defined(DS1_DOLBY_DDP_ENABLED) || defined(DS1_DOLBY_DAP_ENABLED) || defined(DS2_DOLBY_DAP_ENABLED)
void audio_extn_dolby_set_license(struct audio_device *adev);
#else
#define audio_extn_dolby_set_license(adev)              (0)
#endif

#ifndef DS1_DOLBY_DAP_ENABLED
#define audio_extn_dolby_set_endpoint(adev)                 (0)
#else
void audio_extn_dolby_set_endpoint(struct audio_device *adev);
#endif


#if defined(DS1_DOLBY_DDP_ENABLED) || defined(DS2_DOLBY_DAP_ENABLED)
bool audio_extn_is_dolby_format(audio_format_t format);
int audio_extn_dolby_get_snd_codec_id(struct audio_device *adev,
                                      struct stream_out *out,
                                      audio_format_t format);
#else
#define audio_extn_is_dolby_format(format)              (0)
#define audio_extn_dolby_get_snd_codec_id(adev, out, format)       (0)
#endif

#ifndef DS1_DOLBY_DDP_ENABLED
#define audio_extn_ddp_set_parameters(adev, parms)      (0)
#define audio_extn_dolby_send_ddp_endp_params(adev)     (0)
#else
void audio_extn_ddp_set_parameters(struct audio_device *adev,
                                   struct str_parms *parms);
void audio_extn_dolby_send_ddp_endp_params(struct audio_device *adev);

#endif

#ifndef HDMI_PASSTHROUGH_ENABLED
#define audio_extn_passthru_update_stream_configuration(adev, out)            (0)
#define audio_extn_passthru_is_convert_supported(adev, out)                   (0)
#define audio_extn_passthru_is_passt_supported(adev, out)                     (0)
#define audio_extn_passthru_is_passthrough_stream(out)                        (0)
#define audio_extn_passthru_get_buffer_size(info)                             (0)
#define audio_extn_passthru_set_volume(out, mute)                             (0)
#define audio_extn_passthru_set_latency(out, latency)                         (0)
#define audio_extn_passthru_is_supported_format(f) (0)
#define audio_extn_passthru_should_drop_data(o) (0)
#define audio_extn_passthru_on_start(o) do {} while(0)
#define audio_extn_passthru_on_stop(o) do {} while(0)
#define audio_extn_passthru_on_pause(o) do {} while(0)
#define audio_extn_passthru_is_enabled() (0)
#define audio_extn_passthru_is_active() (0)
#define audio_extn_passthru_set_parameters(a, p) (-ENOSYS)
#define audio_extn_passthru_init(a) do {} while(0)
#define audio_extn_passthru_should_standby(o) (1)

#define AUDIO_OUTPUT_FLAG_COMPRESS_PASSTHROUGH  0x1000
#else
bool audio_extn_passthru_is_convert_supported(struct audio_device *adev,
                                                 struct stream_out *out);
bool audio_extn_passthru_is_passt_supported(struct audio_device *adev,
                                         struct stream_out *out);
void audio_extn_passthru_update_stream_configuration(struct audio_device *adev,
                                                 struct stream_out *out);
bool audio_extn_passthru_is_passthrough_stream(struct stream_out *out);
int audio_extn_passthru_get_buffer_size(audio_offload_info_t* info);
int audio_extn_passthru_set_volume(struct stream_out *out, int mute);
int audio_extn_passthru_set_latency(struct stream_out *out, int latency);
bool audio_extn_passthru_is_supported_format(audio_format_t format);
bool audio_extn_passthru_should_drop_data(struct stream_out * out);
void audio_extn_passthru_on_start(struct stream_out *out);
void audio_extn_passthru_on_stop(struct stream_out *out);
void audio_extn_passthru_on_pause(struct stream_out *out);
int audio_extn_passthru_set_parameters(struct audio_device *adev,
                                       struct str_parms *parms);
bool audio_extn_passthru_is_enabled();
bool audio_extn_passthru_is_active();
void audio_extn_passthru_init(struct audio_device *adev);
bool audio_extn_passthru_should_standby(struct stream_out *out);
#endif

#ifndef HFP_ENABLED
#define audio_extn_hfp_is_active(adev)                  (0)
#define audio_extn_hfp_get_usecase()                    (-1)
#else
bool audio_extn_hfp_is_active(struct audio_device *adev);
audio_usecase_t audio_extn_hfp_get_usecase();
#endif

#ifndef DEV_ARBI_ENABLED
#define audio_extn_dev_arbi_init()                  (0)
#define audio_extn_dev_arbi_deinit()                (0)
#define audio_extn_dev_arbi_acquire(snd_device)     (0)
#define audio_extn_dev_arbi_release(snd_device)     (0)
#else
int audio_extn_dev_arbi_init();
int audio_extn_dev_arbi_deinit();
int audio_extn_dev_arbi_acquire(snd_device_t snd_device);
int audio_extn_dev_arbi_release(snd_device_t snd_device);
#endif

#ifndef PM_SUPPORT_ENABLED
#define audio_extn_pm_set_parameters(params) (0)
#define audio_extn_pm_vote(void) (0)
#define audio_extn_pm_unvote(void) (0)
#else
void audio_extn_pm_set_parameters(struct str_parms *parms);
int audio_extn_pm_vote (void);
void audio_extn_pm_unvote(void);
#endif

void audio_extn_utils_update_streams_output_cfg_list(void *platform,
                                  struct mixer *mixer,
                                  struct listnode *streams_output_cfg_list);
void audio_extn_utils_dump_streams_output_cfg_list(
                                  struct listnode *streams_output_cfg_list);
void audio_extn_utils_release_streams_output_cfg_list(
                                  struct listnode *streams_output_cfg_list);
void audio_extn_utils_update_stream_app_type_cfg(void *platform,
                                  struct listnode *streams_output_cfg_list,
                                  audio_devices_t devices,
                                  audio_output_flags_t flags,
                                  audio_format_t format,
                                  uint32_t sample_rate,
                                  uint32_t bit_width,
                                  audio_channel_mask_t channel_mask,
                                  struct stream_app_type_cfg *app_type_cfg);
int audio_extn_utils_send_app_type_cfg(struct audio_device *adev,
                                       struct audio_usecase *usecase);
void audio_extn_utils_send_audio_calibration(struct audio_device *adev,
                                             struct audio_usecase *usecase);
#ifdef DS2_DOLBY_DAP_ENABLED
#define LIB_DS2_DAP_HAL "vendor/lib/libhwdaphal.so"
#define SET_HW_INFO_FUNC "dap_hal_set_hw_info"
typedef enum {
    SND_CARD            = 0,
    HW_ENDPOINT         = 1,
    DMID                = 2,
    DEVICE_BE_ID_MAP    = 3,
    DAP_BYPASS          = 4,
} dap_hal_hw_info_t;
typedef int (*dap_hal_set_hw_info_t)(int32_t hw_info, void* data);
typedef struct {
     int (*device_id_to_be_id)[2];
     int len;
} dap_hal_device_be_id_map_t;

int audio_extn_dap_hal_init(int snd_card);
int audio_extn_dap_hal_deinit();
void audio_extn_dolby_ds2_set_endpoint(struct audio_device *adev);
int audio_extn_ds2_enable(struct audio_device *adev);
int audio_extn_dolby_set_dap_bypass(struct audio_device *adev, int state);
void audio_extn_ds2_set_parameters(struct audio_device *adev,
                                   struct str_parms *parms);

#else
#define audio_extn_dap_hal_init(snd_card)                             (0)
#define audio_extn_dap_hal_deinit()                                   (0)
#define audio_extn_dolby_ds2_set_endpoint(adev)                       (0)
#define audio_extn_ds2_enable(adev)                                   (0)
#define audio_extn_dolby_set_dap_bypass(adev, state)                  (0)
#define audio_extn_ds2_set_parameters(adev, parms);                   (0)
#endif
typedef enum {
    DAP_STATE_ON = 0,
    DAP_STATE_BYPASS,
} dap_state;
#ifndef AUDIO_FORMAT_E_AC3_JOC
#define AUDIO_FORMAT_E_AC3_JOC  0x19000000UL
#endif
#ifndef AUDIO_FORMAT_DTS_LBR
#define AUDIO_FORMAT_DTS_LBR 0x1E000000UL
#endif

#define DIV_ROUND_UP(x, y) (((x) + (y) - 1)/(y))
#define ALIGN(x, y) ((y) * DIV_ROUND_UP((x), (y)))

int b64decode(char *inp, int ilen, uint8_t* outp);
int b64encode(uint8_t *inp, int ilen, char* outp);
int read_line_from_file(const char *path, char *buf, size_t count);
int audio_extn_utils_get_codec_version(const char *snd_card_name, int card_num, char *codec_version);
audio_format_t alsa_format_to_hal(uint32_t alsa_format);
uint32_t hal_format_to_alsa(audio_format_t hal_format);
audio_format_t pcm_format_to_hal(uint32_t pcm_format);
uint32_t hal_format_to_pcm(audio_format_t hal_format);

void audio_extn_utils_update_direct_pcm_fragment_size(struct stream_out *out);

#ifndef KPI_OPTIMIZE_ENABLED
#define audio_extn_perf_lock_init() (0)
#define audio_extn_perf_lock_acquire(handle, duration, opts, size) (0)
#define audio_extn_perf_lock_release(handle) (0)
#else
int audio_extn_perf_lock_init(void);
void audio_extn_perf_lock_acquire(int *handle, int duration,
                                 int *opts, int size);
void audio_extn_perf_lock_release(int *handle);

#endif /* KPI_OPTIMIZE_ENABLED */

#ifndef AUDIO_EXTERNAL_HDMI_ENABLED
#define audio_utils_set_hdmi_channel_status(out, buffer, bytes) (0)
#else
void audio_utils_set_hdmi_channel_status(struct stream_out *out, char * buffer, size_t bytes);
#endif

#ifndef KEEP_ALIVE_ENABLED
#define audio_extn_keep_alive_init(a) do {} while(0)
#define audio_extn_keep_alive_start() do {} while(0)
#define audio_extn_keep_alive_stop() do {} while(0)
#define audio_extn_keep_alive_is_active() (false)
#define audio_extn_keep_alive_set_parameters(adev, parms) (0)
#else
void audio_extn_keep_alive_init(struct audio_device *adev);
void audio_extn_keep_alive_start();
void audio_extn_keep_alive_stop();
bool audio_extn_keep_alive_is_active();
int audio_extn_keep_alive_set_parameters(struct audio_device *adev,
                                         struct str_parms *parms);
#endif


#endif /* AUDIO_EXTN_H */
