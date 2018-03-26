/* Copyright (c) 2017 The Linux Foundation. All rights reserved.
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

#ifndef LOCATION_H
#define LOCATION_H

#include <vector>
#include <stdint.h>
#include <functional>
#include <list>

#define GNSS_NI_REQUESTOR_MAX  256
#define GNSS_NI_MESSAGE_ID_MAX 2048
#define GNSS_SV_MAX 64
#define GNSS_MEASUREMENTS_MAX 64
#define GNSS_UTC_TIME_OFFSET   (3657)

#define GNSS_BUGREPORT_GPS_MIN  (1)
#define GNSS_BUGREPORT_SBAS_MIN (120)
#define GNSS_BUGREPORT_GLO_MIN  (1)
#define GNSS_BUGREPORT_QZSS_MIN (193)
#define GNSS_BUGREPORT_BDS_MIN  (1)
#define GNSS_BUGREPORT_GAL_MIN  (1)

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
} LocationFlagsBits;

typedef uint16_t LocationTechnologyMask;
typedef enum {
    LOCATION_TECHNOLOGY_GNSS_BIT     = (1<<0), // location was calculated using GNSS
    LOCATION_TECHNOLOGY_CELL_BIT     = (1<<1), // location was calculated using Cell
    LOCATION_TECHNOLOGY_WIFI_BIT     = (1<<2), // location was calculated using WiFi
    LOCATION_TECHNOLOGY_SENSORS_BIT  = (1<<3), // location was calculated using Sensors
} LocationTechnologyBits;

typedef enum {
    LOCATION_RELIABILITY_NOT_SET = 0,
    LOCATION_RELIABILITY_VERY_LOW,
    LOCATION_RELIABILITY_LOW,
    LOCATION_RELIABILITY_MEDIUM,
    LOCATION_RELIABILITY_HIGH,
} LocationReliability;

typedef uint32_t GnssLocationInfoFlagMask;
typedef enum {
    GNSS_LOCATION_INFO_ALTITUDE_MEAN_SEA_LEVEL_BIT      = (1<<0), // valid altitude mean sea level
    GNSS_LOCATION_INFO_DOP_BIT                          = (1<<1), // valid pdop, hdop, and vdop
    GNSS_LOCATION_INFO_MAGNETIC_DEVIATION_BIT           = (1<<2), // valid magnetic deviation
    GNSS_LOCATION_INFO_HOR_RELIABILITY_BIT              = (1<<3), // valid horizontal reliability
    GNSS_LOCATION_INFO_VER_RELIABILITY_BIT              = (1<<4), // valid vertical reliability
    GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MAJOR_BIT = (1<<5), // valid elipsode semi major
    GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_SEMI_MINOR_BIT = (1<<6), // valid elipsode semi minor
    GNSS_LOCATION_INFO_HOR_ACCURACY_ELIP_AZIMUTH_BIT    = (1<<7),// valid accuracy elipsode azimuth
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
    LOCATION_CAPABILITIES_OUTDOOR_TRIP_BATCHING_BIT         = (1<<9)
} LocationCapabilitiesBits;

typedef enum {
    LOCATION_TECHNOLOGY_TYPE_GNSS = 0,
} LocationTechnologyType;

// Configures how GPS is locked when GPS is disabled (through GnssDisable)
typedef enum {
    GNSS_CONFIG_GPS_LOCK_NONE = 0, // gps is not locked when GPS is disabled (GnssDisable)
    GNSS_CONFIG_GPS_LOCK_MO,       // gps mobile originated (MO) is locked when GPS is disabled
    GNSS_CONFIG_GPS_LOCK_NI,       // gps network initiated (NI) is locked when GPS is disabled
    GNSS_CONFIG_GPS_LOCK_MO_AND_NI,// gps MO and NI is locked when GPS is disabled
} GnssConfigGpsLock;

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
    GNSS_SV_OPTIONS_HAS_EPHEMER_BIT = (1<<0),
    GNSS_SV_OPTIONS_HAS_ALMANAC_BIT = (1<<1),
    GNSS_SV_OPTIONS_USED_IN_FIX_BIT = (1<<2),
} GnssSvOptionsBits;

typedef enum {
    GNSS_ASSISTANCE_TYPE_SUPL = 0,
    GNSS_ASSISTANCE_TYPE_C2K,
} GnssAssistanceType;

typedef enum {
    GNSS_SUPL_MODE_STANDALONE = 0,
    GNSS_SUPL_MODE_MSB,
    GNSS_SUPL_MODE_MSA,
} GnssSuplMode;

typedef enum {
    BATCHING_MODE_ROUTINE = 0,
    BATCHING_MODE_TRIP
} BatchingMode;

typedef enum {
    BATCHING_STATUS_TRIP_COMPLETED = 0,
    BATCHING_STATUS_POSITION_AVAILABE,
    BATCHING_STATUS_POSITION_UNAVAILABLE
} BatchingStatus;

typedef uint16_t GnssMeasurementsAdrStateMask;
typedef enum {
    GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_UNKNOWN         = 0,
    GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_VALID_BIT       = (1<<0),
    GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_RESET_BIT       = (1<<1),
    GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_CYCLE_SLIP_BIT  = (1<<2),
} GnssMeasurementsAdrStateBits;

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
} GnssAidingDataSvBits;

typedef uint32_t GnssAidingDataSvTypeMask;
typedef enum {
    GNSS_AIDING_DATA_SV_TYPE_GPS_BIT      = (1<<0),
    GNSS_AIDING_DATA_SV_TYPE_GLONASS_BIT  = (1<<1),
    GNSS_AIDING_DATA_SV_TYPE_QZSS_BIT     = (1<<2),
    GNSS_AIDING_DATA_SV_TYPE_BEIDOU_BIT   = (1<<3),
    GNSS_AIDING_DATA_SV_TYPE_GALILEO_BIT  = (1<<4),
} GnssAidingDataSvTypeBits;

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
} GnssAidingData;

typedef struct {
    size_t size;             // set to sizeof(Location)
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
} Location;

typedef struct  {
    size_t size;          // set to sizeof(LocationOptions)
    uint32_t minInterval; // in milliseconds
    uint32_t minDistance; // in meters. if minDistance > 0, gnssSvCallback/gnssNmeaCallback/
                          // gnssMeasurementsCallback may not be called
    GnssSuplMode mode;    // Standalone/MS-Based/MS-Assisted
} LocationOptions;

typedef struct {
    size_t size;
    BatchingMode batchingMode;
} BatchingOptions;

typedef struct {
    size_t size;
    BatchingStatus batchingStatus;
} BatchingStatusInfo;

typedef struct {
    size_t size;                            // set to sizeof(GeofenceOption)
    GeofenceBreachTypeMask breachTypeMask;  // bitwise OR of GeofenceBreachTypeBits
    uint32_t responsiveness;                // in milliseconds
    uint32_t dwellTime;                     // in seconds
} GeofenceOption;

typedef struct  {
    size_t size;      // set to sizeof(GeofenceInfo)
    double latitude;  // in degrees
    double longitude; // in degrees
    double radius;    // in meters
} GeofenceInfo;

typedef struct {
    size_t size;             // set to sizeof(GeofenceBreachNotification)
    size_t count;            // number of ids in array
    uint32_t* ids;           // array of ids that have breached
    Location location;       // location associated with breach
    GeofenceBreachType type; // type of breach
    uint64_t timestamp;      // timestamp of breach
} GeofenceBreachNotification;

typedef struct {
    size_t size;                       // set to sizeof(GeofenceBreachNotification)
    GeofenceStatusAvailable available; // GEOFENCE_STATUS_AVAILABILE_NO/_YES
    LocationTechnologyType techType;   // GNSS
} GeofenceStatusNotification;

typedef struct {
    size_t size;                        // set to sizeof(GnssLocationInfo)
    GnssLocationInfoFlagMask flags;     // bitwise OR of GnssLocationInfoBits for param validity
    float altitudeMeanSeaLevel;         // altitude wrt mean sea level
    float pdop;                         // position dilusion of precision
    float hdop;                         // horizontal dilusion of precision
    float vdop;                         // vertical dilusion of precision
    float magneticDeviation;            // magnetic deviation
    LocationReliability horReliability; // horizontal reliability
    LocationReliability verReliability; // vertical reliability
    float horUncEllipseSemiMajor;       // horizontal elliptical accuracy semi-major axis
    float horUncEllipseSemiMinor;       // horizontal elliptical accuracy semi-minor axis
    float horUncEllipseOrientAzimuth;   // horizontal elliptical accuracy azimuth
} GnssLocationInfoNotification;

typedef struct {
    size_t size;                           // set to sizeof(GnssNiNotification)
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
    size_t size;       // set to sizeof(GnssSv)
    uint16_t svId;     // Unique Identifier
    GnssSvType type;   // type of SV (GPS, SBAS, GLONASS, QZSS, BEIDOU, GALILEO)
    float cN0Dbhz;     // signal strength
    float elevation;   // elevation of SV (in degrees)
    float azimuth;     // azimuth of SV (in degrees)
    GnssSvOptionsMask gnssSvOptionsMask; // Bitwise OR of GnssSvOptionsBits
} GnssSv;

typedef struct {
    size_t size;             // set to sizeof(GnssConfigSetAssistanceServer)
    GnssAssistanceType type; // SUPL or C2K
    const char* hostName;    // null terminated string
    uint32_t port;           // port of server
} GnssConfigSetAssistanceServer;

typedef struct {
    size_t size;                               // set to sizeof(GnssMeasurementsData)
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
} GnssMeasurementsData;

typedef struct {
    size_t size;                          // set to sizeof(GnssMeasurementsClock)
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
    size_t size;                 // set to sizeof(GnssSvNotification)
    size_t count;                // number of SVs in the GnssSv array
    GnssSv gnssSvs[GNSS_SV_MAX]; // information on a number of SVs
} GnssSvNotification;

typedef struct {
    size_t size;         // set to sizeof(GnssNmeaNotification)
    uint64_t timestamp;  // timestamp
    const char* nmea;    // nmea text
    size_t length;       // length of the nmea text
} GnssNmeaNotification;

typedef struct {
    size_t size;         // set to sizeof(GnssMeasurementsNotification)
    size_t count;        // number of items in GnssMeasurements array
    GnssMeasurementsData measurements[GNSS_MEASUREMENTS_MAX];
    GnssMeasurementsClock clock; // clock
} GnssMeasurementsNotification;

typedef struct {
    size_t size;  // set to sizeof(GnssConfig)
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
} GnssConfig;

typedef struct {
    size_t size;                        // set to sizeof
    bool                                mValid;
    Location                            mLocation;
    double                              verticalAccuracyMeters;
    double                              speedAccuracyMetersPerSecond;
    double                              bearingAccuracyDegrees;
    timespec                            mUtcReported;
} GnssDebugLocation;

typedef struct {
    size_t size;                        // set to sizeof
    bool                                mValid;
    int64_t                             timeEstimate;
    float                               timeUncertaintyNs;
    float                               frequencyUncertaintyNsPerSec;
} GnssDebugTime;

typedef struct {
    size_t size;                        // set to sizeof
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
    size_t size;                        // set to sizeof
    GnssDebugLocation                   mLocation;
    GnssDebugTime                       mTime;
    std::vector<GnssDebugSatelliteInfo> mSatelliteInfo;
} GnssDebugReport;

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
    size_t count, // number of locations in arrays
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
    size_t count,      // number of locations in array
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

/* Gives GNSS Measurements information, optional can be NULL
    gnssMeasurementsCallback is called only during a tracking session
    broadcasted to all clients, no matter if a session has started by client */
typedef std::function<void(
    GnssMeasurementsNotification gnssMeasurementsNotification
)> gnssMeasurementsCallback;

typedef struct {
    size_t size; // set to sizeof(LocationCallbacks)
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
    gnssMeasurementsCallback gnssMeasurementsCb;     // optional
    batchingStatusCallback batchingStatusCb;         // optional
} LocationCallbacks;

class LocationAPI
{
private:
    LocationAPI();
    ~LocationAPI();

public:
    /* creates an instance to LocationAPI object.
       Will return NULL if mandatory parameters are invalid or if the maximum number
       of instances have been reached */
    static LocationAPI* createInstance(LocationCallbacks&);

    /* destroy/cleans up the instance, which should be called when LocationAPI object is
       no longer needed. LocationAPI* returned from createInstance will no longer valid
       after destroy is called */
    void destroy();

    /* updates/changes the callbacks that will be called.
        mandatory callbacks must be present for callbacks to be successfully updated
        no return value */
    void updateCallbacks(LocationCallbacks&);

    /* ================================== TRACKING ================================== */

    /* startTracking starts a tracking session, which returns a session id that will be
       used by the other tracking APIs and also in the responseCallback to match command
       with response. locations are reported on the trackingCallback passed in createInstance
       periodically according to LocationOptions.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successfully started
                LOCATION_ERROR_ALREADY_STARTED if a startTracking session is already in progress
                LOCATION_ERROR_CALLBACK_MISSING if no trackingCallback was passed in createInstance
                LOCATION_ERROR_INVALID_PARAMETER if LocationOptions parameter is invalid */
    uint32_t startTracking(LocationOptions&); // returns session id

    /* stopTracking stops a tracking session associated with id parameter.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a tracking session */
    void stopTracking(uint32_t id);

    /* updateTrackingOptions changes the LocationOptions of a tracking session associated with id
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_INVALID_PARAMETER if LocationOptions parameters are invalid
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a tracking session */
    void updateTrackingOptions(uint32_t id, LocationOptions&);

    /* ================================== BATCHING ================================== */

    /* startBatching starts a batching session, which returns a session id that will be
       used by the other batching APIs and also in the responseCallback to match command
       with response. locations are reported on the batchingCallback passed in createInstance
       periodically according to LocationOptions. A batching session starts tracking on
       the low power processor and delivers them in batches by the batchingCallback when
       the batch is full or when getBatchedLocations is called. This allows for the processor
       that calls this API to sleep when the low power processor can batch locations in the
       backgroup and wake up the processor calling the API only when the batch is full, thus
       saving power
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successful
                LOCATION_ERROR_ALREADY_STARTED if a startBatching session is already in progress
                LOCATION_ERROR_CALLBACK_MISSING if no batchingCallback was passed in createInstance
                LOCATION_ERROR_INVALID_PARAMETER if a parameter is invalid
                LOCATION_ERROR_NOT_SUPPORTED if batching is not supported */
    uint32_t startBatching(LocationOptions&, BatchingOptions&); // returns session id

    /* stopBatching stops a batching session associated with id parameter.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with batching session */
    void stopBatching(uint32_t id);

    /* updateBatchingOptions changes the LocationOptions of a batching session associated with id
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_INVALID_PARAMETER if LocationOptions parameters are invalid
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a batching session */
    void updateBatchingOptions(uint32_t id, LocationOptions&, BatchingOptions&);

    /* getBatchedLocations gets a number of locations that are currently stored/batched
       on the low power processor, delivered by the batchingCallback passed in createInstance.
       Location are then deleted from the batch stored on the low power processor.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful, will be followed by batchingCallback call
                LOCATION_ERROR_CALLBACK_MISSING if no batchingCallback was passed in createInstance
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a batching session */
    void getBatchedLocations(uint32_t id, size_t count);

    /* ================================== GEOFENCE ================================== */

    /* addGeofences adds any number of geofences and returns an array of geofence ids that
       will be used by the other geofence APIs and also in the collectiveResponseCallback to
       match command with response. The geofenceBreachCallback will deliver the status of each
       geofence according to the GeofenceOption for each. The geofence id array returned will
       be valid until the collectiveResponseCallback is called and has returned.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successful
                LOCATION_ERROR_CALLBACK_MISSING if no geofenceBreachCallback
                LOCATION_ERROR_INVALID_PARAMETER if any parameters are invalid
                LOCATION_ERROR_NOT_SUPPORTED if geofence is not supported */
    uint32_t* addGeofences(size_t count, GeofenceOption*, GeofenceInfo*); // returns id array

    /* removeGeofences removes any number of geofences. Caller should delete ids array after
       removeGeofences returneds.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a geofence session */
    void removeGeofences(size_t count, uint32_t* ids);

    /* modifyGeofences modifies any number of geofences. Caller should delete ids array after
       modifyGeofences returns.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a geofence session
                LOCATION_ERROR_INVALID_PARAMETER if any parameters are invalid */
    void modifyGeofences(size_t count, uint32_t* ids, GeofenceOption* options);

    /* pauseGeofences pauses any number of geofences, which is similar to removeGeofences,
       only that they can be resumed at any time. Caller should delete ids array after
       pauseGeofences returns.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a geofence session */
    void pauseGeofences(size_t count, uint32_t* ids);

    /* resumeGeofences resumes any number of geofences that are currently paused. Caller should
       delete ids array after resumeGeofences returns.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a geofence session */
    void resumeGeofences(size_t count, uint32_t* ids);

    /* ================================== GNSS ====================================== */

     /* gnssNiResponse is called in response to a gnssNiCallback.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successful
                LOCATION_ERROR_INVALID_PARAMETER if any parameters in GnssNiResponse are invalid
                LOCATION_ERROR_ID_UNKNOWN if id does not match a gnssNiCallback */
    void gnssNiResponse(uint32_t id, GnssNiResponse response);
};

typedef struct {
    size_t size; // set to sizeof(LocationControlCallbacks)
    responseCallback responseCb;                     // mandatory
    collectiveResponseCallback collectiveResponseCb; // mandatory
} LocationControlCallbacks;

class LocationControlAPI
{
private:
    LocationControlAPI();
    ~LocationControlAPI();

public:
    /* creates an instance to LocationControlAPI object.
       Will return NULL if mandatory parameters are invalid or if the maximum number
       of instances have been reached. Only once instance allowed */
    static LocationControlAPI* createInstance(LocationControlCallbacks&);

    /* destroy/cleans up the instance, which should be called when LocationControlAPI object is
       no longer needed. LocationControlAPI* returned from createInstance will no longer valid
       after destroy is called */
    void destroy();

    /* enable will enable specific location technology to be used for calculation locations and
       will effectively start a control session if call is successful, which returns a session id
       that will be returned in responseCallback to match command with response. The session id is
       also needed to call the disable command.
       This effect is global for all clients of LocationAPI
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ALREADY_STARTED if an enable was already called for this techType
                LOCATION_ERROR_INVALID_PARAMETER if any parameters are invalid
                LOCATION_ERROR_GENERAL_FAILURE if failure for any other reason */
    uint32_t enable(LocationTechnologyType techType);

    /* disable will disable specific location technology to be used for calculation locations and
       effectively ends the control session if call is successful.
       id parameter is the session id that was returned in enable responseCallback for techType.
       The session id is no longer valid after disable's responseCallback returns success.
       This effect is global for all clients of LocationAPI
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id was not returned from responseCallback from enable
                LOCATION_ERROR_GENERAL_FAILURE if failure for any other reason */
    void disable(uint32_t id);

    /* gnssUpdateConfig updates the gnss specific configuration, which returns a session id array
       with an id for each of the bits set in GnssConfig.flags, order from low bits to high bits.
       The response for each config that is set will be returned in collectiveResponseCallback.
       The session id array returned will be valid until the collectiveResponseCallback is called
       and has returned. This effect is global for all clients of LocationAPI
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successful
                LOCATION_ERROR_INVALID_PARAMETER if any other parameters are invalid
                LOCATION_ERROR_GENERAL_FAILURE if failure for any other reason */
    uint32_t* gnssUpdateConfig(GnssConfig config);

    /* delete specific gnss aiding data for testing, which returns a session id
       that will be returned in responseCallback to match command with response.
       Only allowed in userdebug builds. This effect is global for all clients of LocationAPI
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_INVALID_PARAMETER if any parameters are invalid
                LOCATION_ERROR_NOT_SUPPORTED if build is not userdebug */
    uint32_t gnssDeleteAidingData(GnssAidingData& data);
};

#endif /* LOCATION_H */
