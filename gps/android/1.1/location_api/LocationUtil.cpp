/* Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
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

namespace android {
namespace hardware {
namespace gnss {
namespace V1_1 {
namespace implementation {

using ::android::hardware::gnss::V1_0::GnssLocation;
using ::android::hardware::gnss::V1_0::GnssConstellationType;
using ::android::hardware::gnss::V1_0::GnssLocationFlags;

void convertGnssLocation(Location& in, GnssLocation& out)
{
    memset(&out, 0, sizeof(GnssLocation));
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

void convertGnssLocation(const GnssLocation& in, Location& out)
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

void convertGnssConstellationType(GnssSvType& in, GnssConstellationType& out)
{
    switch(in) {
        case GNSS_SV_TYPE_GPS:
            out = GnssConstellationType::GPS;
            break;
        case GNSS_SV_TYPE_SBAS:
            out = GnssConstellationType::SBAS;
            break;
        case GNSS_SV_TYPE_GLONASS:
            out = GnssConstellationType::GLONASS;
            break;
        case GNSS_SV_TYPE_QZSS:
            out = GnssConstellationType::QZSS;
            break;
        case GNSS_SV_TYPE_BEIDOU:
            out = GnssConstellationType::BEIDOU;
            break;
        case GNSS_SV_TYPE_GALILEO:
            out = GnssConstellationType::GALILEO;
            break;
        case GNSS_SV_TYPE_UNKNOWN:
        default:
            out = GnssConstellationType::UNKNOWN;
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
}  // namespace V1_1
}  // namespace gnss
}  // namespace hardware
}  // namespace android
