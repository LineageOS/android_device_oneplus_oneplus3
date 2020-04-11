/* Copyright (c) 2012-2019, The Linux Foundation. All rights reserved.
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
 *
 */

#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_nmea"
#include <loc_nmea.h>
#include <math.h>
#include <log_util.h>
#include <loc_pla.h>
#include <loc_cfg.h>

#define GLONASS_SV_ID_OFFSET 64
#define QZSS_SV_ID_OFFSET    (-192)
#define MAX_SV_COUNT_SUPPORTED_IN_ONE_CONSTELLATION  64
#define MAX_SATELLITES_IN_USE 12
#define MSEC_IN_ONE_WEEK      604800000ULL
#define UTC_GPS_OFFSET_MSECS  315964800000ULL

// GNSS system id according to NMEA spec
#define SYSTEM_ID_GPS          1
#define SYSTEM_ID_GLONASS      2
#define SYSTEM_ID_GALILEO      3
#define SYSTEM_ID_BDS          4
#define SYSTEM_ID_QZSS         5
#define SYSTEM_ID_NAVIC        6

//GNSS signal id according to NMEA spec
#define SIGNAL_ID_ALL_SIGNALS  0
#define SIGNAL_ID_GPS_L1CA     1
#define SIGNAL_ID_GPS_L1P      2
#define SIGNAL_ID_GPS_L1M      3
#define SIGNAL_ID_GPS_L2P      4
#define SIGNAL_ID_GPS_L2CM     5
#define SIGNAL_ID_GPS_L2CL     6
#define SIGNAL_ID_GPS_L5I      7
#define SIGNAL_ID_GPS_L5Q      8


#define SIGNAL_ID_GLO_G1CA     1
#define SIGNAL_ID_GLO_G1P      2
#define SIGNAL_ID_GLO_G2CA     3
#define SIGNAL_ID_GLO_G2P      4


#define SIGNAL_ID_GAL_E5A      1
#define SIGNAL_ID_GAL_E5B      2
#define SIGNAL_ID_GAL_E5AB     3
#define SIGNAL_ID_GAL_E6A      4
#define SIGNAL_ID_GAL_E6BC     5
#define SIGNAL_ID_GAL_L1A      6
#define SIGNAL_ID_GAL_L1BC     7

#define SIGNAL_ID_BDS_B1I      1
#define SIGNAL_ID_BDS_B1Q      2
#define SIGNAL_ID_BDS_B1C      3
#define SIGNAL_ID_BDS_B1A      4
#define SIGNAL_ID_BDS_B2A      5
#define SIGNAL_ID_BDS_B2B      6
#define SIGNAL_ID_BDS_B2AB     7
#define SIGNAL_ID_BDS_B3I      8
#define SIGNAL_ID_BDS_B3Q      9
#define SIGNAL_ID_BDS_B3A      0xA
#define SIGNAL_ID_BDS_B2I      0xB
#define SIGNAL_ID_BDS_B2Q      0xC

#define SIGNAL_ID_QZSS_L1CA    1
#define SIGNAL_ID_QZSS_L1CD    2
#define SIGNAL_ID_QZSS_L1CP    3
#define SIGNAL_ID_QZSS_LIS     4
#define SIGNAL_ID_QZSS_L2CM    5
#define SIGNAL_ID_QZSS_L2CL    6
#define SIGNAL_ID_QZSS_L5I     7
#define SIGNAL_ID_QZSS_L5Q     8
#define SIGNAL_ID_QZSS_L6D     9
#define SIGNAL_ID_QZSS_L6E     0xA

#define SIGNAL_ID_NAVIC_L5SPS  1
#define SIGNAL_ID_NAVIC_SSPS   2
#define SIGNAL_ID_NAVIC_L5RS   3
#define SIGNAL_ID_NAVIC_SRS    4
#define SIGNAL_ID_NAVIC_L1SPS  5


typedef struct loc_nmea_sv_meta_s
{
    char talker[3];
    LocGnssConstellationType svType;
    uint64_t mask;
    uint32_t svCount;
    uint32_t totalSvUsedCount;
    uint32_t svIdOffset;
    uint32_t signalId;
    uint32_t systemId;
} loc_nmea_sv_meta;

typedef struct loc_sv_cache_info_s
{
    uint64_t gps_used_mask;
    uint64_t glo_used_mask;
    uint64_t gal_used_mask;
    uint64_t qzss_used_mask;
    uint64_t bds_used_mask;
    uint64_t navic_used_mask;
    uint32_t gps_l1_count;
    uint32_t gps_l5_count;
    uint32_t glo_g1_count;
    uint32_t glo_g2_count;
    uint32_t gal_e1_count;
    uint32_t gal_e5_count;
    uint32_t qzss_l1_count;
    uint32_t qzss_l5_count;
    uint32_t bds_b1_count;
    uint32_t bds_b2_count;
    uint32_t navic_l5_count;
    float hdop;
    float pdop;
    float vdop;
} loc_sv_cache_info;

/*===========================================================================
FUNCTION    convert_Lla_to_Ecef

DESCRIPTION
   Convert LLA to ECEF

DEPENDENCIES
   NONE

RETURN VALUE
   NONE

SIDE EFFECTS
   N/A

===========================================================================*/
static void convert_Lla_to_Ecef(const LocLla& plla, LocEcef& pecef)
{
    double r;

    r = MAJA / sqrt(1.0 - ESQR * sin(plla.lat) * sin(plla.lat));
    pecef.X = (r + plla.alt) * cos(plla.lat) * cos(plla.lon);
    pecef.Y = (r + plla.alt) * cos(plla.lat) * sin(plla.lon);
    pecef.Z = (r * OMES + plla.alt) * sin(plla.lat);
}

/*===========================================================================
FUNCTION    convert_WGS84_to_PZ90

DESCRIPTION
   Convert datum from WGS84 to PZ90

DEPENDENCIES
   NONE

RETURN VALUE
   NONE

SIDE EFFECTS
   N/A

===========================================================================*/
static void convert_WGS84_to_PZ90(const LocEcef& pWGS84, LocEcef& pPZ90)
{
    double deltaX     = DatumConstFromWGS84[0];
    double deltaY     = DatumConstFromWGS84[1];
    double deltaZ     = DatumConstFromWGS84[2];
    double deltaScale = DatumConstFromWGS84[3];
    double rotX       = DatumConstFromWGS84[4];
    double rotY       = DatumConstFromWGS84[5];
    double rotZ       = DatumConstFromWGS84[6];

    pPZ90.X = deltaX + deltaScale * (pWGS84.X + rotZ * pWGS84.Y - rotY * pWGS84.Z);
    pPZ90.Y = deltaY + deltaScale * (pWGS84.Y - rotZ * pWGS84.X + rotX * pWGS84.Z);
    pPZ90.Z = deltaZ + deltaScale * (pWGS84.Z + rotY * pWGS84.X - rotX * pWGS84.Y);
}

/*===========================================================================
FUNCTION    convert_Ecef_to_Lla

DESCRIPTION
   Convert ECEF to LLA

DEPENDENCIES
   NONE

RETURN VALUE
   NONE

SIDE EFFECTS
   N/A

===========================================================================*/
static void convert_Ecef_to_Lla(const LocEcef& pecef, LocLla& plla)
{
    double p, r;
    double EcefA = C_PZ90A;
    double EcefB = C_PZ90B;
    double Ecef1Mf;
    double EcefE2;
    double Mu;
    double Smu;
    double Cmu;
    double Phi;
    double Sphi;
    double N;

    p = sqrt(pecef.X * pecef.X + pecef.Y * pecef.Y);
    r = sqrt(p * p + pecef.Z * pecef.Z);
    if (r < 1.0) {
        plla.lat = 1.0;
        plla.lon = 1.0;
        plla.alt = 1.0;
    }
    Ecef1Mf = 1.0 - (EcefA - EcefB) / EcefA;
    EcefE2 = 1.0 - (EcefB * EcefB) / (EcefA * EcefA);
    if (p > 1.0) {
        Mu = atan2(pecef.Z * (Ecef1Mf + EcefE2 * EcefA / r), p);
    } else {
        if (pecef.Z > 0.0) {
            Mu = M_PI / 2.0;
        } else {
            Mu = -M_PI / 2.0;
        }
    }
    Smu = sin(Mu);
    Cmu = cos(Mu);
    Phi = atan2(pecef.Z * Ecef1Mf + EcefE2 * EcefA * Smu * Smu * Smu,
                Ecef1Mf * (p - EcefE2 * EcefA * Cmu * Cmu * Cmu));
    Sphi = sin(Phi);
    N = EcefA / sqrt(1.0 - EcefE2 * Sphi * Sphi);
    plla.alt = p * cos(Phi) + pecef.Z * Sphi - EcefA * EcefA/N;
    plla.lat = Phi;
    if ( p > 1.0) {
        plla.lon = atan2(pecef.Y, pecef.X);
    } else {
        plla.lon = 0.0;
    }
}

/*===========================================================================
FUNCTION    convert_signalType_to_signalId

DESCRIPTION
   convert signalType to signal ID

DEPENDENCIES
   NONE

RETURN VALUE
   value of signal ID

SIDE EFFECTS
   N/A

===========================================================================*/
static uint32_t convert_signalType_to_signalId(GnssSignalTypeMask signalType)
{
    uint32_t signalId = SIGNAL_ID_ALL_SIGNALS;

    switch (signalType) {
        case GNSS_SIGNAL_GPS_L1CA:
            signalId = SIGNAL_ID_GPS_L1CA;
            break;
        case GNSS_SIGNAL_GPS_L2:
            signalId = SIGNAL_ID_GPS_L2CL;
            break;
        case GNSS_SIGNAL_GPS_L5:
            signalId = SIGNAL_ID_GPS_L5Q;
            break;
        case GNSS_SIGNAL_GLONASS_G1:
            signalId = SIGNAL_ID_GLO_G1CA;
            break;
        case GNSS_SIGNAL_GLONASS_G2:
            signalId = SIGNAL_ID_GLO_G2CA;
            break;
        case GNSS_SIGNAL_GALILEO_E1:
            signalId = SIGNAL_ID_GAL_L1BC;
            break;
        case GNSS_SIGNAL_GALILEO_E5A:
            signalId = SIGNAL_ID_GAL_E5A;
            break;
        case GNSS_SIGNAL_GALILEO_E5B:
            signalId = SIGNAL_ID_GAL_E5B;
            break;
        case GNSS_SIGNAL_QZSS_L1CA:
            signalId = SIGNAL_ID_QZSS_L1CA;
            break;
        case GNSS_SIGNAL_QZSS_L2:
            signalId = SIGNAL_ID_QZSS_L2CL;
            break;
        case GNSS_SIGNAL_QZSS_L5:
            signalId = SIGNAL_ID_QZSS_L5Q;
            break;
        case GNSS_SIGNAL_BEIDOU_B1I:
            signalId = SIGNAL_ID_BDS_B1I;
            break;
        case GNSS_SIGNAL_BEIDOU_B1C:
            signalId = SIGNAL_ID_BDS_B1C;
            break;
        case GNSS_SIGNAL_BEIDOU_B2I:
            signalId = SIGNAL_ID_BDS_B2I;
            break;
        case GNSS_SIGNAL_BEIDOU_B2AI:
        case GNSS_SIGNAL_BEIDOU_B2AQ:
            signalId = SIGNAL_ID_BDS_B2A;
            break;
        case GNSS_SIGNAL_NAVIC_L5:
            signalId = SIGNAL_ID_NAVIC_L5SPS;
            break;
        default:
            signalId = SIGNAL_ID_ALL_SIGNALS;
    }

    return signalId;

}

/*===========================================================================
FUNCTION    get_sv_count_from_mask

DESCRIPTION
   get the sv count from bit mask

DEPENDENCIES
   NONE

RETURN VALUE
   value of sv count

SIDE EFFECTS
   N/A

===========================================================================*/
static uint32_t get_sv_count_from_mask(uint64_t svMask, int totalSvCount)
{
    int index = 0;
    uint32_t svCount = 0;

    if(totalSvCount > MAX_SV_COUNT_SUPPORTED_IN_ONE_CONSTELLATION) {
        LOC_LOGE("total SV count in this constellation %d exceeded limit %d",
                 totalSvCount, MAX_SV_COUNT_SUPPORTED_IN_ONE_CONSTELLATION);
    }
    for(index = 0; index < totalSvCount; index++) {
        if(svMask & 0x1)
            svCount += 1;
        svMask >>= 1;
    }
    return svCount;
}

/*===========================================================================
FUNCTION    loc_nmea_sv_meta_init

DESCRIPTION
   Init loc_nmea_sv_meta passed in

DEPENDENCIES
   NONE

RETURN VALUE
   Pointer to loc_nmea_sv_meta

SIDE EFFECTS
   N/A

===========================================================================*/
static loc_nmea_sv_meta* loc_nmea_sv_meta_init(loc_nmea_sv_meta& sv_meta,
                                               loc_sv_cache_info& sv_cache_info,
                                               GnssSvType svType,
                                               GnssSignalTypeMask signalType,
                                               bool needCombine)
{
    memset(&sv_meta, 0, sizeof(sv_meta));
    sv_meta.svType = svType;

    switch (svType)
    {
        case GNSS_SV_TYPE_GPS:
            sv_meta.talker[0] = 'G';
            sv_meta.talker[1] = 'P';
            sv_meta.mask = sv_cache_info.gps_used_mask;
            sv_meta.systemId = SYSTEM_ID_GPS;
            if (GNSS_SIGNAL_GPS_L1CA == signalType) {
                sv_meta.svCount = sv_cache_info.gps_l1_count;
            } else if (GNSS_SIGNAL_GPS_L5 == signalType) {
                sv_meta.svCount = sv_cache_info.gps_l5_count;
            }
            break;
        case GNSS_SV_TYPE_GLONASS:
            sv_meta.talker[0] = 'G';
            sv_meta.talker[1] = 'L';
            sv_meta.mask = sv_cache_info.glo_used_mask;
            // GLONASS SV ids are from 65-96
            sv_meta.svIdOffset = GLONASS_SV_ID_OFFSET;
            sv_meta.systemId = SYSTEM_ID_GLONASS;
            if (GNSS_SIGNAL_GLONASS_G1 == signalType) {
                sv_meta.svCount = sv_cache_info.glo_g1_count;
            } else if (GNSS_SIGNAL_GLONASS_G2 == signalType) {
                sv_meta.svCount = sv_cache_info.glo_g2_count;
            }
            break;
        case GNSS_SV_TYPE_GALILEO:
            sv_meta.talker[0] = 'G';
            sv_meta.talker[1] = 'A';
            sv_meta.mask = sv_cache_info.gal_used_mask;
            sv_meta.systemId = SYSTEM_ID_GALILEO;
            if (GNSS_SIGNAL_GALILEO_E1 == signalType) {
                sv_meta.svCount = sv_cache_info.gal_e1_count;
            } else if (GNSS_SIGNAL_GALILEO_E5A == signalType) {
                sv_meta.svCount = sv_cache_info.gal_e5_count;
            }
            break;
        case GNSS_SV_TYPE_QZSS:
            sv_meta.talker[0] = 'G';
            sv_meta.talker[1] = 'Q';
            sv_meta.mask = sv_cache_info.qzss_used_mask;
            // QZSS SV ids are from 193-199. So keep svIdOffset -192
            sv_meta.svIdOffset = QZSS_SV_ID_OFFSET;
            sv_meta.systemId = SYSTEM_ID_QZSS;
            if (GNSS_SIGNAL_QZSS_L1CA == signalType) {
                sv_meta.svCount = sv_cache_info.qzss_l1_count;
            } else if (GNSS_SIGNAL_QZSS_L5 == signalType) {
                sv_meta.svCount = sv_cache_info.qzss_l5_count;
            }
            break;
        case GNSS_SV_TYPE_BEIDOU:
            sv_meta.talker[0] = 'G';
            sv_meta.talker[1] = 'B';
            sv_meta.mask = sv_cache_info.bds_used_mask;
            // BDS SV ids are from 201-235. So keep svIdOffset 0
            sv_meta.systemId = SYSTEM_ID_BDS;
            if (GNSS_SIGNAL_BEIDOU_B1I == signalType) {
                sv_meta.svCount = sv_cache_info.bds_b1_count;
            } else if (GNSS_SIGNAL_BEIDOU_B2AI == signalType) {
                sv_meta.svCount = sv_cache_info.bds_b2_count;
            }
            break;
        case GNSS_SV_TYPE_NAVIC:
            sv_meta.talker[0] = 'G';
            sv_meta.talker[1] = 'I';
            sv_meta.mask = sv_cache_info.navic_used_mask;
            // NAVIC SV ids are from 401-414. So keep svIdOffset 0
            sv_meta.systemId = SYSTEM_ID_NAVIC;
            if (GNSS_SIGNAL_NAVIC_L5 == signalType) {
                sv_meta.svCount = sv_cache_info.navic_l5_count;
            }
            break;
        default:
            LOC_LOGE("NMEA Error unknow constellation type: %d", svType);
            return NULL;
    }
    sv_meta.signalId = convert_signalType_to_signalId(signalType);
    sv_meta.totalSvUsedCount =
            get_sv_count_from_mask(sv_cache_info.gps_used_mask,
                    GPS_SV_PRN_MAX - GPS_SV_PRN_MIN + 1) +
            get_sv_count_from_mask(sv_cache_info.glo_used_mask,
                    GLO_SV_PRN_MAX - GLO_SV_PRN_MIN + 1) +
            get_sv_count_from_mask(sv_cache_info.gal_used_mask,
                    GAL_SV_PRN_MAX - GAL_SV_PRN_MIN + 1) +
            get_sv_count_from_mask(sv_cache_info.qzss_used_mask,
                    QZSS_SV_PRN_MAX - QZSS_SV_PRN_MIN + 1) +
            get_sv_count_from_mask(sv_cache_info.bds_used_mask,
                    BDS_SV_PRN_MAX - BDS_SV_PRN_MIN + 1) +
            get_sv_count_from_mask(sv_cache_info.navic_used_mask,
                    NAVIC_SV_PRN_MAX - NAVIC_SV_PRN_MIN + 1);
    if (needCombine &&
                (sv_cache_info.gps_used_mask ? 1 : 0) +
                (sv_cache_info.glo_used_mask ? 1 : 0) +
                (sv_cache_info.gal_used_mask ? 1 : 0) +
                (sv_cache_info.qzss_used_mask ? 1 : 0) +
                (sv_cache_info.bds_used_mask ? 1 : 0) +
                (sv_cache_info.navic_used_mask ? 1 : 0) > 1)
    {
        // If GPS, GLONASS, Galileo, QZSS, BDS etc. are combined
        // to obtain the reported position solution,
        // talker shall be set to GN, to indicate that
        // the satellites are used in a combined solution
        sv_meta.talker[0] = 'G';
        sv_meta.talker[1] = 'N';
    }
    return &sv_meta;
}

/*===========================================================================
FUNCTION    loc_nmea_put_checksum

DESCRIPTION
   Generate NMEA sentences generated based on position report

DEPENDENCIES
   NONE

RETURN VALUE
   Total length of the nmea sentence

SIDE EFFECTS
   N/A

===========================================================================*/
static int loc_nmea_put_checksum(char *pNmea, int maxSize)
{
    uint8_t checksum = 0;
    int length = 0;
    if(NULL == pNmea)
        return 0;

    pNmea++; //skip the $
    while (*pNmea != '\0')
    {
        checksum ^= *pNmea++;
        length++;
    }

    // length now contains nmea sentence string length not including $ sign.
    int checksumLength = snprintf(pNmea,(maxSize-length-1),"*%02X\r\n", checksum);

    // total length of nmea sentence is length of nmea sentence inc $ sign plus
    // length of checksum (+1 is to cover the $ character in the length).
    return (length + checksumLength + 1);
}

/*===========================================================================
FUNCTION    loc_nmea_generate_GSA

DESCRIPTION
   Generate NMEA GSA sentences generated based on position report
   Currently below sentences are generated:
   - $GPGSA : GPS DOP and active SVs
   - $GLGSA : GLONASS DOP and active SVs
   - $GAGSA : GALILEO DOP and active SVs
   - $GNGSA : GNSS DOP and active SVs

DEPENDENCIES
   NONE

RETURN VALUE
   Number of SVs used

SIDE EFFECTS
   N/A

===========================================================================*/
static uint32_t loc_nmea_generate_GSA(const GpsLocationExtended &locationExtended,
                              char* sentence,
                              int bufSize,
                              loc_nmea_sv_meta* sv_meta_p,
                              std::vector<std::string> &nmeaArraystr)
{
    if (!sentence || bufSize <= 0 || !sv_meta_p)
    {
        LOC_LOGE("NMEA Error invalid arguments.");
        return 0;
    }

    char* pMarker = sentence;
    int lengthRemaining = bufSize;
    int length = 0;

    uint32_t svUsedCount = 0;
    uint32_t svUsedList[64] = {0};

    char fixType = '\0';

    const char* talker = sv_meta_p->talker;
    uint32_t svIdOffset = sv_meta_p->svIdOffset;
    uint64_t mask = sv_meta_p->mask;

    if(sv_meta_p->svType != GNSS_SV_TYPE_GLONASS) {
        svIdOffset = 0;
    }

    for (uint8_t i = 1; mask > 0 && svUsedCount < 64; i++)
    {
        if (mask & 1)
            svUsedList[svUsedCount++] = i + svIdOffset;
        mask = mask >> 1;
    }

    if (svUsedCount == 0)
        return 0;

    if (sv_meta_p->totalSvUsedCount == 0)
        fixType = '1'; // no fix
    else if (sv_meta_p->totalSvUsedCount <= 3)
        fixType = '2'; // 2D fix
    else
        fixType = '3'; // 3D fix

    // Start printing the sentence
    // Format: $--GSA,a,x,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,p.p,h.h,v.v,s*cc
    // a : Mode  : A : Automatic, allowed to automatically switch 2D/3D
    // x : Fixtype : 1 (no fix), 2 (2D fix), 3 (3D fix)
    // xx : 12 SV ID
    // p.p : Position DOP (Dilution of Precision)
    // h.h : Horizontal DOP
    // v.v : Vertical DOP
    // s : GNSS System Id
    // cc : Checksum value
    length = snprintf(pMarker, lengthRemaining, "$%sGSA,A,%c,", talker, fixType);

    if (length < 0 || length >= lengthRemaining)
    {
        LOC_LOGE("NMEA Error in string formatting");
        return 0;
    }
    pMarker += length;
    lengthRemaining -= length;

    // Add first 12 satellite IDs
    for (uint8_t i = 0; i < 12; i++)
    {
        if (i < svUsedCount)
            length = snprintf(pMarker, lengthRemaining, "%02d,", svUsedList[i]);
        else
            length = snprintf(pMarker, lengthRemaining, ",");

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return 0;
        }
        pMarker += length;
        lengthRemaining -= length;
    }

    // Add the position/horizontal/vertical DOP values
    if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_DOP)
    {
        length = snprintf(pMarker, lengthRemaining, "%.1f,%.1f,%.1f,",
                locationExtended.pdop,
                locationExtended.hdop,
                locationExtended.vdop);
    }
    else
    {   // no dop
        length = snprintf(pMarker, lengthRemaining, ",,,");
    }
    pMarker += length;
    lengthRemaining -= length;

    // system id
    length = snprintf(pMarker, lengthRemaining, "%d", sv_meta_p->systemId);
    pMarker += length;
    lengthRemaining -= length;

    /* Sentence is ready, add checksum and broadcast */
    length = loc_nmea_put_checksum(sentence, bufSize);
    nmeaArraystr.push_back(sentence);

    return svUsedCount;
}

/*===========================================================================
FUNCTION    loc_nmea_generate_GSV

DESCRIPTION
   Generate NMEA GSV sentences generated based on sv report
   Currently below sentences are generated:
   - $GPGSV: GPS Satellites in View
   - $GLGSV: GLONASS Satellites in View
   - $GAGSV: GALILEO Satellites in View

DEPENDENCIES
   NONE

RETURN VALUE
   NONE

SIDE EFFECTS
   N/A

===========================================================================*/
static void loc_nmea_generate_GSV(const GnssSvNotification &svNotify,
                              char* sentence,
                              int bufSize,
                              loc_nmea_sv_meta* sv_meta_p,
                              std::vector<std::string> &nmeaArraystr)
{
    if (!sentence || bufSize <= 0)
    {
        LOC_LOGE("NMEA Error invalid argument.");
        return;
    }

    char* pMarker = sentence;
    int lengthRemaining = bufSize;
    int length = 0;
    int sentenceCount = 0;
    int sentenceNumber = 1;
    size_t svNumber = 1;

    const char* talker = sv_meta_p->talker;
    uint32_t svIdOffset = sv_meta_p->svIdOffset;
    int svCount = sv_meta_p->svCount;
    if (svCount <= 0)
    {
        LOC_LOGV("No SV in view for talker ID:%s, signal ID:%X", talker, sv_meta_p->signalId);
        return;
    }

    svNumber = 1;
    sentenceNumber = 1;
    sentenceCount = svCount / 4 + (svCount % 4 != 0);

    while (sentenceNumber <= sentenceCount)
    {
        pMarker = sentence;
        lengthRemaining = bufSize;

        length = snprintf(pMarker, lengthRemaining, "$%sGSV,%d,%d,%02d",
                talker, sentenceCount, sentenceNumber, svCount);

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        for (int i=0; (svNumber <= svNotify.count) && (i < 4);  svNumber++)
        {
            GnssSignalTypeMask signalType = svNotify.gnssSvs[svNumber-1].gnssSignalTypeMask;
            if (0 == signalType) {
                // If no signal type in report, it means default L1,G1,E1,B1I
                switch (svNotify.gnssSvs[svNumber - 1].type)
                {
                    case GNSS_SV_TYPE_GPS:
                        signalType = GNSS_SIGNAL_GPS_L1CA;
                        break;
                    case GNSS_SV_TYPE_GLONASS:
                        signalType = GNSS_SIGNAL_GLONASS_G1;
                        break;
                    case GNSS_SV_TYPE_GALILEO:
                        signalType = GNSS_SIGNAL_GALILEO_E1;
                        break;
                    case GNSS_SV_TYPE_QZSS:
                        signalType = GNSS_SIGNAL_QZSS_L1CA;
                        break;
                    case GNSS_SV_TYPE_BEIDOU:
                        signalType = GNSS_SIGNAL_BEIDOU_B1I;
                        break;
                    case GNSS_SV_TYPE_SBAS:
                        signalType = GNSS_SIGNAL_SBAS_L1;
                        break;
                    case GNSS_SV_TYPE_NAVIC:
                        signalType = GNSS_SIGNAL_NAVIC_L5;
                        break;
                    default:
                        LOC_LOGE("NMEA Error unknow constellation type: %d",
                                svNotify.gnssSvs[svNumber - 1].type);
                        continue;
                }
            }

            if (sv_meta_p->svType == svNotify.gnssSvs[svNumber - 1].type &&
                    sv_meta_p->signalId == convert_signalType_to_signalId(signalType))
            {
                uint16_t svId = svNotify.gnssSvs[svNumber - 1].svId;
                // For QZSS we adjusted SV id's in GnssAdapter, we need to re-adjust here
                if (GNSS_SV_TYPE_QZSS == svNotify.gnssSvs[svNumber - 1].type) {
                    svId = svId - (QZSS_SV_PRN_MIN - 1);
                }
                length = snprintf(pMarker, lengthRemaining,",%02d,%02d,%03d,",
                        svId + svIdOffset,
                        (int)(0.5 + svNotify.gnssSvs[svNumber - 1].elevation), //float to int
                        (int)(0.5 + svNotify.gnssSvs[svNumber - 1].azimuth)); //float to int

                if (length < 0 || length >= lengthRemaining)
                {
                    LOC_LOGE("NMEA Error in string formatting");
                    return;
                }
                pMarker += length;
                lengthRemaining -= length;

                if (svNotify.gnssSvs[svNumber - 1].cN0Dbhz > 0)
                {
                    length = snprintf(pMarker, lengthRemaining,"%02d",
                            (int)(0.5 + svNotify.gnssSvs[svNumber - 1].cN0Dbhz)); //float to int

                    if (length < 0 || length >= lengthRemaining)
                    {
                        LOC_LOGE("NMEA Error in string formatting");
                        return;
                    }
                    pMarker += length;
                    lengthRemaining -= length;
                }

                i++;
            }

        }

        // append signalId
        length = snprintf(pMarker, lengthRemaining,",%X",sv_meta_p->signalId);
        pMarker += length;
        lengthRemaining -= length;

        length = loc_nmea_put_checksum(sentence, bufSize);
        nmeaArraystr.push_back(sentence);
        sentenceNumber++;

    }  //while
}

/*===========================================================================
FUNCTION    loc_nmea_generate_DTM

DESCRIPTION
   Generate NMEA DTM sentences generated based on position report

DEPENDENCIES
   NONE

RETURN VALUE
   NONE

SIDE EFFECTS
   N/A

===========================================================================*/
static void loc_nmea_generate_DTM(const LocLla &ref_lla,
                                  const LocLla &local_lla,
                                  char *talker,
                                  char *sentence,
                                  int bufSize)
{
    char* pMarker = sentence;
    int lengthRemaining = bufSize;
    int length = 0;
    int datum_type;
    char ref_datum[4] = {0};
    char local_datum[4] = {0};
    double lla_offset[3] = {0};
    char latHem, longHem;
    double latMins, longMins;



    datum_type = loc_get_datum_type();
    switch (datum_type) {
        case LOC_GNSS_DATUM_WGS84:
            ref_datum[0] = 'W';
            ref_datum[1] = '8';
            ref_datum[2] = '4';
            local_datum[0] = 'P';
            local_datum[1] = '9';
            local_datum[2] = '0';
            break;
        case LOC_GNSS_DATUM_PZ90:
            ref_datum[0] = 'P';
            ref_datum[1] = '9';
            ref_datum[2] = '0';
            local_datum[0] = 'W';
            local_datum[1] = '8';
            local_datum[2] = '4';
            break;
        default:
            break;
    }
    length = snprintf(pMarker , lengthRemaining , "$%sDTM,%s,," , talker, local_datum);
    if (length < 0 || length >= lengthRemaining) {
        LOC_LOGE("NMEA Error in string formatting");
        return;
    }
    pMarker += length;
    lengthRemaining -= length;

    lla_offset[0] = local_lla.lat - ref_lla.lat;
    lla_offset[1] = fmod(local_lla.lon - ref_lla.lon, 360.0);
    if (lla_offset[1] < -180.0) {
        lla_offset[1] += 360.0;
    } else if ( lla_offset[1] > 180.0) {
        lla_offset[1] -= 360.0;
    }
    lla_offset[2] = local_lla.alt - ref_lla.alt;
    if (lla_offset[0] > 0.0) {
        latHem = 'N';
    } else {
        latHem = 'S';
        lla_offset[0] *= -1.0;
    }
    latMins = fmod(lla_offset[0] * 60.0, 60.0);
    if (lla_offset[1] < 0.0) {
        longHem = 'W';
        lla_offset[1] *= -1.0;
    }else {
        longHem = 'E';
    }
    longMins = fmod(lla_offset[1] * 60.0, 60.0);
    length = snprintf(pMarker, lengthRemaining, "%02d%09.6lf,%c,%03d%09.6lf,%c,%.3lf,",
                     (uint8_t)floor(lla_offset[0]), latMins, latHem,
                     (uint8_t)floor(lla_offset[1]), longMins, longHem, lla_offset[2]);
    if (length < 0 || length >= lengthRemaining) {
        LOC_LOGE("NMEA Error in string formatting");
        return;
    }
    pMarker += length;
    lengthRemaining -= length;
    length = snprintf(pMarker , lengthRemaining , "%s" , ref_datum);
    if (length < 0 || length >= lengthRemaining) {
        LOC_LOGE("NMEA Error in string formatting");
        return;
    }
    pMarker += length;
    lengthRemaining -= length;

    length = loc_nmea_put_checksum(sentence, bufSize);
}

/*===========================================================================
FUNCTION    get_utctime_with_leapsecond_transition

DESCRIPTION
   This function returns true if the position report is generated during
   leap second transition period. If not, then the utc timestamp returned
   will be set to the timestamp in the position report. If it is,
   then the utc timestamp returned will need to take into account
   of the leap second transition so that proper calendar year/month/date
   can be calculated from the returned utc timestamp.

DEPENDENCIES
   NONE

RETURN VALUE
   true: position report is generated in leap second transition period.

SIDE EFFECTS
   N/A

===========================================================================*/
static bool get_utctime_with_leapsecond_transition(
        const UlpLocation &location,
        const GpsLocationExtended &locationExtended,
        const LocationSystemInfo &systemInfo,
        LocGpsUtcTime &utcPosTimestamp)
{
    bool inTransition = false;

    // position report is not generated during leap second transition,
    // we can use the UTC timestamp from position report as is
    utcPosTimestamp = location.gpsLocation.timestamp;

    // Check whether we are in leap second transition.
    // If so, per NMEA spec, we need to display the extra second in format of 23:59:60
    // with year/month/date not getting advanced.
    if ((locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_GPS_TIME) &&
        ((systemInfo.systemInfoMask & LOCATION_SYS_INFO_LEAP_SECOND) &&
         (systemInfo.leapSecondSysInfo.leapSecondInfoMask &
          LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT))) {

        const LeapSecondChangeInfo  &leapSecondChangeInfo =
            systemInfo.leapSecondSysInfo.leapSecondChangeInfo;
        const GnssSystemTimeStructType &gpsTimestampLsChange =
            leapSecondChangeInfo.gpsTimestampLsChange;

        uint64_t gpsTimeLsChange = gpsTimestampLsChange.systemWeek * MSEC_IN_ONE_WEEK +
                                   gpsTimestampLsChange.systemMsec;
        uint64_t gpsTimePosReport = locationExtended.gpsTime.gpsWeek * MSEC_IN_ONE_WEEK +
                                    locationExtended.gpsTime.gpsTimeOfWeekMs;
        // we are only dealing with positive leap second change, as negative
        // leap second change has never occurred and should not occur in future
        if (leapSecondChangeInfo.leapSecondsAfterChange >
            leapSecondChangeInfo.leapSecondsBeforeChange) {
            // leap second adjustment is always 1 second at a time. It can happen
            // every quarter end and up to four times per year.
            if ((gpsTimePosReport >= gpsTimeLsChange) &&
                (gpsTimePosReport < (gpsTimeLsChange + 1000))) {
                inTransition = true;
                utcPosTimestamp = gpsTimeLsChange + UTC_GPS_OFFSET_MSECS -
                                  leapSecondChangeInfo.leapSecondsBeforeChange * 1000;

                // we substract 1000 milli-seconds from UTC timestmap in order to calculate the
                // proper year, month and date during leap second transtion.
                // Let us give an example, assuming leap second transition is scheduled on 2019,
                // Dec 31st mid night. When leap second transition is happening,
                // instead of outputting the time as 2020, Jan, 1st, 00 hour, 00 min, and 00 sec.
                // The time need to be displayed as 2019, Dec, 31st, 23 hour, 59 min and 60 sec.
                utcPosTimestamp -= 1000;
            }
        }
    }
    return inTransition;
}

/*===========================================================================
FUNCTION    loc_nmea_get_fix_quality

DESCRIPTION
   This function obtains the fix quality for GGA sentence, mode indicator
   for RMC and VTG sentence based on nav solution mask and tech mask in
   the postion report.

DEPENDENCIES
   NONE

Output parameter
   ggaGpsQuality: gps quality field in GGA sentence
   rmcModeIndicator: mode indicator field in RMC sentence
   vtgModeIndicator: mode indicator field in VTG sentence

SIDE EFFECTS
   N/A

===========================================================================*/
static void loc_nmea_get_fix_quality(const UlpLocation & location,
                                     const GpsLocationExtended & locationExtended,
                                     bool custom_gga_fix_quality,
                                     char ggaGpsQuality[3],
                                     char & rmcModeIndicator,
                                     char & vtgModeIndicator) {

    ggaGpsQuality[0] = '0'; // 0 means no fix
    rmcModeIndicator = 'N'; // N means no fix
    vtgModeIndicator = 'N'; // N means no fix

    do {
        // GGA fix quality is defined in NMEA spec as below:
        // https://www.trimble.com/OEM_ReceiverHelp/V4.44/en/NMEA-0183messages_GGA.html
        // Fix quality: 0 = invalid
        //              1 = GPS fix (SPS)
        //              2 = DGPS fix
        //              3 = PPS fix
        //              4 = Real Time Kinematic
        //              5 = Float RTK
        //              6 = estimated (dead reckoning) (2.3 feature)
        //              7 = Manual input mode
        //              8 = Simulation mode
        if (!(location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_LAT_LONG)){
            break;
        }
        // NOTE: Order of the check is important
        if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_NAV_SOLUTION_MASK) {
            if (LOC_NAV_MASK_PPP_CORRECTION & locationExtended.navSolutionMask) {
                ggaGpsQuality[0] = '2';    // 2 means DGPS fix
                rmcModeIndicator = 'P'; // P means precise
                vtgModeIndicator = 'P'; // P means precise
                break;
            } else if (LOC_NAV_MASK_RTK_FIXED_CORRECTION & locationExtended.navSolutionMask){
                ggaGpsQuality[0] = '4';    // 4 means RTK Fixed fix
                rmcModeIndicator = 'R'; // use R (RTK fixed)
                vtgModeIndicator = 'D'; // use D (differential) as
                                        // no RTK fixed defined for VTG in NMEA 183 spec
                break;
            } else if (LOC_NAV_MASK_RTK_CORRECTION & locationExtended.navSolutionMask){
                ggaGpsQuality[0] = '5';    // 5 means RTK float fix
                rmcModeIndicator = 'F'; // F means RTK float fix
                vtgModeIndicator = 'D'; // use D (differential) as
                                        // no RTK float defined for VTG in NMEA 183 spec
                break;
            } else if (LOC_NAV_MASK_DGNSS_CORRECTION & locationExtended.navSolutionMask){
                ggaGpsQuality[0] = '2';    // 2 means DGPS fix
                rmcModeIndicator = 'D'; // D means differential
                vtgModeIndicator = 'D'; // D means differential
                break;
            } else if (LOC_NAV_MASK_SBAS_CORRECTION_IONO & locationExtended.navSolutionMask){
                ggaGpsQuality[0] = '2';    // 2 means DGPS fix
                rmcModeIndicator = 'D'; // D means differential
                vtgModeIndicator = 'D'; // D means differential
                break;
            }
        }
        // NOTE: Order of the check is important
        if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_POS_TECH_MASK) {
            if (LOC_POS_TECH_MASK_SATELLITE & locationExtended.tech_mask){
                ggaGpsQuality[0] = '1'; // 1 means GPS
                rmcModeIndicator = 'A'; // A means autonomous
                vtgModeIndicator = 'A'; // A means autonomous
                break;
            } else if (LOC_POS_TECH_MASK_SENSORS & locationExtended.tech_mask){
                ggaGpsQuality[0] = '6'; // 6 means estimated (dead reckoning)
                rmcModeIndicator = 'E'; // E means estimated (dead reckoning)
                vtgModeIndicator = 'E'; // E means estimated (dead reckoning)
                break;
            }
        }
    } while (0);

    do {
        // check for customized nmea enabled or not
        // with customized GGA quality enabled
        // PPP fix w/o sensor: 59, PPP fix w/ sensor: 69
        // DGNSS/SBAS correction fix w/o sensor: 2, w/ sensor: 62
        // RTK fixed fix w/o sensor: 4, w/ sensor: 64
        // RTK float fix w/o sensor: 5, w/ sensor: 65
        // SPE fix w/o sensor: 1, and w/ sensor: 61
        // Sensor dead reckoning fix: 6
        if (true == custom_gga_fix_quality) {
            if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_NAV_SOLUTION_MASK) {
                // PPP fix w/o sensor: fix quality will now be 59
                // PPP fix w sensor: fix quality will now be 69
                if (LOC_NAV_MASK_PPP_CORRECTION & locationExtended.navSolutionMask) {
                    if ((locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_POS_TECH_MASK) &&
                        (LOC_POS_TECH_MASK_SENSORS & locationExtended.tech_mask)) {
                        ggaGpsQuality[0] = '6';
                        ggaGpsQuality[1] = '9';
                    } else {
                        ggaGpsQuality[0] = '5';
                        ggaGpsQuality[1] = '9';
                    }
                    break;
                }
            }

            if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_POS_TECH_MASK) {
                if (LOC_POS_TECH_MASK_SENSORS & locationExtended.tech_mask){
                    char ggaQuality_copy = ggaGpsQuality[0];
                    ggaGpsQuality[0] = '6'; // 6 sensor assisted
                    // RTK fixed fix w/ sensor: fix quality will now be 64
                    // RTK float fix w/ sensor: 65
                    // DGNSS and/or SBAS correction fix and w/ sensor: 62
                    // GPS fix without correction and w/ sensor: 61
                    if ((LOC_NAV_MASK_RTK_FIXED_CORRECTION & locationExtended.navSolutionMask)||
                            (LOC_NAV_MASK_RTK_CORRECTION & locationExtended.navSolutionMask)||
                            (LOC_NAV_MASK_DGNSS_CORRECTION & locationExtended.navSolutionMask)||
                            (LOC_NAV_MASK_SBAS_CORRECTION_IONO & locationExtended.navSolutionMask)||
                            (LOC_POS_TECH_MASK_SATELLITE & locationExtended.tech_mask)) {
                        ggaGpsQuality[1] = ggaQuality_copy;
                        break;
                    }
                }
            }
        }
    } while (0);

    LOC_LOGv("gps quality: %s, rmc mode indicator: %c, vtg mode indicator: %c",
             ggaGpsQuality, rmcModeIndicator, vtgModeIndicator);
}

/*===========================================================================
FUNCTION    loc_nmea_generate_pos

DESCRIPTION
   Generate NMEA sentences generated based on position report
   Currently below sentences are generated within this function:
   - $GPGSA : GPS DOP and active SVs
   - $GLGSA : GLONASS DOP and active SVs
   - $GAGSA : GALILEO DOP and active SVs
   - $GNGSA : GNSS DOP and active SVs
   - $--VTG : Track made good and ground speed
   - $--RMC : Recommended minimum navigation information
   - $--GGA : Time, position and fix related data

DEPENDENCIES
   NONE

RETURN VALUE
   0

SIDE EFFECTS
   N/A

===========================================================================*/
void loc_nmea_generate_pos(const UlpLocation &location,
                               const GpsLocationExtended &locationExtended,
                               const LocationSystemInfo &systemInfo,
                               unsigned char generate_nmea,
                               bool custom_gga_fix_quality,
                               std::vector<std::string> &nmeaArraystr)
{
    ENTRY_LOG();

    LocGpsUtcTime utcPosTimestamp = 0;
    bool inLsTransition = false;

    inLsTransition = get_utctime_with_leapsecond_transition
                    (location, locationExtended, systemInfo, utcPosTimestamp);

    time_t utcTime(utcPosTimestamp/1000);
    tm * pTm = gmtime(&utcTime);
    if (NULL == pTm) {
        LOC_LOGE("gmtime failed");
        return;
    }

    char sentence[NMEA_SENTENCE_MAX_LENGTH] = {0};
    char sentence_DTM[NMEA_SENTENCE_MAX_LENGTH] = {0};
    char sentence_RMC[NMEA_SENTENCE_MAX_LENGTH] = {0};
    char sentence_GNS[NMEA_SENTENCE_MAX_LENGTH] = {0};
    char sentence_GGA[NMEA_SENTENCE_MAX_LENGTH] = {0};
    char* pMarker = sentence;
    int lengthRemaining = sizeof(sentence);
    int length = 0;
    int utcYear = pTm->tm_year % 100; // 2 digit year
    int utcMonth = pTm->tm_mon + 1; // tm_mon starts at zero
    int utcDay = pTm->tm_mday;
    int utcHours = pTm->tm_hour;
    int utcMinutes = pTm->tm_min;
    int utcSeconds = pTm->tm_sec;
    int utcMSeconds = (location.gpsLocation.timestamp)%1000;
    int datum_type = loc_get_datum_type();
    LocEcef ecef_w84;
    LocEcef ecef_p90;
    LocLla  lla_w84;
    LocLla  lla_p90;
    LocLla  ref_lla;
    LocLla  local_lla;

    if (inLsTransition) {
        // During leap second transition, we need to display the extra
        // leap second of hour, minute, second as (23:59:60)
        utcHours = 23;
        utcMinutes = 59;
        utcSeconds = 60;
        // As UTC timestamp is freezing during leap second transition,
        // retrieve milli-seconds portion from GPS timestamp.
        utcMSeconds = locationExtended.gpsTime.gpsTimeOfWeekMs % 1000;
    }

   loc_sv_cache_info sv_cache_info = {};

    if (GPS_LOCATION_EXTENDED_HAS_GNSS_SV_USED_DATA & locationExtended.flags) {
        sv_cache_info.gps_used_mask =
                locationExtended.gnss_sv_used_ids.gps_sv_used_ids_mask;
        sv_cache_info.glo_used_mask =
                locationExtended.gnss_sv_used_ids.glo_sv_used_ids_mask;
        sv_cache_info.gal_used_mask =
                locationExtended.gnss_sv_used_ids.gal_sv_used_ids_mask;
        sv_cache_info.bds_used_mask =
                locationExtended.gnss_sv_used_ids.bds_sv_used_ids_mask;
        sv_cache_info.qzss_used_mask =
                locationExtended.gnss_sv_used_ids.qzss_sv_used_ids_mask;
        sv_cache_info.navic_used_mask =
                locationExtended.gnss_sv_used_ids.navic_sv_used_ids_mask;
    }

    if (generate_nmea) {
        char talker[3] = {'G', 'P', '\0'};
        char modeIndicator[7] = {0};
        uint32_t svUsedCount = 0;
        uint32_t count = 0;
        loc_nmea_sv_meta sv_meta;
        // -------------------
        // ---$GPGSA/$GNGSA---
        // -------------------

        count = loc_nmea_generate_GSA(locationExtended, sentence, sizeof(sentence),
                        loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_GPS,
                        GNSS_SIGNAL_GPS_L1CA, true), nmeaArraystr);
        if (count > 0)
        {
            svUsedCount += count;
            talker[0] = sv_meta.talker[0];
            talker[1] = sv_meta.talker[1];
        }

        // -------------------
        // ---$GLGSA/$GNGSA---
        // -------------------

        count = loc_nmea_generate_GSA(locationExtended, sentence, sizeof(sentence),
                        loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_GLONASS,
                        GNSS_SIGNAL_GLONASS_G1, true), nmeaArraystr);
        if (count > 0)
        {
            svUsedCount += count;
            talker[0] = sv_meta.talker[0];
            talker[1] = sv_meta.talker[1];
        }

        // -------------------
        // ---$GAGSA/$GNGSA---
        // -------------------

        count = loc_nmea_generate_GSA(locationExtended, sentence, sizeof(sentence),
                        loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_GALILEO,
                        GNSS_SIGNAL_GALILEO_E1, true), nmeaArraystr);
        if (count > 0)
        {
            svUsedCount += count;
            talker[0] = sv_meta.talker[0];
            talker[1] = sv_meta.talker[1];
        }

        // ----------------------------
        // ---$GBGSA/$GNGSA (BEIDOU)---
        // ----------------------------
        count = loc_nmea_generate_GSA(locationExtended, sentence, sizeof(sentence),
                        loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_BEIDOU,
                        GNSS_SIGNAL_BEIDOU_B1I, true), nmeaArraystr);
        if (count > 0)
        {
            svUsedCount += count;
            talker[0] = sv_meta.talker[0];
            talker[1] = sv_meta.talker[1];
        }

        // --------------------------
        // ---$GQGSA/$GNGSA (QZSS)---
        // --------------------------

        count = loc_nmea_generate_GSA(locationExtended, sentence, sizeof(sentence),
                        loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_QZSS,
                        GNSS_SIGNAL_QZSS_L1CA, true), nmeaArraystr);
        if (count > 0)
        {
            svUsedCount += count;
            talker[0] = sv_meta.talker[0];
            talker[1] = sv_meta.talker[1];
        }

        char ggaGpsQuality[3] = {'0', '\0', '\0'};
        char rmcModeIndicator = 'N';
        char vtgModeIndicator = 'N';
        loc_nmea_get_fix_quality(location, locationExtended, custom_gga_fix_quality,
                                 ggaGpsQuality, rmcModeIndicator, vtgModeIndicator);

        // -------------------
        // ------$--VTG-------
        // -------------------

        pMarker = sentence;
        lengthRemaining = sizeof(sentence);

        if (location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_BEARING)
        {
            float magTrack = location.gpsLocation.bearing;
            if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_MAG_DEV)
            {
                float magTrack = location.gpsLocation.bearing - locationExtended.magneticDeviation;
                if (magTrack < 0.0)
                    magTrack += 360.0;
                else if (magTrack > 360.0)
                    magTrack -= 360.0;
            }

            length = snprintf(pMarker, lengthRemaining, "$%sVTG,%.1lf,T,%.1lf,M,", talker, location.gpsLocation.bearing, magTrack);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining, "$%sVTG,,T,,M,", talker);
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_SPEED)
        {
            float speedKnots = location.gpsLocation.speed * (3600.0/1852.0);
            float speedKmPerHour = location.gpsLocation.speed * 3.6;

            length = snprintf(pMarker, lengthRemaining, "%.1lf,N,%.1lf,K,", speedKnots, speedKmPerHour);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining, ",N,,K,");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        length = snprintf(pMarker, lengthRemaining, "%c", vtgModeIndicator);

        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);

        memset(&ecef_w84, 0, sizeof(ecef_w84));
        memset(&ecef_p90, 0, sizeof(ecef_p90));
        memset(&lla_w84, 0, sizeof(lla_w84));
        memset(&lla_p90, 0, sizeof(lla_p90));
        memset(&ref_lla, 0, sizeof(ref_lla));
        memset(&local_lla, 0, sizeof(local_lla));
        lla_w84.lat = location.gpsLocation.latitude / 180.0 * M_PI;
        lla_w84.lon = location.gpsLocation.longitude / 180.0 * M_PI;
        lla_w84.alt = location.gpsLocation.altitude;

        convert_Lla_to_Ecef(lla_w84, ecef_w84);
        convert_WGS84_to_PZ90(ecef_w84, ecef_p90);
        convert_Ecef_to_Lla(ecef_p90, lla_p90);

        switch (datum_type) {
            case LOC_GNSS_DATUM_WGS84:
                ref_lla.lat = location.gpsLocation.latitude;
                ref_lla.lon = location.gpsLocation.longitude;
                ref_lla.alt = location.gpsLocation.altitude;
                local_lla.lat = lla_p90.lat / M_PI * 180.0;
                local_lla.lon = lla_p90.lon / M_PI * 180.0;
                local_lla.alt = lla_p90.alt;
                break;
            case LOC_GNSS_DATUM_PZ90:
                ref_lla.lat = lla_p90.lat / M_PI * 180.0;
                ref_lla.lon = lla_p90.lon / M_PI * 180.0;
                ref_lla.alt = lla_p90.alt;
                local_lla.lat = location.gpsLocation.latitude;
                local_lla.lon = location.gpsLocation.longitude;
                local_lla.alt = location.gpsLocation.altitude;
                break;
            default:
                break;
        }

        // -------------------
        // ------$--DTM-------
        // -------------------
        loc_nmea_generate_DTM(ref_lla, local_lla, talker, sentence_DTM, sizeof(sentence_DTM));

        // -------------------
        // ------$--RMC-------
        // -------------------

        pMarker = sentence_RMC;
        lengthRemaining = sizeof(sentence_RMC);

        length = snprintf(pMarker, lengthRemaining, "$%sRMC,%02d%02d%02d.%02d,A," ,
                          talker, utcHours, utcMinutes, utcSeconds,utcMSeconds/10);

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_LAT_LONG)
        {
            double latitude = ref_lla.lat;
            double longitude = ref_lla.lon;
            char latHemisphere;
            char lonHemisphere;
            double latMinutes;
            double lonMinutes;

            if (latitude > 0)
            {
                latHemisphere = 'N';
            }
            else
            {
                latHemisphere = 'S';
                latitude *= -1.0;
            }

            if (longitude < 0)
            {
                lonHemisphere = 'W';
                longitude *= -1.0;
            }
            else
            {
                lonHemisphere = 'E';
            }

            latMinutes = fmod(latitude * 60.0 , 60.0);
            lonMinutes = fmod(longitude * 60.0 , 60.0);

            length = snprintf(pMarker, lengthRemaining, "%02d%09.6lf,%c,%03d%09.6lf,%c,",
                              (uint8_t)floor(latitude), latMinutes, latHemisphere,
                              (uint8_t)floor(longitude),lonMinutes, lonHemisphere);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining,",,,,");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_SPEED)
        {
            float speedKnots = location.gpsLocation.speed * (3600.0/1852.0);
            length = snprintf(pMarker, lengthRemaining, "%.1lf,", speedKnots);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining, ",");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_BEARING)
        {
            length = snprintf(pMarker, lengthRemaining, "%.1lf,", location.gpsLocation.bearing);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining, ",");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        length = snprintf(pMarker, lengthRemaining, "%2.2d%2.2d%2.2d,",
                          utcDay, utcMonth, utcYear);

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_MAG_DEV)
        {
            float magneticVariation = locationExtended.magneticDeviation;
            char direction;
            if (magneticVariation < 0.0)
            {
                direction = 'W';
                magneticVariation *= -1.0;
            }
            else
            {
                direction = 'E';
            }

            length = snprintf(pMarker, lengthRemaining, "%.1lf,%c,",
                              magneticVariation, direction);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining, ",,");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        length = snprintf(pMarker, lengthRemaining, "%c", rmcModeIndicator);
        pMarker += length;
        lengthRemaining -= length;

        // hardcode Navigation Status field to 'V'
        length = snprintf(pMarker, lengthRemaining, ",%c", 'V');
        pMarker += length;
        lengthRemaining -= length;

        length = loc_nmea_put_checksum(sentence_RMC, sizeof(sentence_RMC));

        // -------------------
        // ------$--GNS-------
        // -------------------

        pMarker = sentence_GNS;
        lengthRemaining = sizeof(sentence_GNS);

        length = snprintf(pMarker, lengthRemaining, "$%sGNS,%02d%02d%02d.%02d," ,
                          talker, utcHours, utcMinutes, utcSeconds, utcMSeconds/10);

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_LAT_LONG)
        {
            double latitude = ref_lla.lat;
            double longitude = ref_lla.lon;
            char latHemisphere;
            char lonHemisphere;
            double latMinutes;
            double lonMinutes;

            if (latitude > 0)
            {
                latHemisphere = 'N';
            }
            else
            {
                latHemisphere = 'S';
                latitude *= -1.0;
            }

            if (longitude < 0)
            {
                lonHemisphere = 'W';
                longitude *= -1.0;
            }
            else
            {
                lonHemisphere = 'E';
            }

            latMinutes = fmod(latitude * 60.0 , 60.0);
            lonMinutes = fmod(longitude * 60.0 , 60.0);

            length = snprintf(pMarker, lengthRemaining, "%02d%09.6lf,%c,%03d%09.6lf,%c,",
                              (uint8_t)floor(latitude), latMinutes, latHemisphere,
                              (uint8_t)floor(longitude),lonMinutes, lonHemisphere);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining,",,,,");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if(!(sv_cache_info.gps_used_mask ? 1 : 0))
            modeIndicator[0] = 'N';
        else if (LOC_NAV_MASK_SBAS_CORRECTION_IONO & locationExtended.navSolutionMask)
            modeIndicator[0] = 'D';
        else if (LOC_POS_TECH_MASK_SENSORS == locationExtended.tech_mask)
            modeIndicator[0] = 'E';
        else
            modeIndicator[0] = 'A';
        if(!(sv_cache_info.glo_used_mask ? 1 : 0))
            modeIndicator[1] = 'N';
        else if (LOC_POS_TECH_MASK_SENSORS == locationExtended.tech_mask)
            modeIndicator[1] = 'E';
        else
            modeIndicator[1] = 'A';
        if(!(sv_cache_info.gal_used_mask ? 1 : 0))
            modeIndicator[2] = 'N';
        else if (LOC_POS_TECH_MASK_SENSORS == locationExtended.tech_mask)
            modeIndicator[2] = 'E';
        else
            modeIndicator[2] = 'A';
        if(!(sv_cache_info.bds_used_mask ? 1 : 0))
            modeIndicator[3] = 'N';
        else if (LOC_POS_TECH_MASK_SENSORS == locationExtended.tech_mask)
            modeIndicator[3] = 'E';
        else
            modeIndicator[3] = 'A';
        if(!(sv_cache_info.qzss_used_mask ? 1 : 0))
            modeIndicator[4] = 'N';
        else if (LOC_POS_TECH_MASK_SENSORS == locationExtended.tech_mask)
            modeIndicator[4] = 'E';
        else
            modeIndicator[4] = 'A';
        if(!(sv_cache_info.navic_used_mask ? 1 : 0))
            modeIndicator[5] = 'N';
        else if (LOC_POS_TECH_MASK_SENSORS == locationExtended.tech_mask)
            modeIndicator[5] = 'E';
        else
            modeIndicator[5] = 'A';
        modeIndicator[6] = '\0';
        for(int index = 5; index > 0 && 'N' == modeIndicator[index]; index--) {
            modeIndicator[index] = '\0';
        }
        length = snprintf(pMarker, lengthRemaining,"%s,", modeIndicator);

        pMarker += length;
        lengthRemaining -= length;

        if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_DOP) {
            length = snprintf(pMarker, lengthRemaining, "%02d,%.1f,",
                              svUsedCount, locationExtended.hdop);
        }
        else {   // no hdop
            length = snprintf(pMarker, lengthRemaining, "%02d,,",
                              svUsedCount);
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_ALTITUDE_MEAN_SEA_LEVEL)
        {
            length = snprintf(pMarker, lengthRemaining, "%.1lf,",
                              locationExtended.altitudeMeanSeaLevel);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining,",");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if ((location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_ALTITUDE) &&
            (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_ALTITUDE_MEAN_SEA_LEVEL))
        {
            length = snprintf(pMarker, lengthRemaining, "%.1lf,,",
                              ref_lla.alt - locationExtended.altitudeMeanSeaLevel);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining,",,");
        }

        pMarker += length;
        lengthRemaining -= length;

        // hardcode Navigation Status field to 'V'
        length = snprintf(pMarker, lengthRemaining, ",%c", 'V');
        pMarker += length;
        lengthRemaining -= length;

        length = loc_nmea_put_checksum(sentence_GNS, sizeof(sentence_GNS));


        // -------------------
        // ------$--GGA-------
        // -------------------

        pMarker = sentence_GGA;
        lengthRemaining = sizeof(sentence_GGA);

        length = snprintf(pMarker, lengthRemaining, "$%sGGA,%02d%02d%02d.%02d," ,
                          talker, utcHours, utcMinutes, utcSeconds, utcMSeconds/10);

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_LAT_LONG)
        {
            double latitude = ref_lla.lat;
            double longitude = ref_lla.lon;
            char latHemisphere;
            char lonHemisphere;
            double latMinutes;
            double lonMinutes;

            if (latitude > 0)
            {
                latHemisphere = 'N';
            }
            else
            {
                latHemisphere = 'S';
                latitude *= -1.0;
            }

            if (longitude < 0)
            {
                lonHemisphere = 'W';
                longitude *= -1.0;
            }
            else
            {
                lonHemisphere = 'E';
            }

            latMinutes = fmod(latitude * 60.0 , 60.0);
            lonMinutes = fmod(longitude * 60.0 , 60.0);

            length = snprintf(pMarker, lengthRemaining, "%02d%09.6lf,%c,%03d%09.6lf,%c,",
                              (uint8_t)floor(latitude), latMinutes, latHemisphere,
                              (uint8_t)floor(longitude),lonMinutes, lonHemisphere);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining,",,,,");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        // Number of satellites in use, 00-12
        if (svUsedCount > MAX_SATELLITES_IN_USE)
            svUsedCount = MAX_SATELLITES_IN_USE;
        if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_DOP)
        {
            length = snprintf(pMarker, lengthRemaining, "%s,%02d,%.1f,",
                              ggaGpsQuality, svUsedCount, locationExtended.hdop);
        }
        else
        {   // no hdop
            length = snprintf(pMarker, lengthRemaining, "%s,%02d,,",
                              ggaGpsQuality, svUsedCount);
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_ALTITUDE_MEAN_SEA_LEVEL)
        {
            length = snprintf(pMarker, lengthRemaining, "%.1lf,M,",
                              locationExtended.altitudeMeanSeaLevel);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining,",,");
        }

        if (length < 0 || length >= lengthRemaining)
        {
            LOC_LOGE("NMEA Error in string formatting");
            return;
        }
        pMarker += length;
        lengthRemaining -= length;

        if ((location.gpsLocation.flags & LOC_GPS_LOCATION_HAS_ALTITUDE) &&
            (locationExtended.flags & GPS_LOCATION_EXTENDED_HAS_ALTITUDE_MEAN_SEA_LEVEL))
        {
            length = snprintf(pMarker, lengthRemaining, "%.1lf,M,,",
                              ref_lla.alt - locationExtended.altitudeMeanSeaLevel);
        }
        else
        {
            length = snprintf(pMarker, lengthRemaining,",,,");
        }

        length = loc_nmea_put_checksum(sentence_GGA, sizeof(sentence_GGA));

        // ------$--DTM-------
        nmeaArraystr.push_back(sentence_DTM);
        // ------$--RMC-------
        nmeaArraystr.push_back(sentence_RMC);
        if(LOC_GNSS_DATUM_PZ90 == datum_type) {
            // ------$--DTM-------
            nmeaArraystr.push_back(sentence_DTM);
        }
        // ------$--GNS-------
        nmeaArraystr.push_back(sentence_GNS);
        if(LOC_GNSS_DATUM_PZ90 == datum_type) {
            // ------$--DTM-------
            nmeaArraystr.push_back(sentence_DTM);
        }
        // ------$--GGA-------
        nmeaArraystr.push_back(sentence_GGA);

    }
    //Send blank NMEA reports for non-final fixes
    else {
        strlcpy(sentence, "$GPGSA,A,1,,,,,,,,,,,,,,,,", sizeof(sentence));
        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);

        strlcpy(sentence, "$GPVTG,,T,,M,,N,,K,N", sizeof(sentence));
        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);

        strlcpy(sentence, "$GPDTM,,,,,,,,", sizeof(sentence));
        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);

        strlcpy(sentence, "$GPRMC,,V,,,,,,,,,,N,V", sizeof(sentence));
        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);

        strlcpy(sentence, "$GPGNS,,,,,,N,,,,,,,V", sizeof(sentence));
        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);

        strlcpy(sentence, "$GPGGA,,,,,,0,,,,,,,,", sizeof(sentence));
        length = loc_nmea_put_checksum(sentence, sizeof(sentence));
        nmeaArraystr.push_back(sentence);
    }

    EXIT_LOG(%d, 0);
}



/*===========================================================================
FUNCTION    loc_nmea_generate_sv

DESCRIPTION
   Generate NMEA sentences generated based on sv report

DEPENDENCIES
   NONE

RETURN VALUE
   0

SIDE EFFECTS
   N/A

===========================================================================*/
void loc_nmea_generate_sv(const GnssSvNotification &svNotify,
                              std::vector<std::string> &nmeaArraystr)
{
    ENTRY_LOG();

    char sentence[NMEA_SENTENCE_MAX_LENGTH] = {0};
    int svCount = svNotify.count;
    int svNumber = 1;
    loc_sv_cache_info sv_cache_info = {};

    //Count GPS SVs for saparating GPS from GLONASS and throw others
    for(svNumber=1; svNumber <= svCount; svNumber++) {
        if (GNSS_SV_TYPE_GPS == svNotify.gnssSvs[svNumber - 1].type)
        {
            // cache the used in fix mask, as it will be needed to send $GPGSA
            // during the position report
            if (GNSS_SV_OPTIONS_USED_IN_FIX_BIT ==
                    (svNotify.gnssSvs[svNumber - 1].gnssSvOptionsMask &
                      GNSS_SV_OPTIONS_USED_IN_FIX_BIT))
            {
                sv_cache_info.gps_used_mask |= (1ULL << (svNotify.gnssSvs[svNumber - 1].svId - 1));
            }
            if (GNSS_SIGNAL_GPS_L5 == svNotify.gnssSvs[svNumber - 1].gnssSignalTypeMask) {
                sv_cache_info.gps_l5_count++;
            } else {
                // GNSS_SIGNAL_GPS_L1CA or default
                // If no signal type in report, it means default L1
                sv_cache_info.gps_l1_count++;
            }
        }
        else if (GNSS_SV_TYPE_GLONASS == svNotify.gnssSvs[svNumber - 1].type)
        {
            // cache the used in fix mask, as it will be needed to send $GNGSA
            // during the position report
            if (GNSS_SV_OPTIONS_USED_IN_FIX_BIT ==
                    (svNotify.gnssSvs[svNumber - 1].gnssSvOptionsMask &
                      GNSS_SV_OPTIONS_USED_IN_FIX_BIT))
            {
                sv_cache_info.glo_used_mask |= (1ULL << (svNotify.gnssSvs[svNumber - 1].svId - 1));
            }
            if (GNSS_SIGNAL_GLONASS_G2 == svNotify.gnssSvs[svNumber - 1].gnssSignalTypeMask){
                sv_cache_info.glo_g2_count++;
            } else {
                // GNSS_SIGNAL_GLONASS_G1 or default
                // If no signal type in report, it means default G1
                sv_cache_info.glo_g1_count++;
            }
        }
        else if (GNSS_SV_TYPE_GALILEO == svNotify.gnssSvs[svNumber - 1].type)
        {
            // cache the used in fix mask, as it will be needed to send $GAGSA
            // during the position report
            if (GNSS_SV_OPTIONS_USED_IN_FIX_BIT ==
                    (svNotify.gnssSvs[svNumber - 1].gnssSvOptionsMask &
                      GNSS_SV_OPTIONS_USED_IN_FIX_BIT))
            {
                sv_cache_info.gal_used_mask |= (1ULL << (svNotify.gnssSvs[svNumber - 1].svId - 1));
            }
            if(GNSS_SIGNAL_GALILEO_E5A == svNotify.gnssSvs[svNumber - 1].gnssSignalTypeMask){
                sv_cache_info.gal_e5_count++;
            } else {
                // GNSS_SIGNAL_GALILEO_E1 or default
                // If no signal type in report, it means default E1
                sv_cache_info.gal_e1_count++;
            }
        }
        else if (GNSS_SV_TYPE_QZSS == svNotify.gnssSvs[svNumber - 1].type)
        {
            // cache the used in fix mask, as it will be needed to send $PQGSA
            // during the position report
            if (GNSS_SV_OPTIONS_USED_IN_FIX_BIT ==
                (svNotify.gnssSvs[svNumber - 1].gnssSvOptionsMask &
                  GNSS_SV_OPTIONS_USED_IN_FIX_BIT))
            {
                // For QZSS we adjusted SV id's in GnssAdapter, we need to re-adjust here
                sv_cache_info.qzss_used_mask |=
                        (1ULL << (svNotify.gnssSvs[svNumber - 1].svId - (QZSS_SV_PRN_MIN - 1) - 1));
            }
            if (GNSS_SIGNAL_QZSS_L5 == svNotify.gnssSvs[svNumber - 1].gnssSignalTypeMask) {
                sv_cache_info.qzss_l5_count++;
            } else {
                // GNSS_SIGNAL_QZSS_L1CA or default
                // If no signal type in report, it means default L1
                sv_cache_info.qzss_l1_count++;
            }
        }
        else if (GNSS_SV_TYPE_BEIDOU == svNotify.gnssSvs[svNumber - 1].type)
        {
            // cache the used in fix mask, as it will be needed to send $PQGSA
            // during the position report
            if (GNSS_SV_OPTIONS_USED_IN_FIX_BIT ==
                (svNotify.gnssSvs[svNumber - 1].gnssSvOptionsMask &
                  GNSS_SV_OPTIONS_USED_IN_FIX_BIT))
            {
                sv_cache_info.bds_used_mask |= (1ULL << (svNotify.gnssSvs[svNumber - 1].svId - 1));
            }
            if(GNSS_SIGNAL_BEIDOU_B2AI == svNotify.gnssSvs[svNumber - 1].gnssSignalTypeMask){
                sv_cache_info.bds_b2_count++;
            } else {
                // GNSS_SIGNAL_BEIDOU_B1I or default
                // If no signal type in report, it means default B1I
                sv_cache_info.bds_b1_count++;
            }
        }
        else if (GNSS_SV_TYPE_NAVIC == svNotify.gnssSvs[svNumber - 1].type)
        {
            // cache the used in fix mask, as it will be needed to send $PQGSA
            // during the position report
            if (GNSS_SV_OPTIONS_USED_IN_FIX_BIT ==
                (svNotify.gnssSvs[svNumber - 1].gnssSvOptionsMask &
                  GNSS_SV_OPTIONS_USED_IN_FIX_BIT))
            {
                sv_cache_info.navic_used_mask |=
                        (1ULL << (svNotify.gnssSvs[svNumber - 1].svId - 1));
            }
            // GNSS_SIGNAL_NAVIC_L5 is the only signal type for NAVIC
            sv_cache_info.navic_l5_count++;
        }
    }

    loc_nmea_sv_meta sv_meta;
    // ---------------------
    // ------$GPGSV:L1CA----
    // ---------------------

    loc_nmea_generate_GSV(svNotify, sentence, sizeof(sentence),
            loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_GPS,
            GNSS_SIGNAL_GPS_L1CA, false), nmeaArraystr);

    // ---------------------
    // ------$GPGSV:L5------
    // ---------------------

    loc_nmea_generate_GSV(svNotify, sentence, sizeof(sentence),
            loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_GPS,
            GNSS_SIGNAL_GPS_L5, false), nmeaArraystr);
    // ---------------------
    // ------$GLGSV:G1------
    // ---------------------

    loc_nmea_generate_GSV(svNotify, sentence, sizeof(sentence),
            loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_GLONASS,
            GNSS_SIGNAL_GLONASS_G1, false), nmeaArraystr);

    // ---------------------
    // ------$GLGSV:G2------
    // ---------------------

    loc_nmea_generate_GSV(svNotify, sentence, sizeof(sentence),
            loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_GLONASS,
            GNSS_SIGNAL_GLONASS_G2, false), nmeaArraystr);

    // ---------------------
    // ------$GAGSV:E1------
    // ---------------------

    loc_nmea_generate_GSV(svNotify, sentence, sizeof(sentence),
            loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_GALILEO,
            GNSS_SIGNAL_GALILEO_E1, false), nmeaArraystr);

    // -------------------------
    // ------$GAGSV:E5A---------
    // -------------------------
    loc_nmea_generate_GSV(svNotify, sentence, sizeof(sentence),
            loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_GALILEO,
            GNSS_SIGNAL_GALILEO_E5A, false), nmeaArraystr);

    // -----------------------------
    // ------$PQGSV (QZSS):L1CA-----
    // -----------------------------

    loc_nmea_generate_GSV(svNotify, sentence, sizeof(sentence),
            loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_QZSS,
            GNSS_SIGNAL_QZSS_L1CA, false), nmeaArraystr);

    // -----------------------------
    // ------$PQGSV (QZSS):L5-------
    // -----------------------------

    loc_nmea_generate_GSV(svNotify, sentence, sizeof(sentence),
            loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_QZSS,
            GNSS_SIGNAL_QZSS_L5, false), nmeaArraystr);
    // -----------------------------
    // ------$PQGSV (BEIDOU:B1I)----
    // -----------------------------

    loc_nmea_generate_GSV(svNotify, sentence, sizeof(sentence),
            loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_BEIDOU,
            GNSS_SIGNAL_BEIDOU_B1I,false), nmeaArraystr);

    // -----------------------------
    // ------$PQGSV (BEIDOU:B2AI)---
    // -----------------------------

    loc_nmea_generate_GSV(svNotify, sentence, sizeof(sentence),
            loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_BEIDOU,
            GNSS_SIGNAL_BEIDOU_B2AI,false), nmeaArraystr);

    // -----------------------------
    // ------$GIGSV (NAVIC:L5)------
    // -----------------------------

    loc_nmea_generate_GSV(svNotify, sentence, sizeof(sentence),
            loc_nmea_sv_meta_init(sv_meta, sv_cache_info, GNSS_SV_TYPE_NAVIC,
            GNSS_SIGNAL_NAVIC_L5,false), nmeaArraystr);

    EXIT_LOG(%d, 0);
}
