/* Copyright (c) 2013-2017 The Linux Foundation. All rights reserved.
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
#include <time.h>

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

/** Sizes for indoor fields */
#define GPS_LOCATION_MAP_URL_SIZE 400
#define GPS_LOCATION_MAP_INDEX_SIZE 16

/** Position source is ULP */
#define ULP_LOCATION_IS_FROM_HYBRID   0x0001
/** Position source is GNSS only */
#define ULP_LOCATION_IS_FROM_GNSS     0x0002
/** Position source is ZPP only */
#define ULP_LOCATION_IS_FROM_ZPP      0x0004
/** Position is from a Geofence Breach Event */
#define ULP_LOCATION_IS_FROM_GEOFENCE 0X0008
/** Position is from Hardware FLP */
#define ULP_LOCATION_IS_FROM_HW_FLP   0x0010
/** Position is from NLP */
#define ULP_LOCATION_IS_FROM_NLP      0x0020
/** Position is from PIP */
#define ULP_LOCATION_IS_FROM_PIP      0x0040
/** Position is from external DR solution*/
#define ULP_LOCATION_IS_FROM_EXT_DR   0X0080
/** Raw GNSS position fixes */
#define ULP_LOCATION_IS_FROM_GNSS_RAW   0X0100

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
    LOC_SUPPORTED_FEATURE_FDCL /**< Support FDCL */
} loc_supported_feature_enum;

typedef struct {
    /** set to sizeof(UlpLocation) */
    size_t          size;
    LocGpsLocation     gpsLocation;
    /* Provider indicator for HYBRID or GPS */
    uint16_t        position_source;
    LocPosTechMask  tech_mask;
    /*allows HAL to pass additional information related to the location */
    int             rawDataSize;         /* in # of bytes */
    void            * rawData;
    bool            is_indoor;
    float           floor_number;
    char            map_url[GPS_LOCATION_MAP_URL_SIZE];
    unsigned char   map_index[GPS_LOCATION_MAP_INDEX_SIZE];
} UlpLocation;

typedef struct {
    /** set to sizeof(UlpNmea) */
    size_t          size;
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

typedef enum {
    AGPS_CB_PRIORITY_LOW  = 1,
    AGPS_CB_PRIORITY_MED  = 2,
    AGPS_CB_PRIORITY_HIGH = 3
} AgpsCbPriority;

typedef struct {
    void* statusV4Cb;
    AgpsCbPriority cbPriority;
} AgpsCbInfo;

/** GPS extended callback structure. */
typedef struct {
    /** set to sizeof(LocGpsCallbacks) */
    size_t      size;
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
    size_t          size;

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
    LOC_AGPS_SUPL_SERVER
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
typedef uint32_t GpsLocationExtendedFlags;
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
/** GpsLocationExtended has Elapsed Time */
#define GPS_LOCATION_EXTENDED_HAS_ELAPSED_TIME   0x80000

typedef uint32_t LocNavSolutionMask;
/* Bitmask to specify whether SBAS ionospheric correction is used  */
#define LOC_NAV_MASK_SBAS_CORRECTION_IONO ((LocNavSolutionMask)0x0001)
/* Bitmask to specify whether SBAS fast correction is used  */
#define LOC_NAV_MASK_SBAS_CORRECTION_FAST ((LocNavSolutionMask)0x0002)
/**<  Bitmask to specify whether SBAS long-tem correction is used  */
#define LOC_NAV_MASK_SBAS_CORRECTION_LONG ((LocNavSolutionMask)0x0004)
/**<  Bitmask to specify whether SBAS integrity information is used  */
#define LOC_NAV_MASK_SBAS_INTEGRITY ((LocNavSolutionMask)0x0008)

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

/** GPS PRN Range */
#define GPS_SV_PRN_MIN      1
#define GPS_SV_PRN_MAX      32
#define GLO_SV_PRN_MIN      65
#define GLO_SV_PRN_MAX      96
#define QZSS_SV_PRN_MIN     193
#define QZSS_SV_PRN_MAX     197
#define BDS_SV_PRN_MIN      201
#define BDS_SV_PRN_MAX      235
#define GAL_SV_PRN_MIN      301
#define GAL_SV_PRN_MAX      336

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

typedef enum {
    LOC_RELIABILITY_NOT_SET = 0,
    LOC_RELIABILITY_VERY_LOW = 1,
    LOC_RELIABILITY_LOW = 2,
    LOC_RELIABILITY_MEDIUM = 3,
    LOC_RELIABILITY_HIGH = 4
}LocReliability;

typedef struct {
    struct timespec apTimeStamp;
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
} GnssSvUsedInPosition;

/* Body Frame parameters */
typedef struct {
    /** Contains Body frame LocPosDataMask bits. */
   uint32_t        bodyFrameDatamask;
   /* Forward Acceleration in body frame (m/s2)*/
   float           longAccel;
   /* Sideward Acceleration in body frame (m/s2)*/
   float           latAccel;
   /* Vertical Acceleration in body frame (m/s2)*/
   float           vertAccel;
   /* Heading Rate (Radians/second) */
   float           yawRate;
   /* Body pitch (Radians) */
   float           pitch;
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

/** Represents gps location extended. */
typedef struct {
    /** set to sizeof(GpsLocationExtended) */
    size_t          size;
    /** Contains GpsLocationExtendedFlags bits. */
    uint32_t        flags;
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
    /** Nav solution mask to indicate sbas corrections */
    LocNavSolutionMask  navSolutionMask;
    /** Position technology used in computing this fix */
    LocPosTechMask tech_mask;
    /** SV Info source used in computing this fix */
    LocSvInfoSource sv_source;
    /** Body Frame Dynamics: 4wayAcceleration and pitch set with validity */
    LocPositionDynamics bodyFrameData;
    /** GPS Time */
    GPSTimeStruct gpsTime;
    /** Elapsed Time */
    int64_t  elapsedTime;
    /** Dilution of precision associated with this position*/
    LocExtDOP extDOP;
} GpsLocationExtended;

enum loc_sess_status {
    LOC_SESS_SUCCESS,
    LOC_SESS_INTERMEDIATE,
    LOC_SESS_FAILURE
};

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

// all bitmasks of general supported NMEA sentenses - debug is not part of this
#define LOC_NMEA_ALL_GENERAL_SUPPORTED_MASK  (LOC_NMEA_MASK_GGA_V02 | LOC_NMEA_MASK_RMC_V02 | \
              LOC_NMEA_MASK_GSV_V02 | LOC_NMEA_MASK_GSA_V02 | LOC_NMEA_MASK_VTG_V02 | \
        LOC_NMEA_MASK_PQXFI_V02 | LOC_NMEA_MASK_PSTIS_V02 | LOC_NMEA_MASK_GLGSV_V02 | \
        LOC_NMEA_MASK_GNGSA_V02 | LOC_NMEA_MASK_GNGNS_V02 | LOC_NMEA_MASK_GARMC_V02 | \
        LOC_NMEA_MASK_GAGSV_V02 | LOC_NMEA_MASK_GAGSA_V02 | LOC_NMEA_MASK_GAVTG_V02 | \
        LOC_NMEA_MASK_GAGGA_V02 | LOC_NMEA_MASK_PQGSA_V02 | LOC_NMEA_MASK_PQGSV_V02)

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
    LOC_API_ADAPTER_GNSS_MEASUREMENT_REPORT,          //GNSS Measurement Report
    LOC_API_ADAPTER_GNSS_SV_POLYNOMIAL_REPORT,        //GNSS SV Polynomial Report
    LOC_API_ADAPTER_GDT_UPLOAD_BEGIN_REQ,              // GDT upload start request
    LOC_API_ADAPTER_GDT_UPLOAD_END_REQ,                // GDT upload end request
    LOC_API_ADAPTER_GNSS_MEASUREMENT,                  // GNSS Measurement report
    LOC_API_ADAPTER_REQUEST_TIMEZONE,                  // Timezone injection request
    LOC_API_ADAPTER_REPORT_GENFENCE_DWELL_REPORT,      // Geofence dwell report
    LOC_API_ADAPTER_REQUEST_SRN_DATA,                  // request srn data from AP
    LOC_API_ADAPTER_REQUEST_POSITION_INJECTION,        // Position injection request
    LOC_API_ADAPTER_BATCH_STATUS,                      // batch status
    LOC_API_ADAPTER_FDCL_SERVICE_REQ,                  // FDCL service request
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
#define GNSS_LOC_SV_MEAS_LIST_MAX_SIZE              16

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

    ULP_GNSS_SV_POLY_VALID_FLAGS

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


typedef enum
{
    GNSS_LOC_SV_SYSTEM_GPS                    = 1,
    /**< GPS satellite. */
    GNSS_LOC_SV_SYSTEM_GALILEO                = 2,
    /**< GALILEO satellite. */
    GNSS_LOC_SV_SYSTEM_SBAS                   = 3,
    /**< SBAS satellite. */
    GNSS_LOC_SV_SYSTEM_COMPASS                = 4,
    /**< COMPASS satellite. */
    GNSS_LOC_SV_SYSTEM_GLONASS                = 5,
    /**< GLONASS satellite. */
    GNSS_LOC_SV_SYSTEM_BDS                    = 6,
    /**< BDS satellite. */
    GNSS_LOC_SV_SYSTEM_QZSS                   = 7
    /**< QZSS satellite. */
} Gnss_LocSvSystemEnumType;

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
    size_t                          size;
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
    size_t      size;
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
    size_t          size;
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
}Gnss_InterSystemBiasStructType;


typedef struct
{
    size_t          size;
    uint16_t        systemWeek;
    /**< System week number for GPS, BDS and GAL satellite systems. \n
         Set to 65535 when invalid or not available. \n
         Not valid for GLONASS system. \n
       */

    uint32_t        systemMsec;
    /**< System time msec. Time of Week for GPS, BDS, GAL and
         Time of Day for GLONASS.
         - Units: msec \n
      */
    float           systemClkTimeBias;
    /**< System clock time bias \n
         - Units: msec \n
         System time = systemMsec - systemClkTimeBias \n
      */
    float           systemClkTimeUncMs;
    /**< Single sided maximum time bias uncertainty \n
                                                    - Units: msec \n
      */
}Gnss_LocSystemTimeStructType;

typedef struct {

  size_t        size;
  uint8_t       gloFourYear;
  /**<   GLONASS four year number from 1996. Refer to GLONASS ICD.\n
        Applicable only for GLONASS and shall be ignored for other constellations. \n
        If unknown shall be set to 255
        */

  uint16_t      gloDays;
  /**<   GLONASS day number in four years. Refer to GLONASS ICD.
        Applicable only for GLONASS and shall be ignored for other constellations. \n
        If unknown shall be set to 65535
        */

  uint32_t      gloMsec;
  /**<   GLONASS time of day in msec. Refer to GLONASS ICD.
            - Units: msec \n
        */

  float         gloClkTimeBias;
  /**<   System clock time bias (sub-millisecond) \n
            - Units: msec \n
        System time = systemMsec - systemClkTimeBias \n
    */

  float         gloClkTimeUncMs;
  /**<   Single sided maximum time bias uncertainty \n
                - Units: msec \n
        */
}Gnss_LocGloTimeStructType;  /* Type */

typedef struct {

  size_t    size;
  uint32_t  refFCount;
  /**<   Receiver frame counter value at reference tick */

  uint8_t   systemRtc_valid;
  /**<   Validity indicator for System RTC */

  uint64_t  systemRtcMs;
  /**<   Platform system RTC value \n
        - Units: msec \n
        */

  uint32_t  sourceOfTime;
  /**<   Source of time information */

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
    size_t              size;
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
    size_t                          size;
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
} Gnss_SVMeasurementStructType;

/**< Maximum number of satellites in measurement block for given system. */

typedef struct
{
    size_t                          size;
    Gnss_LocSvSystemEnumType        system;
    /**< Specifies the Satellite System Type
    */
    bool                            isSystemTimeValid;
    /**< Indicates whether System Time is Valid:\n
         - 0x01 (TRUE) --  System Time is valid \n
         - 0x00 (FALSE) -- System Time is not valid
    */
    Gnss_LocSystemTimeStructType    systemTime;
    /**< System Time Information \n
    */
    bool                            isGloTime_valid;
    Gnss_LocGloTimeStructType       gloTime;

    bool                            isSystemTimeExt_valid;
    Gnss_LocGnssTimeExtStructType   systemTimeExt;

    uint8_t                         numSvs;
    /* Number of SVs in this report block */

    Gnss_SVMeasurementStructType    svMeasurement[GNSS_LOC_SV_MEAS_LIST_MAX_SIZE];
    /**< Satellite measurement Information \n
    */
} Gnss_ClockMeasurementStructType;


typedef struct
{
    size_t                                      size;
    uint8_t                                     seqNum;
    /**< Current message Number */
    uint8_t                                     maxMessageNum;
    /**< Maximum number of message that will be sent for present time epoch. */

    bool                                     leapSecValid;
    Gnss_LeapSecondInfoStructType               leapSec;

    Gnss_InterSystemBiasStructType              gpsGloInterSystemBias;

    Gnss_InterSystemBiasStructType              gpsBdsInterSystemBias;

    Gnss_InterSystemBiasStructType              gpsGalInterSystemBias;

    Gnss_InterSystemBiasStructType              bdsGloInterSystemBias;

    Gnss_InterSystemBiasStructType              galGloInterSystemBias;

    Gnss_InterSystemBiasStructType              galBdsInterSystemBias;

    bool                                     clockFreqValid;
    Gnss_LocRcvrClockFrequencyInfoStructType    clockFreq;   /* Freq */
    bool                                     gnssMeasValid;
    Gnss_ClockMeasurementStructType             gnssMeas;
    Gnss_ApTimeStampStructType               timeStamp;

} GnssSvMeasurementSet;

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
    size_t      size;
    uint16_t     gnssSvId;
    /* GPS: 1-32, GLO: 65-96, 0: Invalid,
       SBAS: 120-151, BDS:201-237,GAL:301 to 336
       All others are reserved
    */
    int8_t      freqNum;
    /* Freq index, only valid if u_SysInd is GLO */

    uint8_t     svPolyFlags;
    /* Indicate the validity of the elements
    as per Gnss_SvPolyStatusMaskType
    */

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
} GnssSvPolynomial;

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
    size_t                 size;
    Gnss_SrnTech           srnTechType; /* SRN Technology type in request */
    bool                   srnRequest; /* scan - start(true) or stop(false) */
    bool                   e911Mode; /* If in E911 emergency */
    Gnss_Srn_MacAddr_Type  macAddrType; /* SRN AP MAC Address type */
} GnssSrnDataReq;

/*
 * Represents the status of AGNSS augmented to support IPv4.
 */
struct AGnssExtStatusIpV4 {
    AGpsExtType type;
    LocAGpsStatusValue status;
    /*
     * 32-bit IPv4 address.
     */
    uint32_t ipV4Addr;
};

/*
 * Represents the status of AGNSS augmented to support IPv6.
 */
struct AGnssExtStatusIpV6 {
    AGpsExtType type;
    LocAGpsStatusValue status;
    /*
     * 128-bit IPv6 address.
     */
    uint8_t ipV6Addr[16];
};

/* ODCPI Request Info */
enum OdcpiRequestType {
    ODCPI_REQUEST_TYPE_START,
    ODCPI_REQUEST_TYPE_STOP
};
struct OdcpiRequestInfo {
    size_t size;
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
 * Callback with AGNSS(IpV6) status information.
 *
 * @param status Will be of type AGnssExtStatusIpV6.
 */
typedef void (*AgnssStatusIpV6Cb)(AGnssExtStatusIpV6 status);

/* Constructs for interaction with loc_net_iface library */
typedef void (*LocAgpsOpenResultCb)(bool isSuccess, AGpsExtType agpsType, const char* apn,
        AGpsBearerType bearerType, void* userDataPtr);

typedef void (*LocAgpsCloseResultCb)(bool isSuccess, AGpsExtType agpsType, void* userDataPtr);

/* Shared resources of LocIpc */
#define LOC_IPC_HAL "/dev/socket/location/socket_hal"
#define LOC_IPC_XTRA "/dev/socket/location/xtra/socket_xtra"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GPS_EXTENDED_C_H */
