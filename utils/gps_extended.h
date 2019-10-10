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

#ifndef GPS_EXTENDED_H
#define GPS_EXTENDED_H

/**
 * @file
 * @brief C++ declarations for GPS types
 */

#include <gps_extended_c.h>
#if defined(USE_GLIB) || defined(OFF_TARGET)
#include <string.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



struct LocPosMode
{
    LocPositionMode mode;
    LocGpsPositionRecurrence recurrence;
    uint32_t min_interval;
    uint32_t preferred_accuracy;
    uint32_t preferred_time;
    bool share_position;
    char credentials[14];
    char provider[8];
    GnssPowerMode powerMode;
    uint32_t timeBetweenMeasurements;
    LocPosMode(LocPositionMode m, LocGpsPositionRecurrence recr,
               uint32_t gap, uint32_t accu, uint32_t time,
               bool sp, const char* cred, const char* prov,
               GnssPowerMode pMode = GNSS_POWER_MODE_INVALID,
               uint32_t tbm = 0) :
        mode(m), recurrence(recr),
        min_interval(gap < GPS_MIN_POSSIBLE_FIX_INTERVAL_MS ?
                     GPS_MIN_POSSIBLE_FIX_INTERVAL_MS : gap),
        preferred_accuracy(accu), preferred_time(time),
        share_position(sp), powerMode(pMode),
        timeBetweenMeasurements(tbm) {
        memset(credentials, 0, sizeof(credentials));
        memset(provider, 0, sizeof(provider));
        if (NULL != cred) {
            memcpy(credentials, cred, sizeof(credentials)-1);
        }
        if (NULL != prov) {
            memcpy(provider, prov, sizeof(provider)-1);
        }
    }

    inline LocPosMode() :
        mode(LOC_POSITION_MODE_MS_BASED),
        recurrence(LOC_GPS_POSITION_RECURRENCE_PERIODIC),
        min_interval(GPS_DEFAULT_FIX_INTERVAL_MS),
        preferred_accuracy(50), preferred_time(120000),
        share_position(true), powerMode(GNSS_POWER_MODE_INVALID),
        timeBetweenMeasurements(GPS_DEFAULT_FIX_INTERVAL_MS) {
        memset(credentials, 0, sizeof(credentials));
        memset(provider, 0, sizeof(provider));
    }

    inline bool equals(const LocPosMode &anotherMode) const
    {
        return anotherMode.mode == mode &&
            anotherMode.recurrence == recurrence &&
            anotherMode.min_interval == min_interval &&
            anotherMode.preferred_accuracy == preferred_accuracy &&
            anotherMode.preferred_time == preferred_time &&
            anotherMode.powerMode == powerMode &&
            anotherMode.timeBetweenMeasurements == timeBetweenMeasurements &&
            !strncmp(anotherMode.credentials, credentials, sizeof(credentials)-1) &&
            !strncmp(anotherMode.provider, provider, sizeof(provider)-1);
    }

    void logv() const;
};


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GPS_EXTENDED_H */

