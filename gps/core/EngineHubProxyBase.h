/* Copyright (c) 2018, The Linux Foundation. All rights reserved.
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
#ifndef ENGINE_HUB_PROXY_BASE_H
#define ENGINE_HUB_PROXY_BASE_H

namespace loc_core {

class EngineHubProxyBase {
public:
    inline EngineHubProxyBase() {
    }
    inline virtual ~EngineHubProxyBase() {}

    // gnss session related functions
    inline virtual bool gnssStartFix() {
        return false;
    }

    inline virtual bool gnssStopFix() {
        return false;
    }

    inline virtual bool gnssSetFixMode(const LocPosMode &params) {
        (void) params;
        return false;
    }

    inline virtual bool gnssDeleteAidingData(const GnssAidingData &aidingData) {
        (void) aidingData;
        return false;
    }

    // GNSS reports
    inline virtual bool gnssReportPosition(const UlpLocation &location,
                                           const GpsLocationExtended &locationExtended,
                                           enum loc_sess_status status) {
        (void) location;
        (void) locationExtended;
        (void) status;
        return false;
    }

    inline virtual bool gnssReportSv(const GnssSvNotification& svNotify) {
        (void) svNotify;
        return false;
    }

    inline virtual bool gnssReportSvMeasurement(const GnssSvMeasurementSet& svMeasurementSet) {
        (void) svMeasurementSet;
        return false;
    }

    inline virtual bool gnssReportSvPolynomial(const GnssSvPolynomial& svPolynomial) {
        (void) svPolynomial;
        return false;
    }

    inline virtual bool gnssReportSvEphemeris(const GnssSvEphemerisReport& svEphemeris) {
        (void) svEphemeris;
        return false;
    }

    inline virtual bool gnssReportSystemInfo(const LocationSystemInfo& systemInfo) {
        (void) systemInfo;
        return false;
    }

    inline virtual bool gnssReportKlobucharIonoModel(const GnssKlobucharIonoModel& ionoModel) {
        (void) ionoModel;
        return false;
    }

    inline virtual bool gnssReportAdditionalSystemInfo(
            const GnssAdditionalSystemInfo& additionalSystemInfo) {
        (void) additionalSystemInfo;
        return false;
    }
};

typedef std::function<void(int count, EngineLocationInfo* locationArr)>
        GnssAdapterReportEnginePositionsEventCb;

typedef std::function<void(const GnssSvNotification& svNotify,
                           bool fromEngineHub)>
        GnssAdapterReportSvEventCb;

typedef std::function<void(const GnssAidingDataSvMask& svDataMask)>
        GnssAdapterReqAidingDataCb;

// potential parameters: message queue: MsgTask * msgTask;
// callback function to report back dr and ppe position and sv report
typedef EngineHubProxyBase* (getEngHubProxyFn)(
        const MsgTask * msgTask,
        IOsObserver* osObserver,
        GnssAdapterReportEnginePositionsEventCb positionEventCb,
        GnssAdapterReportSvEventCb svEventCb,
        GnssAdapterReqAidingDataCb reqAidingDataCb);

} // namespace loc_core

#endif // ENGINE_HUB_PROXY_BASE_H
