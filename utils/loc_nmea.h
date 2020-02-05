/* Copyright (c) 2012-2013, 2015-2017 The Linux Foundation. All rights reserved.
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

#ifndef LOC_ENG_NMEA_H
#define LOC_ENG_NMEA_H

#include <gps_extended.h>
#include <vector>
#include <string>
#define NMEA_SENTENCE_MAX_LENGTH 200

/** gnss datum type */
#define LOC_GNSS_DATUM_WGS84          0
#define LOC_GNSS_DATUM_PZ90           1

/* len of semi major axis of ref ellips*/
#define MAJA               (6378137.0)
/* flattening coef of ref ellipsoid*/
#define FLAT               (1.0/298.2572235630)
/* 1st eccentricity squared*/
#define ESQR               (FLAT*(2.0 - FLAT))
/*1 minus eccentricity squared*/
#define OMES               (1.0 - ESQR)
#define MILARCSEC2RAD      (4.848136811095361e-09)
/*semi major axis */
#define C_PZ90A            (6378136.0)
/*semi minor axis */
#define C_PZ90B            (6356751.3618)
/* Transformation from WGS84 to PZ90
 * Cx,Cy,Cz,Rs,Rx,Ry,Rz,C_SYS_A,C_SYS_B*/
const double DatumConstFromWGS84[9] =
        {+0.003, +0.001, 0.000, (1.0+(0.000*1E-6)), (-0.019*MILARCSEC2RAD),
        (+0.042*MILARCSEC2RAD), (-0.002*MILARCSEC2RAD), C_PZ90A, C_PZ90B};

/** Represents a LTP*/
typedef struct {
    double     lat;
    double     lon;
    double     alt;
} LocLla;

/** Represents a ECEF*/
typedef struct {
    double     X;
    double     Y;
    double     Z;
} LocEcef;

void loc_nmea_generate_sv(const GnssSvNotification &svNotify,
                              std::vector<std::string> &nmeaArraystr);

void loc_nmea_generate_pos(const UlpLocation &location,
                               const GpsLocationExtended &locationExtended,
                               const LocationSystemInfo &systemInfo,
                               unsigned char generate_nmea,
                               bool custom_gga_fix_quality,
                               std::vector<std::string> &nmeaArraystr);

#define DEBUG_NMEA_MINSIZE 6
#define DEBUG_NMEA_MAXSIZE 4096
inline bool loc_nmea_is_debug(const char* nmea, int length) {
    return ((nullptr != nmea) &&
            (length >= DEBUG_NMEA_MINSIZE) && (length <= DEBUG_NMEA_MAXSIZE) &&
            (nmea[0] == '$') && (nmea[1] == 'P') && (nmea[2] == 'Q') && (nmea[3] == 'W'));
}

#endif // LOC_ENG_NMEA_H
