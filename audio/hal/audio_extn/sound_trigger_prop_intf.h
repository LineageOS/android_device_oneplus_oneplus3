/*
 * Extrapolated / reversed header for Sound Trigger
 */

#ifndef SOUND_TRIGGER_PROP_INTF_H
#define SOUND_TRIGGER_PROP_INTF_H

struct sound_trigger_session_info {
    int capture_handle;
    void* pcm;
    struct pcm_config config;
};

enum audio_event_type {
    AUDIO_EVENT_CAPTURE_DEVICE_INACTIVE,
    AUDIO_EVENT_CAPTURE_DEVICE_ACTIVE,
    AUDIO_EVENT_PLAYBACK_STREAM_INACTIVE,
    AUDIO_EVENT_PLAYBACK_STREAM_ACTIVE,
    AUDIO_EVENT_STOP_LAB,
    AUDIO_EVENT_SSR,
    AUDIO_EVENT_NUM_ST_SESSIONS,
    AUDIO_EVENT_READ_SAMPLES,
};

enum sound_trigger_event_type {
    ST_EVENT_SESSION_REGISTER,
    ST_EVENT_SESSION_DEREGISTER
};
typedef enum sound_trigger_event_type sound_trigger_event_type_t;

enum ssr_event_status {
    SND_CARD_STATUS_OFFLINE,
    SND_CARD_STATUS_ONLINE,
    CPE_STATUS_OFFLINE,
    CPE_STATUS_ONLINE
};

struct sound_trigger_event_info {
    struct sound_trigger_session_info st_ses;
};
typedef struct sound_trigger_event_info sound_trigger_event_info_t;

struct audio_read_samples_info {
    struct sound_trigger_session_info *ses_info;
    void *buf;
    size_t num_bytes;
};

struct audio_event_info {
    union {
        enum ssr_event_status status;
        int value;
        struct sound_trigger_session_info ses_info;
        struct audio_read_samples_info aud_info;
    }u;
};
typedef struct audio_event_info audio_event_info_t;

typedef int (*sound_trigger_hw_call_back_t)(enum audio_event_type,
                                            struct audio_event_info*);
#endif
