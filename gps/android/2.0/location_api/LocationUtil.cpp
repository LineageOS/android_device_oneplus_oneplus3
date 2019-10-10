/* Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
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

#include <LocationUtil.h>
#include <log_util.h>
#include <inttypes.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

using ::android::hardware::gnss::V2_0::GnssLocation;
using ::android::hardware::gnss::V2_0::GnssConstellationType;
using ::android::hardware::gnss::V1_0::GnssLocationFlags;

void convertGnssLocation(Location& in, V1_0::GnssLocation& out)
{
    memset(&out, 0, sizeof(V1_0::GnssLocation));
    if (in.flags & LOCATION_HAS_LAT_LONG_BIT) {
        out.gnssLocationFlags |= GnssLocationFlags::HAS_LAT_LONG;
        out.latitudeDegrees = in.latitude;
        out.longitudeDegrees = in.longitude;
    }
    if (in.flags & LOCATION_HAS_ALTITUDE_BIT) {
        out.gnssLocationFlags |= GnssLocationFlags::HAS_ALTITUDE;
        out.altitudeMeters = in.altitude;
    }
    if (in.flags & LOCATION_HAS_SPEED_BIT) {
        out.gnssLocationFlags |= GnssLocationFlags::HAS_SPEED;
        out.speedMetersPerSec = in.speed;
    }
    if (in.flags & LOCATION_HAS_BEARING_BIT) {
        out.gnssLocationFlags |= GnssLocationFlags::HAS_BEARING;
        out.bearingDegrees = in.bearing;
    }
    if (in.flags & LOCATION_HAS_ACCURACY_BIT) {
        out.gnssLocationFlags |= GnssLocationFlags::HAS_HORIZONTAL_ACCURACY;
        out.horizontalAccuracyMeters = in.accuracy;
    }
    if (in.flags & LOCATION_HAS_VERTICAL_ACCURACY_BIT) {
        out.gnssLocationFlags |= GnssLocationFlags::HAS_VERTICAL_ACCURACY;
        out.verticalAccuracyMeters = in.verticalAccuracy;
    }
    if (in.flags & LOCATION_HAS_SPEED_ACCURACY_BIT) {
        out.gnssLocationFlags |= GnssLocationFlags::HAS_SPEED_ACCURACY;
        out.speedAccuracyMetersPerSecond = in.speedAccuracy;
    }
    if (in.flags & LOCATION_HAS_BEARING_ACCURACY_BIT) {
        out.gnssLocationFlags |= GnssLocationFlags::HAS_BEARING_ACCURACY;
        out.bearingAccuracyDegrees = in.bearingAccuracy;
    }

    out.timestamp = static_cast<V1_0::GnssUtcTime>(in.timestamp);
}

void convertGnssLocation(Location& in, V2_0::GnssLocation& out)
{
    memset(&out, 0, sizeof(V2_0::GnssLocation));
    convertGnssLocation(in, out.v1_0);

    struct timespec sinceBootTime;
    struct timespec currentTime;
    if (0 == clock_gettime(CLOCK_BOOTTIME,&sinceBootTime) &&
        0 == clock_gettime(CLOCK_REALTIME,&currentTime)) {

        int64_t sinceBootTimeNanos = sinceBootTime.tv_sec*1000000000 + sinceBootTime.tv_nsec;
        int64_t currentTimeNanos = currentTime.tv_sec*1000000000 + currentTime.tv_nsec;
        int64_t locationTimeNanos = in.timestamp*1000000;
        LOC_LOGD("%s]: sinceBootTimeNanos:%" PRIi64 " currentTimeNanos:%" PRIi64 ""
                " locationTimeNanos:%" PRIi64 "",
                __FUNCTION__, sinceBootTimeNanos, currentTimeNanos, locationTimeNanos);
        if (currentTimeNanos >= locationTimeNanos) {
            int64_t ageTimeNanos = currentTimeNanos - locationTimeNanos;
            LOC_LOGD("%s]: ageTimeNanos:%" PRIi64 ")", __FUNCTION__, ageTimeNanos);
            if (ageTimeNanos >= 0 && ageTimeNanos <= sinceBootTimeNanos) {
                out.elapsedRealtime.flags |= ElapsedRealtimeFlags::HAS_TIMESTAMP_NS;
                out.elapsedRealtime.timestampNs = sinceBootTimeNanos - ageTimeNanos;
                out.elapsedRealtime.flags |= ElapsedRealtimeFlags::HAS_TIME_UNCERTAINTY_NS;
                // time uncertainty is 1 ms since it is calculated from utc time that is in ms
                out.elapsedRealtime.timeUncertaintyNs = 1000000;
                LOC_LOGD("%s]: timestampNs:%" PRIi64 ")",
                        __FUNCTION__, out.elapsedRealtime.timestampNs);
            }
        }
    }

}

void convertGnssLocation(const V1_0::GnssLocation& in, Location& out)
{
    memset(&out, 0, sizeof(out));
    if (in.gnssLocationFlags & GnssLocationFlags::HAS_LAT_LONG) {
        out.flags |= LOCATION_HAS_LAT_LONG_BIT;
        out.latitude = in.latitudeDegrees;
        out.longitude = in.longitudeDegrees;
    }
    if (in.gnssLocationFlags & GnssLocationFlags::HAS_ALTITUDE) {
        out.flags |= LOCATION_HAS_ALTITUDE_BIT;
        out.altitude = in.altitudeMeters;
    }
    if (in.gnssLocationFlags & GnssLocationFlags::HAS_SPEED) {
        out.flags |= LOCATION_HAS_SPEED_BIT;
        out.speed = in.speedMetersPerSec;
    }
    if (in.gnssLocationFlags & GnssLocationFlags::HAS_BEARING) {
        out.flags |= LOCATION_HAS_BEARING_BIT;
        out.bearing = in.bearingDegrees;
    }
    if (in.gnssLocationFlags & GnssLocationFlags::HAS_HORIZONTAL_ACCURACY) {
        out.flags |= LOCATION_HAS_ACCURACY_BIT;
        out.accuracy = in.horizontalAccuracyMeters;
    }
    if (in.gnssLocationFlags & GnssLocationFlags::HAS_VERTICAL_ACCURACY) {
        out.flags |= LOCATION_HAS_VERTICAL_ACCURACY_BIT;
        out.verticalAccuracy = in.verticalAccuracyMeters;
    }
    if (in.gnssLocationFlags & GnssLocationFlags::HAS_SPEED_ACCURACY) {
        out.flags |= LOCATION_HAS_SPEED_ACCURACY_BIT;
        out.speedAccuracy = in.speedAccuracyMetersPerSecond;
    }
    if (in.gnssLocationFlags & GnssLocationFlags::HAS_BEARING_ACCURACY) {
        out.flags |= LOCATION_HAS_BEARING_ACCURACY_BIT;
        out.bearingAccuracy = in.bearingAccuracyDegrees;
    }

    out.timestamp = static_cast<uint64_t>(in.timestamp);
}

void convertGnssLocation(const V2_0::GnssLocation& in, Location& out)
{
    memset(&out, 0, sizeof(out));
    convertGnssLocation(in.v1_0, out);
}

void convertGnssConstellationType(GnssSvType& in, V1_0::GnssConstellationType& out)
{
    switch(in) {
        case GNSS_SV_TYPE_GPS:
            out = V1_0::GnssConstellationType::GPS;
            break;
        case GNSS_SV_TYPE_SBAS:
            out = V1_0::GnssConstellationType::SBAS;
            break;
        case GNSS_SV_TYPE_GLONASS:
            out = V1_0::GnssConstellationType::GLONASS;
            break;
        case GNSS_SV_TYPE_QZSS:
            out = V1_0::GnssConstellationType::QZSS;
            break;
        case GNSS_SV_TYPE_BEIDOU:
            out = V1_0::GnssConstellationType::BEIDOU;
            break;
        case GNSS_SV_TYPE_GALILEO:
            out = V1_0::GnssConstellationType::GALILEO;
            break;
        case GNSS_SV_TYPE_UNKNOWN:
        default:
            out = V1_0::GnssConstellationType::UNKNOWN;
            break;
    }
}

void convertGnssConstellationType(GnssSvType& in, V2_0::GnssConstellationType& out)
{
    switch(in) {
        case GNSS_SV_TYPE_GPS:
            out = V2_0::GnssConstellationType::GPS;
            break;
        case GNSS_SV_TYPE_SBAS:
            out = V2_0::GnssConstellationType::SBAS;
            break;
        case GNSS_SV_TYPE_GLONASS:
            out = V2_0::GnssConstellationType::GLONASS;
            break;
        case GNSS_SV_TYPE_QZSS:
            out = V2_0::GnssConstellationType::QZSS;
            break;
        case GNSS_SV_TYPE_BEIDOU:
            out = V2_0::GnssConstellationType::BEIDOU;
            break;
        case GNSS_SV_TYPE_GALILEO:
            out = V2_0::GnssConstellationType::GALILEO;
            break;
        case GNSS_SV_TYPE_NAVIC:
            out = V2_0::GnssConstellationType::IRNSS;
            break;
        case GNSS_SV_TYPE_UNKNOWN:
        default:
            out = V2_0::GnssConstellationType::UNKNOWN;
            break;
    }
}

void convertGnssEphemerisType(GnssEphemerisType& in, GnssDebug::SatelliteEphemerisType& out)
{
    switch(in) {
        case GNSS_EPH_TYPE_EPHEMERIS:
            out = GnssDebug::SatelliteEphemerisType::EPHEMERIS;
            break;
        case GNSS_EPH_TYPE_ALMANAC:
            out = GnssDebug::SatelliteEphemerisType::ALMANAC_ONLY;
            break;
        case GNSS_EPH_TYPE_UNKNOWN:
        default:
            out = GnssDebug::SatelliteEphemerisType::NOT_AVAILABLE;
            break;
    }
}

void convertGnssEphemerisSource(GnssEphemerisSource& in, GnssDebug::SatelliteEphemerisSource& out)
{
    switch(in) {
        case GNSS_EPH_SOURCE_DEMODULATED:
            out = GnssDebug::SatelliteEphemerisSource::DEMODULATED;
            break;
        case GNSS_EPH_SOURCE_SUPL_PROVIDED:
            out = GnssDebug::SatelliteEphemerisSource::SUPL_PROVIDED;
            break;
        case GNSS_EPH_SOURCE_OTHER_SERVER_PROVIDED:
            out = GnssDebug::SatelliteEphemerisSource::OTHER_SERVER_PROVIDED;
            break;
        case GNSS_EPH_SOURCE_LOCAL:
        case GNSS_EPH_SOURCE_UNKNOWN:
        default:
            out = GnssDebug::SatelliteEphemerisSource::OTHER;
            break;
    }
}

void convertGnssEphemerisHealth(GnssEphemerisHealth& in, GnssDebug::SatelliteEphemerisHealth& out)
{
    switch(in) {
        case GNSS_EPH_HEALTH_GOOD:
            out = GnssDebug::SatelliteEphemerisHealth::GOOD;
            break;
        case GNSS_EPH_HEALTH_BAD:
            out = GnssDebug::SatelliteEphemerisHealth::BAD;
            break;
        case GNSS_EPH_HEALTH_UNKNOWN:
        default:
            out = GnssDebug::SatelliteEphemerisHealth::UNKNOWN;
            break;
    }
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
