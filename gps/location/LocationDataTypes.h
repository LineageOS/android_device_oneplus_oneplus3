/* Copyright (c) 2018-2019 The Linux Foundation. All rights reserved.
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
 *     * Neither the name of The Linux Foundation nor the names of its
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
 */

#ifndef LOCATIONDATATYPES_H
#define LOCATIONDATATYPES_H

#include <vector>
#include <stdint.h>
#include <functional>
#include <list>
#include <string.h>

#define GNSS_NI_REQUESTOR_MAX  (256)
#define GNSS_NI_MESSAGE_ID_MAX (2048)
#define GNSS_SV_MAX            (176)
#define GNSS_MEASUREMENTS_MAX  (128)
#define GNSS_UTC_TIME_OFFSET   (3657)

#define GNSS_BUGREPORT_GPS_MIN    (1)
#define GNSS_BUGREPORT_SBAS_MIN   (120)
#define GNSS_BUGREPORT_GLO_MIN    (1)
#define GNSS_BUGREPORT_QZSS_MIN   (193)
#define GNSS_BUGREPORT_BDS_MIN    (1)
#define GNSS_BUGREPORT_GAL_MIN    (1)
#define GNSS_BUGREPORT_NAVIC_MIN  (1)

#define GNSS_MAX_NAME_LENGTH    (8)

typedef enum {
    LOCATION_ERROR_SUCCESS = 0,
    LOCATION_ERROR_GENERAL_FAILURE,
    LOCATION_ERROR_CALLBACK_MISSING,
    LOCATION_ERROR_INVALID_PARAMETER,
    LOCATION_ERROR_ID_EXISTS,
    LOCATION_ERROR_ID_UNKNOWN,
    LOCATION_ERROR_ALREADY_STARTED,
    LOCATION_ERROR_GEOFENCES_AT_MAX,
    LOCATION_ERROR_NOT_SUPPORTED
} LocationError;

// Flags to indicate which values are valid in a Location
typedef uint16_t LocationFlagsMask;
typedef enum {
    LOCATION_HAS_LAT_LONG_BIT          = (1<<0), // location has valid latitude and longitude
    LOCATION_HAS_ALTITUDE_BIT          = (1<<1), // location has valid altitude
    LOCATION_HAS_SPEED_BIT             = (1<<2), // location has valid speed
    LOCATION_HAS_BEARING_BIT           = (1<<3), // location has valid bearing
    LOCATION_HAS_ACCURACY_BIT          = (1<<4), // location has valid accuracy
    LOCATION_HAS_VERTICAL_ACCURACY_BIT = (1<<5), // location has valid vertical accuracy
    LOCATION_HAS_SPEED_ACCURACY_BIT    = (1<<6), // location has valid speed accuracy
    LOCATION_HAS_BEARING_ACCURACY_BIT  = (1<<7), // location has valid bearing accuracy
    LOCATION_HAS_SPOOF_MASK            = (1<<8), // location has valid spoof mask
} LocationFlagsBits;

typedef uint16_t LocationTechnologyMask;
typedef enum {
    LOCATION_TECHNOLOGY_GNSS_BIT     = (1<<0), // location was calculated using GNSS
    LOCATION_TECHNOLOGY_CELL_BIT     = (1<<1), // location was calculated using Cell
    LOCATION_TECHNOLOGY_WIFI_BIT     = (1<<2), // location was calculated using WiFi
    LOCATION_TECHNOLOGY_SENSORS_BIT  = (1<<3), // location was calculated using Sensors
} LocationTechnologyBits;

typedef uint32_t LocationSpoofMask;
typedef enum {
    LOCATION_POSTION_SPOOFED             = (1<<0), // location position spoofed
    LOCATION_TIME_SPOOFED                = (1<<1), // location time spoofed
    LOCATION_NAVIGATION_DATA_SPOOFED     = (1<<2), // location navigation data spoofed
} LocationSpoofBits;

typedef enum {
    LOCATION_RELIABILITY_NOT_SET = 0,
    LOCATION_RELIABILITY_VERY_LOW,
    LOCATION_RELIABILITY_LOW,
    LOCATION_RELIABILITY_MEDIUM,
    LOCATION_RELIABILITY_HIGH,
} LocationReliability;

typedef uint32_t GnssLocationNavSolutionMask;
typedef enum {
    LOCATION_SBAS_CORRECTION_IONO_BIT  = (1<<0), // SBAS ionospheric correction is used
    LOCATION_SBAS_CORRECTION_FAST_BIT  = (1<<1), // SBAS fast correction is used
    LOCATION_SBAS_CORRECTION_LONG_BIT  = (1<<2), // SBAS long-tem correction is used
    LOCATION_SBAS_INTEGRITY_BIT        = (1<<3), // SBAS integrity information is used
    LOCATION_NAV_CORRECTION_DGNSS_BIT  = (1<<4), // Position Report is DGNSS corrected
    LOCATION_NAV_CORRECTION_RTK_BIT    = (1<<5), // Position Report is RTK corrected
    LOCATION_NAV_CORRECTION_PPP_BIT    = (1<<6) // Position Report is PPP corrected
} GnssLocationNavSolutionBits;

typedef uint32_t GnssLocationPosTechMask;
typedef enum {
    LOCATION_POS_TECH_DEFAULT_BIT                  = 0,
    LOCATION_POS_TECH_SATELLITE_BIT                = (1<<0),
    LOCATION_POS_TECH_CELLID_BIT                   = (1<<1),
    LOCATION_POS_TECH_WIFI_BIT                     = (1<<2),
    LOCATION_POS_TECH_SENSORS_BIT                  = (1<<3),
    LOCATION_POS_TECH_REFERENCE_LOCATION_BIT       = (1<<4),
    LOCATION_POS_TECH_INJECTED_COARSE_POSITION_BIT = (1<<5),
    LOCATION_POS_TECH_AFLT_BIT                     = (1<<6),
    LOCATION_POS_TECH_HYBRID_BIT                   = (1<<7),
    LOCATION_POS_TECH_PPE_BIT                      = (1<<8)
} GnssLocationPosTechBits;

typedef uint32_t GnssLocationPosDataMask;
typedef enum {
    LOCATION_NAV_DATA_HAS_LONG_ACCEL_BIT  = (1<<0), // Navigation data has Forward Acceleration
    LOCATION_NAV_DATA_HAS_LAT_ACCEL_BIT   = (1<<1), // Navigation data has Sideward Acceleration
    LOCATION_NAV_DATA_HAS_VERT_ACCEL_BIT  = (1<<2), // Navigation data has Vertical Acceleration
    LOCATION_NAV_DATA_HAS_YAW_RATE_BIT    = (1<<3), // Navigation data has Heading Rate
    LOCATION_NAV_DATA_HAS_PITCH_BIT       = (1<<4),  // Navigation data has Body pitch
    // Navigation data has Forward Acceleration uncertainty
    LOCATION_NAV_DATA_HAS_LONG_ACCEL_UNC_BIT = (1<<5),
    // Navigation data has Sideward Acceleration uncertainty
    LOCATION_NAV_DATA_HAS_LAT_ACCEL_UNC_BIT  = (1<<6),
    // Navigation data has Vertical Acceleration uncertainty
    LOCATION_NAV_DATA_HAS_VERT_ACCEL_UNC_BIT = (1<<7),
    // Navigation data has Heading Rate uncertainty
    LOCATION_NAV_DATA_HAS_YAW_RATE_UNC_BIT   = (1<<8),
    // Navigation data has Body pitch uncertainty
    LOCATION_NAV_DATA_HAS_PITCH_UNC_BIT      = (1<<9)
} GnssLocationPosDataBits;

typedef uint32_t GnssLocationInfoFlagMask;
typedef enum {
    GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT      = (1<<0), // valid altitude mean sea level
    GNSS_LOCATION_INFO_DOP_BIT                          = (1<<1), // valid pdop, hdop, and vdop
    GNSS_LOCATION_INFO_MAGNETIC_DEVIATION_BIT           = (1<<2), // valid magnetic deviation
    GNSS_LOCATION_INFO_HOR_RELIABILITY_BIT              = (1<<3), // valid horizontal reliability
    GNSS_LOCATION_INFO_VER_RELIABILITY_BIT              = (1<<4), // valid vertical reliability
    GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MAJOR_BIT = (1<<5), // valid elipsode semi major
    GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MINOR_BIT = (1<<6), // valid elipsode semi minor
    GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_AZIMUTH_BIT    = (1<<7), // valid accuracy elipsode azimuth
    GNSS_LOCATION_INFO_GNSS_SV_USED_DATA_BIT            = (1<<8), // valid svUsedInPosition,
                                                                  //       numOfMeasReceived
                                                                  //       and measUsageInfo
    GNSS_LOCATION_INFO_NAV_SOLUTION_MASK_BIT            = (1<<9), // valid navSolutionMask
    GNSS_LOCATION_INFO_POS_TECH_MASK_BIT                = (1<<10),// valid LocPosTechMask
    GNSS_LOCATION_INFO_SV_SOURCE_INFO_BIT               = (1<<11),// valid LocSvInfoSource
    GNSS_LOCATION_INFO_POS_DYNAMICS_DATA_BIT            = (1<<12),// valid position dynamics data
    GNSS_LOCATION_INFO_EXT_DOP_BIT                      = (1<<13),// valid gdop, tdop
    GNSS_LOCATION_INFO_NORTH_STD_DEV_BIT                = (1<<14),// valid North standard deviation
    GNSS_LOCATION_INFO_EAST_STD_DEV_BIT                 = (1<<15),// valid East standard deviation
    GNSS_LOCATION_INFO_NORTH_VEL_BIT                    = (1<<16),// valid North Velocity
    GNSS_LOCATION_INFO_EAST_VEL_BIT                     = (1<<17),// valid East Velocity
    GNSS_LOCATION_INFO_UP_VEL_BIT                       = (1<<18),// valid Up Velocity
    GNSS_LOCATION_INFO_NORTH_VEL_UNC_BIT                = (1<<19),// valid North Velocity Uncertainty
    GNSS_LOCATION_INFO_EAST_VEL_UNC_BIT                 = (1<<20),// valid East Velocity Uncertainty
    GNSS_LOCATION_INFO_UP_VEL_UNC_BIT                   = (1<<21),// valid Up Velocity Uncertainty
    GNSS_LOCATION_INFO_LEAP_SECONDS_BIT                 = (1<<22),// valid leap seconds
    GNSS_LOCATION_INFO_TIME_UNC_BIT                     = (1<<23),// valid time uncertainty
    GNSS_LOCATION_INFO_NUM_SV_USED_IN_POSITION_BIT      = (1<<24), // number of SV used in position
    GNSS_LOCATION_INFO_CALIBRATION_CONFIDENCE_BIT       = (1<<25), // valid sensor cal confidence
    GNSS_LOCATION_INFO_CALIBRATION_STATUS_BIT           = (1<<26), // valid sensor cal status
    GNSS_LOCATION_INFO_OUTPUT_ENG_TYPE_BIT              = (1<<27), // valid output engine type
    GNSS_LOCATION_INFO_OUTPUT_ENG_MASK_BIT              = (1<<28), // valid output engine mask
} GnssLocationInfoFlagBits;

typedef enum {
    GEOFENCE_BREACH_ENTER = 0,
    GEOFENCE_BREACH_EXIT,
    GEOFENCE_BREACH_DWELL_IN,
    GEOFENCE_BREACH_DWELL_OUT,
    GEOFENCE_BREACH_UNKNOWN,
} GeofenceBreachType;

typedef uint16_t GeofenceBreachTypeMask;
typedef enum {
    GEOFENCE_BREACH_ENTER_BIT     = (1<<0),
    GEOFENCE_BREACH_EXIT_BIT      = (1<<1),
    GEOFENCE_BREACH_DWELL_IN_BIT  = (1<<2),
    GEOFENCE_BREACH_DWELL_OUT_BIT = (1<<3),
} GeofenceBreachTypeBits;

typedef enum {
    GEOFENCE_STATUS_AVAILABILE_NO = 0,
    GEOFENCE_STATUS_AVAILABILE_YES,
} GeofenceStatusAvailable;

typedef uint32_t LocationCapabilitiesMask;
typedef enum {
    // supports startTracking API with minInterval param
    LOCATION_CAPABILITIES_TIME_BASED_TRACKING_BIT           = (1<<0),
    // supports startBatching API with minInterval param
    LOCATION_CAPABILITIES_TIME_BASED_BATCHING_BIT           = (1<<1),
    // supports startTracking API with minDistance param
    LOCATION_CAPABILITIES_DISTANCE_BASED_TRACKING_BIT       = (1<<2),
    // supports startBatching API with minDistance param
    LOCATION_CAPABILITIES_DISTANCE_BASED_BATCHING_BIT       = (1<<3),
    // supports addGeofences API
    LOCATION_CAPABILITIES_GEOFENCE_BIT                      = (1<<4),
    // supports GnssMeasurementsCallback
    LOCATION_CAPABILITIES_GNSS_MEASUREMENTS_BIT             = (1<<5),
    // supports startTracking/startBatching API with LocationOptions.mode of MSB (Ms Based)
    LOCATION_CAPABILITIES_GNSS_MSB_BIT                      = (1<<6),
    // supports startTracking/startBatching API with LocationOptions.mode of MSA (MS Assisted)
    LOCATION_CAPABILITIES_GNSS_MSA_BIT                      = (1<<7),
    // supports debug nmea sentences in the debugNmeaCallback
    LOCATION_CAPABILITIES_DEBUG_NMEA_BIT                    = (1<<8),
    // support outdoor trip batching
    LOCATION_CAPABILITIES_OUTDOOR_TRIP_BATCHING_BIT         = (1<<9),
    // support constellation enablement
    LOCATION_CAPABILITIES_CONSTELLATION_ENABLEMENT_BIT      = (1<<10),
    // support agpm
    LOCATION_CAPABILITIES_AGPM_BIT                          = (1<<11),
    // support location privacy
    LOCATION_CAPABILITIES_PRIVACY_BIT                       = (1<<12),
} LocationCapabilitiesBits;

typedef enum {
    LOCATION_TECHNOLOGY_TYPE_GNSS = 0,
} LocationTechnologyType;

// Configures how GPS is locked when GPS is disabled (through GnssDisable)
enum {
    GNSS_CONFIG_GPS_LOCK_NONE = 0, // gps is not locked when GPS is disabled (GnssDisable)
    GNSS_CONFIG_GPS_LOCK_MO,       // gps mobile originated (MO) is locked when GPS is disabled
    GNSS_CONFIG_GPS_LOCK_NI,       // gps network initiated (NI) is locked when GPS is disabled
    GNSS_CONFIG_GPS_LOCK_MO_AND_NI,// gps MO and NI is locked when GPS is disabled
};
typedef int32_t GnssConfigGpsLock;

// SUPL version
typedef enum {
    GNSS_CONFIG_SUPL_VERSION_1_0_0 = 1,
    GNSS_CONFIG_SUPL_VERSION_2_0_0,
    GNSS_CONFIG_SUPL_VERSION_2_0_2,
} GnssConfigSuplVersion;

// LTE Positioning Profile
typedef enum {
    GNSS_CONFIG_LPP_PROFILE_RRLP_ON_LTE = 0,              // RRLP on LTE (Default)
    GNSS_CONFIG_LPP_PROFILE_USER_PLANE,                   // LPP User Plane (UP) on LTE
    GNSS_CONFIG_LPP_PROFILE_CONTROL_PLANE,                // LPP_Control_Plane (CP)
    GNSS_CONFIG_LPP_PROFILE_USER_PLANE_AND_CONTROL_PLANE, // Both LPP UP and CP
} GnssConfigLppProfile;

// Technology for LPPe Control Plane
typedef uint16_t GnssConfigLppeControlPlaneMask;
typedef enum {
    GNSS_CONFIG_LPPE_CONTROL_PLANE_DBH_BIT                  = (1<<0), // DBH
    GNSS_CONFIG_LPPE_CONTROL_PLANE_WLAN_AP_MEASUREMENTS_BIT = (1<<1), // WLAN_AP_MEASUREMENTS
    GNSS_CONFIG_LPPE_CONTROL_PLANE_SRN_AP_MEASUREMENTS_BIT = (1<<2), // SRN_AP_MEASUREMENTS
    GNSS_CONFIG_LPPE_CONTROL_PLANE_SENSOR_BARO_MEASUREMENTS_BIT = (1<<3),
                                                             // SENSOR_BARO_MEASUREMENTS
} GnssConfigLppeControlPlaneBits;

// Technology for LPPe User Plane
typedef uint16_t GnssConfigLppeUserPlaneMask;
typedef enum {
    GNSS_CONFIG_LPPE_USER_PLANE_DBH_BIT                  = (1<<0), // DBH
    GNSS_CONFIG_LPPE_USER_PLANE_WLAN_AP_MEASUREMENTS_BIT = (1<<1), // WLAN_AP_MEASUREMENTS
    GNSS_CONFIG_LPPE_USER_PLANE_SRN_AP_MEASUREMENTS_BIT = (1<<2), // SRN_AP_MEASUREMENTS
    GNSS_CONFIG_LPPE_USER_PLANE_SENSOR_BARO_MEASUREMENTS_BIT = (1<<3),
                                                            // SENSOR_BARO_MEASUREMENTS
} GnssConfigLppeUserPlaneBits;

// Positioning Protocol on A-GLONASS system
typedef uint16_t GnssConfigAGlonassPositionProtocolMask;
typedef enum {
    GNSS_CONFIG_RRC_CONTROL_PLANE_BIT = (1<<0),  // RRC Control Plane
    GNSS_CONFIG_RRLP_USER_PLANE_BIT   = (1<<1),  // RRLP User Plane
    GNSS_CONFIG_LLP_USER_PLANE_BIT    = (1<<2),  // LPP User Plane
    GNSS_CONFIG_LLP_CONTROL_PLANE_BIT = (1<<3),  // LPP Control Plane
} GnssConfigAGlonassPositionProtocolBits;

typedef enum {
    GNSS_CONFIG_EMERGENCY_PDN_FOR_EMERGENCY_SUPL_NO = 0,
    GNSS_CONFIG_EMERGENCY_PDN_FOR_EMERGENCY_SUPL_YES,
} GnssConfigEmergencyPdnForEmergencySupl;

typedef enum {
    GNSS_CONFIG_SUPL_EMERGENCY_SERVICES_NO = 0,
    GNSS_CONFIG_SUPL_EMERGENCY_SERVICES_YES,
} GnssConfigSuplEmergencyServices;

typedef uint16_t GnssConfigSuplModeMask;
typedef enum {
    GNSS_CONFIG_SUPL_MODE_MSB_BIT = (1<<0),
    GNSS_CONFIG_SUPL_MODE_MSA_BIT = (1<<1),
} GnssConfigSuplModeBits;

typedef uint32_t GnssConfigFlagsMask;
typedef enum {
    GNSS_CONFIG_FLAGS_GPS_LOCK_VALID_BIT                   = (1<<0),
    GNSS_CONFIG_FLAGS_SUPL_VERSION_VALID_BIT               = (1<<1),
    GNSS_CONFIG_FLAGS_SET_ASSISTANCE_DATA_VALID_BIT        = (1<<2),
    GNSS_CONFIG_FLAGS_LPP_PROFILE_VALID_BIT                = (1<<3),
    GNSS_CONFIG_FLAGS_LPPE_CONTROL_PLANE_VALID_BIT         = (1<<4),
    GNSS_CONFIG_FLAGS_LPPE_USER_PLANE_VALID_BIT            = (1<<5),
    GNSS_CONFIG_FLAGS_AGLONASS_POSITION_PROTOCOL_VALID_BIT = (1<<6),
    GNSS_CONFIG_FLAGS_EM_PDN_FOR_EM_SUPL_VALID_BIT         = (1<<7),
    GNSS_CONFIG_FLAGS_SUPL_EM_SERVICES_BIT                 = (1<<8),
    GNSS_CONFIG_FLAGS_SUPL_MODE_BIT                        = (1<<9),
    GNSS_CONFIG_FLAGS_BLACKLISTED_SV_IDS_BIT               = (1<<10),
    GNSS_CONFIG_FLAGS_EMERGENCY_EXTENSION_SECONDS_BIT      = (1<<11),
} GnssConfigFlagsBits;

typedef enum {
    GNSS_NI_ENCODING_TYPE_NONE = 0,
    GNSS_NI_ENCODING_TYPE_GSM_DEFAULT,
    GNSS_NI_ENCODING_TYPE_UTF8,
    GNSS_NI_ENCODING_TYPE_UCS2,
} GnssNiEncodingType;

typedef enum {
    GNSS_NI_TYPE_VOICE = 0,
    GNSS_NI_TYPE_SUPL,
    GNSS_NI_TYPE_CONTROL_PLANE,
    GNSS_NI_TYPE_EMERGENCY_SUPL
} GnssNiType;

typedef uint16_t GnssNiOptionsMask;
typedef enum {
    GNSS_NI_OPTIONS_NOTIFICATION_BIT     = (1<<0),
    GNSS_NI_OPTIONS_VERIFICATION_BIT     = (1<<1),
    GNSS_NI_OPTIONS_PRIVACY_OVERRIDE_BIT = (1<<2),
} GnssNiOptionsBits;

typedef enum {
    GNSS_NI_RESPONSE_ACCEPT = 1,
    GNSS_NI_RESPONSE_DENY,
    GNSS_NI_RESPONSE_NO_RESPONSE,
    GNSS_NI_RESPONSE_IGNORE,
} GnssNiResponse;

typedef enum {
    GNSS_SV_TYPE_UNKNOWN = 0,
    GNSS_SV_TYPE_GPS,
    GNSS_SV_TYPE_SBAS,
    GNSS_SV_TYPE_GLONASS,
    GNSS_SV_TYPE_QZSS,
    GNSS_SV_TYPE_BEIDOU,
    GNSS_SV_TYPE_GALILEO,
    GNSS_SV_TYPE_NAVIC,
} GnssSvType;

typedef enum {
    GNSS_EPH_TYPE_UNKNOWN = 0,
    GNSS_EPH_TYPE_EPHEMERIS,
    GNSS_EPH_TYPE_ALMANAC,
} GnssEphemerisType;

typedef enum {
    GNSS_EPH_SOURCE_UNKNOWN = 0,
    GNSS_EPH_SOURCE_DEMODULATED,
    GNSS_EPH_SOURCE_SUPL_PROVIDED,
    GNSS_EPH_SOURCE_OTHER_SERVER_PROVIDED,
    GNSS_EPH_SOURCE_LOCAL,
} GnssEphemerisSource;

typedef enum {
    GNSS_EPH_HEALTH_UNKNOWN = 0,
    GNSS_EPH_HEALTH_GOOD,
    GNSS_EPH_HEALTH_BAD,
} GnssEphemerisHealth;

typedef uint16_t GnssSvOptionsMask;
typedef enum {
    GNSS_SV_OPTIONS_HAS_EPHEMER_BIT             = (1<<0),
    GNSS_SV_OPTIONS_HAS_ALMANAC_BIT             = (1<<1),
    GNSS_SV_OPTIONS_USED_IN_FIX_BIT             = (1<<2),
    GNSS_SV_OPTIONS_HAS_CARRIER_FREQUENCY_BIT   = (1<<3),
} GnssSvOptionsBits;

typedef enum {
    GNSS_ASSISTANCE_TYPE_SUPL = 0,
    GNSS_ASSISTANCE_TYPE_C2K,
    GNSS_ASSISTANCE_TYPE_SUPL_EIMS,
    GNSS_ASSISTANCE_TYPE_SUPL_IMS,
} GnssAssistanceType;

typedef enum {
    GNSS_SUPL_MODE_STANDALONE = 0,
    GNSS_SUPL_MODE_MSB,
    GNSS_SUPL_MODE_MSA,
} GnssSuplMode;

typedef enum {
    BATCHING_MODE_ROUTINE = 0,   // positions are reported when batched positions memory is full
    BATCHING_MODE_TRIP,          // positions are reported when a certain distance is covered
    BATCHING_MODE_NO_AUTO_REPORT // no report of positions automatically, instead queried on demand
} BatchingMode;

typedef enum {
    BATCHING_STATUS_TRIP_COMPLETED = 0,
    BATCHING_STATUS_POSITION_AVAILABE,
    BATCHING_STATUS_POSITION_UNAVAILABLE
} BatchingStatus;

typedef uint16_t GnssMeasurementsAdrStateMask;
typedef enum {
    GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_UNKNOWN                 = 0,
    GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_VALID_BIT               = (1<<0),
    GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_RESET_BIT               = (1<<1),
    GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_CYCLE_SLIP_BIT          = (1<<2),
    GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_HALF_CYCLE_RESOLVED_BIT = (1<<3),
} GnssMeasurementsAdrStateBits;

typedef enum {
    GNSS_MEASUREMENTS_CODE_TYPE_A       = 0,
    GNSS_MEASUREMENTS_CODE_TYPE_B       = 1,
    GNSS_MEASUREMENTS_CODE_TYPE_C       = 2,
    GNSS_MEASUREMENTS_CODE_TYPE_I       = 3,
    GNSS_MEASUREMENTS_CODE_TYPE_L       = 4,
    GNSS_MEASUREMENTS_CODE_TYPE_M       = 5,
    GNSS_MEASUREMENTS_CODE_TYPE_P       = 6,
    GNSS_MEASUREMENTS_CODE_TYPE_Q       = 7,
    GNSS_MEASUREMENTS_CODE_TYPE_S       = 8,
    GNSS_MEASUREMENTS_CODE_TYPE_W       = 9,
    GNSS_MEASUREMENTS_CODE_TYPE_X       = 10,
    GNSS_MEASUREMENTS_CODE_TYPE_Y       = 11,
    GNSS_MEASUREMENTS_CODE_TYPE_Z       = 12,
    GNSS_MEASUREMENTS_CODE_TYPE_N       = 13,
    GNSS_MEASUREMENTS_CODE_TYPE_OTHER   = 255,
} GnssMeasurementsCodeType;

typedef uint32_t GnssMeasurementsDataFlagsMask;
typedef enum {
    GNSS_MEASUREMENTS_DATA_SV_ID_BIT                        = (1<<0),
    GNSS_MEASUREMENTS_DATA_SV_TYPE_BIT                      = (1<<1),
    GNSS_MEASUREMENTS_DATA_STATE_BIT                        = (1<<2),
    GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_BIT             = (1<<3),
    GNSS_MEASUREMENTS_DATA_RECEIVED_SV_TIME_UNCERTAINTY_BIT = (1<<4),
    GNSS_MEASUREMENTS_DATA_CARRIER_TO_NOISE_BIT             = (1<<5),
    GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_BIT             = (1<<6),
    GNSS_MEASUREMENTS_DATA_PSEUDORANGE_RATE_UNCERTAINTY_BIT = (1<<7),
    GNSS_MEASUREMENTS_DATA_ADR_STATE_BIT                    = (1<<8),
    GNSS_MEASUREMENTS_DATA_ADR_BIT                          = (1<<9),
    GNSS_MEASUREMENTS_DATA_ADR_UNCERTAINTY_BIT              = (1<<10),
    GNSS_MEASUREMENTS_DATA_CARRIER_FREQUENCY_BIT            = (1<<11),
    GNSS_MEASUREMENTS_DATA_CARRIER_CYCLES_BIT               = (1<<12),
    GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_BIT                = (1<<13),
    GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_UNCERTAINTY_BIT    = (1<<14),
    GNSS_MEASUREMENTS_DATA_MULTIPATH_INDICATOR_BIT          = (1<<15),
    GNSS_MEASUREMENTS_DATA_SIGNAL_TO_NOISE_RATIO_BIT        = (1<<16),
    GNSS_MEASUREMENTS_DATA_AUTOMATIC_GAIN_CONTROL_BIT       = (1<<17),
} GnssMeasurementsDataFlagsBits;

typedef uint32_t GnssMeasurementsStateMask;
typedef enum {
    GNSS_MEASUREMENTS_STATE_UNKNOWN_BIT               = 0,
    GNSS_MEASUREMENTS_STATE_CODE_LOCK_BIT             = (1<<0),
    GNSS_MEASUREMENTS_STATE_BIT_SYNC_BIT              = (1<<1),
    GNSS_MEASUREMENTS_STATE_SUBFRAME_SYNC_BIT         = (1<<2),
    GNSS_MEASUREMENTS_STATE_TOW_DECODED_BIT           = (1<<3),
    GNSS_MEASUREMENTS_STATE_MSEC_AMBIGUOUS_BIT        = (1<<4),
    GNSS_MEASUREMENTS_STATE_SYMBOL_SYNC_BIT           = (1<<5),
    GNSS_MEASUREMENTS_STATE_GLO_STRING_SYNC_BIT       = (1<<6),
    GNSS_MEASUREMENTS_STATE_GLO_TOD_DECODED_BIT       = (1<<7),
    GNSS_MEASUREMENTS_STATE_BDS_D2_BIT_SYNC_BIT       = (1<<8),
    GNSS_MEASUREMENTS_STATE_BDS_D2_SUBFRAME_SYNC_BIT  = (1<<9),
    GNSS_MEASUREMENTS_STATE_GAL_E1BC_CODE_LOCK_BIT    = (1<<10),
    GNSS_MEASUREMENTS_STATE_GAL_E1C_2ND_CODE_LOCK_BIT = (1<<11),
    GNSS_MEASUREMENTS_STATE_GAL_E1B_PAGE_SYNC_BIT     = (1<<12),
    GNSS_MEASUREMENTS_STATE_SBAS_SYNC_BIT             = (1<<13),
    GNSS_MEASUREMENTS_STATE_TOW_KNOWN_BIT             = (1<<14),
    GNSS_MEASUREMENTS_STATE_GLO_TOD_KNOWN_BIT         = (1<<15),
    GNSS_MEASUREMENTS_STATE_2ND_CODE_LOCK_BIT         = (1<<16),
} GnssMeasurementsStateBits;

typedef enum {
    GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_UNKNOWN = 0,
    GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_PRESENT,
    GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_NOT_PRESENT,
} GnssMeasurementsMultipathIndicator;

typedef uint32_t GnssMeasurementsClockFlagsMask;
typedef enum {
    GNSS_MEASUREMENTS_CLOCK_FLAGS_LEAP_SECOND_BIT                  = (1<<0),
    GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_BIT                         = (1<<1),
    GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_UNCERTAINTY_BIT             = (1<<2),
    GNSS_MEASUREMENTS_CLOCK_FLAGS_FULL_BIAS_BIT                    = (1<<3),
    GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_BIT                         = (1<<4),
    GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_UNCERTAINTY_BIT             = (1<<5),
    GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_BIT                        = (1<<6),
    GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_UNCERTAINTY_BIT            = (1<<7),
    GNSS_MEASUREMENTS_CLOCK_FLAGS_HW_CLOCK_DISCONTINUITY_COUNT_BIT = (1<<8),
} GnssMeasurementsClockFlagsBits;

typedef uint32_t GnssAidingDataSvMask;
typedef enum {
    GNSS_AIDING_DATA_SV_EPHEMERIS_BIT    = (1<<0), // ephemeris
    GNSS_AIDING_DATA_SV_ALMANAC_BIT      = (1<<1), // almanac
    GNSS_AIDING_DATA_SV_HEALTH_BIT       = (1<<2), // health
    GNSS_AIDING_DATA_SV_DIRECTION_BIT    = (1<<3), // direction
    GNSS_AIDING_DATA_SV_STEER_BIT        = (1<<4), // steer
    GNSS_AIDING_DATA_SV_ALMANAC_CORR_BIT = (1<<5), // almanac correction
    GNSS_AIDING_DATA_SV_BLACKLIST_BIT    = (1<<6), // blacklist SVs
    GNSS_AIDING_DATA_SV_SA_DATA_BIT      = (1<<7), // sensitivity assistance data
    GNSS_AIDING_DATA_SV_NO_EXIST_BIT     = (1<<8), // SV does not exist
    GNSS_AIDING_DATA_SV_IONOSPHERE_BIT   = (1<<9), // ionosphere correction
    GNSS_AIDING_DATA_SV_TIME_BIT         = (1<<10),// reset satellite time
    GNSS_AIDING_DATA_SV_MB_DATA          = (1<<11),// delete multiband data
    GNSS_AIDING_DATA_SV_POLY_BIT         = (1<<12),// poly
} GnssAidingDataSvBits;

typedef uint32_t GnssAidingDataSvTypeMask;
typedef enum {
    GNSS_AIDING_DATA_SV_TYPE_GPS_BIT      = (1<<0),
    GNSS_AIDING_DATA_SV_TYPE_GLONASS_BIT  = (1<<1),
    GNSS_AIDING_DATA_SV_TYPE_QZSS_BIT     = (1<<2),
    GNSS_AIDING_DATA_SV_TYPE_BEIDOU_BIT   = (1<<3),
    GNSS_AIDING_DATA_SV_TYPE_GALILEO_BIT  = (1<<4),
    GNSS_AIDING_DATA_SV_TYPE_NAVIC_BIT    = (1<<5),
} GnssAidingDataSvTypeBits;

/* Gnss constellation type mask */
typedef uint16_t GnssConstellationTypeMask;
typedef enum {
    GNSS_CONSTELLATION_TYPE_GPS_BIT      = (1<<0),
    GNSS_CONSTELLATION_TYPE_GLONASS_BIT  = (1<<1),
    GNSS_CONSTELLATION_TYPE_QZSS_BIT     = (1<<2),
    GNSS_CONSTELLATION_TYPE_BEIDOU_BIT   = (1<<3),
    GNSS_CONSTELLATION_TYPE_GALILEO_BIT  = (1<<4),
    GNSS_CONSTELLATION_TYPE_SBAS_BIT     = (1<<5),
    GNSS_CONSTELLATION_TYPE_NAVIC_BIT    = (1<<6)
} GnssConstellationTypeBits;

#define GNSS_CONSTELLATION_TYPE_MASK_ALL\
        (GNSS_CONSTELLATION_TYPE_GPS_BIT     | GNSS_CONSTELLATION_TYPE_GLONASS_BIT |\
         GNSS_CONSTELLATION_TYPE_QZSS_BIT    | GNSS_CONSTELLATION_TYPE_BEIDOU_BIT  |\
         GNSS_CONSTELLATION_TYPE_GALILEO_BIT | GNSS_CONSTELLATION_TYPE_SBAS_BIT    |\
         GNSS_CONSTELLATION_TYPE_NAVIC_BIT)

/** GNSS Signal Type and RF Band */
typedef uint32_t GnssSignalTypeMask;
typedef enum {
    /** GPS L1CA Signal */
    GNSS_SIGNAL_GPS_L1CA            = (1<<0),
    /** GPS L1C Signal */
    GNSS_SIGNAL_GPS_L1C             = (1<<1),
    /** GPS L2 RF Band */
    GNSS_SIGNAL_GPS_L2              = (1<<2),
    /** GPS L5 RF Band */
    GNSS_SIGNAL_GPS_L5              = (1<<3),
    /** GLONASS G1 (L1OF) RF Band */
    GNSS_SIGNAL_GLONASS_G1          = (1<<4),
    /** GLONASS G2 (L2OF) RF Band */
    GNSS_SIGNAL_GLONASS_G2          = (1<<5),
    /** GALILEO E1 RF Band */
    GNSS_SIGNAL_GALILEO_E1          = (1<<6),
    /** GALILEO E5A RF Band */
    GNSS_SIGNAL_GALILEO_E5A         = (1<<7),
    /** GALILEO E5B RF Band */
    GNSS_SIGNAL_GALILEO_E5B         = (1<<8),
    /** BEIDOU B1_I RF Band */
    GNSS_SIGNAL_BEIDOU_B1I          = (1<<9),
    /** BEIDOU B1C RF Band */
    GNSS_SIGNAL_BEIDOU_B1C          = (1<<10),
    /** BEIDOU B2_I RF Band */
    GNSS_SIGNAL_BEIDOU_B2I          = (1<<11),
    /** BEIDOU B2A_I RF Band */
    GNSS_SIGNAL_BEIDOU_B2AI         = (1<<12),
    /** QZSS L1CA RF Band */
    GNSS_SIGNAL_QZSS_L1CA           = (1<<13),
    /** QZSS L1S RF Band */
    GNSS_SIGNAL_QZSS_L1S            = (1<<14),
    /** QZSS L2 RF Band */
    GNSS_SIGNAL_QZSS_L2             = (1<<15),
    /** QZSS L5 RF Band */
    GNSS_SIGNAL_QZSS_L5             = (1<<16),
    /** SBAS L1 RF Band */
    GNSS_SIGNAL_SBAS_L1             = (1<<17),
    /** NAVIC L5 RF Band */
    GNSS_SIGNAL_NAVIC_L5            = (1<<18),
    /** BEIDOU B2A_Q RF Band */
    GNSS_SIGNAL_BEIDOU_B2AQ         = (1<<19)
} GnssSignalTypeBits;

#define GNSS_SIGNAL_TYPE_MASK_ALL\
    (GNSS_SIGNAL_GPS_L1CA | GNSS_SIGNAL_GPS_L1C | GNSS_SIGNAL_GPS_L2 |\
     GNSS_SIGNAL_GPS_L5| GNSS_SIGNAL_GLONASS_G1 | GNSS_SIGNAL_GLONASS_G2 |\
     GNSS_SIGNAL_GALILEO_E1 | GNSS_SIGNAL_GALILEO_E5A | GNSS_SIGNAL_GALILEO_E5B |\
     GNSS_SIGNAL_BEIDOU_B1I | GNSS_SIGNAL_BEIDOU_B1C | GNSS_SIGNAL_BEIDOU_B2I|\
     GNSS_SIGNAL_BEIDOU_B2AI | GNSS_SIGNAL_QZSS_L1CA | GNSS_SIGNAL_QZSS_L1S |\
     GNSS_SIGNAL_QZSS_L2| GNSS_SIGNAL_QZSS_L5 | GNSS_SIGNAL_SBAS_L1 |\
     GNSS_SIGNAL_NAVIC_L5 | GNSS_SIGNAL_BEIDOU_B2AQ)

typedef enum
{
    GNSS_LOC_SV_SYSTEM_UNKNOWN                = 0,
    /** unknown sv system. */
    GNSS_LOC_SV_SYSTEM_MIN                    = 1,
    /**< Min enum of valid SV system. */
    GNSS_LOC_SV_SYSTEM_GPS                    = 1,
    /**< GPS satellite. */
    GNSS_LOC_SV_SYSTEM_GALILEO                = 2,
    /**< GALILEO satellite. */
    GNSS_LOC_SV_SYSTEM_SBAS                   = 3,
    /**< SBAS satellite. */
    GNSS_LOC_SV_SYSTEM_GLONASS                = 4,
    /**< GLONASS satellite. */
    GNSS_LOC_SV_SYSTEM_BDS                    = 5,
    /**< BDS satellite. */
    GNSS_LOC_SV_SYSTEM_QZSS                   = 6,
    /**< QZSS satellite. */
    GNSS_LOC_SV_SYSTEM_NAVIC                  = 7,
    /**< QZSS satellite. */
    GNSS_LOC_SV_SYSTEM_MAX                    = 7,
    /**< Max enum of valid SV system. */
} Gnss_LocSvSystemEnumType;

typedef enum {
    GNSS_LOC_SIGNAL_TYPE_GPS_L1CA = 0,          /**<  GPS L1CA Signal  */
    GNSS_LOC_SIGNAL_TYPE_GPS_L1C = 1,           /**<  GPS L1C Signal  */
    GNSS_LOC_SIGNAL_TYPE_GPS_L2C_L = 2,         /**<  GPS L2C_L RF Band  */
    GNSS_LOC_SIGNAL_TYPE_GPS_L5_Q = 3,          /**<  GPS L5_Q RF Band  */
    GNSS_LOC_SIGNAL_TYPE_GLONASS_G1 = 4,        /**<  GLONASS G1 (L1OF) RF Band  */
    GNSS_LOC_SIGNAL_TYPE_GLONASS_G2 = 5,        /**<  GLONASS G2 (L2OF) RF Band  */
    GNSS_LOC_SIGNAL_TYPE_GALILEO_E1_C = 6,      /**<  GALILEO E1_C RF Band  */
    GNSS_LOC_SIGNAL_TYPE_GALILEO_E5A_Q = 7,     /**<  GALILEO E5A_Q RF Band  */
    GNSS_LOC_SIGNAL_TYPE_GALILEO_E5B_Q = 8,     /**<  GALILEO E5B_Q RF Band  */
    GNSS_LOC_SIGNAL_TYPE_BEIDOU_B1_I = 9,       /**<  BEIDOU B1_I RF Band  */
    GNSS_LOC_SIGNAL_TYPE_BEIDOU_B1C = 10,       /**<  BEIDOU B1C RF Band  */
    GNSS_LOC_SIGNAL_TYPE_BEIDOU_B2_I = 11,      /**<  BEIDOU B2_I RF Band  */
    GNSS_LOC_SIGNAL_TYPE_BEIDOU_B2A_I = 12,     /**<  BEIDOU B2A_I RF Band  */
    GNSS_LOC_SIGNAL_TYPE_QZSS_L1CA = 13,        /**<  QZSS L1CA RF Band  */
    GNSS_LOC_SIGNAL_TYPE_QZSS_L1S = 14,         /**<  QZSS L1S RF Band  */
    GNSS_LOC_SIGNAL_TYPE_QZSS_L2C_L = 15,       /**<  QZSS L2C_L RF Band  */
    GNSS_LOC_SIGNAL_TYPE_QZSS_L5_Q = 16,        /**<  QZSS L5_Q RF Band  */
    GNSS_LOC_SIGNAL_TYPE_SBAS_L1_CA = 17,       /**<  SBAS L1_CA RF Band  */
    GNSS_LOC_SIGNAL_TYPE_NAVIC_L5 = 18,         /**<  NAVIC L5 RF Band */
    GNSS_LOC_SIGNAL_TYPE_BEIDOU_B2A_Q = 19,     /**<  BEIDOU B2A_Q RF Band  */
    GNSS_LOC_MAX_NUMBER_OF_SIGNAL_TYPES = 20    /**<  Maximum number of signal types */
} Gnss_LocSignalEnumType;

typedef uint32_t PositioningEngineMask;
typedef enum {
    STANDARD_POSITIONING_ENGINE = (1 << 0),
    DEAD_RECKONING_ENGINE       = (1 << 1),
    PRECISE_POSITIONING_ENGINE  = (1 << 2)
} PositioningEngineBits;

typedef uint64_t GnssDataMask;
typedef enum {
    // Jammer Indicator is available
    GNSS_LOC_DATA_JAMMER_IND_BIT = (1ULL << 0),
    // AGC is available
    GNSS_LOC_DATA_AGC_BIT = (1ULL << 1)
} GnssDataBits;

typedef uint32_t GnssSystemTimeStructTypeFlags;
typedef enum {
    GNSS_SYSTEM_TIME_WEEK_VALID             = (1 << 0),
    GNSS_SYSTEM_TIME_WEEK_MS_VALID          = (1 << 1),
    GNSS_SYSTEM_CLK_TIME_BIAS_VALID         = (1 << 2),
    GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID     = (1 << 3),
    GNSS_SYSTEM_REF_FCOUNT_VALID            = (1 << 4),
    GNSS_SYSTEM_NUM_CLOCK_RESETS_VALID      = (1 << 5)
} GnssSystemTimeTypeBits;

typedef uint32_t GnssGloTimeStructTypeFlags;
typedef enum {
    GNSS_CLO_DAYS_VALID                     = (1 << 0),
    GNSS_GLO_MSEC_VALID                     = (1 << 1),
    GNSS_GLO_CLK_TIME_BIAS_VALID            = (1 << 2),
    GNSS_GLO_CLK_TIME_BIAS_UNC_VALID        = (1 << 3),
    GNSS_GLO_REF_FCOUNT_VALID               = (1 << 4),
    GNSS_GLO_NUM_CLOCK_RESETS_VALID         = (1 << 5),
    GNSS_GLO_FOUR_YEAR_VALID                = (1 << 6)
} GnssGloTimeTypeBits;

typedef struct {
    GnssAidingDataSvMask svMask;         // bitwise OR of GnssAidingDataSvBits
    GnssAidingDataSvTypeMask svTypeMask; // bitwise OR of GnssAidingDataSvTypeBits
} GnssAidingDataSv;

typedef uint32_t GnssAidingDataCommonMask;
typedef enum {
    GNSS_AIDING_DATA_COMMON_POSITION_BIT      = (1<<0), // position estimate
    GNSS_AIDING_DATA_COMMON_TIME_BIT          = (1<<1), // reset all clock values
    GNSS_AIDING_DATA_COMMON_UTC_BIT           = (1<<2), // UTC estimate
    GNSS_AIDING_DATA_COMMON_RTI_BIT           = (1<<3), // RTI
    GNSS_AIDING_DATA_COMMON_FREQ_BIAS_EST_BIT = (1<<4), // frequency bias estimate
    GNSS_AIDING_DATA_COMMON_CELLDB_BIT        = (1<<5), // all celldb info
} GnssAidingDataCommonBits;

typedef struct {
    GnssAidingDataCommonMask mask; // bitwise OR of GnssAidingDataCommonBits
} GnssAidingDataCommon;

typedef struct {
    bool deleteAll;              // if true, delete all aiding data and ignore other params
    GnssAidingDataSv sv;         // SV specific aiding data
    GnssAidingDataCommon common; // common aiding data
    PositioningEngineMask posEngineMask; // engines to perform the delete operation on.
} GnssAidingData;

typedef uint16_t DrCalibrationStatusMask;
typedef enum {
    // Indicate that roll calibration is needed. Need to take more turns on level ground
    DR_ROLL_CALIBRATION_NEEDED  = (1<<0),
    // Indicate that pitch calibration is needed. Need to take more turns on level ground
    DR_PITCH_CALIBRATION_NEEDED = (1<<1),
    // Indicate that yaw calibration is needed. Need to accelerate in a straight line
    DR_YAW_CALIBRATION_NEEDED   = (1<<2),
    // Indicate that odo calibration is needed. Need to accelerate in a straight line
    DR_ODO_CALIBRATION_NEEDED   = (1<<3),
    // Indicate that gyro calibration is needed. Need to take more turns on level ground
    DR_GYRO_CALIBRATION_NEEDED  = (1<<4)
} DrCalibrationStatusBits;

typedef struct {
    uint32_t size;           // set to sizeof(Location)
    LocationFlagsMask flags; // bitwise OR of LocationFlagsBits to mark which params are valid
    uint64_t timestamp;      // UTC timestamp for location fix, milliseconds since January 1, 1970
    double latitude;         // in degrees
    double longitude;        // in degrees
    double altitude;         // in meters above the WGS 84 reference ellipsoid
    float speed;             // in meters per second
    float bearing;           // in degrees; range [0, 360)
    float accuracy;          // in meters
    float verticalAccuracy;  // in meters
    float speedAccuracy;     // in meters/second
    float bearingAccuracy;   // in degrees (0 to 359.999)
    LocationTechnologyMask techMask;
    LocationSpoofMask      spoofMask;
} Location;

typedef enum {
    LOC_REQ_ENGINE_FUSED_BIT = (1<<0),
    LOC_REQ_ENGINE_SPE_BIT   = (1<<1),
    LOC_REQ_ENGINE_PPE_BIT   = (1<<2),
} LocReqEngineTypeMask;

typedef enum {
    LOC_OUTPUT_ENGINE_FUSED   = 0,
    /** This is the GNSS fix from modem */
    LOC_OUTPUT_ENGINE_SPE     = 1,
    /** This is the GNSS fix with correction PPP/RTK correction */
    LOC_OUTPUT_ENGINE_PPE     = 2,
    LOC_OUTPUT_ENGINE_COUNT,
} LocOutputEngineType;

struct LocationOptions {
    uint32_t size;          // set to sizeof(LocationOptions)
    uint32_t minInterval; // in milliseconds
    uint32_t minDistance; // in meters. if minDistance > 0, gnssSvCallback/gnssNmeaCallback/
                          // gnssMeasurementsCallback may not be called
    GnssSuplMode mode;    // Standalone/MS-Based/MS-Assisted
    // behavior when this field is 0:
    //  if engine hub is running, this will be fused fix,
    //  if engine hub is not running, this will be SPE fix
    LocReqEngineTypeMask locReqEngTypeMask;

    inline LocationOptions() :
            size(0), minInterval(0), minDistance(0), mode(GNSS_SUPL_MODE_STANDALONE),
            locReqEngTypeMask((LocReqEngineTypeMask)0) {}
};

typedef enum {
    GNSS_POWER_MODE_INVALID = 0,
    GNSS_POWER_MODE_M1,  /* Improved Accuracy Mode */
    GNSS_POWER_MODE_M2,  /* Normal Mode */
    GNSS_POWER_MODE_M3,  /* Background Mode */
    GNSS_POWER_MODE_M4,  /* Background Mode */
    GNSS_POWER_MODE_M5   /* Background Mode */
} GnssPowerMode;

struct TrackingOptions : LocationOptions {
    GnssPowerMode powerMode; /* Power Mode to be used for time based tracking
                                sessions */
    uint32_t tbm;  /* Time interval between measurements specified in millis.
                      Applicable to background power modes */

    inline TrackingOptions() :
            LocationOptions(), powerMode(GNSS_POWER_MODE_INVALID), tbm(0) {}
    inline TrackingOptions(uint32_t s, GnssPowerMode m, uint32_t t) :
            LocationOptions(), powerMode(m), tbm(t) { LocationOptions::size = s; }
    inline TrackingOptions(const LocationOptions& options) :
            LocationOptions(options), powerMode(GNSS_POWER_MODE_INVALID), tbm(0) {}
    inline void setLocationOptions(const LocationOptions& options) {
        size = sizeof(TrackingOptions);
        minInterval = options.minInterval;
        minDistance = options.minDistance;
        mode = options.mode;
        locReqEngTypeMask = options.locReqEngTypeMask;
    }
    inline LocationOptions getLocationOptions() {
        LocationOptions locOption;
        locOption.size = sizeof(locOption);
        locOption.minDistance = minDistance;
        locOption.minInterval = minInterval;
        locOption.mode = mode;
        locOption.locReqEngTypeMask = locReqEngTypeMask;
        return locOption;
    }
};

struct BatchingOptions : LocationOptions {
    BatchingMode batchingMode;

    inline BatchingOptions() :
            LocationOptions(), batchingMode(BATCHING_MODE_ROUTINE) {}
    inline BatchingOptions(uint32_t s, BatchingMode m) :
            LocationOptions(), batchingMode(m) { LocationOptions::size = s; }
    inline BatchingOptions(const LocationOptions& options) :
            LocationOptions(options), batchingMode(BATCHING_MODE_ROUTINE) {}
    inline void setLocationOptions(const LocationOptions& options) {
        minInterval = options.minInterval;
        minDistance = options.minDistance;
        mode = options.mode;
    }
};

typedef struct {
    uint32_t size;
    BatchingStatus batchingStatus;
} BatchingStatusInfo;

typedef struct {
    uint32_t size;                          // set to sizeof(GeofenceOption)
    GeofenceBreachTypeMask breachTypeMask;  // bitwise OR of GeofenceBreachTypeBits
    uint32_t responsiveness;                // in milliseconds
    uint32_t dwellTime;                     // in seconds
} GeofenceOption;

typedef struct {
    uint32_t size;    // set to sizeof(GeofenceInfo)
    double latitude;  // in degrees
    double longitude; // in degrees
    double radius;    // in meters
} GeofenceInfo;

typedef struct {
    uint32_t size;             // set to sizeof(GeofenceBreachNotification)
    uint32_t count;            // number of ids in array
    uint32_t* ids;           // array of ids that have breached
    Location location;       // location associated with breach
    GeofenceBreachType type; // type of breach
    uint64_t timestamp;      // timestamp of breach
} GeofenceBreachNotification;

typedef struct {
    uint32_t size;                       // set to sizeof(GeofenceBreachNotification)
    GeofenceStatusAvailable available; // GEOFENCE_STATUS_AVAILABILE_NO/_YES
    LocationTechnologyType techType;   // GNSS
} GeofenceStatusNotification;

typedef struct {
    uint64_t gpsSvUsedIdsMask;
    uint64_t gloSvUsedIdsMask;
    uint64_t galSvUsedIdsMask;
    uint64_t bdsSvUsedIdsMask;
    uint64_t qzssSvUsedIdsMask;
    uint64_t navicSvUsedIdsMask;
} GnssLocationSvUsedInPosition;

typedef struct {
    /** GnssSignalType mask */
    GnssSignalTypeMask gnssSignalType;
   /** Specifies GNSS Constellation Type */
    Gnss_LocSvSystemEnumType gnssConstellation;
    /**  GNSS SV ID.
     For GPS:      1 to 32
     For GLONASS:  65 to 96. When slot-number to SV ID mapping is unknown, set as 255.
     For SBAS:     120 to 151
     For QZSS-L1CA:193 to 197
     For BDS:      201 to 237
     For GAL:      301 to 336
     For NAVIC:    401 to 414  */
    uint16_t gnssSvId;
} GnssMeasUsageInfo;

/** @struct
    Body Frame parameters
*/
typedef struct {
    GnssLocationPosDataMask bodyFrameDataMask; // Contains Body frame LocPosDataMask bits
    float longAccel;                           // Forward Acceleration in body frame (m/s2)
    float latAccel;                            // Sideward Acceleration in body frame (m/s2)
    float vertAccel;                           // Vertical Acceleration in body frame (m/s2)
    float yawRate;                             // Heading Rate (Radians/second)
    float pitch;                               // Body pitch (Radians)
    float longAccelUnc;   // Uncertainty of Forward Acceleration in body frame
    float latAccelUnc;    // Uncertainty of Side-ward Acceleration in body frame
    float vertAccelUnc;   // Uncertainty of Vertical Acceleration in body frame
    float yawRateUnc;     // Uncertainty of Heading Rate
    float pitchUnc;       // Uncertainty of Body pitch
} GnssLocationPositionDynamics;

typedef struct {
    /** Validity mask for below fields */
    GnssSystemTimeStructTypeFlags validityMask;
    /** Extended week number at reference tick.
    Unit: Week.
    Set to 65535 if week number is unknown.
    For GPS:
      Calculated from midnight, Jan. 6, 1980.
      OTA decoded 10 bit GPS week is extended to map between:
      [NV6264 to (NV6264 + 1023)].
      NV6264: Minimum GPS week number configuration.
      Default value of NV6264: 1738
    For BDS:
      Calculated from 00:00:00 on January 1, 2006 of Coordinated Universal Time (UTC).
    For GAL:
      Calculated from 00:00 UT on Sunday August 22, 1999 (midnight between August 21 and August 22).
   */
    uint16_t systemWeek;
    /** Time in to the current week at reference tick.
       Unit: Millisecond. Range: 0 to 604799999.
       Check for systemClkTimeUncMs before use */
    uint32_t systemMsec;
    /** System clock time bias (sub-millisecond)
        Units: Millisecond
        Note: System time (TOW Millisecond) = systemMsec - systemClkTimeBias.
        Check for systemClkTimeUncMs before use. */
    float systemClkTimeBias;
    /** Single sided maximum time bias uncertainty
        Units: Millisecond */
    float systemClkTimeUncMs;
    /** FCount (free running HW timer) value. Don't use for relative time purpose
         due to possible discontinuities.
         Unit: Millisecond */
    uint32_t refFCount;
    /** Number of clock resets/discontinuities detected, affecting the local hardware counter value. */
    uint32_t numClockResets;
} GnssSystemTimeStructType;

typedef struct {
    /** GLONASS day number in four years. Refer to GLONASS ICD.
        Applicable only for GLONASS and shall be ignored for other constellations.
        If unknown shall be set to 65535 */
    uint16_t gloDays;
    /** Validity mask for below fields */
    GnssGloTimeStructTypeFlags validityMask;
    /** GLONASS time of day in Millisecond. Refer to GLONASS ICD.
        Units: Millisecond
        Check for gloClkTimeUncMs before use */
    uint32_t gloMsec;
    /** GLONASS clock time bias (sub-millisecond)
        Units: Millisecond
        Note: GLO time (TOD Millisecond) = gloMsec - gloClkTimeBias.
        Check for gloClkTimeUncMs before use. */
    float gloClkTimeBias;
    /** Single sided maximum time bias uncertainty
        Units: Millisecond */
    float gloClkTimeUncMs;
    /** FCount (free running HW timer) value. Don't use for relative time purpose
        due to possible discontinuities.
        Unit: Millisecond */
    uint32_t  refFCount;
    /** Number of clock resets/discontinuities detected, affecting the local hardware counter value. */
    uint32_t numClockResets;
    /** GLONASS four year number from 1996. Refer to GLONASS ICD.
        Applicable only for GLONASS and shall be ignored for other constellations.
        If unknown shall be set to 255 */
    uint8_t gloFourYear;
} GnssGloTimeStructType;

typedef union {
    GnssSystemTimeStructType gpsSystemTime;
    GnssSystemTimeStructType galSystemTime;
    GnssSystemTimeStructType bdsSystemTime;
    GnssSystemTimeStructType qzssSystemTime;
    GnssGloTimeStructType    gloSystemTime;
    GnssSystemTimeStructType navicSystemTime;
} SystemTimeStructUnion;
    /** Time applicability of PVT report */
typedef struct {
    /** Specifies GNSS system time reported. Mandatory field */
    Gnss_LocSvSystemEnumType gnssSystemTimeSrc;
    /** Reporting of GPS system time is recommended.
      If GPS time is unknown & other satellite system time is known,
      it should be reported.
      Mandatory field
     */
    SystemTimeStructUnion u;
} GnssSystemTime;

typedef struct {
    uint32_t size;                      // set to sizeof(GnssLocationInfo)
    Location location;                  // basic locaiton info, latitude, longitude, and etc
    GnssLocationInfoFlagMask flags;     // bitwise OR of GnssLocationInfoBits for param validity
    float altitudeMeanSeaLevel;         // altitude wrt mean sea level
    float pdop;                         // position dilusion of precision
    float hdop;                         // horizontal dilusion of precision
    float vdop;                         // vertical dilusion of precision
    float gdop;                         // geometric  dilution of precision
    float tdop;                         // time dilution of precision
    float magneticDeviation;            // magnetic deviation
    LocationReliability horReliability; // horizontal reliability
    LocationReliability verReliability; // vertical reliability
    float horUncEllipseSemiMajor;       // horizontal elliptical accuracy semi-major axis
    float horUncEllipseSemiMinor;       // horizontal elliptical accuracy semi-minor axis
    float horUncEllipseOrientAzimuth;   // horizontal elliptical accuracy azimuth
    float northStdDeviation;            // North standard deviation Unit: Meters
    float eastStdDeviation;             // East standard deviation. Unit: Meters
    float northVelocity;                // North Velocity.Unit: Meters/sec
    float eastVelocity;                 // East Velocity Unit Meters/sec
    float upVelocity;                   // Up Velocity. Unit Meters/sec
    float northVelocityStdDeviation;
    float eastVelocityStdDeviation;
    float upVelocityStdDeviation;
    uint16_t numSvUsedInPosition;
    GnssLocationSvUsedInPosition svUsedInPosition;// Gnss sv used in position data
    GnssLocationNavSolutionMask navSolutionMask;  // Nav solution mask to indicate sbas corrections
    GnssLocationPosTechMask posTechMask;          // Position technology used in computing this fix
    GnssLocationPositionDynamics bodyFrameData;   // Body Frame Dynamics: 4wayAcceleration and
                                                  // pitch set with validity
    GnssSystemTime gnssSystemTime;            // GNSS System Time
    uint8_t numOfMeasReceived; // Number of measurements received for use in fix.
    GnssMeasUsageInfo measUsageInfo[GNSS_SV_MAX]; // GNSS Measurement Usage info
    uint8_t leapSeconds;                          // leap second
    float timeUncMs;                              // Time uncertainty in milliseconds
    uint8_t calibrationConfidence;                // Sensor calibration confidence percent,
                                                  // in range of [0, 100]
    DrCalibrationStatusMask calibrationStatus;    // Sensor calibration status
    // location engine type. When the fix. when the type is set to
    // LOC_ENGINE_SRC_FUSED, the fix is the propagated/aggregated
    // reports from all engines running on the system (e.g.:
    // DR/SPE/PPE). To check which location engine contributes to
    // the fused output, check for locOutputEngMask.
    LocOutputEngineType locOutputEngType;
    // when loc output eng type is set to fused, this field
    // indicates the set of engines contribute to the fix.
    PositioningEngineMask locOutputEngMask;
} GnssLocationInfoNotification;

typedef struct {
    uint32_t size;                           // set to sizeof(GnssNiNotification)
    GnssNiType type;                       // type of NI (Voice, SUPL, Control Plane)
    GnssNiOptionsMask options;             // bitwise OR of GnssNiOptionsBits
    uint32_t timeout;                      // time (seconds) to wait for user input
    GnssNiResponse timeoutResponse;        // the response that should be sent when timeout expires
    char requestor[GNSS_NI_REQUESTOR_MAX]; // the requestor that is making the request
    GnssNiEncodingType requestorEncoding;  // the encoding type for requestor
    char message[GNSS_NI_MESSAGE_ID_MAX];  // the message to show user
    GnssNiEncodingType messageEncoding;    // the encoding type for message
    char extras[GNSS_NI_MESSAGE_ID_MAX];
} GnssNiNotification;

typedef struct {
    uint32_t size;       // set to sizeof(GnssSv)
    uint16_t svId;     // Unique Identifier
    GnssSvType type;   // type of SV (GPS, SBAS, GLONASS, QZSS, BEIDOU, GALILEO)
    float cN0Dbhz;     // signal strength
    float elevation;   // elevation of SV (in degrees)
    float azimuth;     // azimuth of SV (in degrees)
    GnssSvOptionsMask gnssSvOptionsMask; // Bitwise OR of GnssSvOptionsBits
    float carrierFrequencyHz; // carrier frequency of the signal tracked
    GnssSignalTypeMask gnssSignalTypeMask; // Specifies GNSS signal type
} GnssSv;

struct GnssConfigSetAssistanceServer {
    uint32_t size;             // set to sizeof(GnssConfigSetAssistanceServer)
    GnssAssistanceType type; // SUPL or C2K
    const char* hostName;    // null terminated string
    uint32_t port;           // port of server

    inline bool equals(const GnssConfigSetAssistanceServer& config) {
        if (config.type == type && config.port == port &&
               ((NULL == config.hostName && NULL == hostName) ||
                (NULL != config.hostName && NULL != hostName &&
                     0 == strcmp(config.hostName, hostName)))) {
            return true;
        }
        return false;
    }
};

typedef struct {
    uint32_t size;                               // set to sizeof(GnssMeasurementsData)
    GnssMeasurementsDataFlagsMask flags;       // bitwise OR of GnssMeasurementsDataFlagsBits
    int16_t svId;
    GnssSvType svType;
    double timeOffsetNs;
    GnssMeasurementsStateMask stateMask;       // bitwise OR of GnssMeasurementsStateBits
    int64_t receivedSvTimeNs;
    int64_t receivedSvTimeUncertaintyNs;
    double carrierToNoiseDbHz;
    double pseudorangeRateMps;
    double pseudorangeRateUncertaintyMps;
    GnssMeasurementsAdrStateMask adrStateMask; // bitwise OR of GnssMeasurementsAdrStateBits
    double adrMeters;
    double adrUncertaintyMeters;
    float carrierFrequencyHz;
    int64_t carrierCycles;
    double carrierPhase;
    double carrierPhaseUncertainty;
    GnssMeasurementsMultipathIndicator multipathIndicator;
    double signalToNoiseRatioDb;
    double agcLevelDb;
    GnssMeasurementsCodeType codeType;
    char otherCodeTypeName[GNSS_MAX_NAME_LENGTH];
} GnssMeasurementsData;

typedef struct {
    uint32_t size;                          // set to sizeof(GnssMeasurementsClock)
    GnssMeasurementsClockFlagsMask flags; // bitwise OR of GnssMeasurementsClockFlagsBits
    int16_t leapSecond;
    int64_t timeNs;
    double timeUncertaintyNs;
    int64_t fullBiasNs;
    double biasNs;
    double biasUncertaintyNs;
    double driftNsps;
    double driftUncertaintyNsps;
    uint32_t hwClockDiscontinuityCount;
} GnssMeasurementsClock;

typedef struct {
    uint32_t size;                 // set to sizeof(GnssSvNotification)
    uint32_t count;                // number of SVs in the GnssSv array
    bool gnssSignalTypeMaskValid;
    GnssSv gnssSvs[GNSS_SV_MAX]; // information on a number of SVs
} GnssSvNotification;

typedef struct {
    uint32_t size;         // set to sizeof(GnssNmeaNotification)
    uint64_t timestamp;  // timestamp
    const char* nmea;    // nmea text
    uint32_t length;       // length of the nmea text
} GnssNmeaNotification;

typedef struct {
    uint32_t size;                 // set to sizeof(GnssDataNotification)
    GnssDataMask  gnssDataMask[GNSS_LOC_MAX_NUMBER_OF_SIGNAL_TYPES];  // bitwise OR of GnssDataBits
    double        jammerInd[GNSS_LOC_MAX_NUMBER_OF_SIGNAL_TYPES];     // Jammer Indication
    double        agc[GNSS_LOC_MAX_NUMBER_OF_SIGNAL_TYPES];           // Automatic gain control
} GnssDataNotification;

typedef struct {
    uint32_t size;         // set to sizeof(GnssMeasurementsNotification)
    uint32_t count;        // number of items in GnssMeasurements array
    GnssMeasurementsData measurements[GNSS_MEASUREMENTS_MAX];
    GnssMeasurementsClock clock; // clock
} GnssMeasurementsNotification;

typedef uint32_t GnssSvId;

struct GnssSvIdSource{
    uint32_t size;              // set to sizeof(GnssSvIdSource)
    GnssSvType constellation;   // constellation for the sv to blacklist
    GnssSvId svId;             // sv id to blacklist
};
inline bool operator ==(GnssSvIdSource const& left, GnssSvIdSource const& right) {
    return left.size == right.size &&
            left.constellation == right.constellation && left.svId == right.svId;
}

#define GNSS_SV_CONFIG_ALL_BITS_ENABLED_MASK ((uint64_t)0xFFFFFFFFFFFFFFFF)
typedef struct {
    uint32_t size; // set to sizeof(GnssSvIdConfig)

    // GLONASS - SV 65 maps to bit 0
#define GNSS_SV_CONFIG_GLO_INITIAL_SV_ID 65
    uint64_t gloBlacklistSvMask;

    // BEIDOU - SV 201 maps to bit 0
#define GNSS_SV_CONFIG_BDS_INITIAL_SV_ID 201
    uint64_t bdsBlacklistSvMask;

    // QZSS - SV 193 maps to bit 0
#define GNSS_SV_CONFIG_QZSS_INITIAL_SV_ID 193
    uint64_t qzssBlacklistSvMask;

    // GAL - SV 301 maps to bit 0
#define GNSS_SV_CONFIG_GAL_INITIAL_SV_ID 301
    uint64_t galBlacklistSvMask;
} GnssSvIdConfig;

struct GnssConfig{
    uint32_t size;  // set to sizeof(GnssConfig)
    GnssConfigFlagsMask flags; // bitwise OR of GnssConfigFlagsBits to mark which params are valid
    GnssConfigGpsLock gpsLock;
    GnssConfigSuplVersion suplVersion;
    GnssConfigSetAssistanceServer assistanceServer;
    GnssConfigLppProfile lppProfile;
    GnssConfigLppeControlPlaneMask lppeControlPlaneMask;
    GnssConfigLppeUserPlaneMask lppeUserPlaneMask;
    GnssConfigAGlonassPositionProtocolMask aGlonassPositionProtocolMask;
    GnssConfigEmergencyPdnForEmergencySupl emergencyPdnForEmergencySupl;
    GnssConfigSuplEmergencyServices suplEmergencyServices;
    GnssConfigSuplModeMask suplModeMask; //bitwise OR of GnssConfigSuplModeBits
    std::vector<GnssSvIdSource> blacklistedSvIds;
    uint32_t emergencyExtensionSeconds;

    inline bool equals(const GnssConfig& config) {
        if (flags == config.flags &&
                gpsLock == config.gpsLock &&
                suplVersion == config.suplVersion &&
                assistanceServer.equals(config.assistanceServer) &&
                lppProfile == config.lppProfile &&
                lppeControlPlaneMask == config.lppeControlPlaneMask &&
                lppeUserPlaneMask == config.lppeUserPlaneMask &&
                aGlonassPositionProtocolMask == config.aGlonassPositionProtocolMask &&
                emergencyPdnForEmergencySupl == config.emergencyPdnForEmergencySupl &&
                suplEmergencyServices == config.suplEmergencyServices &&
                suplModeMask == config.suplModeMask  &&
                blacklistedSvIds == config.blacklistedSvIds &&
                emergencyExtensionSeconds == config.emergencyExtensionSeconds) {
            return true;
        }
        return false;
    }
};

typedef struct {
    uint32_t size;                        // set to sizeof
    bool                                mValid;
    Location                            mLocation;
    double                              verticalAccuracyMeters;
    double                              speedAccuracyMetersPerSecond;
    double                              bearingAccuracyDegrees;
    timespec                            mUtcReported;
} GnssDebugLocation;

typedef struct {
    uint32_t size;                        // set to sizeof
    bool                                mValid;
    int64_t                             timeEstimate;
    float                               timeUncertaintyNs;
    float                               frequencyUncertaintyNsPerSec;
} GnssDebugTime;

typedef struct {
    uint32_t size;                        // set to sizeof
    uint32_t                            svid;
    GnssSvType                          constellation;
    GnssEphemerisType                   mEphemerisType;
    GnssEphemerisSource                 mEphemerisSource;
    GnssEphemerisHealth                 mEphemerisHealth;
    float                               ephemerisAgeSeconds;
    bool                                serverPredictionIsAvailable;
    float                               serverPredictionAgeSeconds;
} GnssDebugSatelliteInfo;

typedef struct {
    uint32_t size;                        // set to sizeof
    GnssDebugLocation                   mLocation;
    GnssDebugTime                       mTime;
    std::vector<GnssDebugSatelliteInfo> mSatelliteInfo;
} GnssDebugReport;

typedef uint32_t LeapSecondSysInfoMask;
typedef enum {
    // current leap second info is available. This info will only
    // be available if the leap second change info is not available.
    //
    // If leap second change info is avaiable, to figure out
    // the current leap second info, compare current gps time with
    // the gps timestamp of leap second change to know whether to choose
    // leapSecondBefore or leapSecondAfter as current leap second.
    LEAP_SECOND_SYS_INFO_CURRENT_LEAP_SECONDS_BIT = (1ULL << 0),
    // the last known leap change event is available.
    // The info can be available on two scenario:
    // 1: this leap second change event has been scheduled and yet to happen
    // 2: this leap second change event has already happened and next
    //    leap second change event has not yet been scheduled.
    LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT = (1ULL << 1),
} LeapSecondSysInfoDataBits;

struct LeapSecondChangeInfo {
    // GPS timestamp that corrresponds to the last known
    // leap second change event.
    //
    // The info can be available on two scenario:
    // 1: this leap second change event has been scheduled and yet to happen
    // 2: this leap second change event has already happened and next
    //    leap second change event has not yet been scheduled.
    GnssSystemTimeStructType gpsTimestampLsChange;
    // Number of leap seconds prior to the leap second change event
    // that corresponds to the timestamp at gpsTimestampLsChange.
    uint8_t leapSecondsBeforeChange;
    // Number of leap seconds after the leap second change event
    // that corresponds to the timestamp at gpsTimestampLsChange.
    uint8_t leapSecondsAfterChange;
};

struct LeapSecondSystemInfo {
    LeapSecondSysInfoMask leapSecondInfoMask;
    uint8_t               leapSecondCurrent;
    LeapSecondChangeInfo  leapSecondChangeInfo;
};

typedef uint32_t LocationSystemInfoMask;
typedef enum {
    // contains current leap second or leap second change info
    LOCATION_SYS_INFO_LEAP_SECOND = (1ULL << 0),
} LocationSystemInfoDataBits;

struct LocationSystemInfo {
    LocationSystemInfoMask systemInfoMask;
    LeapSecondSystemInfo   leapSecondSysInfo;
};

/* Provides the capabilities of the system
   capabilities callback is called once soon after createInstance is called */
typedef std::function<void(
    LocationCapabilitiesMask capabilitiesMask // bitwise OR of LocationCapabilitiesBits
)> capabilitiesCallback;

/* Used by tracking, batching, and miscellanous APIs
   responseCallback is called for every Tracking, Batching API, and Miscellanous API */
typedef std::function<void(
    LocationError err, // if not SUCCESS, then id is not valid
    uint32_t id        // id to be associated to the request
)> responseCallback;

/* Used by APIs that gets more than one LocationError in it's response
   collectiveResponseCallback is called for every geofence API call.
   ids array and LocationError array are only valid until collectiveResponseCallback returns. */
typedef std::function<void(
    uint32_t count, // number of locations in arrays
    LocationError* errs, // array of LocationError associated to the request
    uint32_t* ids // array of ids to be associated to the request
)> collectiveResponseCallback;

/* Used for startTracking API, optional can be NULL
   trackingCallback is called when delivering a location in a tracking session
   broadcasted to all clients, no matter if a session has started by client */
typedef std::function<void(
    Location location
)> trackingCallback;

/* Used for startBatching API, optional can be NULL
   batchingCallback is called when delivering locations in a batching session.
   broadcasted to all clients, no matter if a session has started by client */
typedef std::function<void(
    uint32_t count,      // number of locations in array
    Location* location, // array of locations
    BatchingOptions batchingOptions // Batching options
)> batchingCallback;

typedef std::function<void(
    BatchingStatusInfo batchingStatus, // batch status
    std::list<uint32_t> & listOfCompletedTrips
)> batchingStatusCallback;

/* Gives GNSS Location information, optional can be NULL
    gnssLocationInfoCallback is called only during a tracking session
    broadcasted to all clients, no matter if a session has started by client */
typedef std::function<void(
    GnssLocationInfoNotification gnssLocationInfoNotification
)> gnssLocationInfoCallback;

/* Gives default combined location information from all engines and
   location information individually from selected engines.
   This callback is only used when there are multiple engines
   running in the system.

   optional can be NULL

   engineLocationsInfoCallback is called only during a tracking session
   broadcasted to all clients, no matter if a session has started by client */
typedef std::function<void(
    uint32_t count,
    GnssLocationInfoNotification* engineLocationInfoNotification
)> engineLocationsInfoCallback;

/* Used for addGeofences API, optional can be NULL
   geofenceBreachCallback is called when any number of geofences have a state change */
typedef std::function<void(
    GeofenceBreachNotification geofenceBreachNotification
)> geofenceBreachCallback;

/* Used for addGeofences API, optional can be NULL
       geofenceStatusCallback is called when any number of geofences have a status change */
typedef std::function<void(
    GeofenceStatusNotification geofenceStatusNotification
)> geofenceStatusCallback;

/* Network Initiated request, optional can be NULL
   This callback should be responded to by calling gnssNiResponse */
typedef std::function<void(
    uint32_t id, // id that should be used to respond by calling gnssNiResponse
    GnssNiNotification gnssNiNotification
)> gnssNiCallback;

/* Gives GNSS SV information, optional can be NULL
    gnssSvCallback is called only during a tracking session
    broadcasted to all clients, no matter if a session has started by client */
typedef std::function<void(
    GnssSvNotification gnssSvNotification
)> gnssSvCallback;

/* Gives GNSS NMEA data, optional can be NULL
    gnssNmeaCallback is called only during a tracking session
    broadcasted to all clients, no matter if a session has started by client */
typedef std::function<void(
    GnssNmeaNotification gnssNmeaNotification
)> gnssNmeaCallback;

/* Gives GNSS data, optional can be NULL
    gnssDataCallback is called only during a tracking session
    broadcasted to all clients, no matter if a session has started by client */
typedef std::function<void(
    GnssDataNotification gnssDataNotification
)> gnssDataCallback;

/* Gives GNSS Measurements information, optional can be NULL
    gnssMeasurementsCallback is called only during a tracking session
    broadcasted to all clients, no matter if a session has started by client */
typedef std::function<void(
    GnssMeasurementsNotification gnssMeasurementsNotification
)> gnssMeasurementsCallback;

/* Provides the current GNSS configuration to the client */
typedef std::function<void(
    GnssConfig& config
)> gnssConfigCallback;

/* LocationSystemInfoCb is for receiving rare occuring location
   system information update. optional, can be NULL.
*/
typedef std::function<void(
    LocationSystemInfo locationSystemInfo
)> locationSystemInfoCallback;

typedef std::function<void(
)> locationApiDestroyCompleteCallback;

typedef uint16_t LocationAdapterTypeMask;
typedef enum {
    LOCATION_ADAPTER_GNSS_TYPE_BIT      = (1<<0), // adapter type is GNSS
    LOCATION_ADAPTER_BATCHING_TYPE_BIT  = (1<<1), // adapter type is BATCHING
    LOCATION_ADAPTER_GEOFENCE_TYPE_BIT  = (1<<2)  // adapter type is geo fence
} LocationAdapterTypeBits;

typedef struct {
    uint32_t size; // set to sizeof(LocationCallbacks)
    capabilitiesCallback capabilitiesCb;             // mandatory
    responseCallback responseCb;                     // mandatory
    collectiveResponseCallback collectiveResponseCb; // mandatory
    trackingCallback trackingCb;                     // optional
    batchingCallback batchingCb;                     // optional
    geofenceBreachCallback geofenceBreachCb;         // optional
    geofenceStatusCallback geofenceStatusCb;         // optional
    gnssLocationInfoCallback gnssLocationInfoCb;     // optional
    gnssNiCallback gnssNiCb;                         // optional
    gnssSvCallback gnssSvCb;                         // optional
    gnssNmeaCallback gnssNmeaCb;                     // optional
    gnssDataCallback gnssDataCb;                     // optional
    gnssMeasurementsCallback gnssMeasurementsCb;     // optional
    batchingStatusCallback batchingStatusCb;         // optional
    locationSystemInfoCallback locationSystemInfoCb; // optional
    engineLocationsInfoCallback engineLocationsInfoCb;     // optional
} LocationCallbacks;

#endif /* LOCATIONDATATYPES_H */
