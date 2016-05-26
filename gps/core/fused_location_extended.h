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

#ifndef FUSED_LOCATION_EXTENDED_H
#define FUSED_LOCATION_EXTENDED_H

/** Batching default ID for dummy batching session*/
#define GPS_BATCHING_DEFAULT_ID                 1

/** This cap is used to decide the FLP session cache
size on AP. If the BATCH_SIZE in flp.conf is less than
GPS_AP_BATCHING_SIZE_CAP, FLP session cache size will
be twice the BATCH_SIZE defined in flp.conf. Otherwise,
FLP session cache size will be equal to the BATCH_SIZE.*/
#define GPS_AP_BATCHING_SIZE_CAP               40

#define GPS_BATCHING_OPERATION_SUCCEESS         1
#define GPS_BATCHING_OPERATION_FAILURE          0

/** GPS extended batching flags*/
#define GPS_EXT_BATCHING_ON_FULL        0x0000001
#define GPS_EXT_BATCHING_ON_FIX         0x000000

typedef struct {
    double max_power_allocation_mW;
    uint32_t sources_to_use;
    uint32_t flags;
    int64_t period_ns;
} FlpExtBatchOptions;

/** GpsLocationExtended has valid latitude and longitude. */
#define GPS_LOCATION_EXTENDED_HAS_LAT_LONG   (1U<<0)
/** GpsLocationExtended has valid altitude. */
#define GPS_LOCATION_EXTENDED_HAS_ALTITUDE   (1U<<1)
/** GpsLocationExtended has valid speed. */
#define GPS_LOCATION_EXTENDED_HAS_SPEED      (1U<<2)
/** GpsLocationExtended has valid bearing. */
#define GPS_LOCATION_EXTENDED_HAS_BEARING    (1U<<4)
/** GpsLocationExtended has valid accuracy. */
#define GPS_LOCATION_EXTENDED_HAS_ACCURACY   (1U<<8)

/** GPS extended supports geofencing */
#define GPS_EXTENDED_CAPABILITY_GEOFENCE     0x0000001
/** GPS extended supports batching */
#define GPS_EXTENDED_CAPABILITY_BATCHING     0x0000002

typedef struct FlpExtLocation_s {
    size_t          size;
    uint16_t        flags;
    double          latitude;
    double          longitude;
    double          altitude;
    float           speed;
    float           bearing;
    float           accuracy;
    int64_t         timestamp;
    uint32_t        sources_used;
} FlpExtLocation;

#endif /* FUSED_LOCATION_EXTENDED_H */
