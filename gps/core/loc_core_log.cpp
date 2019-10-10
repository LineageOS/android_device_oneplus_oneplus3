/* Copyright (c) 2011-2015, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_core_log"

#include <log_util.h>
#include <loc_log.h>
#include <loc_core_log.h>
#include <loc_pla.h>

void LocPosMode::logv() const
{
    LOC_LOGV ("Position mode: %s\n  Position recurrence: %s\n  "
              "min interval: %d\n  preferred accuracy: %d\n  "
              "preferred time: %d\n  credentials: %s  provider: %s \n  "
              "power mode: %d\n  tbm %d",
              loc_get_position_mode_name(mode),
              loc_get_position_recurrence_name(recurrence),
              min_interval,
              preferred_accuracy,
              preferred_time,
              credentials,
              provider,
              powerMode,
              timeBetweenMeasurements);
}

/* GPS status names */
static const loc_name_val_s_type gps_status_name[] =
{
    NAME_VAL( LOC_GPS_STATUS_NONE ),
    NAME_VAL( LOC_GPS_STATUS_SESSION_BEGIN ),
    NAME_VAL( LOC_GPS_STATUS_SESSION_END ),
    NAME_VAL( LOC_GPS_STATUS_ENGINE_ON ),
    NAME_VAL( LOC_GPS_STATUS_ENGINE_OFF ),
};
static const int gps_status_num = sizeof(gps_status_name) / sizeof(loc_name_val_s_type);

/* Find Android GPS status name */
const char* loc_get_gps_status_name(LocGpsStatusValue gps_status)
{
   return loc_get_name_from_val(gps_status_name, gps_status_num,
         (long) gps_status);
}



static const loc_name_val_s_type loc_eng_position_modes[] =
{
    NAME_VAL( LOC_POSITION_MODE_STANDALONE ),
    NAME_VAL( LOC_POSITION_MODE_MS_BASED ),
    NAME_VAL( LOC_POSITION_MODE_MS_ASSISTED ),
    NAME_VAL( LOC_POSITION_MODE_RESERVED_1 ),
    NAME_VAL( LOC_POSITION_MODE_RESERVED_2 ),
    NAME_VAL( LOC_POSITION_MODE_RESERVED_3 ),
    NAME_VAL( LOC_POSITION_MODE_RESERVED_4 ),
    NAME_VAL( LOC_POSITION_MODE_RESERVED_5 )
};
static const int loc_eng_position_mode_num = sizeof(loc_eng_position_modes) / sizeof(loc_name_val_s_type);

const char* loc_get_position_mode_name(LocGpsPositionMode mode)
{
    return loc_get_name_from_val(loc_eng_position_modes, loc_eng_position_mode_num, (long) mode);
}



static const loc_name_val_s_type loc_eng_position_recurrences[] =
{
    NAME_VAL( LOC_GPS_POSITION_RECURRENCE_PERIODIC ),
    NAME_VAL( LOC_GPS_POSITION_RECURRENCE_SINGLE )
};
static const int loc_eng_position_recurrence_num = sizeof(loc_eng_position_recurrences) / sizeof(loc_name_val_s_type);

const char* loc_get_position_recurrence_name(LocGpsPositionRecurrence recur)
{
    return loc_get_name_from_val(loc_eng_position_recurrences, loc_eng_position_recurrence_num, (long) recur);
}

const char* loc_get_aiding_data_mask_names(LocGpsAidingData /*data*/)
{
    return NULL;
}


static const loc_name_val_s_type loc_eng_agps_types[] =
{
    NAME_VAL( LOC_AGPS_TYPE_INVALID ),
    NAME_VAL( LOC_AGPS_TYPE_ANY ),
    NAME_VAL( LOC_AGPS_TYPE_SUPL ),
    NAME_VAL( LOC_AGPS_TYPE_C2K ),
    NAME_VAL( LOC_AGPS_TYPE_WWAN_ANY )
};
static const int loc_eng_agps_type_num = sizeof(loc_eng_agps_types) / sizeof(loc_name_val_s_type);

const char* loc_get_agps_type_name(LocAGpsType type)
{
    return loc_get_name_from_val(loc_eng_agps_types, loc_eng_agps_type_num, (long) type);
}


static const loc_name_val_s_type loc_eng_ni_types[] =
{
    NAME_VAL( LOC_GPS_NI_TYPE_VOICE ),
    NAME_VAL( LOC_GPS_NI_TYPE_UMTS_SUPL ),
    NAME_VAL( LOC_GPS_NI_TYPE_UMTS_CTRL_PLANE ),
    NAME_VAL( LOC_GPS_NI_TYPE_EMERGENCY_SUPL )
};
static const int loc_eng_ni_type_num = sizeof(loc_eng_ni_types) / sizeof(loc_name_val_s_type);

const char* loc_get_ni_type_name(LocGpsNiType type)
{
    return loc_get_name_from_val(loc_eng_ni_types, loc_eng_ni_type_num, (long) type);
}


static const loc_name_val_s_type loc_eng_ni_responses[] =
{
    NAME_VAL( LOC_GPS_NI_RESPONSE_ACCEPT ),
    NAME_VAL( LOC_GPS_NI_RESPONSE_DENY ),
    NAME_VAL( LOC_GPS_NI_RESPONSE_DENY )
};
static const int loc_eng_ni_reponse_num = sizeof(loc_eng_ni_responses) / sizeof(loc_name_val_s_type);

const char* loc_get_ni_response_name(LocGpsUserResponseType response)
{
    return loc_get_name_from_val(loc_eng_ni_responses, loc_eng_ni_reponse_num, (long) response);
}


static const loc_name_val_s_type loc_eng_ni_encodings[] =
{
    NAME_VAL( LOC_GPS_ENC_NONE ),
    NAME_VAL( LOC_GPS_ENC_SUPL_GSM_DEFAULT ),
    NAME_VAL( LOC_GPS_ENC_SUPL_UTF8 ),
    NAME_VAL( LOC_GPS_ENC_SUPL_UCS2 ),
    NAME_VAL( LOC_GPS_ENC_UNKNOWN )
};
static const int loc_eng_ni_encoding_num = sizeof(loc_eng_ni_encodings) / sizeof(loc_name_val_s_type);

const char* loc_get_ni_encoding_name(LocGpsNiEncodingType encoding)
{
    return loc_get_name_from_val(loc_eng_ni_encodings, loc_eng_ni_encoding_num, (long) encoding);
}

static const loc_name_val_s_type loc_eng_agps_bears[] =
{
    NAME_VAL( AGPS_APN_BEARER_INVALID ),
    NAME_VAL( AGPS_APN_BEARER_IPV4 ),
    NAME_VAL( AGPS_APN_BEARER_IPV6 ),
    NAME_VAL( AGPS_APN_BEARER_IPV4V6 )
};
static const int loc_eng_agps_bears_num = sizeof(loc_eng_agps_bears) / sizeof(loc_name_val_s_type);

const char* loc_get_agps_bear_name(AGpsBearerType bearer)
{
    return loc_get_name_from_val(loc_eng_agps_bears, loc_eng_agps_bears_num, (long) bearer);
}

static const loc_name_val_s_type loc_eng_server_types[] =
{
    NAME_VAL( LOC_AGPS_CDMA_PDE_SERVER ),
    NAME_VAL( LOC_AGPS_CUSTOM_PDE_SERVER ),
    NAME_VAL( LOC_AGPS_MPC_SERVER ),
    NAME_VAL( LOC_AGPS_SUPL_SERVER )
};
static const int loc_eng_server_types_num = sizeof(loc_eng_server_types) / sizeof(loc_name_val_s_type);

const char* loc_get_server_type_name(LocServerType type)
{
    return loc_get_name_from_val(loc_eng_server_types, loc_eng_server_types_num, (long) type);
}

static const loc_name_val_s_type loc_eng_position_sess_status_types[] =
{
    NAME_VAL( LOC_SESS_SUCCESS ),
    NAME_VAL( LOC_SESS_INTERMEDIATE ),
    NAME_VAL( LOC_SESS_FAILURE )
};
static const int loc_eng_position_sess_status_num = sizeof(loc_eng_position_sess_status_types) / sizeof(loc_name_val_s_type);

const char* loc_get_position_sess_status_name(enum loc_sess_status status)
{
    return loc_get_name_from_val(loc_eng_position_sess_status_types, loc_eng_position_sess_status_num, (long) status);
}

static const loc_name_val_s_type loc_eng_agps_status_names[] =
{
    NAME_VAL( LOC_GPS_REQUEST_AGPS_DATA_CONN ),
    NAME_VAL( LOC_GPS_RELEASE_AGPS_DATA_CONN ),
    NAME_VAL( LOC_GPS_AGPS_DATA_CONNECTED ),
    NAME_VAL( LOC_GPS_AGPS_DATA_CONN_DONE ),
    NAME_VAL( LOC_GPS_AGPS_DATA_CONN_FAILED )
};
static const int loc_eng_agps_status_num = sizeof(loc_eng_agps_status_names) / sizeof(loc_name_val_s_type);

const char* loc_get_agps_status_name(LocAGpsStatusValue status)
{
    return loc_get_name_from_val(loc_eng_agps_status_names, loc_eng_agps_status_num, (long) status);
}
