/*
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
 */

#define LOG_TAG "voice_processing"
/*#define LOG_NDEBUG 0*/
#include <stdlib.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <cutils/log.h>
#include <cutils/list.h>
#include <hardware/audio_effect.h>
#include <audio_effects/effect_aec.h>
#include <audio_effects/effect_agc.h>
#include <audio_effects/effect_ns.h>


//------------------------------------------------------------------------------
// local definitions
//------------------------------------------------------------------------------

#define EFFECTS_DESCRIPTOR_LIBRARY_PATH "/system/lib/soundfx/libqcomvoiceprocessingdescriptors.so"

// types of pre processing modules
enum effect_id
{
    AEC_ID,        // Acoustic Echo Canceler
    NS_ID,         // Noise Suppressor
//ENABLE_AGC    AGC_ID,        // Automatic Gain Control
    NUM_ID
};

// Session state
enum session_state {
    SESSION_STATE_INIT,        // initialized
    SESSION_STATE_CONFIG       // configuration received
};

// Effect/Preprocessor state
enum effect_state {
    EFFECT_STATE_INIT,         // initialized
    EFFECT_STATE_CREATED,      // webRTC engine created
    EFFECT_STATE_CONFIG,       // configuration received/disabled
    EFFECT_STATE_ACTIVE        // active/enabled
};

// Effect context
struct effect_s {
    const struct effect_interface_s *itfe;
    uint32_t id;                // type of pre processor (enum effect_id)
    uint32_t state;             // current state (enum effect_state)
    struct session_s *session;  // session the effect is on
};

// Session context
struct session_s {
    struct listnode node;
    effect_config_t config;
    struct effect_s effects[NUM_ID]; // effects in this session
    uint32_t state;                  // current state (enum session_state)
    int id;                          // audio session ID
    int io;                          // handle of input stream this session is on
    uint32_t created_msk;            // bit field containing IDs of crested pre processors
    uint32_t enabled_msk;            // bit field containing IDs of enabled pre processors
    uint32_t processed_msk;          // bit field containing IDs of pre processors already
};


//------------------------------------------------------------------------------
// Default Effect descriptors. Device specific descriptors should be defined in
// libqcomvoiceprocessing.<product_name>.so if needed.
//------------------------------------------------------------------------------

// UUIDs for effect types have been generated from http://www.itu.int/ITU-T/asn1/uuid.html
// as the pre processing effects are not defined by OpenSL ES

// Acoustic Echo Cancellation
static const effect_descriptor_t qcom_default_aec_descriptor = {
        { 0x7b491460, 0x8d4d, 0x11e0, 0xbd61, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } }, // type
        { 0x0f8d0d2a, 0x59e5, 0x45fe, 0xb6e4, { 0x24, 0x8c, 0x8a, 0x79, 0x91, 0x09 } }, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_PRE_PROC|EFFECT_FLAG_DEVICE_IND),
        0,
        0,
        "Acoustic Echo Canceler",
        "Qualcomm Fluence"
};

// Noise suppression
static const effect_descriptor_t qcom_default_ns_descriptor = {
        { 0x58b4b260, 0x8e06, 0x11e0, 0xaa8e, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } }, // type
        { 0x1d97bb0b, 0x9e2f, 0x4403, 0x9ae3, { 0x58, 0xc2, 0x55, 0x43, 0x06, 0xf8 } }, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_PRE_PROC|EFFECT_FLAG_DEVICE_IND),
        0,
        0,
        "Noise Suppression",
        "Qualcomm Fluence"
};

//ENABLE_AGC
// Automatic Gain Control
//static const effect_descriptor_t qcom_default_agc_descriptor = {
//        { 0x0a8abfe0, 0x654c, 0x11e0, 0xba26, { 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b } }, // type
//        { 0x0dd49521, 0x8c59, 0x40b1, 0xb403, { 0xe0, 0x8d, 0x5f, 0x01, 0x87, 0x5e } }, // uuid
//        EFFECT_CONTROL_API_VERSION,
//        (EFFECT_FLAG_TYPE_PRE_PROC|EFFECT_FLAG_DEVICE_IND),
//        0,
//        0,
//        "Automatic Gain Control",
//        "Qualcomm Fluence"
//};

const effect_descriptor_t *descriptors[NUM_ID] = {
        &qcom_default_aec_descriptor,
        &qcom_default_ns_descriptor,
//ENABLE_AGC       &qcom_default_agc_descriptor,
};


static int init_status = 1;
struct listnode session_list;
static const struct effect_interface_s effect_interface;
static const effect_uuid_t * uuid_to_id_table[NUM_ID];

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------

static const effect_uuid_t * id_to_uuid(int id)
{
    if (id >= NUM_ID)
        return EFFECT_UUID_NULL;

    return uuid_to_id_table[id];
}

static uint32_t uuid_to_id(const effect_uuid_t * uuid)
{
    size_t i;
    for (i = 0; i < NUM_ID; i++)
        if (memcmp(uuid, uuid_to_id_table[i], sizeof(*uuid)) == 0)
            break;

    return i;
}

//------------------------------------------------------------------------------
// Effect functions
//------------------------------------------------------------------------------

static void session_set_fx_enabled(struct session_s *session, uint32_t id, bool enabled);

#define BAD_STATE_ABORT(from, to) \
        LOG_ALWAYS_FATAL("Bad state transition from %d to %d", from, to);

static int effect_set_state(struct effect_s *effect, uint32_t state)
{
    int status = 0;
    ALOGV("effect_set_state() id %d, new %d old %d", effect->id, state, effect->state);
    switch(state) {
    case EFFECT_STATE_INIT:
        switch(effect->state) {
        case EFFECT_STATE_ACTIVE:
            session_set_fx_enabled(effect->session, effect->id, false);
        case EFFECT_STATE_CONFIG:
        case EFFECT_STATE_CREATED:
        case EFFECT_STATE_INIT:
            break;
        default:
            BAD_STATE_ABORT(effect->state, state);
        }
        break;
    case EFFECT_STATE_CREATED:
        switch(effect->state) {
        case EFFECT_STATE_INIT:
            break;
        case EFFECT_STATE_CREATED:
        case EFFECT_STATE_ACTIVE:
        case EFFECT_STATE_CONFIG:
            ALOGE("effect_set_state() invalid transition");
            status = -ENOSYS;
            break;
        default:
            BAD_STATE_ABORT(effect->state, state);
        }
        break;
    case EFFECT_STATE_CONFIG:
        switch(effect->state) {
        case EFFECT_STATE_INIT:
            ALOGE("effect_set_state() invalid transition");
            status = -ENOSYS;
            break;
        case EFFECT_STATE_ACTIVE:
            session_set_fx_enabled(effect->session, effect->id, false);
            break;
        case EFFECT_STATE_CREATED:
        case EFFECT_STATE_CONFIG:
            break;
        default:
            BAD_STATE_ABORT(effect->state, state);
        }
        break;
    case EFFECT_STATE_ACTIVE:
        switch(effect->state) {
        case EFFECT_STATE_INIT:
        case EFFECT_STATE_CREATED:
            ALOGE("effect_set_state() invalid transition");
            status = -ENOSYS;
            break;
        case EFFECT_STATE_ACTIVE:
            // enabling an already enabled effect is just ignored
            break;
        case EFFECT_STATE_CONFIG:
            session_set_fx_enabled(effect->session, effect->id, true);
            break;
        default:
            BAD_STATE_ABORT(effect->state, state);
        }
        break;
    default:
        BAD_STATE_ABORT(effect->state, state);
    }

    if (status == 0)
        effect->state = state;

    return status;
}

static int effect_init(struct effect_s *effect, uint32_t id)
{
    effect->itfe = &effect_interface;
    effect->id = id;
    effect->state = EFFECT_STATE_INIT;
    return 0;
}

static int effect_create(struct effect_s *effect,
               struct session_s *session,
               effect_handle_t  *interface)
{
    effect->session = session;
    *interface = (effect_handle_t)&effect->itfe;
    return effect_set_state(effect, EFFECT_STATE_CREATED);
}

static int effect_release(struct effect_s *effect)
{
    return effect_set_state(effect, EFFECT_STATE_INIT);
}


//------------------------------------------------------------------------------
// Session functions
//------------------------------------------------------------------------------

static int session_init(struct session_s *session)
{
    size_t i;
    int status = 0;

    session->state = SESSION_STATE_INIT;
    session->id = 0;
    session->io = 0;
    session->created_msk = 0;
    for (i = 0; i < NUM_ID && status == 0; i++)
        status = effect_init(&session->effects[i], i);

    return status;
}


static int session_create_effect(struct session_s *session,
                                 int32_t id,
                                 effect_handle_t  *interface)
{
    int status = -ENOMEM;

    ALOGV("session_create_effect() %s, created_msk %08x",
          id == AEC_ID ? "AEC" : id == NS_ID ? "NS" : "?", session->created_msk);

    if (session->created_msk == 0) {
        session->config.inputCfg.samplingRate = 16000;
        session->config.inputCfg.channels = AUDIO_CHANNEL_IN_MONO;
        session->config.inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
        session->config.outputCfg.samplingRate = 16000;
        session->config.outputCfg.channels = AUDIO_CHANNEL_IN_MONO;
        session->config.outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
        session->enabled_msk = 0;
        session->processed_msk = 0;
    }
    status = effect_create(&session->effects[id], session, interface);
    if (status < 0)
        goto error;

    ALOGV("session_create_effect() OK");
    session->created_msk |= (1<<id);
    return status;

error:
    return status;
}

static int session_release_effect(struct session_s *session,
                                  struct effect_s *fx)
{
    ALOGW_IF(effect_release(fx) != 0, " session_release_effect() failed for id %d", fx->id);

    session->created_msk &= ~(1<<fx->id);
    if (session->created_msk == 0)
    {
        ALOGV("session_release_effect() last effect: removing session");
        list_remove(&session->node);
        free(session);
    }

    return 0;
}


static int session_set_config(struct session_s *session, effect_config_t *config)
{
    int status;

    if (config->inputCfg.samplingRate != config->outputCfg.samplingRate ||
            config->inputCfg.format != config->outputCfg.format ||
            config->inputCfg.format != AUDIO_FORMAT_PCM_16_BIT)
        return -EINVAL;

    ALOGV("session_set_config() sampling rate %d channels %08x",
         config->inputCfg.samplingRate, config->inputCfg.channels);

    // if at least one process is enabled, do not accept configuration changes
    if (session->enabled_msk) {
        if (session->config.inputCfg.samplingRate != config->inputCfg.samplingRate ||
                session->config.inputCfg.channels != config->inputCfg.channels ||
                session->config.outputCfg.channels != config->outputCfg.channels)
            return -ENOSYS;
        else
            return 0;
    }

    memcpy(&session->config, config, sizeof(effect_config_t));

    session->state = SESSION_STATE_CONFIG;
    return 0;
}

static void session_get_config(struct session_s *session, effect_config_t *config)
{
    memcpy(config, &session->config, sizeof(effect_config_t));

    config->inputCfg.mask = config->outputCfg.mask =
            (EFFECT_CONFIG_SMP_RATE | EFFECT_CONFIG_CHANNELS | EFFECT_CONFIG_FORMAT);
}


static void session_set_fx_enabled(struct session_s *session, uint32_t id, bool enabled)
{
    if (enabled) {
        if(session->enabled_msk == 0) {
            /* do first enable here */
        }
        session->enabled_msk |= (1 << id);
    } else {
        session->enabled_msk &= ~(1 << id);
        if(session->enabled_msk == 0) {
            /* do last enable here */
        }
    }
    ALOGV("session_set_fx_enabled() id %d, enabled %d enabled_msk %08x",
         id, enabled, session->enabled_msk);
    session->processed_msk = 0;
}

//------------------------------------------------------------------------------
// Global functions
//------------------------------------------------------------------------------

static struct session_s *get_session(int32_t id, int32_t  sessionId, int32_t  ioId)
{
    size_t i;
    int free = -1;
    struct listnode *node;
    struct session_s *session;

    list_for_each(node, &session_list) {
        session = node_to_item(node, struct session_s, node);
        if (session->io == ioId) {
            if (session->created_msk & (1 << id)) {
                ALOGV("get_session() effect %d already created", id);
                return NULL;
            }
            ALOGV("get_session() found session %p", session);
            return session;
        }
    }

    session = (struct session_s *)calloc(1, sizeof(struct session_s));
    if (session == NULL) {
        ALOGE("get_session() fail to allocate memory");
        return NULL;
    }
    session_init(session);
    session->id = sessionId;
    session->io = ioId;
    list_add_tail(&session_list, &session->node);

    ALOGV("get_session() created session %p", session);

    return session;
}

static int init() {
    void *lib_handle;
    const effect_descriptor_t *desc;

    if (init_status <= 0)
        return init_status;

    if (access(EFFECTS_DESCRIPTOR_LIBRARY_PATH, R_OK) == 0) {
        lib_handle = dlopen(EFFECTS_DESCRIPTOR_LIBRARY_PATH, RTLD_NOW);
        if (lib_handle == NULL) {
            ALOGE("%s: DLOPEN failed for %s", __func__, EFFECTS_DESCRIPTOR_LIBRARY_PATH);
        } else {
            ALOGV("%s: DLOPEN successful for %s", __func__, EFFECTS_DESCRIPTOR_LIBRARY_PATH);
            desc = (const effect_descriptor_t *)dlsym(lib_handle,
                                                        "qcom_product_aec_descriptor");
            if (desc)
                descriptors[AEC_ID] = desc;

            desc = (const effect_descriptor_t *)dlsym(lib_handle,
                                                        "qcom_product_ns_descriptor");
            if (desc)
                descriptors[NS_ID] = desc;

//ENABLE_AGC
//            desc = (const effect_descriptor_t *)dlsym(lib_handle,
//                                                        "qcom_product_agc_descriptor");
//            if (desc)
//                descriptors[AGC_ID] = desc;
        }
    }

    uuid_to_id_table[AEC_ID] = FX_IID_AEC;
    uuid_to_id_table[NS_ID] = FX_IID_NS;
//ENABLE_AGC uuid_to_id_table[AGC_ID] = FX_IID_AGC;

    list_init(&session_list);

    init_status = 0;
    return init_status;
}

static const effect_descriptor_t *get_descriptor(const effect_uuid_t *uuid)
{
    size_t i;
    for (i = 0; i < NUM_ID; i++)
        if (memcmp(&descriptors[i]->uuid, uuid, sizeof(effect_uuid_t)) == 0)
            return descriptors[i];

    return NULL;
}


//------------------------------------------------------------------------------
// Effect Control Interface Implementation
//------------------------------------------------------------------------------

static int fx_process(effect_handle_t     self,
                            audio_buffer_t    *inBuffer,
                            audio_buffer_t    *outBuffer)
{
    struct effect_s *effect = (struct effect_s *)self;
    struct session_s *session;

    if (effect == NULL) {
        ALOGV("fx_process() ERROR effect == NULL");
        return -EINVAL;
    }

    if (inBuffer == NULL  || inBuffer->raw == NULL  ||
            outBuffer == NULL || outBuffer->raw == NULL) {
        ALOGW("fx_process() ERROR bad pointer");
        return -EINVAL;
    }

    session = (struct session_s *)effect->session;

    session->processed_msk |= (1<<effect->id);

    if ((session->processed_msk & session->enabled_msk) == session->enabled_msk) {
        effect->session->processed_msk = 0;
        return 0;
    } else
        return -ENODATA;
}

static int fx_command(effect_handle_t  self,
                            uint32_t            cmdCode,
                            uint32_t            cmdSize,
                            void                *pCmdData,
                            uint32_t            *replySize,
                            void                *pReplyData)
{
    struct effect_s *effect = (struct effect_s *)self;

    if (effect == NULL)
        return -EINVAL;

    //ALOGV("fx_command: command %d cmdSize %d",cmdCode, cmdSize);

    switch (cmdCode) {
        case EFFECT_CMD_INIT:
            if (pReplyData == NULL || *replySize != sizeof(int))
                return -EINVAL;

            *(int *)pReplyData = 0;
            break;

        case EFFECT_CMD_SET_CONFIG: {
            if (pCmdData    == NULL||
                    cmdSize     != sizeof(effect_config_t)||
                    pReplyData  == NULL||
                    *replySize  != sizeof(int)) {
                ALOGV("fx_command() EFFECT_CMD_SET_CONFIG invalid args");
                return -EINVAL;
            }
            *(int *)pReplyData = session_set_config(effect->session, (effect_config_t *)pCmdData);
            if (*(int *)pReplyData != 0)
                break;

            if (effect->state != EFFECT_STATE_ACTIVE)
                *(int *)pReplyData = effect_set_state(effect, EFFECT_STATE_CONFIG);

        } break;

        case EFFECT_CMD_GET_CONFIG:
            if (pReplyData == NULL ||
                    *replySize != sizeof(effect_config_t)) {
                ALOGV("fx_command() EFFECT_CMD_GET_CONFIG invalid args");
                return -EINVAL;
            }

            session_get_config(effect->session, (effect_config_t *)pReplyData);
            break;

        case EFFECT_CMD_RESET:
            break;

        case EFFECT_CMD_GET_PARAM: {
            if (pCmdData == NULL ||
                    cmdSize < (int)sizeof(effect_param_t) ||
                    pReplyData == NULL ||
                    *replySize < (int)sizeof(effect_param_t) ||
                    // constrain memcpy below
                    ((effect_param_t *)pCmdData)->psize > *replySize - sizeof(effect_param_t)) {
                ALOGV("fx_command() EFFECT_CMD_GET_PARAM invalid args");
                return -EINVAL;
            }
            effect_param_t *p = (effect_param_t *)pCmdData;

            memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + p->psize);
            p = (effect_param_t *)pReplyData;
            p->status = -ENOSYS;

        } break;

        case EFFECT_CMD_SET_PARAM: {
            if (pCmdData == NULL||
                    cmdSize < (int)sizeof(effect_param_t) ||
                    pReplyData == NULL ||
                    *replySize != sizeof(int32_t)) {
                ALOGV("fx_command() EFFECT_CMD_SET_PARAM invalid args");
                return -EINVAL;
            }
            effect_param_t *p = (effect_param_t *) pCmdData;

            if (p->psize != sizeof(int32_t)) {
                ALOGV("fx_command() EFFECT_CMD_SET_PARAM invalid param format");
                return -EINVAL;
            }
            *(int *)pReplyData = -ENOSYS;
        } break;

        case EFFECT_CMD_ENABLE:
            if (pReplyData == NULL || *replySize != sizeof(int)) {
                ALOGV("fx_command() EFFECT_CMD_ENABLE invalid args");
                return -EINVAL;
            }
            *(int *)pReplyData = effect_set_state(effect, EFFECT_STATE_ACTIVE);
            break;

        case EFFECT_CMD_DISABLE:
            if (pReplyData == NULL || *replySize != sizeof(int)) {
                ALOGV("fx_command() EFFECT_CMD_DISABLE invalid args");
                return -EINVAL;
            }
            *(int *)pReplyData  = effect_set_state(effect, EFFECT_STATE_CONFIG);
            break;

        case EFFECT_CMD_SET_DEVICE:
        case EFFECT_CMD_SET_INPUT_DEVICE:
        case EFFECT_CMD_SET_VOLUME:
        case EFFECT_CMD_SET_AUDIO_MODE:
            if (pCmdData == NULL ||
                    cmdSize != sizeof(uint32_t)) {
                ALOGV("fx_command() %s invalid args",
                      cmdCode == EFFECT_CMD_SET_DEVICE ? "EFFECT_CMD_SET_DEVICE" :
                      cmdCode == EFFECT_CMD_SET_INPUT_DEVICE ? "EFFECT_CMD_SET_INPUT_DEVICE" :
                      cmdCode == EFFECT_CMD_SET_VOLUME ? "EFFECT_CMD_SET_VOLUME" :
                      cmdCode == EFFECT_CMD_SET_AUDIO_MODE ? "EFFECT_CMD_SET_AUDIO_MODE" :
                       "");
                return -EINVAL;
            }
            ALOGV("fx_command() %s value %08x",
                  cmdCode == EFFECT_CMD_SET_DEVICE ? "EFFECT_CMD_SET_DEVICE" :
                  cmdCode == EFFECT_CMD_SET_INPUT_DEVICE ? "EFFECT_CMD_SET_INPUT_DEVICE" :
                  cmdCode == EFFECT_CMD_SET_VOLUME ? "EFFECT_CMD_SET_VOLUME" :
                  cmdCode == EFFECT_CMD_SET_AUDIO_MODE ? "EFFECT_CMD_SET_AUDIO_MODE":
                  "",
                  *(int *)pCmdData);
            break;

        default:
            return -EINVAL;
    }
    return 0;
}


static int fx_get_descriptor(effect_handle_t   self,
                                  effect_descriptor_t *pDescriptor)
{
    struct effect_s *effect = (struct effect_s *)self;

    if (effect == NULL || pDescriptor == NULL)
        return -EINVAL;

    *pDescriptor = *descriptors[effect->id];

    return 0;
}


// effect_handle_t interface implementation for effect
static const struct effect_interface_s effect_interface = {
    fx_process,
    fx_command,
    fx_get_descriptor,
    NULL
};

//------------------------------------------------------------------------------
// Effect Library Interface Implementation
//------------------------------------------------------------------------------

static int lib_create(const effect_uuid_t *uuid,
                            int32_t             sessionId,
                            int32_t             ioId,
                            effect_handle_t  *pInterface)
{
    ALOGV("lib_create: uuid: %08x session %d IO: %d", uuid->timeLow, sessionId, ioId);

    int status;
    const effect_descriptor_t *desc;
    struct session_s *session;
    uint32_t id;

    if (init() != 0)
        return init_status;

    desc =  get_descriptor(uuid);

    if (desc == NULL) {
        ALOGW("lib_create: fx not found uuid: %08x", uuid->timeLow);
        return -EINVAL;
    }
    id = uuid_to_id(&desc->type);
    if (id >= NUM_ID) {
        ALOGW("lib_create: fx not found type: %08x", desc->type.timeLow);
        return -EINVAL;
    }

    session = get_session(id, sessionId, ioId);

    if (session == NULL) {
        ALOGW("lib_create: no more session available");
        return -EINVAL;
    }

    status = session_create_effect(session, id, pInterface);

    if (status < 0 && session->created_msk == 0) {
        list_remove(&session->node);
        free(session);
    }
    return status;
}

static int lib_release(effect_handle_t interface)
{
    struct listnode *node;
    struct session_s *session;

    ALOGV("lib_release %p", interface);
    if (init() != 0)
        return init_status;

    struct effect_s *fx = (struct effect_s *)interface;

    list_for_each(node, &session_list) {
        session = node_to_item(node, struct session_s, node);
        if (session == fx->session) {
            session_release_effect(fx->session, fx);
            return 0;
        }
    }

    return -EINVAL;
}

static int lib_get_descriptor(const effect_uuid_t *uuid,
                                   effect_descriptor_t *pDescriptor)
{
    const effect_descriptor_t *desc;

    if (pDescriptor == NULL || uuid == NULL)
        return -EINVAL;

    if (init() != 0)
        return init_status;

    desc = get_descriptor(uuid);
    if (desc == NULL) {
        ALOGV("lib_get_descriptor() not found");
        return  -EINVAL;
    }

    ALOGV("lib_get_descriptor() got fx %s", desc->name);

    *pDescriptor = *desc;
    return 0;
}

// This is the only symbol that needs to be exported
__attribute__ ((visibility ("default")))
audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    tag : AUDIO_EFFECT_LIBRARY_TAG,
    version : EFFECT_LIBRARY_API_VERSION,
    name : "MSM8960 Audio Preprocessing Library",
    implementor : "The Android Open Source Project",
    create_effect : lib_create,
    release_effect : lib_release,
    get_descriptor : lib_get_descriptor
};
