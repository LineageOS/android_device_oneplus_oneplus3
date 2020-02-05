/* Copyright (c) 2013-2019 The Linux Foundation. All rights reserved.
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

#ifndef GPS_EXTENDED_C_H
#define GPS_EXTENDED_C_H

#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <loc_gps.h>
#include <LocationAPI.h>

struct timespec32_t {
  uint32_t  tv_sec;   /* seconds */
  uint32_t  tv_nsec;  /* and nanoseconds */
};


/**
 * @file
 * @brief C++ declarations for GPS types
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Location has valid source information. */
#define LOCATION_HAS_SOURCE_INFO   0x0020
/** LocGpsLocation has valid "is indoor?" flag */
#define LOC_GPS_LOCATION_HAS_IS_INDOOR   0x0040
/** LocGpsLocation has valid floor number */
#define LOC_GPS_LOCATION_HAS_FLOOR_NUMBER   0x0080
/** LocGpsLocation has valid map URL*/
#define LOC_GPS_LOCATION_HAS_MAP_URL   0x0100
/** LocGpsLocation has valid map index */
#define LOC_GPS_LOCATION_HAS_MAP_INDEX   0x0200

#define GNSS_INVALID_JAMMER_IND 0x7FFFFFFF

/** Sizes for indoor fields */
#define GPS_LOCATION_MAP_URL_SIZE 400
#define GPS_LOCATION_MAP_INDEX_SIZE 16

/** Position source is ULP */
#define ULP_LOCATION_IS_FROM_HYBRID   0x0001
/** Position source is GNSS only */
#define ULP_LOCATION_IS_FROM_GNSS     0x0002
/** Position is from a Geofence Breach Event */
#define ULP_LOCATION_IS_FROM_GEOFENCE 0X0004
/** Position is from Hardware FLP */
#define ULP_LOCATION_IS_FROM_HW_FLP   0x0008
/** Position is from NLP */
#define ULP_LOCATION_IS_FROM_NLP      0x0010
/** Position is from external DR solution*/
#define ULP_LOCATION_IS_FROM_EXT_DR   0X0020
/** Raw GNSS position fixes */
#define ULP_LOCATION_IS_FROM_GNSS_RAW   0X0040

typedef uint32_t LocSvInfoSource;
/** SVinfo source is GNSS/DR */
#define ULP_SVINFO_IS_FROM_GNSS       ((LocSvInfoSource)0x0001)
/** Raw SVinfo from GNSS */
#define ULP_SVINFO_IS_FROM_DR         ((LocSvInfoSource)0x0002)

#define ULP_MIN_INTERVAL_INVALID 0xffffffff
#define ULP_MAX_NMEA_STRING_SIZE 201

/*Emergency SUPL*/
#define LOC_GPS_NI_TYPE_EMERGENCY_SUPL    4

#define LOC_AGPS_CERTIFICATE_MAX_LENGTH 2000
#define LOC_AGPS_CERTIFICATE_MAX_SLOTS 10

/* TBM Threshold for tracking in background power mode : in millis */
#define TRACKING_TBM_THRESHOLD_MILLIS 480000

/**  Maximum number of satellites in an ephemeris report.  */
#define GNSS_EPHEMERIS_LIST_MAX_SIZE_V02 32

typedef uint32_t LocPosTechMask;
#define LOC_POS_TECH_MASK_DEFAULT ((LocPosTechMask)0x00000000)
#define LOC_POS_TECH_MASK_SATELLITE ((LocPosTechMask)0x00000001)
#define LOC_POS_TECH_MASK_CELLID ((LocPosTechMask)0x00000002)
#define LOC_POS_TECH_MASK_WIFI ((LocPosTechMask)0x00000004)
#define LOC_POS_TECH_MASK_SENSORS ((LocPosTechMask)0x00000008)
#define LOC_POS_TECH_MASK_REFERENCE_LOCATION ((LocPosTechMask)0x00000010)
#define LOC_POS_TECH_MASK_INJECTED_COARSE_POSITION ((LocPosTechMask)0x00000020)
#define LOC_POS_TECH_MASK_AFLT ((LocPosTechMask)0x00000040)
#define LOC_POS_TECH_MASK_HYBRID ((LocPosTechMask)0x00000080)
#define LOC_POS_TECH_MASK_PPE ((LocPosTechMask)0x00000100)

enum loc_registration_mask_status {
    LOC_REGISTRATION_MASK_ENABLED,
    LOC_REGISTRATION_MASK_DISABLED,
    LOC_REGISTRATION_MASK_SET
};

typedef enum {
    LOC_SUPPORTED_FEATURE_ODCPI_2_V02 = 0, /**<  Support ODCPI version 2 feature  */
    LOC_SUPPORTED_FEATURE_WIFI_AP_DATA_INJECT_2_V02, /**<  Support Wifi AP data inject version 2 feature  */
    LOC_SUPPORTED_FEATURE_DEBUG_NMEA_V02, /**< Support debug NMEA feature */
    LOC_SUPPORTED_FEATURE_GNSS_ONLY_POSITION_REPORT, /**< Support GNSS Only position reports */
    LOC_SUPPORTED_FEATURE_FDCL, /**< Support FDCL */
    LOC_SUPPORTED_FEATURE_CONSTELLATION_ENABLEMENT_V02, /**< Support constellation enablement */
    LOC_SUPPORTED_FEATURE_AGPM_V02, /**< Support AGPM feature */
    LOC_SUPPORTED_FEATURE_XTRA_INTEGRITY, /**< Support XTRA integrity */
    LOC_SUPPORTED_FEATURE_FDCL_2, /**< Support FDCL V2 */
    LOC_SUPPORTED_FEATURE_LOCATION_PRIVACY /**< Support location privacy */
} loc_supported_feature_enum;

typedef struct {
    /** set to sizeof(UlpLocation) */
    uint32_t          size;
    LocGpsLocation     gpsLocation;
    /* Provider indicator for HYBRID or GPS */
    uint16_t        position_source;
    LocPosTechMask  tech_mask;
    bool            unpropagatedPosition;
} UlpLocation;

typedef struct {
    /** set to sizeof(UlpNmea) */
    uint32_t          size;
    char            nmea_str[ULP_MAX_NMEA_STRING_SIZE];
    unsigned int    len;
} UlpNmea;


/** AGPS type */
typedef int8_t AGpsExtType;
#define LOC_AGPS_TYPE_INVALID       -1
#define LOC_AGPS_TYPE_ANY           0
#define LOC_AGPS_TYPE_SUPL          1
#define LOC_AGPS_TYPE_C2K           2
#define LOC_AGPS_TYPE_WWAN_ANY      3
#define LOC_AGPS_TYPE_WIFI          4
#define LOC_AGPS_TYPE_SUPL_ES       5

/** SSID length */
#define SSID_BUF_SIZE (32+1)

typedef int16_t AGpsBearerType;
#define AGPS_APN_BEARER_INVALID     0
#define AGPS_APN_BEARER_IPV4        1
#define AGPS_APN_BEARER_IPV6        2
#define AGPS_APN_BEARER_IPV4V6      3

typedef uint32_t LocApnTypeMask;
/**<  Denotes APN type for Default/Internet traffic  */
#define LOC_APN_TYPE_MASK_DEFAULT   ((LocApnTypeMask)0x00000001)
/**<  Denotes  APN type for IP Multimedia Subsystem  */
#define LOC_APN_TYPE_MASK_IMS       ((LocApnTypeMask)0x00000002)
/**<  Denotes APN type for Multimedia Messaging Service  */
#define LOC_APN_TYPE_MASK_MMS       ((LocApnTypeMask)0x00000004)
/**<  Denotes APN type for Dial Up Network  */
#define LOC_APN_TYPE_MASK_DUN       ((LocApnTypeMask)0x00000008)
/**<  Denotes APN type for Secure User Plane Location  */
#define LOC_APN_TYPE_MASK_SUPL      ((LocApnTypeMask)0x00000010)
/**<  Denotes APN type for High Priority Mobile Data  */
#define LOC_APN_TYPE_MASK_HIPRI     ((LocApnTypeMask)0x00000020)
/**<  Denotes APN type for over the air administration  */
#define LOC_APN_TYPE_MASK_FOTA      ((LocApnTypeMask)0x00000040)
/**<  Denotes APN type for Carrier Branded Services  */
#define LOC_APN_TYPE_MASK_CBS       ((LocApnTypeMask)0x00000080)
/**<  Denotes APN type for Initial Attach  */
#define LOC_APN_TYPE_MASK_IA        ((LocApnTypeMask)0x00000100)
/**<  Denotes APN type for emergency  */
#define LOC_APN_TYPE_MASK_EMERGENCY ((LocApnTypeMask)0x00000200)

typedef uint32_t AGpsTypeMask;
#define AGPS_ATL_TYPE_SUPL       ((AGpsTypeMask)0x00000001)
#define AGPS_ATL_TYPE_SUPL_ES   ((AGpsTypeMask)0x00000002)
#define AGPS_ATL_TYPE_WWAN       ((AGpsTypeMask)0x00000004)

typedef struct {
    void* statusV4Cb;
    AGpsTypeMask atlType;
} AgpsCbInfo;

typedef struct {
    void* visibilityControlCb;
    void* isInEmergencySession;
} NfwCbInfo;

/** GPS extended callback structure. */
typedef struct {
    /** set to sizeof(LocGpsCallbacks) */
    uint32_t      size;
    loc_gps_set_capabilities set_capabilities_cb;
    loc_gps_acquire_wakelock acquire_wakelock_cb;
    loc_gps_release_wakelock release_wakelock_cb;
    loc_gps_create_thread create_thread_cb;
    loc_gps_request_utc_time request_utc_time_cb;
} GpsExtCallbacks;

/** Callback to report the xtra server url to the client.
 *  The client should use this url when downloading xtra unless overwritten
 *  in the gps.conf file
 */
typedef void (* report_xtra_server)(const char*, const char*, const char*);

/** Callback structure for the XTRA interface. */
typedef struct {
    loc_gps_xtra_download_request download_request_cb;
    loc_gps_create_thread create_thread_cb;
    report_xtra_server report_xtra_server_cb;
} GpsXtraExtCallbacks;

/** Represents the status of AGPS. */
typedef struct {
    /** set to sizeof(AGpsExtStatus) */
    uint32_t          size;

    AGpsExtType type;
    LocAGpsStatusValue status;
    uint32_t        ipv4_addr;
    struct sockaddr_storage addr;
    char            ssid[SSID_BUF_SIZE];
    char            password[SSID_BUF_SIZE];
} AGpsExtStatus;

/** Callback with AGPS status information.
 *  Can only be called from a thread created by create_thread_cb.
 */
typedef void (* agps_status_extended)(AGpsExtStatus* status);

/** Callback structure for the AGPS interface. */
typedef struct {
    agps_status_extended status_cb;
    loc_gps_create_thread create_thread_cb;
} AGpsExtCallbacks;


typedef void (*loc_ni_notify_callback)(LocGpsNiNotification *notification, bool esEnalbed);
/** GPS NI callback structure. */
typedef struct
{
    /**
     * Sends the notification request from HAL to GPSLocationProvider.
     */
    loc_ni_notify_callback notify_cb;
} GpsNiExtCallbacks;

typedef enum loc_server_type {
    LOC_AGPS_CDMA_PDE_SERVER,
    LOC_AGPS_CUSTOM_PDE_SERVER,
    LOC_AGPS_MPC_SERVER,
    LOC_AGPS_SUPL_SERVER,
    LOC_AGPS_MO_SUPL_SERVER
} LocServerType;

typedef enum loc_position_mode_type {
    LOC_POSITION_MODE_INVALID = -1,
    LOC_POSITION_MODE_STANDALONE = 0,
    LOC_POSITION_MODE_MS_BASED,
    LOC_POSITION_MODE_MS_ASSISTED,
    LOC_POSITION_MODE_RESERVED_1,
    LOC_POSITION_MODE_RESERVED_2,
    LOC_POSITION_MODE_RESERVED_3,
    LOC_POSITION_MODE_RESERVED_4,
    LOC_POSITION_MODE_RESERVED_5

} LocPositionMode;

/**
 * @brief Minimum allowed value for fix interval.
 *
 * This value is a sanity limit in GPS framework. The hardware has own internal
 * limits that may not match this value
 *
 * @sa GPS_DEFAULT_FIX_INTERVAL_MS
 */

#define GPS_MIN_POSSIBLE_FIX_INTERVAL_MS 100
/**
 * @brief Default value for fix interval.
 *
 * This value is used by default whenever appropriate.
 *
 * @sa GPS_MIN_POSSIBLE_FIX_INTERVAL_MS
 */
#define GPS_DEFAULT_FIX_INTERVAL_MS      1000

/** Flags to indicate which values are valid in a GpsLocationExtended. */
typedef uint64_t GpsLocationExtendedFlags;
/** GpsLocationExtended has valid pdop, hdop, vdop. */
#define GPS_LOCATION_EXTENDED_HAS_DOP 0x0001
/** GpsLocationExtended has valid altitude mean sea level. */
#define GPS_LOCATION_EXTENDED_HAS_ALTITUDE_MEAN_SEA_LEVEL 0x0002
/** UlpLocation has valid magnetic deviation. */
#define GPS_LOCATION_EXTENDED_HAS_MAG_DEV 0x0004
/** UlpLocation has valid mode indicator. */
#define GPS_LOCATION_EXTENDED_HAS_MODE_IND 0x0008
/** GpsLocationExtended has valid vertical uncertainty */
#define GPS_LOCATION_EXTENDED_HAS_VERT_UNC 0x0010
/** GpsLocationExtended has valid speed uncertainty */
#define GPS_LOCATION_EXTENDED_HAS_SPEED_UNC 0x0020
/** GpsLocationExtended has valid heading uncertainty */
#define GPS_LOCATION_EXTENDED_HAS_BEARING_UNC 0x0040
/** GpsLocationExtended has valid horizontal reliability */
#define GPS_LOCATION_EXTENDED_HAS_HOR_RELIABILITY 0x0080
/** GpsLocationExtended has valid vertical reliability */
#define GPS_LOCATION_EXTENDED_HAS_VERT_RELIABILITY 0x0100
/** GpsLocationExtended has valid Horizontal Elliptical Uncertainty (Semi-Major Axis) */
#define GPS_LOCATION_EXTENDED_HAS_HOR_ELIP_UNC_MAJOR 0x0200
/** GpsLocationExtended has valid Horizontal Elliptical Uncertainty (Semi-Minor Axis) */
#define GPS_LOCATION_EXTENDED_HAS_HOR_ELIP_UNC_MINOR 0x0400
/** GpsLocationExtended has valid Elliptical Horizontal Uncertainty Azimuth */
#define GPS_LOCATION_EXTENDED_HAS_HOR_ELIP_UNC_AZIMUTH 0x0800
/** GpsLocationExtended has valid gnss sv used in position data */
#define GPS_LOCATION_EXTENDED_HAS_GNSS_SV_USED_DATA 0x1000
/** GpsLocationExtended has valid navSolutionMask */
#define GPS_LOCATION_EXTENDED_HAS_NAV_SOLUTION_MASK 0x2000
/** GpsLocationExtended has valid LocPosTechMask */
#define GPS_LOCATION_EXTENDED_HAS_POS_TECH_MASK   0x4000
/** GpsLocationExtended has valid LocSvInfoSource */
#define GPS_LOCATION_EXTENDED_HAS_SV_SOURCE_INFO   0x8000
/** GpsLocationExtended has valid position dynamics data */
#define GPS_LOCATION_EXTENDED_HAS_POS_DYNAMICS_DATA   0x10000
/** GpsLocationExtended has GPS Time */
#define GPS_LOCATION_EXTENDED_HAS_GPS_TIME   0x20000
/** GpsLocationExtended has Extended Dilution of Precision */
#define GPS_LOCATION_EXTENDED_HAS_EXT_DOP   0x40000
/** GpsLocationExtended has North standard deviation */
#define GPS_LOCATION_EXTENDED_HAS_NORTH_STD_DEV   0x80000
/** GpsLocationExtended has East standard deviation*/
#define GPS_LOCATION_EXTENDED_HAS_EAST_STD_DEV   0x100000
/** GpsLocationExtended has North Velocity */
#define GPS_LOCATION_EXTENDED_HAS_NORTH_VEL   0x200000
/** GpsLocationExtended has East Velocity */
#define GPS_LOCATION_EXTENDED_HAS_EAST_VEL   0x400000
/** GpsLocationExtended has up Velocity */
#define GPS_LOCATION_EXTENDED_HAS_UP_VEL   0x800000
/** GpsLocationExtended has North Velocity Uncertainty */
#define GPS_LOCATION_EXTENDED_HAS_NORTH_VEL_UNC   0x1000000
/** GpsLocationExtended has East Velocity Uncertainty */
#define GPS_LOCATION_EXTENDED_HAS_EAST_VEL_UNC   0x2000000
/** GpsLocationExtended has up Velocity Uncertainty */
#define GPS_LOCATION_EXTENDED_HAS_UP_VEL_UNC   0x4000000
/** GpsLocationExtended has Clock Bias */
#define GPS_LOCATION_EXTENDED_HAS_CLOCK_BIAS   0x8000000
/** GpsLocationExtended has Clock Bias std deviation*/
#define GPS_LOCATION_EXTENDED_HAS_CLOCK_BIAS_STD_DEV   0x10000000
/** GpsLocationExtended has Clock drift*/
#define GPS_LOCATION_EXTENDED_HAS_CLOCK_DRIFT   0x20000000
/** GpsLocationExtended has Clock drift std deviation**/
#define GPS_LOCATION_EXTENDED_HAS_CLOCK_DRIFT_STD_DEV    0x40000000
/** GpsLocationExtended has leap seconds **/
#define GPS_LOCATION_EXTENDED_HAS_LEAP_SECONDS           0x80000000
/** GpsLocationExtended has time uncertainty **/
#define GPS_LOCATION_EXTENDED_HAS_TIME_UNC               0x100000000
/** GpsLocationExtended has heading rate  **/
#define GPS_LOCATION_EXTENDED_HAS_HEADING_RATE           0x200000000
/** GpsLocationExtended has multiband signals  **/
#define GPS_LOCATION_EXTENDED_HAS_MULTIBAND              0x400000000
/** GpsLocationExtended has sensor calibration confidence */
#define GPS_LOCATION_EXTENDED_HAS_CALIBRATION_CONFIDENCE 0x800000000
/** GpsLocationExtended has sensor calibration status */
#define GPS_LOCATION_EXTENDED_HAS_CALIBRATION_STATUS     0x1000000000
/** GpsLocationExtended has the engine type that produced this
 *  position, the bit mask will only be set when there are two
 *  or more position engines running in the system */
#define GPS_LOCATION_EXTENDED_HAS_OUTPUT_ENG_TYPE       0x2000000000
 /** GpsLocationExtended has the engine mask that indicates the
  *     set of engines contribute to the fix. */
#define GPS_LOCATION_EXTENDED_HAS_OUTPUT_ENG_MASK       0x4000000000

typedef uint32_t LocNavSolutionMask;
/* Bitmask to specify whether SBAS ionospheric correction is used  */
#define LOC_NAV_MASK_SBAS_CORRECTION_IONO ((LocNavSolutionMask)0x0001)
/* Bitmask to specify whether SBAS fast correction is used  */
#define LOC_NAV_MASK_SBAS_CORRECTION_FAST ((LocNavSolutionMask)0x0002)
/**<  Bitmask to specify whether SBAS long-tem correction is used  */
#define LOC_NAV_MASK_SBAS_CORRECTION_LONG ((LocNavSolutionMask)0x0004)
/**<  Bitmask to specify whether SBAS integrity information is used  */
#define LOC_NAV_MASK_SBAS_INTEGRITY ((LocNavSolutionMask)0x0008)
/**<  Bitmask to specify whether Position Report is DGNSS corrected  */
#define LOC_NAV_MASK_DGNSS_CORRECTION ((LocNavSolutionMask)0x0010)
/**<  Bitmask to specify whether Position Report is RTK corrected   */
#define LOC_NAV_MASK_RTK_CORRECTION ((LocNavSolutionMask)0x0020)
/**<  Bitmask to specify whether Position Report is PPP corrected   */
#define LOC_NAV_MASK_PPP_CORRECTION ((LocNavSolutionMask)0x0040)
/**<  Bitmask to specify whether Position Report is RTK fixed corrected   */
#define LOC_NAV_MASK_RTK_FIXED_CORRECTION ((LocNavSolutionMask)0x0080)

typedef uint32_t LocPosDataMask;
/* Bitmask to specify whether Navigation data has Forward Acceleration  */
#define LOC_NAV_DATA_HAS_LONG_ACCEL ((LocPosDataMask)0x0001)
/* Bitmask to specify whether Navigation data has Sideward Acceleration */
#define LOC_NAV_DATA_HAS_LAT_ACCEL ((LocPosDataMask)0x0002)
/* Bitmask to specify whether Navigation data has Vertical Acceleration */
#define LOC_NAV_DATA_HAS_VERT_ACCEL ((LocPosDataMask)0x0004)
/* Bitmask to specify whether Navigation data has Heading Rate */
#define LOC_NAV_DATA_HAS_YAW_RATE ((LocPosDataMask)0x0008)
/* Bitmask to specify whether Navigation data has Body pitch */
#define LOC_NAV_DATA_HAS_PITCH ((LocPosDataMask)0x0010)
/* Bitmask to specify whether Navigation data has Forward Acceleration Unc  */
#define LOC_NAV_DATA_HAS_LONG_ACCEL_UNC ((LocPosDataMask)0x0020)
/* Bitmask to specify whether Navigation data has Sideward Acceleration Unc*/
#define LOC_NAV_DATA_HAS_LAT_ACCEL_UNC ((LocPosDataMask)0x0040)
/* Bitmask to specify whether Navigation data has Vertical Acceleration Unc*/
#define LOC_NAV_DATA_HAS_VERT_ACCEL_UNC ((LocPosDataMask)0x0080)
/* Bitmask to specify whether Navigation data has Heading Rate Unc*/
#define LOC_NAV_DATA_HAS_YAW_RATE_UNC ((LocPosDataMask)0x0100)
/* Bitmask to specify whether Navigation data has Body pitch Unc*/
#define LOC_NAV_DATA_HAS_PITCH_UNC ((LocPosDataMask)0x0200)

typedef uint32_t GnssAdditionalSystemInfoMask;
/* Bitmask to specify whether Tauc is valid */
#define GNSS_ADDITIONAL_SYSTEMINFO_HAS_TAUC ((GnssAdditionalSystemInfoMask)0x0001)
/* Bitmask to specify whether leapSec is valid */
#define GNSS_ADDITIONAL_SYSTEMINFO_HAS_LEAP_SEC ((GnssAdditionalSystemInfoMask)0x0002)


/** GPS PRN Range */
#define GPS_SV_PRN_MIN      1
#define GPS_SV_PRN_MAX      32
#define SBAS_SV_PRN_MIN     33
#define SBAS_SV_PRN_MAX     64
#define GLO_SV_PRN_MIN      65
#define GLO_SV_PRN_MAX      96
#define QZSS_SV_PRN_MIN     193
#define QZSS_SV_PRN_MAX     197
#define BDS_SV_PRN_MIN      201
#define BDS_SV_PRN_MAX      237
#define GAL_SV_PRN_MIN      301
#define GAL_SV_PRN_MAX      336
#define NAVIC_SV_PRN_MIN    401
#define NAVIC_SV_PRN_MAX    414

typedef enum {
    LOC_RELIABILITY_NOT_SET = 0,
    LOC_RELIABILITY_VERY_LOW = 1,
    LOC_RELIABILITY_LOW = 2,
    LOC_RELIABILITY_MEDIUM = 3,
    LOC_RELIABILITY_HIGH = 4
}LocReliability;

typedef enum {
    LOC_IN_EMERGENCY_UNKNOWN = 0,
    LOC_IN_EMERGENCY_SET = 1,
    LOC_IN_EMERGENCY_NOT_SET = 2
}LocInEmergency;

typedef struct {
    struct timespec32_t apTimeStamp;
    /*boottime received from pps-ktimer*/
    float apTimeStampUncertaintyMs;
    /* timestamp uncertainty in milli seconds */
}Gnss_ApTimeStampStructType;

typedef struct {
    uint64_t gps_sv_used_ids_mask;
    uint64_t glo_sv_used_ids_mask;
    uint64_t gal_sv_used_ids_mask;
    uint64_t bds_sv_used_ids_mask;
    uint64_t qzss_sv_used_ids_mask;
    uint64_t navic_sv_used_ids_mask;
} GnssSvUsedInPosition;

typedef struct {
    uint64_t gps_l1ca_sv_used_ids_mask;     // GPS L1CA
    uint64_t gps_l1c_sv_used_ids_mask;      // GPS L1C
    uint64_t gps_l2_sv_used_ids_mask;       // GPS L2
    uint64_t gps_l5_sv_used_ids_mask;       // GPS L5
    uint64_t glo_g1_sv_used_ids_mask;       // GLO G1
    uint64_t glo_g2_sv_used_ids_mask;       // GLO G2
    uint64_t gal_e1_sv_used_ids_mask;       // GAL E1
    uint64_t gal_e5a_sv_used_ids_mask;      // GAL E5A
    uint64_t gal_e5b_sv_used_ids_mask;      // GAL E5B
    uint64_t bds_b1i_sv_used_ids_mask;      // BDS B1I
    uint64_t bds_b1c_sv_used_ids_mask;      // BDS B1C
    uint64_t bds_b2i_sv_used_ids_mask;      // BDS B2I
    uint64_t bds_b2ai_sv_used_ids_mask;     // BDS B2AI
    uint64_t qzss_l1ca_sv_used_ids_mask;    // QZSS L1CA
    uint64_t qzss_l1s_sv_used_ids_mask;     // QZSS L1S
    uint64_t qzss_l2_sv_used_ids_mask;      // QZSS L2
    uint64_t qzss_l5_sv_used_ids_mask;      // QZSS L5
    uint64_t sbas_l1_sv_used_ids_mask;      // SBAS L1
    uint64_t bds_b2aq_sv_used_ids_mask;     // BDS B2AQ
} GnssSvMbUsedInPosition;

/* Body Frame parameters */
typedef struct {
    /** Contains Body frame LocPosDataMask bits. */
   uint32_t        bodyFrameDatamask;
   /* Forward Acceleration in body frame (m/s2)*/
   float           longAccel;
   /** Uncertainty of Forward Acceleration in body frame */
   float           longAccelUnc;
   /* Sideward Acceleration in body frame (m/s2)*/
   float           latAccel;
   /** Uncertainty of Side-ward Acceleration in body frame */
   float           latAccelUnc;
   /* Vertical Acceleration in body frame (m/s2)*/
   float           vertAccel;
   /** Uncertainty of Vertical Acceleration in body frame */
   float           vertAccelUnc;
   /* Heading Rate (Radians/second) */
   float           yawRate;
   /** Uncertainty of Heading Rate */
   float           yawRateUnc;
   /* Body pitch (Radians) */
   float           pitch;
   /** Uncertainty of Body pitch */
   float           pitchRadUnc;
}LocPositionDynamics;

typedef struct {

  /**  Position dilution of precision.
       Range: 1 (highest accuracy) to 50 (lowest accuracy) */
  float PDOP;

  /**  Horizontal dilution of precision.
       Range: 1 (highest accuracy) to 50 (lowest accuracy) */
  float HDOP;

  /**  Vertical dilution of precision.
       Range: 1 (highest accuracy) to 50 (lowest accuracy) */
  float VDOP;

  /**  geometric  dilution of precision.
       Range: 1 (highest accuracy) to 50 (lowest accuracy) */
  float GDOP;

  /**  time dilution of precision.
       Range: 1 (highest accuracy) to 50 (lowest accuracy) */
  float TDOP;
}LocExtDOP;

/* GPS Time structure */
typedef struct {

  /**<   Current GPS week as calculated from midnight, Jan. 6, 1980. \n
       - Units: Weeks */
  uint16_t gpsWeek;

  /**<   Amount of time into the current GPS week. \n
       - Units: Milliseconds */
  uint32_t gpsTimeOfWeekMs;
}GPSTimeStruct;

typedef uint8_t CarrierPhaseAmbiguityType;
#define CARRIER_PHASE_AMBIGUITY_RESOLUTION_NONE ((CarrierPhaseAmbiguityType)0)
#define CARRIER_PHASE_AMBIGUITY_RESOLUTION_FLOAT ((CarrierPhaseAmbiguityType)1)
#define CARRIER_PHASE_AMBIGUITY_RESOLUTION_FIXED ((CarrierPhaseAmbiguityType)2)

typedef uint16_t GnssMeasUsageStatusBitMask;
/** Used in fix */
#define GNSS_MEAS_USED_IN_PVT ((GnssMeasUsageStatusBitMask)0x00000001ul)
/** Measurement is Bad */
#define GNSS_MEAS_USAGE_STATUS_BAD_MEAS ((GnssMeasUsageStatusBitMask)0x00000002ul)
/** Measurement has too low C/N */
#define GNSS_MEAS_USAGE_STATUS_CNO_TOO_LOW ((GnssMeasUsageStatusBitMask)0x00000004ul)
/** Measurement has too low elevation */
#define GNSS_MEAS_USAGE_STATUS_ELEVATION_TOO_LOW ((GnssMeasUsageStatusBitMask)0x00000008ul)
/** No ephemeris available for this measurement */
#define GNSS_MEAS_USAGE_STATUS_NO_EPHEMERIS ((GnssMeasUsageStatusBitMask)0x00000010ul)
/** No corrections available for the measurement */
#define GNSS_MEAS_USAGE_STATUS_NO_CORRECTIONS ((GnssMeasUsageStatusBitMask)0x00000020ul)
/** Corrections has timed out for the measurement */
#define GNSS_MEAS_USAGE_STATUS_CORRECTION_TIMEOUT ((GnssMeasUsageStatusBitMask)0x00000040ul)
/** Measurement is unhealthy */
#define GNSS_MEAS_USAGE_STATUS_UNHEALTHY ((GnssMeasUsageStatusBitMask)0x00000080ul)
/** Configuration is disabled for this measurement */
#define GNSS_MEAS_USAGE_STATUS_CONFIG_DISABLED ((GnssMeasUsageStatusBitMask)0x00000100ul)
/** Measurement not used for other reasons */
#define GNSS_MEAS_USAGE_STATUS_OTHER ((GnssMeasUsageStatusBitMask)0x00000200ul)

/** Flags to indicate valid fields in epMeasUsageInfo */
typedef uint16_t GnssMeasUsageInfoValidityMask;
#define GNSS_PSEUDO_RANGE_RESIDUAL_VALID        ((GnssMeasUsageInfoValidityMask)0x00000001ul)
#define GNSS_DOPPLER_RESIDUAL_VALID             ((GnssMeasUsageInfoValidityMask)0x00000002ul)
#define GNSS_CARRIER_PHASE_RESIDUAL_VALID       ((GnssMeasUsageInfoValidityMask)0x00000004ul)
#define GNSS_CARRIER_PHASE_AMBIGUITY_TYPE_VALID ((GnssMeasUsageInfoValidityMask)0x00000008ul)

typedef uint16_t GnssSvPolyStatusMask;
#define GNSS_SV_POLY_SRC_ALM_CORR_V02 ((GnssSvPolyStatusMask)0x01)
#define GNSS_SV_POLY_GLO_STR4_V02 ((GnssSvPolyStatusMask)0x02)
#define GNSS_SV_POLY_DELETE_V02 ((GnssSvPolyStatusMask)0x04)
#define GNSS_SV_POLY_SRC_GAL_FNAV_OR_INAV_V02 ((GnssSvPolyStatusMask)0x08)
typedef uint16_t GnssSvPolyStatusMaskValidity;
#define GNSS_SV_POLY_SRC_ALM_CORR_VALID_V02 ((GnssSvPolyStatusMaskValidity)0x01)
#define GNSS_SV_POLY_GLO_STR4_VALID_V02 ((GnssSvPolyStatusMaskValidity)0x02)
#define GNSS_SV_POLY_DELETE_VALID_V02 ((GnssSvPolyStatusMaskValidity)0x04)
#define GNSS_SV_POLY_SRC_GAL_FNAV_OR_INAV_VALID_V02 ((GnssSvPolyStatusMaskValidity)0x08)


typedef struct {
    /** Specifies GNSS signal type
        Mandatory Field*/
    GnssSignalTypeMask gnssSignalType;
    /** Specifies GNSS Constellation Type
        Mandatory Field*/
    Gnss_LocSvSystemEnumType gnssConstellation;
    /**  GNSS SV ID.
         For GPS:      1 to 32
         For GLONASS:  65 to 96. When slot-number to SV ID mapping is unknown, set as 255.
         For SBAS:     120 to 151
         For QZSS-L1CA:193 to 197
         For BDS:      201 to 237
         For GAL:      301 to 336 */
    uint16_t gnssSvId;
    /** GLONASS frequency number + 7.
        Valid only for a GLONASS system and
        is to be ignored for all other systems.
        Range: 1 to 14 */
    uint8_t gloFrequency;
    /** Carrier phase ambiguity type. */
    CarrierPhaseAmbiguityType carrierPhaseAmbiguityType;
    /** Validity mask */
    GnssMeasUsageStatusBitMask measUsageStatusMask;
    /** Specifies measurement usage status
        Mandatory Field*/
    GnssMeasUsageInfoValidityMask validityMask;
    /** Computed pseudorange residual.
        Unit: Meters */
    float pseudorangeResidual;
    /** Computed doppler residual.
        Unit: Meters/sec*/
    float dopplerResidual;
    /** Computed carrier phase residual.
        Unit: Cycles*/
    float carrierPhaseResidual;
    /** Carrier phase ambiguity value.
        Unit: Cycles*/
    float carrierPhasAmbiguity;
} GpsMeasUsageInfo;


/** Represents gps location extended. */
typedef struct {
    /** set to sizeof(GpsLocationExtended) */
    uint32_t          size;
    /** Contains GpsLocationExtendedFlags bits. */
    uint64_t        flags;
    /** Contains the Altitude wrt mean sea level */
    float           altitudeMeanSeaLevel;
    /** Contains Position Dilusion of Precision. */
    float           pdop;
    /** Contains Horizontal Dilusion of Precision. */
    float           hdop;
    /** Contains Vertical Dilusion of Precision. */
    float           vdop;
    /** Contains Magnetic Deviation. */
    float           magneticDeviation;
    /** vertical uncertainty in meters */
    float           vert_unc;
    /** speed uncertainty in m/s */
    float           speed_unc;
    /** heading uncertainty in degrees (0 to 359.999) */
    float           bearing_unc;
    /** horizontal reliability. */
    LocReliability  horizontal_reliability;
    /** vertical reliability. */
    LocReliability  vertical_reliability;
    /*  Horizontal Elliptical Uncertainty (Semi-Major Axis) */
    float           horUncEllipseSemiMajor;
    /*  Horizontal Elliptical Uncertainty (Semi-Minor Axis) */
    float           horUncEllipseSemiMinor;
    /*    Elliptical Horizontal Uncertainty Azimuth */
    float           horUncEllipseOrientAzimuth;

    Gnss_ApTimeStampStructType               timeStamp;
    /** Gnss sv used in position data */
    GnssSvUsedInPosition gnss_sv_used_ids;
    /** Gnss sv used in position data for multiband */
    GnssSvMbUsedInPosition gnss_mb_sv_used_ids;
    /** Nav solution mask to indicate sbas corrections */
    LocNavSolutionMask  navSolutionMask;
    /** Position technology used in computing this fix */
    LocPosTechMask tech_mask;
    /** SV Info source used in computing this fix */
    LocSvInfoSource sv_source;
    /** Body Frame Dynamics: 4wayAcceleration and pitch set with validity */
    GnssLocationPositionDynamics bodyFrameData;
    /** GPS Time */
    GPSTimeStruct gpsTime;
    GnssSystemTime gnssSystemTime;
    /** Dilution of precision associated with this position*/
    LocExtDOP extDOP;
    /** North standard deviation.
        Unit: Meters */
    float northStdDeviation;
    /** East standard deviation.
        Unit: Meters */
    float eastStdDeviation;
    /** North Velocity.
        Unit: Meters/sec */
    float northVelocity;
    /** East Velocity.
        Unit: Meters/sec */
    float eastVelocity;
    /** Up Velocity.
        Unit: Meters/sec */
    float upVelocity;
    /** North Velocity standard deviation.
        Unit: Meters/sec */
    float northVelocityStdDeviation;
    /** East Velocity standard deviation.
        Unit: Meters/sec */
    float eastVelocityStdDeviation;
    /** Up Velocity standard deviation
        Unit: Meters/sec */
    float upVelocityStdDeviation;
    /** Estimated clock bias. Unit: Nano seconds */
    float clockbiasMeter;
    /** Estimated clock bias std deviation. Unit: Nano seconds */
    float clockBiasStdDeviationMeter;
    /** Estimated clock drift. Unit: Meters/sec */
    float clockDrift;
    /** Estimated clock drift std deviation. Unit: Meters/sec */
    float clockDriftStdDeviation;
    /** Number of valid reference stations. Range:[0-4] */
    uint8_t numValidRefStations;
    /** Reference station(s) number */
    uint16_t referenceStation[4];
    /** Number of measurements received for use in fix.
        Shall be used as maximum index in-to svUsageInfo[].
        Set to 0, if svUsageInfo reporting is not supported.
        Range: 0-EP_GNSS_MAX_MEAS */
    uint8_t numOfMeasReceived;
    /** Measurement Usage Information */
    GpsMeasUsageInfo measUsageInfo[GNSS_SV_MAX];
    /** Leap Seconds */
    uint8_t leapSeconds;
    /** Time uncertainty in milliseconds   */
    float timeUncMs;
    /** Heading Rate is in NED frame.
        Range: 0 to 359.999. 946
        Unit: Degrees per Seconds */
    float headingRateDeg;
    /** Sensor calibration confidence percent. Range: 0 - 100 */
    uint8_t calibrationConfidence;
    DrCalibrationStatusMask calibrationStatus;
    /* location engine type. When the fix. when the type is set to
        LOC_ENGINE_SRC_FUSED, the fix is the propagated/aggregated
        reports from all engines running on the system (e.g.:
        DR/SPE/PPE). To check which location engine contributes to
        the fused output, check for locOutputEngMask. */
    LocOutputEngineType locOutputEngType;
    /* when loc output eng type is set to fused, this field
        indicates the set of engines contribute to the fix. */
    PositioningEngineMask locOutputEngMask;
} GpsLocationExtended;

enum loc_sess_status {
    LOC_SESS_SUCCESS,
    LOC_SESS_INTERMEDIATE,
    LOC_SESS_FAILURE
};

// struct that contains complete position info from engine
typedef struct {
    UlpLocation location;
    GpsLocationExtended locationExtended;
    enum loc_sess_status sessionStatus;
} EngineLocationInfo;

// Nmea sentence types mask
typedef uint32_t NmeaSentenceTypesMask;
#define LOC_NMEA_MASK_GGA_V02   ((NmeaSentenceTypesMask)0x00000001) /**<  Enable GGA type  */
#define LOC_NMEA_MASK_RMC_V02   ((NmeaSentenceTypesMask)0x00000002) /**<  Enable RMC type  */
#define LOC_NMEA_MASK_GSV_V02   ((NmeaSentenceTypesMask)0x00000004) /**<  Enable GSV type  */
#define LOC_NMEA_MASK_GSA_V02   ((NmeaSentenceTypesMask)0x00000008) /**<  Enable GSA type  */
#define LOC_NMEA_MASK_VTG_V02   ((NmeaSentenceTypesMask)0x00000010) /**<  Enable VTG type  */
#define LOC_NMEA_MASK_PQXFI_V02 ((NmeaSentenceTypesMask)0x00000020) /**<  Enable PQXFI type  */
#define LOC_NMEA_MASK_PSTIS_V02 ((NmeaSentenceTypesMask)0x00000040) /**<  Enable PSTIS type  */
#define LOC_NMEA_MASK_GLGSV_V02 ((NmeaSentenceTypesMask)0x00000080) /**<  Enable GLGSV type  */
#define LOC_NMEA_MASK_GNGSA_V02 ((NmeaSentenceTypesMask)0x00000100) /**<  Enable GNGSA type  */
#define LOC_NMEA_MASK_GNGNS_V02 ((NmeaSentenceTypesMask)0x00000200) /**<  Enable GNGNS type  */
#define LOC_NMEA_MASK_GARMC_V02 ((NmeaSentenceTypesMask)0x00000400) /**<  Enable GARMC type  */
#define LOC_NMEA_MASK_GAGSV_V02 ((NmeaSentenceTypesMask)0x00000800) /**<  Enable GAGSV type  */
#define LOC_NMEA_MASK_GAGSA_V02 ((NmeaSentenceTypesMask)0x00001000) /**<  Enable GAGSA type  */
#define LOC_NMEA_MASK_GAVTG_V02 ((NmeaSentenceTypesMask)0x00002000) /**<  Enable GAVTG type  */
#define LOC_NMEA_MASK_GAGGA_V02 ((NmeaSentenceTypesMask)0x00004000) /**<  Enable GAGGA type  */
#define LOC_NMEA_MASK_PQGSA_V02 ((NmeaSentenceTypesMask)0x00008000) /**<  Enable PQGSA type  */
#define LOC_NMEA_MASK_PQGSV_V02 ((NmeaSentenceTypesMask)0x00010000) /**<  Enable PQGSV type  */
#define LOC_NMEA_MASK_DEBUG_V02 ((NmeaSentenceTypesMask)0x00020000) /**<  Enable DEBUG type  */
#define LOC_NMEA_MASK_GPDTM_V02 ((NmeaSentenceTypesMask)0x00040000) /**<  Enable GPDTM type  */
#define LOC_NMEA_MASK_GNGGA_V02 ((NmeaSentenceTypesMask)0x00080000) /**<  Enable GNGGA type  */
#define LOC_NMEA_MASK_GNRMC_V02 ((NmeaSentenceTypesMask)0x00100000) /**<  Enable GNRMC type  */
#define LOC_NMEA_MASK_GNVTG_V02 ((NmeaSentenceTypesMask)0x00200000) /**<  Enable GNVTG type  */
#define LOC_NMEA_MASK_GAGNS_V02 ((NmeaSentenceTypesMask)0x00400000) /**<  Enable GAGNS type  */
#define LOC_NMEA_MASK_GBGGA_V02 ((NmeaSentenceTypesMask)0x00800000) /**<  Enable GBGGA type  */
#define LOC_NMEA_MASK_GBGSA_V02 ((NmeaSentenceTypesMask)0x01000000) /**<  Enable GBGSA type  */
#define LOC_NMEA_MASK_GBGSV_V02 ((NmeaSentenceTypesMask)0x02000000) /**<  Enable GBGSV type  */
#define LOC_NMEA_MASK_GBRMC_V02 ((NmeaSentenceTypesMask)0x04000000) /**<  Enable GBRMC type  */
#define LOC_NMEA_MASK_GBVTG_V02 ((NmeaSentenceTypesMask)0x08000000) /**<  Enable GBVTG type  */
#define LOC_NMEA_MASK_GQGSV_V02 ((NmeaSentenceTypesMask)0x10000000) /**<  Enable GQGSV type  */
#define LOC_NMEA_MASK_GIGSV_V02 ((NmeaSentenceTypesMask)0x20000000) /**<  Enable GIGSV type  */
#define LOC_NMEA_MASK_GNDTM_V02 ((NmeaSentenceTypesMask)0x40000000) /**<  Enable GNDTM type  */


// all bitmasks of general supported NMEA sentenses - debug is not part of this
#define LOC_NMEA_ALL_GENERAL_SUPPORTED_MASK  (LOC_NMEA_MASK_GGA_V02 | LOC_NMEA_MASK_RMC_V02 | \
              LOC_NMEA_MASK_GSV_V02 | LOC_NMEA_MASK_GSA_V02 | LOC_NMEA_MASK_VTG_V02 | \
        LOC_NMEA_MASK_PQXFI_V02 | LOC_NMEA_MASK_PSTIS_V02 | LOC_NMEA_MASK_GLGSV_V02 | \
        LOC_NMEA_MASK_GNGSA_V02 | LOC_NMEA_MASK_GNGNS_V02 | LOC_NMEA_MASK_GARMC_V02 | \
        LOC_NMEA_MASK_GAGSV_V02 | LOC_NMEA_MASK_GAGSA_V02 | LOC_NMEA_MASK_GAVTG_V02 | \
        LOC_NMEA_MASK_GAGGA_V02 | LOC_NMEA_MASK_PQGSA_V02 | LOC_NMEA_MASK_PQGSV_V02 | \
        LOC_NMEA_MASK_GPDTM_V02 | LOC_NMEA_MASK_GNGGA_V02 | LOC_NMEA_MASK_GNRMC_V02 | \
        LOC_NMEA_MASK_GNVTG_V02 | LOC_NMEA_MASK_GAGNS_V02 | LOC_NMEA_MASK_GBGGA_V02 | \
        LOC_NMEA_MASK_GBGSA_V02 | LOC_NMEA_MASK_GBGSV_V02 | LOC_NMEA_MASK_GBRMC_V02 | \
        LOC_NMEA_MASK_GBVTG_V02 | LOC_NMEA_MASK_GQGSV_V02 | LOC_NMEA_MASK_GIGSV_V02 | \
        LOC_NMEA_MASK_GNDTM_V02)

typedef enum {
  LOC_ENG_IF_REQUEST_SENDER_ID_QUIPC = 0,
  LOC_ENG_IF_REQUEST_SENDER_ID_MSAPM,
  LOC_ENG_IF_REQUEST_SENDER_ID_MSAPU,
  LOC_ENG_IF_REQUEST_SENDER_ID_GPSONE_DAEMON,
  LOC_ENG_IF_REQUEST_SENDER_ID_MODEM,
  LOC_ENG_IF_REQUEST_SENDER_ID_UNKNOWN
} loc_if_req_sender_id_e_type;


#define smaller_of(a, b) (((a) > (b)) ? (b) : (a))
#define MAX_APN_LEN 100

// This will be overridden by the individual adapters
// if necessary.
#define DEFAULT_IMPL(rtv)                                     \
{                                                             \
    LOC_LOGD("%s: default implementation invoked", __func__); \
    return rtv;                                               \
}

enum loc_api_adapter_err {
    LOC_API_ADAPTER_ERR_SUCCESS             = 0,
    LOC_API_ADAPTER_ERR_GENERAL_FAILURE     = 1,
    LOC_API_ADAPTER_ERR_UNSUPPORTED         = 2,
    LOC_API_ADAPTER_ERR_INVALID_HANDLE      = 4,
    LOC_API_ADAPTER_ERR_INVALID_PARAMETER   = 5,
    LOC_API_ADAPTER_ERR_ENGINE_BUSY         = 6,
    LOC_API_ADAPTER_ERR_PHONE_OFFLINE       = 7,
    LOC_API_ADAPTER_ERR_TIMEOUT             = 8,
    LOC_API_ADAPTER_ERR_SERVICE_NOT_PRESENT = 9,
    LOC_API_ADAPTER_ERR_INTERNAL            = 10,

    /* equating engine down to phone offline, as they are the same errror */
    LOC_API_ADAPTER_ERR_ENGINE_DOWN         = LOC_API_ADAPTER_ERR_PHONE_OFFLINE,
    LOC_API_ADAPTER_ERR_FAILURE             = 101,
    LOC_API_ADAPTER_ERR_UNKNOWN
};

enum loc_api_adapter_event_index {
    LOC_API_ADAPTER_REPORT_POSITION = 0,               // Position report comes in loc_parsed_position_s_type
    LOC_API_ADAPTER_REPORT_SATELLITE,                  // Satellite in view report
    LOC_API_ADAPTER_REPORT_NMEA_1HZ,                   // NMEA report at 1HZ rate
    LOC_API_ADAPTER_REPORT_NMEA_POSITION,              // NMEA report at position report rate
    LOC_API_ADAPTER_REQUEST_NI_NOTIFY_VERIFY,          // NI notification/verification request
    LOC_API_ADAPTER_REQUEST_ASSISTANCE_DATA,           // Assistance data, eg: time, predicted orbits request
    LOC_API_ADAPTER_REQUEST_LOCATION_SERVER,           // Request for location server
    LOC_API_ADAPTER_REPORT_IOCTL,                      // Callback report for loc_ioctl
    LOC_API_ADAPTER_REPORT_STATUS,                     // Misc status report: eg, engine state
    LOC_API_ADAPTER_REQUEST_WIFI,                      //
    LOC_API_ADAPTER_SENSOR_STATUS,                     //
    LOC_API_ADAPTER_REQUEST_TIME_SYNC,                 //
    LOC_API_ADAPTER_REPORT_SPI,                        //
    LOC_API_ADAPTER_REPORT_NI_GEOFENCE,                //
    LOC_API_ADAPTER_GEOFENCE_GEN_ALERT,                //
    LOC_API_ADAPTER_REPORT_GENFENCE_BREACH,            //
    LOC_API_ADAPTER_PEDOMETER_CTRL,                    //
    LOC_API_ADAPTER_MOTION_CTRL,                       //
    LOC_API_ADAPTER_REQUEST_WIFI_AP_DATA,              // Wifi ap data
    LOC_API_ADAPTER_BATCH_FULL,                        // Batching on full
    LOC_API_ADAPTER_BATCHED_POSITION_REPORT,           // Batching on fix
    LOC_API_ADAPTER_BATCHED_GENFENCE_BREACH_REPORT,    //
    LOC_API_ADAPTER_GNSS_MEASUREMENT_REPORT,           // GNSS Measurement Report
    LOC_API_ADAPTER_GNSS_SV_POLYNOMIAL_REPORT,         // GNSS SV Polynomial Report
    LOC_API_ADAPTER_GDT_UPLOAD_BEGIN_REQ,              // GDT upload start request
    LOC_API_ADAPTER_GDT_UPLOAD_END_REQ,                // GDT upload end request
    LOC_API_ADAPTER_GNSS_MEASUREMENT,                  // GNSS Measurement report
    LOC_API_ADAPTER_REQUEST_TIMEZONE,                  // Timezone injection request
    LOC_API_ADAPTER_REPORT_GENFENCE_DWELL_REPORT,      // Geofence dwell report
    LOC_API_ADAPTER_REQUEST_SRN_DATA,                  // request srn data from AP
    LOC_API_ADAPTER_REQUEST_POSITION_INJECTION,        // Position injection request
    LOC_API_ADAPTER_BATCH_STATUS,                      // batch status
    LOC_API_ADAPTER_FDCL_SERVICE_REQ,                  // FDCL service request
    LOC_API_ADAPTER_REPORT_UNPROPAGATED_POSITION,      // Unpropagated Position report
    LOC_API_ADAPTER_BS_OBS_DATA_SERVICE_REQ,           // BS observation data request
    LOC_API_ADAPTER_GNSS_SV_EPHEMERIS_REPORT,          // GNSS SV Ephemeris Report
    LOC_API_ADAPTER_LOC_SYSTEM_INFO,                   // Location system info event
    LOC_API_ADAPTER_GNSS_NHZ_MEASUREMENT_REPORT,       // GNSS SV nHz measurement report
    LOC_API_ADAPTER_EVENT_REPORT_INFO,                 // Event report info
    LOC_API_ADAPTER_EVENT_MAX
};

#define LOC_API_ADAPTER_BIT_PARSED_POSITION_REPORT           (1<<LOC_API_ADAPTER_REPORT_POSITION)
#define LOC_API_ADAPTER_BIT_SATELLITE_REPORT                 (1<<LOC_API_ADAPTER_REPORT_SATELLITE)
#define LOC_API_ADAPTER_BIT_NMEA_1HZ_REPORT                  (1<<LOC_API_ADAPTER_REPORT_NMEA_1HZ)
#define LOC_API_ADAPTER_BIT_NMEA_POSITION_REPORT             (1<<LOC_API_ADAPTER_REPORT_NMEA_POSITION)
#define LOC_API_ADAPTER_BIT_NI_NOTIFY_VERIFY_REQUEST         (1<<LOC_API_ADAPTER_REQUEST_NI_NOTIFY_VERIFY)
#define LOC_API_ADAPTER_BIT_ASSISTANCE_DATA_REQUEST          (1<<LOC_API_ADAPTER_REQUEST_ASSISTANCE_DATA)
#define LOC_API_ADAPTER_BIT_LOCATION_SERVER_REQUEST          (1<<LOC_API_ADAPTER_REQUEST_LOCATION_SERVER)
#define LOC_API_ADAPTER_BIT_IOCTL_REPORT                     (1<<LOC_API_ADAPTER_REPORT_IOCTL)
#define LOC_API_ADAPTER_BIT_STATUS_REPORT                    (1<<LOC_API_ADAPTER_REPORT_STATUS)
#define LOC_API_ADAPTER_BIT_REQUEST_WIFI                     (1<<LOC_API_ADAPTER_REQUEST_WIFI)
#define LOC_API_ADAPTER_BIT_SENSOR_STATUS                    (1<<LOC_API_ADAPTER_SENSOR_STATUS)
#define LOC_API_ADAPTER_BIT_REQUEST_TIME_SYNC                (1<<LOC_API_ADAPTER_REQUEST_TIME_SYNC)
#define LOC_API_ADAPTER_BIT_REPORT_SPI                       (1<<LOC_API_ADAPTER_REPORT_SPI)
#define LOC_API_ADAPTER_BIT_REPORT_NI_GEOFENCE               (1<<LOC_API_ADAPTER_REPORT_NI_GEOFENCE)
#define LOC_API_ADAPTER_BIT_GEOFENCE_GEN_ALERT               (1<<LOC_API_ADAPTER_GEOFENCE_GEN_ALERT)
#define LOC_API_ADAPTER_BIT_REPORT_GENFENCE_BREACH           (1<<LOC_API_ADAPTER_REPORT_GENFENCE_BREACH)
#define LOC_API_ADAPTER_BIT_BATCHED_GENFENCE_BREACH_REPORT   (1<<LOC_API_ADAPTER_BATCHED_GENFENCE_BREACH_REPORT)
#define LOC_API_ADAPTER_BIT_PEDOMETER_CTRL                   (1<<LOC_API_ADAPTER_PEDOMETER_CTRL)
#define LOC_API_ADAPTER_BIT_MOTION_CTRL                      (1<<LOC_API_ADAPTER_MOTION_CTRL)
#define LOC_API_ADAPTER_BIT_REQUEST_WIFI_AP_DATA             (1<<LOC_API_ADAPTER_REQUEST_WIFI_AP_DATA)
#define LOC_API_ADAPTER_BIT_BATCH_FULL                       (1<<LOC_API_ADAPTER_BATCH_FULL)
#define LOC_API_ADAPTER_BIT_BATCHED_POSITION_REPORT          (1<<LOC_API_ADAPTER_BATCHED_POSITION_REPORT)
#define LOC_API_ADAPTER_BIT_GNSS_MEASUREMENT_REPORT          (1<<LOC_API_ADAPTER_GNSS_MEASUREMENT_REPORT)
#define LOC_API_ADAPTER_BIT_GNSS_SV_POLYNOMIAL_REPORT        (1<<LOC_API_ADAPTER_GNSS_SV_POLYNOMIAL_REPORT)
#define LOC_API_ADAPTER_BIT_GDT_UPLOAD_BEGIN_REQ             (1<<LOC_API_ADAPTER_GDT_UPLOAD_BEGIN_REQ)
#define LOC_API_ADAPTER_BIT_GDT_UPLOAD_END_REQ               (1<<LOC_API_ADAPTER_GDT_UPLOAD_END_REQ)
#define LOC_API_ADAPTER_BIT_GNSS_MEASUREMENT                 (1<<LOC_API_ADAPTER_GNSS_MEASUREMENT)
#define LOC_API_ADAPTER_BIT_REQUEST_TIMEZONE                 (1<<LOC_API_ADAPTER_REQUEST_TIMEZONE)
#define LOC_API_ADAPTER_BIT_REPORT_GENFENCE_DWELL            (1<<LOC_API_ADAPTER_REPORT_GENFENCE_DWELL_REPORT)
#define LOC_API_ADAPTER_BIT_REQUEST_SRN_DATA                 (1<<LOC_API_ADAPTER_REQUEST_SRN_DATA)
#define LOC_API_ADAPTER_BIT_POSITION_INJECTION_REQUEST       (1<<LOC_API_ADAPTER_REQUEST_POSITION_INJECTION)
#define LOC_API_ADAPTER_BIT_BATCH_STATUS                     (1<<LOC_API_ADAPTER_BATCH_STATUS)
#define LOC_API_ADAPTER_BIT_FDCL_SERVICE_REQ                 (1ULL<<LOC_API_ADAPTER_FDCL_SERVICE_REQ)
#define LOC_API_ADAPTER_BIT_PARSED_UNPROPAGATED_POSITION_REPORT (1ULL<<LOC_API_ADAPTER_REPORT_UNPROPAGATED_POSITION)
#define LOC_API_ADAPTER_BIT_BS_OBS_DATA_SERVICE_REQ          (1ULL<<LOC_API_ADAPTER_BS_OBS_DATA_SERVICE_REQ)
#define LOC_API_ADAPTER_BIT_GNSS_SV_EPHEMERIS_REPORT         (1ULL<<LOC_API_ADAPTER_GNSS_SV_EPHEMERIS_REPORT)
#define LOC_API_ADAPTER_BIT_LOC_SYSTEM_INFO                  (1ULL<<LOC_API_ADAPTER_LOC_SYSTEM_INFO)
#define LOC_API_ADAPTER_BIT_GNSS_NHZ_MEASUREMENT             (1ULL<<LOC_API_ADAPTER_GNSS_NHZ_MEASUREMENT_REPORT)
#define LOC_API_ADAPTER_BIT_EVENT_REPORT_INFO                (1ULL<<LOC_API_ADAPTER_EVENT_REPORT_INFO)

typedef uint64_t LOC_API_ADAPTER_EVENT_MASK_T;

typedef enum loc_api_adapter_msg_to_check_supported {
    LOC_API_ADAPTER_MESSAGE_LOCATION_BATCHING,               // Batching 1.0
    LOC_API_ADAPTER_MESSAGE_BATCHED_GENFENCE_BREACH,         // Geofence Batched Breach
    LOC_API_ADAPTER_MESSAGE_DISTANCE_BASE_TRACKING,          // DBT 2.0
    LOC_API_ADAPTER_MESSAGE_ADAPTIVE_LOCATION_BATCHING,      // Batching 1.5
    LOC_API_ADAPTER_MESSAGE_DISTANCE_BASE_LOCATION_BATCHING, // Batching 2.0
    LOC_API_ADAPTER_MESSAGE_UPDATE_TBF_ON_THE_FLY,           // Updating Tracking TBF On The Fly
    LOC_API_ADAPTER_MESSAGE_OUTDOOR_TRIP_BATCHING,           // Outdoor Trip Batching

    LOC_API_ADAPTER_MESSAGE_MAX
} LocCheckingMessagesID;

typedef int IzatDevId_t;

typedef uint32_t LOC_GPS_LOCK_MASK;
#define isGpsLockNone(lock) ((lock) == 0)
#define isGpsLockMO(lock) ((lock) & ((LOC_GPS_LOCK_MASK)1))
#define isGpsLockMT(lock) ((lock) & ((LOC_GPS_LOCK_MASK)2))
#define isGpsLockAll(lock) (((lock) & ((LOC_GPS_LOCK_MASK)3)) == 3)

/*++ ***********************************************
**  Satellite Measurement and Satellite Polynomial
**  Structure definitions
**  ***********************************************
--*/
#define GNSS_SV_POLY_VELOCITY_COEF_MAX_SIZE         12
#define GNSS_SV_POLY_XYZ_0_TH_ORDER_COEFF_MAX_SIZE  3
#define GNSS_SV_POLY_XYZ_N_TH_ORDER_COEFF_MAX_SIZE  9
#define GNSS_SV_POLY_SV_CLKBIAS_COEFF_MAX_SIZE      4
/** Max number of GNSS SV measurement */
#define GNSS_LOC_SV_MEAS_LIST_MAX_SIZE              128

enum ulp_gnss_sv_measurement_valid_flags{

    ULP_GNSS_SV_MEAS_GPS_TIME = 0,
    ULP_GNSS_SV_MEAS_PSUEDO_RANGE,
    ULP_GNSS_SV_MEAS_MS_IN_WEEK,
    ULP_GNSS_SV_MEAS_SUB_MSEC,
    ULP_GNSS_SV_MEAS_CARRIER_PHASE,
    ULP_GNSS_SV_MEAS_DOPPLER_SHIFT,
    ULP_GNSS_SV_MEAS_CNO,
    ULP_GNSS_SV_MEAS_LOSS_OF_LOCK,

    ULP_GNSS_SV_MEAS_MAX_VALID_FLAGS
};

#define ULP_GNSS_SV_MEAS_BIT_GPS_TIME        (1<<ULP_GNSS_SV_MEAS_GPS_TIME)
#define ULP_GNSS_SV_MEAS_BIT_PSUEDO_RANGE    (1<<ULP_GNSS_SV_MEAS_PSUEDO_RANGE)
#define ULP_GNSS_SV_MEAS_BIT_MS_IN_WEEK      (1<<ULP_GNSS_SV_MEAS_MS_IN_WEEK)
#define ULP_GNSS_SV_MEAS_BIT_SUB_MSEC        (1<<ULP_GNSS_SV_MEAS_SUB_MSEC)
#define ULP_GNSS_SV_MEAS_BIT_CARRIER_PHASE   (1<<ULP_GNSS_SV_MEAS_CARRIER_PHASE)
#define ULP_GNSS_SV_MEAS_BIT_DOPPLER_SHIFT   (1<<ULP_GNSS_SV_MEAS_DOPPLER_SHIFT)
#define ULP_GNSS_SV_MEAS_BIT_CNO             (1<<ULP_GNSS_SV_MEAS_CNO)
#define ULP_GNSS_SV_MEAS_BIT_LOSS_OF_LOCK    (1<<ULP_GNSS_SV_MEAS_LOSS_OF_LOCK)

enum ulp_gnss_sv_poly_valid_flags{

    ULP_GNSS_SV_POLY_GLO_FREQ = 0,
    ULP_GNSS_SV_POLY_T0,
    ULP_GNSS_SV_POLY_IODE,
    ULP_GNSS_SV_POLY_FLAG,
    ULP_GNSS_SV_POLY_POLYCOEFF_XYZ0,
    ULP_GNSS_SV_POLY_POLYCOEFF_XYZN,
    ULP_GNSS_SV_POLY_POLYCOEFF_OTHER,
    ULP_GNSS_SV_POLY_SV_POSUNC,
    ULP_GNSS_SV_POLY_IONODELAY,
    ULP_GNSS_SV_POLY_IONODOT,
    ULP_GNSS_SV_POLY_SBAS_IONODELAY,
    ULP_GNSS_SV_POLY_SBAS_IONODOT,
    ULP_GNSS_SV_POLY_TROPODELAY,
    ULP_GNSS_SV_POLY_ELEVATION,
    ULP_GNSS_SV_POLY_ELEVATIONDOT,
    ULP_GNSS_SV_POLY_ELEVATIONUNC,
    ULP_GNSS_SV_POLY_VELO_COEFF,
    ULP_GNSS_SV_POLY_ENHANCED_IOD,
    ULP_GNSS_SV_POLY_GPS_ISC_L1CA,
    ULP_GNSS_SV_POLY_GPS_ISC_L2C,
    ULP_GNSS_SV_POLY_GPS_ISC_L5I5,
    ULP_GNSS_SV_POLY_GPS_ISC_L5Q5,
    ULP_GNSS_SV_POLY_GPS_TGD,
    ULP_GNSS_SV_POLY_GLO_TGD_G1G2,
    ULP_GNSS_SV_POLY_BDS_TGD_B1,
    ULP_GNSS_SV_POLY_BDS_TGD_B2,
    ULP_GNSS_SV_POLY_BDS_TGD_B2A,
    ULP_GNSS_SV_POLY_BDS_ISC_B2A,
    ULP_GNSS_SV_POLY_GAL_BGD_E1E5A,
    ULP_GNSS_SV_POLY_GAL_BGD_E1E5B,
    ULP_GNSS_SV_POLY_NAVIC_TGD_L5
};

#define ULP_GNSS_SV_POLY_BIT_GLO_FREQ               (1<<ULP_GNSS_SV_POLY_GLO_FREQ)
#define ULP_GNSS_SV_POLY_BIT_T0                     (1<<ULP_GNSS_SV_POLY_T0)
#define ULP_GNSS_SV_POLY_BIT_IODE                   (1<<ULP_GNSS_SV_POLY_IODE)
#define ULP_GNSS_SV_POLY_BIT_FLAG                   (1<<ULP_GNSS_SV_POLY_FLAG)
#define ULP_GNSS_SV_POLY_BIT_POLYCOEFF_XYZ0         (1<<ULP_GNSS_SV_POLY_POLYCOEFF_XYZ0)
#define ULP_GNSS_SV_POLY_BIT_POLYCOEFF_XYZN         (1<<ULP_GNSS_SV_POLY_POLYCOEFF_XYZN)
#define ULP_GNSS_SV_POLY_BIT_POLYCOEFF_OTHER        (1<<ULP_GNSS_SV_POLY_POLYCOEFF_OTHER)
#define ULP_GNSS_SV_POLY_BIT_SV_POSUNC              (1<<ULP_GNSS_SV_POLY_SV_POSUNC)
#define ULP_GNSS_SV_POLY_BIT_IONODELAY              (1<<ULP_GNSS_SV_POLY_IONODELAY)
#define ULP_GNSS_SV_POLY_BIT_IONODOT                (1<<ULP_GNSS_SV_POLY_IONODOT)
#define ULP_GNSS_SV_POLY_BIT_SBAS_IONODELAY         (1<<ULP_GNSS_SV_POLY_SBAS_IONODELAY)
#define ULP_GNSS_SV_POLY_BIT_SBAS_IONODOT           (1<<ULP_GNSS_SV_POLY_SBAS_IONODOT)
#define ULP_GNSS_SV_POLY_BIT_TROPODELAY             (1<<ULP_GNSS_SV_POLY_TROPODELAY)
#define ULP_GNSS_SV_POLY_BIT_ELEVATION              (1<<ULP_GNSS_SV_POLY_ELEVATION)
#define ULP_GNSS_SV_POLY_BIT_ELEVATIONDOT           (1<<ULP_GNSS_SV_POLY_ELEVATIONDOT)
#define ULP_GNSS_SV_POLY_BIT_ELEVATIONUNC           (1<<ULP_GNSS_SV_POLY_ELEVATIONUNC)
#define ULP_GNSS_SV_POLY_BIT_VELO_COEFF             (1<<ULP_GNSS_SV_POLY_VELO_COEFF)
#define ULP_GNSS_SV_POLY_BIT_ENHANCED_IOD           (1<<ULP_GNSS_SV_POLY_ENHANCED_IOD)
#define ULP_GNSS_SV_POLY_BIT_GPS_ISC_L1CA           (1<<ULP_GNSS_SV_POLY_GPS_ISC_L1CA)
#define ULP_GNSS_SV_POLY_BIT_GPS_ISC_L2C            (1<<ULP_GNSS_SV_POLY_GPS_ISC_L2C)
#define ULP_GNSS_SV_POLY_BIT_GPS_ISC_L5I5           (1<<ULP_GNSS_SV_POLY_GPS_ISC_L5I5)
#define ULP_GNSS_SV_POLY_BIT_GPS_ISC_L5Q5           (1<<ULP_GNSS_SV_POLY_GPS_ISC_L5Q5)
#define ULP_GNSS_SV_POLY_BIT_GPS_TGD                (1<<ULP_GNSS_SV_POLY_GPS_TGD)
#define ULP_GNSS_SV_POLY_BIT_GLO_TGD_G1G2           (1<<ULP_GNSS_SV_POLY_GLO_TGD_G1G2)
#define ULP_GNSS_SV_POLY_BIT_BDS_TGD_B1             (1<<ULP_GNSS_SV_POLY_BDS_TGD_B1)
#define ULP_GNSS_SV_POLY_BIT_BDS_TGD_B2             (1<<ULP_GNSS_SV_POLY_BDS_TGD_B2)
#define ULP_GNSS_SV_POLY_BIT_BDS_TGD_B2A            (1<<ULP_GNSS_SV_POLY_BDS_TGD_B2A)
#define ULP_GNSS_SV_POLY_BIT_BDS_ISC_B2A            (1<<ULP_GNSS_SV_POLY_BDS_ISC_B2A)
#define ULP_GNSS_SV_POLY_BIT_GAL_BGD_E1E5A          (1<<ULP_GNSS_SV_POLY_GAL_BGD_E1E5A)
#define ULP_GNSS_SV_POLY_BIT_GAL_BGD_E1E5B          (1<<ULP_GNSS_SV_POLY_GAL_BGD_E1E5B)
#define ULP_GNSS_SV_POLY_BIT_NAVIC_TGD_L5           (1<<ULP_GNSS_SV_POLY_NAVIC_TGD_L5)

typedef enum
{
    GNSS_LOC_FREQ_SOURCE_INVALID = 0,
    /**< Source of the frequency is invalid */
    GNSS_LOC_FREQ_SOURCE_EXTERNAL = 1,
    /**< Source of the frequency is from external injection */
    GNSS_LOC_FREQ_SOURCE_PE_CLK_REPORT = 2,
    /**< Source of the frequency is from Navigation engine */
    GNSS_LOC_FREQ_SOURCE_UNKNOWN = 3
    /**< Source of the frequency is unknown */
} Gnss_LocSourceofFreqEnumType;

typedef struct
{
    uint32_t                          size;
    float                           clockDrift;
    /**< Receiver clock Drift \n
         - Units: meter per sec \n
    */
    float                           clockDriftUnc;
    /**< Receiver clock Drift uncertainty \n
         - Units: meter per sec \n
    */
    Gnss_LocSourceofFreqEnumType    sourceOfFreq;
}Gnss_LocRcvrClockFrequencyInfoStructType;

typedef struct
{
    uint32_t      size;
    uint8_t     leapSec;
    /**< GPS time leap second delta to UTC time  \n
         - Units: sec \n
       */
    uint8_t     leapSecUnc;
    /**< Uncertainty for GPS leap second \n
         - Units: sec \n
       */
}Gnss_LeapSecondInfoStructType;

typedef enum
{
   GNSS_LOC_SYS_TIME_BIAS_VALID                = 0x01,
   /**< System time bias valid */
   GNSS_LOC_SYS_TIME_BIAS_UNC_VALID            = 0x02,
   /**< System time bias uncertainty valid */
}Gnss_LocInterSystemBiasValidMaskType;

typedef struct
{
    uint32_t          size;
    uint32_t        validMask;
    /* Validity mask as per Gnss_LocInterSystemBiasValidMaskType */

    float           timeBias;
    /**< System-1 to System-2 Time Bias  \n
        - Units: msec \n
    */
    float           timeBiasUnc;
    /**< System-1 to System-2 Time Bias uncertainty  \n
        - Units: msec \n
    */
} Gnss_InterSystemBiasStructType;


typedef struct {

  uint32_t    size;

  uint8_t   systemRtc_valid;
  /**<   Validity indicator for System RTC */

  uint64_t  systemRtcMs;
  /**<   Platform system RTC value \n
        - Units: msec \n
        */

}Gnss_LocGnssTimeExtStructType;

typedef enum
{
    GNSS_LOC_MEAS_STATUS_NULL                    = 0x00000000,
    /**< No information state */
    GNSS_LOC_MEAS_STATUS_SM_VALID                = 0x00000001,
    /**< Code phase is known */
    GNSS_LOC_MEAS_STATUS_SB_VALID                = 0x00000002,
    /**< Sub-bit time is known */
    GNSS_LOC_MEAS_STATUS_MS_VALID                = 0x00000004,
    /**< Satellite time is known */
    GNSS_LOC_MEAS_STATUS_BE_CONFIRM              = 0x00000008,
    /**< Bit edge is confirmed from signal   */
    GNSS_LOC_MEAS_STATUS_VELOCITY_VALID          = 0x00000010,
    /**< Satellite Doppler measured */
    GNSS_LOC_MEAS_STATUS_VELOCITY_FINE           = 0x00000020,
    /**< TRUE: Fine Doppler measured, FALSE: Coarse Doppler measured */
    GNSS_LOC_MEAS_STATUS_FROM_RNG_DIFF           = 0x00000200,
    /**< Range update from Satellite differences */
    GNSS_LOC_MEAS_STATUS_FROM_VE_DIFF            = 0x00000400,
    /**< Doppler update from Satellite differences */
    GNSS_LOC_MEAS_STATUS_DONT_USE_X              = 0x00000800,
    /**< Don't use measurement if bit is set */
    GNSS_LOC_MEAS_STATUS_DONT_USE_M              = 0x000001000,
    /**< Don't use measurement if bit is set */
    GNSS_LOC_MEAS_STATUS_DONT_USE_D              = 0x000002000,
    /**< Don't use measurement if bit is set */
    GNSS_LOC_MEAS_STATUS_DONT_USE_S              = 0x000004000,
    /**< Don't use measurement if bit is set */
    GNSS_LOC_MEAS_STATUS_DONT_USE_P              = 0x000008000
    /**< Don't use measurement if bit is set */
}Gnss_LocSvMeasStatusMaskType;

typedef struct
{
    uint32_t              size;
    uint32_t            svMs;
    /**<  Satellite time milisecond.\n
          For GPS, BDS, GAL range of 0 thru (604800000-1) \n
          For GLONASS range of 0 thru (86400000-1) \n
          Valid when PD_LOC_MEAS_STATUS_MS_VALID bit is set in measurement status \n
          Note: All SV times in the current measurement block are alredy propagated to common reference time epoch. \n
            - Units: msec \n
       */
    float               svSubMs;
    /**<Satellite time sub-millisecond. \n
        Total SV Time = svMs + svSubMs \n
        - Units: msec \n
       */
    float               svTimeUncMs;
    /**<  Satellite Time uncertainty \n
          - Units: msec \n
       */
    float               dopplerShift;
    /**< Satellite Doppler \n
            - Units: meter per sec \n
       */
    float               dopplerShiftUnc;
    /**< Satellite Doppler uncertainty\n
            - Units: meter per sec \n
       */
}Gnss_LocSVTimeSpeedStructType;

typedef enum
{
  GNSS_SV_STATE_IDLE = 0,
  GNSS_SV_STATE_SEARCH = 1,
  GNSS_SV_STATE_SEARCH_VERIFY = 2,
  GNSS_SV_STATE_BIT_EDGE = 3,
  GNSS_SV_STATE_VERIFY_TRACK = 4,
  GNSS_SV_STATE_TRACK = 5,
  GNSS_SV_STATE_RESTART = 6,
  GNSS_SV_STATE_DPO_TRACK = 7
} Gnss_LocSVStateEnumType;

typedef enum
{
  GNSS_LOC_SVINFO_MASK_HAS_EPHEMERIS   = 0x01,
  /**< Ephemeris is available for this SV */
  GNSS_LOC_SVINFO_MASK_HAS_ALMANAC     = 0x02
  /**< Almanac is available for this SV */
}Gnss_LocSvInfoMaskT;

typedef enum
{
  GNSS_LOC_SV_SRCH_STATUS_IDLE      = 1,
    /**< SV is not being actively processed */
  GNSS_LOC_SV_SRCH_STATUS_SEARCH    = 2,
    /**< The system is searching for this SV */
  GNSS_LOC_SV_SRCH_STATUS_TRACK     = 3
    /**< SV is being tracked */
}Gnss_LocSvSearchStatusEnumT;

typedef struct
{
    uint32_t                          size;
    Gnss_LocSvSystemEnumType        gnssSystem;
    // 0 signal type mask indicates invalid value
    GnssSignalTypeMask              gnssSignalTypeMask;
    uint16_t                        gnssSvId;
    /**< GNSS SV ID.
         \begin{itemize1}
         \item Range:  \begin{itemize1}
           \item For GPS:      1 to 32
           \item For GLONASS:  1 to 32
           \item For SBAS:     120 to 151
           \item For BDS:      201 to 237
         \end{itemize1} \end{itemize1}
        The GPS and GLONASS SVs can be disambiguated using the system field.
    */
    uint8_t                         gloFrequency;
    /**< GLONASS frequency number + 7 \n
         Valid only for GLONASS System \n
         Shall be ignored for all other systems \n
          - Range: 1 to 14 \n
    */
    Gnss_LocSvSearchStatusEnumT     svStatus;
    /**< Satellite search state \n
        @ENUM()
    */
    bool                         healthStatus_valid;
    /**< SV Health Status validity flag\n
        - 0: Not valid \n
        - 1: Valid \n
    */
    uint8_t                         healthStatus;
    /**< Health status.
         \begin{itemize1}
         \item    Range: 0 to 1; 0 = unhealthy, \n 1 = healthy, 2 = unknown
         \vspace{-0.18in} \end{itemize1}
    */
    Gnss_LocSvInfoMaskT             svInfoMask;
    /**< Indicates whether almanac and ephemeris information is available. \n
        @MASK()
    */
    uint64_t                        measurementStatus;
    /**< Bitmask indicating SV measurement status.
        Valid bitmasks: \n
        If any MSB bit in 0xFFC0000000000000 DONT_USE is set, the measurement
        must not be used by the client.
        @MASK()
    */
    uint16_t                        CNo;
    /**< Carrier to Noise ratio  \n
        - Units: 0.1 dBHz \n
    */
    uint16_t                          gloRfLoss;
    /**< GLONASS Rf loss reference to Antenna. \n
         - Units: dB, Scale: 0.1 \n
    */
    bool                         lossOfLock;
    /**< Loss of signal lock indicator  \n
         - 0: Signal in continuous track \n
         - 1: Signal not in track \n
    */
    int16_t                         measLatency;
    /**< Age of the measurement. Positive value means measurement precedes ref time. \n
         - Units: msec \n
    */
    Gnss_LocSVTimeSpeedStructType   svTimeSpeed;
    /**< Unfiltered SV Time and Speed information
    */
    float                           dopplerAccel;
    /**< Satellite Doppler Accelertion\n
         - Units: Hz/s \n
    */
    bool                         multipathEstValid;
    /**< Multipath estimate validity flag\n
        - 0: Multipath estimate not valid \n
        - 1: Multipath estimate valid \n
    */
    float                           multipathEstimate;
    /**< Estimate of multipath in measurement\n
         - Units: Meters \n
    */
    bool                         fineSpeedValid;
    /**< Fine speed validity flag\n
         - 0: Fine speed not valid \n
         - 1: Fine speed valid \n
    */
    float                           fineSpeed;
    /**< Carrier phase derived speed \n
         - Units: m/s \n
    */
    bool                         fineSpeedUncValid;
    /**< Fine speed uncertainty validity flag\n
         - 0: Fine speed uncertainty not valid \n
         - 1: Fine speed uncertainty valid \n
    */
    float                           fineSpeedUnc;
    /**< Carrier phase derived speed \n
        - Units: m/s \n
    */
    bool                         carrierPhaseValid;
    /**< Carrier Phase measurement validity flag\n
         - 0: Carrier Phase not valid \n
         - 1: Carrier Phase valid \n
    */
    double                          carrierPhase;
    /**< Carrier phase measurement [L1 cycles] \n
    */
    bool                         cycleSlipCountValid;
     /**< Cycle slup count validity flag\n
         - 0: Not valid \n
         - 1: Valid \n
    */
    uint8_t                         cycleSlipCount;
    /**< Increments when a CSlip is detected */

    bool                         svDirectionValid;
    /**< Validity flag for SV direction */

    float                           svAzimuth;
    /**< Satellite Azimuth
        - Units: radians \n
    */
    float                           svElevation;
    /**< Satellite Elevation
        - Units: radians \n
    */
    uint64_t                        validMeasStatusMask;
    /**< Bitmask indicating SV measurement status Validity.
        Valid bitmasks: \n
        If any MSB bit in 0xFFC0000000000000 DONT_USE is set, the measurement
        must not be used by the client.
        @MASK()
    */
    bool                         carrierPhaseUncValid;
    /**< Validity flag for SV direction */

    float                           carrierPhaseUnc;


} Gnss_SVMeasurementStructType;


typedef uint64_t GpsSvMeasHeaderFlags;
#define GNSS_SV_MEAS_HEADER_HAS_LEAP_SECOND                  0x00000001
#define GNSS_SV_MEAS_HEADER_HAS_CLOCK_FREQ                   0x00000002
#define GNSS_SV_MEAS_HEADER_HAS_AP_TIMESTAMP                 0x00000004
#define GNSS_SV_MEAS_HEADER_HAS_GPS_GLO_INTER_SYSTEM_BIAS    0x00000008
#define GNSS_SV_MEAS_HEADER_HAS_GPS_BDS_INTER_SYSTEM_BIAS    0x00000010
#define GNSS_SV_MEAS_HEADER_HAS_GPS_GAL_INTER_SYSTEM_BIAS    0x00000020
#define GNSS_SV_MEAS_HEADER_HAS_BDS_GLO_INTER_SYSTEM_BIAS    0x00000040
#define GNSS_SV_MEAS_HEADER_HAS_GAL_GLO_INTER_SYSTEM_BIAS    0x00000080
#define GNSS_SV_MEAS_HEADER_HAS_GAL_BDS_INTER_SYSTEM_BIAS    0x00000100
#define GNSS_SV_MEAS_HEADER_HAS_GPS_SYSTEM_TIME              0x00000200
#define GNSS_SV_MEAS_HEADER_HAS_GAL_SYSTEM_TIME              0x00000400
#define GNSS_SV_MEAS_HEADER_HAS_BDS_SYSTEM_TIME              0x00000800
#define GNSS_SV_MEAS_HEADER_HAS_QZSS_SYSTEM_TIME             0x00001000
#define GNSS_SV_MEAS_HEADER_HAS_GLO_SYSTEM_TIME              0x00002000
#define GNSS_SV_MEAS_HEADER_HAS_GPS_SYSTEM_TIME_EXT          0x00004000
#define GNSS_SV_MEAS_HEADER_HAS_GAL_SYSTEM_TIME_EXT          0x00008000
#define GNSS_SV_MEAS_HEADER_HAS_BDS_SYSTEM_TIME_EXT          0x00010000
#define GNSS_SV_MEAS_HEADER_HAS_QZSS_SYSTEM_TIME_EXT         0x00020000
#define GNSS_SV_MEAS_HEADER_HAS_GLO_SYSTEM_TIME_EXT          0x00040000
#define GNSS_SV_MEAS_HEADER_HAS_GPSL1L5_TIME_BIAS            0x00080000
#define GNSS_SV_MEAS_HEADER_HAS_GALE1E5A_TIME_BIAS           0x00100000
#define GNSS_SV_MEAS_HEADER_HAS_GPS_NAVIC_INTER_SYSTEM_BIAS  0x00200000
#define GNSS_SV_MEAS_HEADER_HAS_GAL_NAVIC_INTER_SYSTEM_BIAS  0x00400000
#define GNSS_SV_MEAS_HEADER_HAS_GLO_NAVIC_INTER_SYSTEM_BIAS  0x00800000
#define GNSS_SV_MEAS_HEADER_HAS_BDS_NAVIC_INTER_SYSTEM_BIAS  0x01000000
#define GNSS_SV_MEAS_HEADER_HAS_NAVIC_SYSTEM_TIME            0x02000000
#define GNSS_SV_MEAS_HEADER_HAS_NAVIC_SYSTEM_TIME_EXT        0x04000000

typedef struct
{
    uint32_t                                      size;
    // see defines in GNSS_SV_MEAS_HEADER_HAS_XXX_XXX
    uint64_t                                    flags;

    Gnss_LeapSecondInfoStructType               leapSec;

    Gnss_LocRcvrClockFrequencyInfoStructType    clockFreq;   /* Freq */

    Gnss_ApTimeStampStructType                  apBootTimeStamp;

    Gnss_InterSystemBiasStructType              gpsGloInterSystemBias;
    Gnss_InterSystemBiasStructType              gpsBdsInterSystemBias;
    Gnss_InterSystemBiasStructType              gpsGalInterSystemBias;
    Gnss_InterSystemBiasStructType              bdsGloInterSystemBias;
    Gnss_InterSystemBiasStructType              galGloInterSystemBias;
    Gnss_InterSystemBiasStructType              galBdsInterSystemBias;
    Gnss_InterSystemBiasStructType              gpsNavicInterSystemBias;
    Gnss_InterSystemBiasStructType              galNavicInterSystemBias;
    Gnss_InterSystemBiasStructType              gloNavicInterSystemBias;
    Gnss_InterSystemBiasStructType              bdsNavicInterSystemBias;
    Gnss_InterSystemBiasStructType              gpsL1L5TimeBias;
    Gnss_InterSystemBiasStructType              galE1E5aTimeBias;

    GnssSystemTimeStructType                    gpsSystemTime;
    GnssSystemTimeStructType                    galSystemTime;
    GnssSystemTimeStructType                    bdsSystemTime;
    GnssSystemTimeStructType                    qzssSystemTime;
    GnssSystemTimeStructType                    navicSystemTime;
    GnssGloTimeStructType                       gloSystemTime;

    /** GPS system RTC time information. */
    Gnss_LocGnssTimeExtStructType               gpsSystemTimeExt;
    /** GAL system RTC time information. */
    Gnss_LocGnssTimeExtStructType               galSystemTimeExt;
    /** BDS system RTC time information. */
    Gnss_LocGnssTimeExtStructType               bdsSystemTimeExt;
    /** QZSS system RTC time information. */
    Gnss_LocGnssTimeExtStructType               qzssSystemTimeExt;
    /** GLONASS system RTC time information. */
    Gnss_LocGnssTimeExtStructType               gloSystemTimeExt;
    /** NAVIC system RTC time information. */
    Gnss_LocGnssTimeExtStructType               navicSystemTimeExt;
} GnssSvMeasurementHeader;

typedef struct {
    uint32_t                        size;
    bool                          isNhz;
    GnssSvMeasurementHeader       svMeasSetHeader;
    uint32_t                      svMeasCount;
    Gnss_SVMeasurementStructType  svMeas[GNSS_LOC_SV_MEAS_LIST_MAX_SIZE];

} GnssSvMeasurementSet;

typedef struct {
    uint32_t size;                  // set to sizeof(GnssMeasurements)
    GnssSvMeasurementSet            gnssSvMeasurementSet;
    GnssMeasurementsNotification    gnssMeasNotification;
} GnssMeasurements;

typedef enum
{
   GNSS_SV_POLY_COEFF_VALID             = 0x01,
   /**< SV position in orbit coefficients are valid */
   GNSS_SV_POLY_IONO_VALID              = 0x02,
   /**< Iono estimates are valid */

   GNSS_SV_POLY_TROPO_VALID             = 0x04,
   /**< Tropo estimates are valid */

   GNSS_SV_POLY_ELEV_VALID              = 0x08,
   /**< Elevation, rate, uncertainty are valid */

   GNSS_SV_POLY_SRC_ALM_CORR            = 0x10,
   /**< Polynomials based on XTRA */

   GNSS_SV_POLY_SBAS_IONO_VALID         = 0x20,
   /**< SBAS IONO and rate are valid */

   GNSS_SV_POLY_GLO_STR4                = 0x40
   /**< GLONASS String 4 has been received */
}Gnss_SvPolyStatusMaskType;


typedef struct
{
    uint32_t      size;
    uint16_t     gnssSvId;
    /* GPS: 1-32, GLO: 65-96, 0: Invalid,
       SBAS: 120-151, BDS:201-237,GAL:301 to 336
       All others are reserved
    */
    int8_t      freqNum;
    /* Freq index, only valid if u_SysInd is GLO */

    GnssSvPolyStatusMaskValidity svPolyStatusMaskValidity;
    GnssSvPolyStatusMask         svPolyStatusMask;

    uint32_t    is_valid;

    uint16_t     iode;
    /* Ephemeris reference time
       GPS:Issue of Data Ephemeris used [unitless].
       GLO: Tb 7-bit, refer to ICD02
    */
    double      T0;
    /* Reference time for polynominal calculations
       GPS: Secs in week.
       GLO: Full secs since Jan/01/96
    */
    double      polyCoeffXYZ0[GNSS_SV_POLY_XYZ_0_TH_ORDER_COEFF_MAX_SIZE];
    /* C0X, C0Y, C0Z */
    double      polyCoefXYZN[GNSS_SV_POLY_XYZ_N_TH_ORDER_COEFF_MAX_SIZE];
    /* C1X, C2X ... C2Z, C3Z */
    float       polyCoefOther[GNSS_SV_POLY_SV_CLKBIAS_COEFF_MAX_SIZE];
    /* C0T, C1T, C2T, C3T */
    float       svPosUnc;       /* SV position uncertainty [m]. */
    float       ionoDelay;    /* Ionospheric delay at d_T0 [m]. */
    float       ionoDot;      /* Iono delay rate [m/s].  */
    float       sbasIonoDelay;/* SBAS Ionospheric delay at d_T0 [m]. */
    float       sbasIonoDot;  /* SBAS Iono delay rate [m/s].  */
    float       tropoDelay;   /* Tropospheric delay [m]. */
    float       elevation;    /* Elevation [rad] at d_T0 */
    float       elevationDot;      /* Elevation rate [rad/s] */
    float       elevationUnc;      /* SV elevation [rad] uncertainty */
    double      velCoef[GNSS_SV_POLY_VELOCITY_COEF_MAX_SIZE];
    /* Coefficients of velocity poly */
    uint32_t    enhancedIOD;    /*  Enhanced Reference Time */
    float gpsIscL1ca;
    float gpsIscL2c;
    float gpsIscL5I5;
    float gpsIscL5Q5;
    float gpsTgd;
    float gloTgdG1G2;
    float bdsTgdB1;
    float bdsTgdB2;
    float bdsTgdB2a;
    float bdsIscB2a;
    float galBgdE1E5a;
    float galBgdE1E5b;
    float navicTgdL5;
} GnssSvPolynomial;

typedef enum {
    GNSS_EPH_ACTION_UPDATE_SRC_UNKNOWN_V02 = 0, /**<Update ephemeris. Source of ephemeris is unknown  */
    GNSS_EPH_ACTION_UPDATE_SRC_OTA_V02         = 1, /**<Update ephemeris. Source of ephemeris is OTA  */
    GNSS_EPH_ACTION_UPDATE_SRC_NETWORK_V02     = 2, /**<Update ephemeris. Source of ephemeris is Network  */
    GNSS_EPH_ACTION_UPDATE_MAX_V02         = 999, /**<Max value for update ephemeris action. DO NOT USE  */
    GNSS_EPH_ACTION_DELETE_SRC_UNKNOWN_V02 = 1000, /**<Delete previous ephemeris from unknown source  */
    GNSS_EPH_ACTION_DELETE_SRC_NETWORK_V02 = 1001, /**<Delete previous ephemeris from network  */
    GNSS_EPH_ACTION_DELETE_SRC_OTA_V02     = 1002, /**<Delete previous ephemeris from OTA  */
    GNSS_EPH_ACTION_DELETE_MAX_V02     = 1999, /**<Max value for delete ephemeris action. DO NOT USE  */
} GnssEphAction;

typedef enum {
    GAL_EPH_SIGNAL_SRC_UNKNOWN_V02 = 0, /**<  GALILEO signal is unknown  */
    GAL_EPH_SIGNAL_SRC_E1B_V02     = 1, /**<  GALILEO signal is E1B  */
    GAL_EPH_SIGNAL_SRC_E5A_V02     = 2, /**<  GALILEO signal is E5A  */
    GAL_EPH_SIGNAL_SRC_E5B_V02     = 3, /**<  GALILEO signal is E5B  */
} GalEphSignalSource;

typedef struct {
    uint16_t gnssSvId;
    /**<   GNSS SV ID.
      - Type: uint16
      \begin{itemize1}
      \item    Range:    \begin{itemize1}
        \item    For GPS:     1 to 32
        \item    For QZSS:    193 to 197
        \item    For BDS:     201 to 237
        \item    For GAL:     301 to 336
      \vspace{-0.18in} \end{itemize1} \end{itemize1} */

    GnssEphAction updateAction;
    /**<   Specifies the action and source of ephemeris. \n
    - Type: int32 enum */

    uint16_t IODE;
    /**<   Issue of data ephemeris used (unit-less). \n
        GPS: IODE 8 bits.\n
        BDS: AODE 5 bits. \n
        GAL: SIS IOD 10 bits. \n
        - Type: uint16
        - Units: Unit-less */

    double aSqrt;
    /**<   Square root of semi-major axis. \n
      - Type: double
      - Units: Square Root of Meters */

    double deltaN;
    /**<   Mean motion difference from computed value. \n
      - Type: double
      - Units: Radians/Second */

    double m0;
    /**<   Mean anomaly at reference time. \n
      - Type: double
      - Units: Radians */

    double eccentricity;
    /**<   Eccentricity . \n
      - Type: double
      - Units: Unit-less */

    double omega0;
    /**<   Longitude of ascending node of orbital plane at the weekly epoch. \n
      - Type: double
      - Units: Radians */

    double i0;
    /**<   Inclination angle at reference time. \n
      - Type: double
      - Units: Radians */

    double omega;
    /**<   Argument of Perigee. \n
      - Type: double
      - Units: Radians */

    double omegaDot;
    /**<   Rate of change of right ascension. \n
      - Type: double
      - Units: Radians/Second */

    double iDot;
    /**<   Rate of change of inclination angle. \n
      - Type: double
      - Units: Radians/Second */

    double cUc;
    /**<   Amplitude of the cosine harmonic correction term to the argument of latitude. \n
      - Type: double
      - Units: Radians */

    double cUs;
    /**<   Amplitude of the sine harmonic correction term to the argument of latitude. \n
      - Type: double
      - Units: Radians */

    double cRc;
    /**<   Amplitude of the cosine harmonic correction term to the orbit radius. \n
      - Type: double
      - Units: Meters */

    double cRs;
    /**<   Amplitude of the sine harmonic correction term to the orbit radius. \n
      - Type: double
      - Units: Meters */

    double cIc;
    /**<   Amplitude of the cosine harmonic correction term to the angle of inclination. \n
      - Type: double
      - Units: Radians */

    double cIs;
    /**<   Amplitude of the sine harmonic correction term to the angle of inclination. \n
      - Type: double
      - Units: Radians */

    uint32_t toe;
    /**<   Reference time of ephemeris. \n
      - Type: uint32
      - Units: Seconds */

    uint32_t toc;
    /**<   Clock data reference time of week.  \n
      - Type: uint32
      - Units: Seconds */

    double af0;
    /**<   Clock bias correction coefficient. \n
      - Type: double
      - Units: Seconds */

    double af1;
    /**<   Clock drift coefficient. \n
      - Type: double
      - Units: Seconds/Second */

    double af2;
    /**<   Clock drift rate correction coefficient. \n
      - Type: double
      - Units: Seconds/Seconds^2 */

} GnssEphCommon;

/* GPS Navigation Model Info */
typedef struct {
    GnssEphCommon commonEphemerisData;
    /**<   Common ephemeris data.   */

    uint8_t signalHealth;
    /**<   Signal health. \n
         Bit 0 : L5 Signal Health. \n
         Bit 1 : L2 Signal Health. \n
         Bit 2 : L1 Signal Health. \n
         - Type: uint8
         - Values: 3 bit mask of signal health, where set bit indicates unhealthy signal */

    uint8_t URAI;
    /**<   User Range Accuracy Index. \n
         - Type: uint8
         - Units: Unit-less */

    uint8_t codeL2;
    /**<   Indicates which codes are commanded ON for the L2 channel (2-bits). \n
         - Type: uint8
         Valid Values: \n
         - 00 : Reserved
         - 01 : P code ON
         - 10 : C/A code ON */

    uint8_t dataFlagL2P;
    /**<   L2 P-code indication flag. \n
         - Type: uint8
         - Value 1 indicates that the Nav data stream was commanded OFF on the P-code of the L2 channel. */

    double tgd;
    /**<   Time of group delay. \n
         - Type: double
         - Units: Seconds */

    uint8_t fitInterval;
    /**<   Indicates the curve-fit interval used by the CS. \n
         - Type: uint8
         Valid Values:
         - 0 : Four hours
         - 1 : Greater than four hours */

    uint16_t IODC;
    /**<   Issue of Data, Clock. \n
         - Type: uint16
         - Units: Unit-less */
} GpsEphemeris;

/* GLONASS Navigation Model Info */
typedef struct {

    uint16_t gnssSvId;
    /**<   GNSS SV ID.
       - Type: uint16
       - Range: 65 to 96 if known. When the slot number to SV ID mapping is unknown, set to 255 */

    GnssEphAction updateAction;
    /**<   Specifies the action and source of ephemeris. \n
    - Type: int32 enum */

    uint8_t bnHealth;
    /**<   SV health flags. \n
       - Type: uint8
       Valid Values: \n
    - 0 : Healthy
    - 1 : Unhealthy */

    uint8_t lnHealth;
    /**<   Ln SV health flags. GLONASS-M. \n
       - Type: uint8
       Valid Values: \n
    - 0 : Healthy
    - 1 : Unhealthy */

    uint8_t tb;
    /**<   Index of a time interval within current day according to UTC(SU) + 03 hours 00 min. \n
       - Type: uint8
       - Units: Unit-less */

    uint8_t ft;
    /**<   SV accuracy index. \n
       - Type: uint8
       - Units: Unit-less */

    uint8_t gloM;
    /**<   GLONASS-M flag. \n
       - Type: uint8
       Valid Values: \n
    - 0 : GLONASS
    - 1 : GLONASS-M */

    uint8_t enAge;
    /**<   Characterizes "Age" of current information. \n
       - Type: uint8
       - Units: Days */

    uint8_t gloFrequency;
    /**<   GLONASS frequency number + 8. \n
       - Type: uint8
       - Range: 1 to 14
    */

    uint8_t p1;
    /**<   Time interval between two adjacent values of tb parameter. \n
       - Type: uint8
       - Units: Minutes */

    uint8_t p2;
    /**<   Flag of oddness ("1") or evenness ("0") of the value of tb \n
       for intervals 30 or 60 minutes. \n
       - Type: uint8 */

    float deltaTau;
    /**<   Time difference between navigation RF signal transmitted in L2 sub-band \n
       and aviation RF signal transmitted in L1 sub-band. \n
       - Type: floating point
       - Units: Seconds */

    double position[3];
    /**<   Satellite XYZ position. \n
       - Type: array of doubles
       - Units: Meters */

    double velocity[3];
    /**<   Satellite XYZ velocity. \n
       - Type: array of doubles
       - Units: Meters/Second */

    double acceleration[3];
    /**<   Satellite XYZ sola-luni acceleration. \n
       - Type: array of doubles
       - Units: Meters/Second^2 */

    float tauN;
    /**<   Satellite clock correction relative to GLONASS time. \n
       - Type: floating point
       - Units: Seconds */

    float gamma;
    /**<   Relative deviation of predicted carrier frequency value \n
       from nominal value at the instant tb. \n
       - Type: floating point
       - Units: Unit-less */

    double toe;
    /**<   Complete ephemeris time, including N4, NT and Tb. \n
       [(N4-1)*1461 + (NT-1)]*86400 + tb*900 \n
       - Type: double
       - Units: Seconds */

    uint16_t nt;
    /**<   Current date, calendar number of day within four-year interval. \n
       Starting from the 1-st of January in a leap year. \n
       - Type: uint16
       - Units: Days */
} GlonassEphemeris;

/* BDS Navigation Model Info */
typedef struct {

    GnssEphCommon commonEphemerisData;
    /**<   Common ephemeris data.   */

    uint8_t svHealth;
    /**<   Satellite health information applied to both B1 and B2 (SatH1). \n
       - Type: uint8
       Valid Values: \n
       - 0 : Healthy
       - 1 : Unhealthy */

    uint8_t AODC;
    /**<   Age of data clock. \n
       - Type: uint8
       - Units: Hours */

    double tgd1;
    /**<   Equipment group delay differential on B1 signal. \n
       - Type: double
       - Units: Nano-Seconds */

    double tgd2;
    /**<   Equipment group delay differential on B2 signal. \n
       - Type: double
       - Units: Nano-Seconds */

    uint8_t URAI;
    /**<   User range accuracy index (4-bits). \n
       - Type: uint8
       - Units: Unit-less */
} BdsEphemeris;

/* GALIELO Navigation Model Info */
typedef struct {

    GnssEphCommon commonEphemerisData;
    /**<   Common ephemeris data.   */

    GalEphSignalSource dataSourceSignal;
    /**<   Galileo Signal Source. \n
    Valid Values: \n
      - GAL_EPH_SIGNAL_SRC_UNKNOWN (0) --  GALILEO signal is unknown
      - GAL_EPH_SIGNAL_SRC_E1B (1) --  GALILEO signal is E1B
      - GAL_EPH_SIGNAL_SRC_E5A (2) --  GALILEO signal is E5A
      - GAL_EPH_SIGNAL_SRC_E5B (3) --  GALILEO signal is E5B  */

    uint8_t sisIndex;
    /**<   Signal-in-space index for dual frequency E1-E5b/E5a depending on dataSignalSource. \n
       - Type: uint8
       - Units: Unit-less */

    double bgdE1E5a;
    /**<   E1-E5a Broadcast group delay from F/Nav (E5A). \n
       - Type: double
       - Units: Seconds */

    double bgdE1E5b;
    /**<   E1-E5b Broadcast group delay from I/Nav (E1B or E5B). \n
       For E1B or E5B signal, both bgdE1E5a and bgdE1E5b are valid. \n
       For E5A signal, only bgdE1E5a is valid. \n
       Signal source identified using dataSignalSource. \n
       - Type: double
       - Units: Seconds */

    uint8_t svHealth;
    /**<   SV health status of signal identified by dataSourceSignal. \n
       - Type: uint8
       Valid Values: \n
       - 0 : Healthy
       - 1 : Unhealthy */
} GalileoEphemeris;

/** GPS Navigation model for each SV */
typedef struct {
    uint16_t numOfEphemeris;
    GpsEphemeris gpsEphemerisData[GNSS_EPHEMERIS_LIST_MAX_SIZE_V02];
} GpsEphemerisResponse;

/** GLONASS Navigation model for each SV */
typedef struct {
    uint16_t numOfEphemeris;
    GlonassEphemeris gloEphemerisData[GNSS_EPHEMERIS_LIST_MAX_SIZE_V02];
} GlonassEphemerisResponse;

/** BDS Navigation model for each SV */
typedef struct {
    uint16_t numOfEphemeris;
    BdsEphemeris bdsEphemerisData[GNSS_EPHEMERIS_LIST_MAX_SIZE_V02];
} BdsEphemerisResponse;

/** GALILEO Navigation model for each SV */
typedef struct {
    uint16_t numOfEphemeris;
    GalileoEphemeris galEphemerisData[GNSS_EPHEMERIS_LIST_MAX_SIZE_V02];
} GalileoEphemerisResponse;

/** QZSS Navigation model for each SV */
typedef struct {
    uint16_t numOfEphemeris;
    GpsEphemeris qzssEphemerisData[GNSS_EPHEMERIS_LIST_MAX_SIZE_V02];
} QzssEphemerisResponse;


typedef struct {
    /** Indicates GNSS Constellation Type
        Mandatory field */
    Gnss_LocSvSystemEnumType gnssConstellation;

    /** GPS System Time of the ephemeris report */
    bool isSystemTimeValid;
    GnssSystemTimeStructType systemTime;

    union {
       /** GPS Ephemeris */
       GpsEphemerisResponse gpsEphemeris;
       /** GLONASS Ephemeris */
       GlonassEphemerisResponse glonassEphemeris;
       /** BDS Ephemeris */
       BdsEphemerisResponse bdsEphemeris;
       /** GALILEO Ephemeris */
       GalileoEphemerisResponse galileoEphemeris;
       /** QZSS Ephemeris */
       QzssEphemerisResponse qzssEphemeris;
    } ephInfo;
} GnssSvEphemerisReport;

typedef struct {
    /** GPS System Time of the iono model report */
    bool isSystemTimeValid;
    GnssSystemTimeStructType systemTime;

    /** Indicates GNSS Constellation Type */
    Gnss_LocSvSystemEnumType gnssConstellation;

    float alpha0;
    /**<   Klobuchar Model Parameter Alpha 0.
         - Type: float
         - Unit: Seconds
    */

    float alpha1;
    /**<   Klobuchar Model Parameter Alpha 1.
         - Type: float
         - Unit: Seconds / Semi-Circle
    */

    float alpha2;
    /**<   Klobuchar Model Parameter Alpha 2.
         - Type: float
         - Unit: Seconds / Semi-Circle^2
    */

    float alpha3;
    /**<   Klobuchar Model Parameter Alpha 3.
         - Type: float
         - Unit: Seconds / Semi-Circle^3
    */

    float beta0;
    /**<   Klobuchar Model Parameter Beta 0.
         - Type: float
         - Unit: Seconds
    */

    float beta1;
    /**<   Klobuchar Model Parameter Beta 1.
         - Type: float
         - Unit: Seconds / Semi-Circle
    */

    float beta2;
    /**<   Klobuchar Model Parameter Beta 2.
         - Type: float
         - Unit: Seconds / Semi-Circle^2
    */

    float beta3;
    /**<   Klobuchar Model Parameter Beta 3.
         - Type: float
         - Unit: Seconds / Semi-Circle^3
    */
} GnssKlobucharIonoModel;

typedef struct {
        /** GPS System Time of the report */
    bool isSystemTimeValid;
    GnssSystemTimeStructType systemTime;

    GnssAdditionalSystemInfoMask validityMask;
    double tauC;
    int8_t leapSec;
} GnssAdditionalSystemInfo;

/* Various Short Range Node Technology type*/
typedef enum {
    SRN_AP_DATA_TECH_TYPE_NONE,
    SRN_AP_DATA_TECH_TYPE_BT,
    SRN_AP_DATA_TECH_TYPE_BTLE,
    SRN_AP_DATA_TECH_TYPE_NFC,
    SRN_AP_DATA_TECH_TYPE_MOBILE_CODE,
    SRN_AP_DATA_TECH_TYPE_OTHER
} Gnss_SrnTech;

/* Mac Address type requested by modem */
typedef enum {
    SRN_AP_DATA_PUBLIC_MAC_ADDR_TYPE_INVALID, /* No valid mac address type send */
    SRN_AP_DATA_PUBLIC_MAC_ADDR_TYPE_PUBLIC, /* SRN AP MAC Address type PUBLIC  */
    SRN_AP_DATA_PUBLIC_MAC_ADDR_TYPE_PRIVATE, /* SRN AP MAC Address type PRIVATE  */
    SRN_AP_DATA_PUBLIC_MAC_ADDR_TYPE_OTHER, /* SRN AP MAC Address type OTHER  */
}Gnss_Srn_MacAddr_Type;

typedef struct
{
    uint32_t                 size;
    Gnss_SrnTech           srnTechType; /* SRN Technology type in request */
    bool                   srnRequest; /* scan - start(true) or stop(false) */
    bool                   e911Mode; /* If in E911 emergency */
    Gnss_Srn_MacAddr_Type  macAddrType; /* SRN AP MAC Address type */
} GnssSrnDataReq;

/* Provides the current GNSS SV Type configuration to the client.
 * This is fetched via direct call to GNSS Adapter bypassing
 * Location API */
typedef std::function<void(
    const GnssSvTypeConfig& config
)> GnssSvTypeConfigCallback;

/*
 * Represents the status of AGNSS augmented to support IPv4.
 */
struct AGnssExtStatusIpV4 {
    AGpsExtType         type;
    LocApnTypeMask      apnTypeMask;
    LocAGpsStatusValue  status;
    /*
     * 32-bit IPv4 address.
     */
    uint32_t            ipV4Addr;
};

/*
 * Represents the status of AGNSS augmented to support IPv6.
 */
struct AGnssExtStatusIpV6 {
    AGpsExtType         type;
    LocApnTypeMask      apnTypeMask;
    LocAGpsStatusValue  status;
    /*
     * 128-bit IPv6 address.
     */
    uint8_t             ipV6Addr[16];
};

/*
* Represents the the Nfw Notification structure
*/
#define GNSS_MAX_NFW_APP_STRING_LEN 64
#define GNSS_MAX_NFW_STRING_LEN  20

typedef enum {
    GNSS_NFW_CTRL_PLANE = 0,
    GNSS_NFW_SUPL = 1,
    GNSS_NFW_IMS = 10,
    GNSS_NFW_SIM = 11,
    GNSS_NFW_OTHER_PROTOCOL_STACK = 100
} GnssNfwProtocolStack;

typedef enum {
    GNSS_NFW_CARRIER = 0,
    GNSS_NFW_OEM = 10,
    GNSS_NFW_MODEM_CHIPSET_VENDOR = 11,
    GNSS_NFW_GNSS_CHIPSET_VENDOR = 12,
    GNSS_NFW_OTHER_CHIPSET_VENDOR = 13,
    GNSS_NFW_AUTOMOBILE_CLIENT = 20,
    GNSS_NFW_OTHER_REQUESTOR = 100
} GnssNfwRequestor;

typedef enum {
    GNSS_NFW_REJECTED = 0,
    GNSS_NFW_ACCEPTED_NO_LOCATION_PROVIDED = 1,
    GNSS_NFW_ACCEPTED_LOCATION_PROVIDED = 2,
} GnssNfwResponseType;

typedef struct {
    char                    proxyAppPackageName[GNSS_MAX_NFW_APP_STRING_LEN];
    GnssNfwProtocolStack    protocolStack;
    char                    otherProtocolStackName[GNSS_MAX_NFW_STRING_LEN];
    GnssNfwRequestor        requestor;
    char                    requestorId[GNSS_MAX_NFW_STRING_LEN];
    GnssNfwResponseType     responseType;
    bool                    inEmergencyMode;
    bool                    isCachedLocation;
} GnssNfwNotification;

/* ODCPI Request Info */
enum OdcpiRequestType {
    ODCPI_REQUEST_TYPE_START,
    ODCPI_REQUEST_TYPE_STOP
};
struct OdcpiRequestInfo {
    uint32_t size;
    OdcpiRequestType type;
    uint32_t tbfMillis;
    bool isEmergencyMode;
};
/* Callback to send ODCPI request to framework */
typedef std::function<void(const OdcpiRequestInfo& request)> OdcpiRequestCallback;

/*
 * Callback with AGNSS(IpV4) status information.
 *
 * @param status Will be of type AGnssExtStatusIpV4.
 */
typedef void (*AgnssStatusIpV4Cb)(AGnssExtStatusIpV4 status);

/*
* Callback with NFW information.
*/
typedef void(*NfwStatusCb)(GnssNfwNotification notification);
typedef bool(*IsInEmergencySession)(void);

/*
 * Callback with AGNSS(IpV6) status information.
 *
 * @param status Will be of type AGnssExtStatusIpV6.
 */
typedef void (*AgnssStatusIpV6Cb)(AGnssExtStatusIpV6 status);

/* Constructs for interaction with loc_net_iface library */
typedef void (*LocAgpsOpenResultCb)(bool isSuccess, AGpsExtType agpsType, const char* apn,
        AGpsBearerType bearerType, void* userDataPtr);

typedef void (*LocAgpsCloseResultCb)(bool isSuccess, AGpsExtType agpsType, void* userDataPtr);

enum PowerStateType {
    POWER_STATE_UNKNOWN = 0,
    POWER_STATE_SUSPEND = 1,
    POWER_STATE_RESUME  = 2,
    POWER_STATE_SHUTDOWN = 3
};

/* Shared resources of LocIpc */
#define LOC_IPC_HAL                    "/dev/socket/location/socket_hal"
#define LOC_IPC_XTRA                   "/dev/socket/location/xtra/socket_xtra"

#define SOCKET_DIR_LOCATION            "/dev/socket/location/"
#define SOCKET_DIR_EHUB                "/dev/socket/location/ehub/"
#define SOCKET_TO_LOCATION_HAL_DAEMON  "/dev/socket/loc_client/hal_daemon"

#define SOCKET_LOC_CLIENT_DIR          "/dev/socket/loc_client/"
#define EAP_LOC_CLIENT_DIR             "/data/vendor/location/extap_locclient/"

#define LOC_CLIENT_NAME_PREFIX         "toclient"
#define LOC_INTAPI_NAME_PREFIX         "toIntapiClient"

typedef uint64_t NetworkHandle;
#define NETWORK_HANDLE_UNKNOWN  ~0
#define MAX_NETWORK_HANDLES 10

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GPS_EXTENDED_C_H */
