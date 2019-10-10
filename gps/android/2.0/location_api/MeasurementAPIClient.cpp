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

#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_MeasurementAPIClient"

#include <log_util.h>
#include <loc_cfg.h>

#include "LocationUtil.h"
#include "MeasurementAPIClient.h"

namespace android {
namespace hardware {
namespace gnss {
namespace V2_0 {
namespace implementation {

using ::android::hardware::gnss::V1_0::IGnssMeasurement;
using ::android::hardware::gnss::V2_0::IGnssMeasurementCallback;

static void convertGnssData(GnssMeasurementsNotification& in,
        V1_0::IGnssMeasurementCallback::GnssData& out);
static void convertGnssData_1_1(GnssMeasurementsNotification& in,
        V1_1::IGnssMeasurementCallback::GnssData& out);
static void convertGnssData_2_0(GnssMeasurementsNotification& in,
        V2_0::IGnssMeasurementCallback::GnssData& out);
static void convertGnssMeasurement(GnssMeasurementsData& in,
        V1_0::IGnssMeasurementCallback::GnssMeasurement& out);
static void convertGnssClock(GnssMeasurementsClock& in, IGnssMeasurementCallback::GnssClock& out);
static void convertGnssMeasurementsCodeType(GnssMeasurementsCodeType& in,
        ::android::hardware::hidl_string& out);

MeasurementAPIClient::MeasurementAPIClient() :
    mGnssMeasurementCbIface(nullptr),
    mGnssMeasurementCbIface_1_1(nullptr),
    mGnssMeasurementCbIface_2_0(nullptr),
    mTracking(false)
{
    LOC_LOGD("%s]: ()", __FUNCTION__);
}

MeasurementAPIClient::~MeasurementAPIClient()
{
    LOC_LOGD("%s]: ()", __FUNCTION__);
}

// for GpsInterface
Return<IGnssMeasurement::GnssMeasurementStatus>
MeasurementAPIClient::measurementSetCallback(const sp<V1_0::IGnssMeasurementCallback>& callback)
{
    LOC_LOGD("%s]: (%p)", __FUNCTION__, &callback);

    mMutex.lock();
    mGnssMeasurementCbIface = callback;
    mMutex.unlock();

    return startTracking();
}

Return<IGnssMeasurement::GnssMeasurementStatus>
MeasurementAPIClient::measurementSetCallback_1_1(
        const sp<V1_1::IGnssMeasurementCallback>& callback,
        GnssPowerMode powerMode, uint32_t timeBetweenMeasurement)
{
    LOC_LOGD("%s]: (%p) (powermode: %d) (tbm: %d)",
            __FUNCTION__, &callback, (int)powerMode, timeBetweenMeasurement);

    mMutex.lock();
    mGnssMeasurementCbIface_1_1 = callback;
    mMutex.unlock();

    return startTracking(powerMode, timeBetweenMeasurement);
}

Return<IGnssMeasurement::GnssMeasurementStatus>
MeasurementAPIClient::measurementSetCallback_2_0(
    const sp<V2_0::IGnssMeasurementCallback>& callback,
    GnssPowerMode powerMode, uint32_t timeBetweenMeasurement)
{
    LOC_LOGD("%s]: (%p) (powermode: %d) (tbm: %d)",
        __FUNCTION__, &callback, (int)powerMode, timeBetweenMeasurement);

    mMutex.lock();
    mGnssMeasurementCbIface_2_0 = callback;
    mMutex.unlock();

    return startTracking(powerMode, timeBetweenMeasurement);
}

Return<IGnssMeasurement::GnssMeasurementStatus>
MeasurementAPIClient::startTracking(
        GnssPowerMode powerMode, uint32_t timeBetweenMeasurement)
{
    LocationCallbacks locationCallbacks;
    memset(&locationCallbacks, 0, sizeof(LocationCallbacks));
    locationCallbacks.size = sizeof(LocationCallbacks);

    locationCallbacks.trackingCb = nullptr;
    locationCallbacks.batchingCb = nullptr;
    locationCallbacks.geofenceBreachCb = nullptr;
    locationCallbacks.geofenceStatusCb = nullptr;
    locationCallbacks.gnssLocationInfoCb = nullptr;
    locationCallbacks.gnssNiCb = nullptr;
    locationCallbacks.gnssSvCb = nullptr;
    locationCallbacks.gnssNmeaCb = nullptr;

    locationCallbacks.gnssMeasurementsCb = nullptr;
    if (mGnssMeasurementCbIface_2_0 != nullptr ||
        mGnssMeasurementCbIface_1_1 != nullptr ||
        mGnssMeasurementCbIface != nullptr) {
        locationCallbacks.gnssMeasurementsCb =
            [this](GnssMeasurementsNotification gnssMeasurementsNotification) {
                onGnssMeasurementsCb(gnssMeasurementsNotification);
            };
    }

    locAPISetCallbacks(locationCallbacks);

    TrackingOptions options = {};
    memset(&options, 0, sizeof(TrackingOptions));
    options.size = sizeof(TrackingOptions);
    options.minInterval = 1000;
    options.mode = GNSS_SUPL_MODE_STANDALONE;
    if (GNSS_POWER_MODE_INVALID != powerMode) {
        options.powerMode = powerMode;
        options.tbm = timeBetweenMeasurement;
    }

    mTracking = true;
    LOC_LOGD("%s]: start tracking session", __FUNCTION__);
    locAPIStartTracking(options);
    return IGnssMeasurement::GnssMeasurementStatus::SUCCESS;
}

// for GpsMeasurementInterface
void MeasurementAPIClient::measurementClose() {
    LOC_LOGD("%s]: ()", __FUNCTION__);
    mTracking = false;
    locAPIStopTracking();
}

// callbacks
void MeasurementAPIClient::onGnssMeasurementsCb(
        GnssMeasurementsNotification gnssMeasurementsNotification)
{
    LOC_LOGD("%s]: (count: %u active: %d)",
            __FUNCTION__, gnssMeasurementsNotification.count, mTracking);
    if (mTracking) {
        mMutex.lock();
        sp<V1_0::IGnssMeasurementCallback> gnssMeasurementCbIface = nullptr;
        sp<V1_1::IGnssMeasurementCallback> gnssMeasurementCbIface_1_1 = nullptr;
        sp<V2_0::IGnssMeasurementCallback> gnssMeasurementCbIface_2_0 = nullptr;
        if (mGnssMeasurementCbIface_2_0 != nullptr) {
            gnssMeasurementCbIface_2_0 = mGnssMeasurementCbIface_2_0;
        } else if (mGnssMeasurementCbIface_1_1 != nullptr) {
            gnssMeasurementCbIface_1_1 = mGnssMeasurementCbIface_1_1;
        } else if (mGnssMeasurementCbIface != nullptr) {
            gnssMeasurementCbIface = mGnssMeasurementCbIface;
        }
        mMutex.unlock();

        if (gnssMeasurementCbIface_2_0 != nullptr) {
            V2_0::IGnssMeasurementCallback::GnssData gnssData;
            convertGnssData_2_0(gnssMeasurementsNotification, gnssData);
            auto r = gnssMeasurementCbIface_2_0->gnssMeasurementCb_2_0(gnssData);
            if (!r.isOk()) {
                LOC_LOGE("%s] Error from gnssMeasurementCb description=%s",
                    __func__, r.description().c_str());
            }
        } else if (gnssMeasurementCbIface_1_1 != nullptr) {
            V1_1::IGnssMeasurementCallback::GnssData gnssData;
            convertGnssData_1_1(gnssMeasurementsNotification, gnssData);
            auto r = gnssMeasurementCbIface_1_1->gnssMeasurementCb(gnssData);
            if (!r.isOk()) {
                LOC_LOGE("%s] Error from gnssMeasurementCb description=%s",
                    __func__, r.description().c_str());
            }
        } else if (gnssMeasurementCbIface != nullptr) {
            V1_0::IGnssMeasurementCallback::GnssData gnssData;
            convertGnssData(gnssMeasurementsNotification, gnssData);
            auto r = gnssMeasurementCbIface->GnssMeasurementCb(gnssData);
            if (!r.isOk()) {
                LOC_LOGE("%s] Error from GnssMeasurementCb description=%s",
                    __func__, r.description().c_str());
            }
        }
    }
}

static void convertGnssMeasurement(GnssMeasurementsData& in,
        V1_0::IGnssMeasurementCallback::GnssMeasurement& out)
{
    memset(&out, 0, sizeof(out));
    if (in.flags & GNSS_MEASUREMENTS_DATA_SIGNAL_TO_NOISE_RATIO_BIT)
        out.flags |= IGnssMeasurementCallback::GnssMeasurementFlags::HAS_SNR;
    if (in.flags & GNSS_MEASUREMENTS_DATA_CARRIER_FREQUENCY_BIT)
        out.flags |= IGnssMeasurementCallback::GnssMeasurementFlags::HAS_CARRIER_FREQUENCY;
    if (in.flags & GNSS_MEASUREMENTS_DATA_CARRIER_CYCLES_BIT)
        out.flags |= IGnssMeasurementCallback::GnssMeasurementFlags::HAS_CARRIER_CYCLES;
    if (in.flags & GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_BIT)
        out.flags |= IGnssMeasurementCallback::GnssMeasurementFlags::HAS_CARRIER_PHASE;
    if (in.flags & GNSS_MEASUREMENTS_DATA_CARRIER_PHASE_UNCERTAINTY_BIT)
        out.flags |= IGnssMeasurementCallback::GnssMeasurementFlags::HAS_CARRIER_PHASE_UNCERTAINTY;
    if (in.flags & GNSS_MEASUREMENTS_DATA_AUTOMATIC_GAIN_CONTROL_BIT)
        out.flags |= IGnssMeasurementCallback::GnssMeasurementFlags::HAS_AUTOMATIC_GAIN_CONTROL;
    out.svid = in.svId;
    convertGnssConstellationType(in.svType, out.constellation);
    out.timeOffsetNs = in.timeOffsetNs;
    if (in.stateMask & GNSS_MEASUREMENTS_STATE_CODE_LOCK_BIT)
        out.state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_CODE_LOCK;
    if (in.stateMask & GNSS_MEASUREMENTS_STATE_BIT_SYNC_BIT)
        out.state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_BIT_SYNC;
    if (in.stateMask & GNSS_MEASUREMENTS_STATE_SUBFRAME_SYNC_BIT)
        out.state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_SUBFRAME_SYNC;
    if (in.stateMask & GNSS_MEASUREMENTS_STATE_TOW_DECODED_BIT)
        out.state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_TOW_DECODED;
    if (in.stateMask & GNSS_MEASUREMENTS_STATE_MSEC_AMBIGUOUS_BIT)
        out.state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_MSEC_AMBIGUOUS;
    if (in.stateMask & GNSS_MEASUREMENTS_STATE_SYMBOL_SYNC_BIT)
        out.state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_SYMBOL_SYNC;
    if (in.stateMask & GNSS_MEASUREMENTS_STATE_GLO_STRING_SYNC_BIT)
        out.state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_GLO_STRING_SYNC;
    if (in.stateMask & GNSS_MEASUREMENTS_STATE_GLO_TOD_DECODED_BIT)
        out.state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_GLO_TOD_DECODED;
    if (in.stateMask & GNSS_MEASUREMENTS_STATE_BDS_D2_BIT_SYNC_BIT)
        out.state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_BDS_D2_BIT_SYNC;
    if (in.stateMask & GNSS_MEASUREMENTS_STATE_BDS_D2_SUBFRAME_SYNC_BIT)
        out.state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_BDS_D2_SUBFRAME_SYNC;
    if (in.stateMask & GNSS_MEASUREMENTS_STATE_GAL_E1BC_CODE_LOCK_BIT)
        out.state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_GAL_E1BC_CODE_LOCK;
    if (in.stateMask & GNSS_MEASUREMENTS_STATE_GAL_E1C_2ND_CODE_LOCK_BIT)
        out.state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_GAL_E1C_2ND_CODE_LOCK;
    if (in.stateMask & GNSS_MEASUREMENTS_STATE_GAL_E1B_PAGE_SYNC_BIT)
        out.state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_GAL_E1B_PAGE_SYNC;
    if (in.stateMask &  GNSS_MEASUREMENTS_STATE_SBAS_SYNC_BIT)
        out.state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_SBAS_SYNC;
    out.receivedSvTimeInNs = in.receivedSvTimeNs;
    out.receivedSvTimeUncertaintyInNs = in.receivedSvTimeUncertaintyNs;
    out.cN0DbHz = in.carrierToNoiseDbHz;
    out.pseudorangeRateMps = in.pseudorangeRateMps;
    out.pseudorangeRateUncertaintyMps = in.pseudorangeRateUncertaintyMps;
    if (in.adrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_VALID_BIT)
        out.accumulatedDeltaRangeState |=
            IGnssMeasurementCallback::GnssAccumulatedDeltaRangeState::ADR_STATE_VALID;
    if (in.adrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_RESET_BIT)
        out.accumulatedDeltaRangeState |=
            IGnssMeasurementCallback::GnssAccumulatedDeltaRangeState::ADR_STATE_RESET;
    if (in.adrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_CYCLE_SLIP_BIT)
        out.accumulatedDeltaRangeState |=
            IGnssMeasurementCallback::GnssAccumulatedDeltaRangeState::ADR_STATE_CYCLE_SLIP;
    out.accumulatedDeltaRangeM = in.adrMeters;
    out.accumulatedDeltaRangeUncertaintyM = in.adrUncertaintyMeters;
    out.carrierFrequencyHz = in.carrierFrequencyHz;
    out.carrierCycles = in.carrierCycles;
    out.carrierPhase = in.carrierPhase;
    out.carrierPhaseUncertainty = in.carrierPhaseUncertainty;
    uint8_t indicator =
        static_cast<uint8_t>(IGnssMeasurementCallback::GnssMultipathIndicator::INDICATOR_UNKNOWN);
    if (in.multipathIndicator & GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_PRESENT)
        indicator |= IGnssMeasurementCallback::GnssMultipathIndicator::INDICATOR_PRESENT;
    if (in.multipathIndicator & GNSS_MEASUREMENTS_MULTIPATH_INDICATOR_NOT_PRESENT)
        indicator |= IGnssMeasurementCallback::GnssMultipathIndicator::INDICATIOR_NOT_PRESENT;
    out.multipathIndicator =
        static_cast<IGnssMeasurementCallback::GnssMultipathIndicator>(indicator);
    out.snrDb = in.signalToNoiseRatioDb;
    out.agcLevelDb = in.agcLevelDb;
}

static void convertGnssClock(GnssMeasurementsClock& in, IGnssMeasurementCallback::GnssClock& out)
{
    memset(&out, 0, sizeof(IGnssMeasurementCallback::GnssClock));
    if (in.flags & GNSS_MEASUREMENTS_CLOCK_FLAGS_LEAP_SECOND_BIT)
        out.gnssClockFlags |= IGnssMeasurementCallback::GnssClockFlags::HAS_LEAP_SECOND;
    if (in.flags & GNSS_MEASUREMENTS_CLOCK_FLAGS_TIME_UNCERTAINTY_BIT)
        out.gnssClockFlags |= IGnssMeasurementCallback::GnssClockFlags::HAS_TIME_UNCERTAINTY;
    if (in.flags & GNSS_MEASUREMENTS_CLOCK_FLAGS_FULL_BIAS_BIT)
        out.gnssClockFlags |= IGnssMeasurementCallback::GnssClockFlags::HAS_FULL_BIAS;
    if (in.flags & GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_BIT)
        out.gnssClockFlags |= IGnssMeasurementCallback::GnssClockFlags::HAS_BIAS;
    if (in.flags & GNSS_MEASUREMENTS_CLOCK_FLAGS_BIAS_UNCERTAINTY_BIT)
        out.gnssClockFlags |= IGnssMeasurementCallback::GnssClockFlags::HAS_BIAS_UNCERTAINTY;
    if (in.flags & GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_BIT)
        out.gnssClockFlags |= IGnssMeasurementCallback::GnssClockFlags::HAS_DRIFT;
    if (in.flags & GNSS_MEASUREMENTS_CLOCK_FLAGS_DRIFT_UNCERTAINTY_BIT)
        out.gnssClockFlags |= IGnssMeasurementCallback::GnssClockFlags::HAS_DRIFT_UNCERTAINTY;
    out.leapSecond = in.leapSecond;
    out.timeNs = in.timeNs;
    out.timeUncertaintyNs = in.timeUncertaintyNs;
    out.fullBiasNs = in.fullBiasNs;
    out.biasNs = in.biasNs;
    out.biasUncertaintyNs = in.biasUncertaintyNs;
    out.driftNsps = in.driftNsps;
    out.driftUncertaintyNsps = in.driftUncertaintyNsps;
    out.hwClockDiscontinuityCount = in.hwClockDiscontinuityCount;
}

static void convertGnssData(GnssMeasurementsNotification& in,
        V1_0::IGnssMeasurementCallback::GnssData& out)
{
    out.measurementCount = in.count;
    if (out.measurementCount > static_cast<uint32_t>(V1_0::GnssMax::SVS_COUNT)) {
        LOC_LOGW("%s]: Too many measurement %u. Clamps to %d.",
                __FUNCTION__,  out.measurementCount, V1_0::GnssMax::SVS_COUNT);
        out.measurementCount = static_cast<uint32_t>(V1_0::GnssMax::SVS_COUNT);
    }
    for (size_t i = 0; i < out.measurementCount; i++) {
        convertGnssMeasurement(in.measurements[i], out.measurements[i]);
    }
    convertGnssClock(in.clock, out.clock);
}

static void convertGnssData_1_1(GnssMeasurementsNotification& in,
        V1_1::IGnssMeasurementCallback::GnssData& out)
{
    out.measurements.resize(in.count);
    for (size_t i = 0; i < in.count; i++) {
        convertGnssMeasurement(in.measurements[i], out.measurements[i].v1_0);
        if (in.measurements[i].adrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_VALID_BIT)
            out.measurements[i].accumulatedDeltaRangeState |=
            IGnssMeasurementCallback::GnssAccumulatedDeltaRangeState::ADR_STATE_VALID;
        if (in.measurements[i].adrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_RESET_BIT)
            out.measurements[i].accumulatedDeltaRangeState |=
            IGnssMeasurementCallback::GnssAccumulatedDeltaRangeState::ADR_STATE_RESET;
        if (in.measurements[i].adrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_CYCLE_SLIP_BIT)
            out.measurements[i].accumulatedDeltaRangeState |=
            IGnssMeasurementCallback::GnssAccumulatedDeltaRangeState::ADR_STATE_CYCLE_SLIP;
        if (in.measurements[i].adrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_HALF_CYCLE_RESOLVED_BIT)
            out.measurements[i].accumulatedDeltaRangeState |=
            IGnssMeasurementCallback::GnssAccumulatedDeltaRangeState::ADR_STATE_HALF_CYCLE_RESOLVED;
    }
    convertGnssClock(in.clock, out.clock);
}

static void convertGnssData_2_0(GnssMeasurementsNotification& in,
        V2_0::IGnssMeasurementCallback::GnssData& out)
{
    out.measurements.resize(in.count);
    for (size_t i = 0; i < in.count; i++) {
        convertGnssMeasurement(in.measurements[i], out.measurements[i].v1_1.v1_0);
        convertGnssConstellationType(in.measurements[i].svType, out.measurements[i].constellation);
        convertGnssMeasurementsCodeType(in.measurements[i].codeType, out.measurements[i].codeType);
        if (in.measurements[i].adrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_VALID_BIT)
            out.measurements[i].v1_1.accumulatedDeltaRangeState |=
            IGnssMeasurementCallback::GnssAccumulatedDeltaRangeState::ADR_STATE_VALID;
        if (in.measurements[i].adrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_RESET_BIT)
            out.measurements[i].v1_1.accumulatedDeltaRangeState |=
            IGnssMeasurementCallback::GnssAccumulatedDeltaRangeState::ADR_STATE_RESET;
        if (in.measurements[i].adrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_CYCLE_SLIP_BIT)
            out.measurements[i].v1_1.accumulatedDeltaRangeState |=
            IGnssMeasurementCallback::GnssAccumulatedDeltaRangeState::ADR_STATE_CYCLE_SLIP;
        if (in.measurements[i].adrStateMask & GNSS_MEASUREMENTS_ACCUMULATED_DELTA_RANGE_STATE_HALF_CYCLE_RESOLVED_BIT)
            out.measurements[i].v1_1.accumulatedDeltaRangeState |=
            IGnssMeasurementCallback::GnssAccumulatedDeltaRangeState::ADR_STATE_HALF_CYCLE_RESOLVED;
        if (in.measurements[i].stateMask & GNSS_MEASUREMENTS_STATE_CODE_LOCK_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_CODE_LOCK;
        if (in.measurements[i].stateMask & GNSS_MEASUREMENTS_STATE_BIT_SYNC_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_BIT_SYNC;
        if (in.measurements[i].stateMask & GNSS_MEASUREMENTS_STATE_SUBFRAME_SYNC_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_SUBFRAME_SYNC;
        if (in.measurements[i].stateMask & GNSS_MEASUREMENTS_STATE_TOW_DECODED_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_TOW_DECODED;
        if (in.measurements[i].stateMask & GNSS_MEASUREMENTS_STATE_MSEC_AMBIGUOUS_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_MSEC_AMBIGUOUS;
        if (in.measurements[i].stateMask & GNSS_MEASUREMENTS_STATE_SYMBOL_SYNC_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_SYMBOL_SYNC;
        if (in.measurements[i].stateMask & GNSS_MEASUREMENTS_STATE_GLO_STRING_SYNC_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_GLO_STRING_SYNC;
        if (in.measurements[i].stateMask & GNSS_MEASUREMENTS_STATE_GLO_TOD_DECODED_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_GLO_TOD_DECODED;
        if (in.measurements[i].stateMask & GNSS_MEASUREMENTS_STATE_BDS_D2_BIT_SYNC_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_BDS_D2_BIT_SYNC;
        if (in.measurements[i].stateMask & GNSS_MEASUREMENTS_STATE_BDS_D2_SUBFRAME_SYNC_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_BDS_D2_SUBFRAME_SYNC;
        if (in.measurements[i].stateMask & GNSS_MEASUREMENTS_STATE_GAL_E1BC_CODE_LOCK_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_GAL_E1BC_CODE_LOCK;
        if (in.measurements[i].stateMask & GNSS_MEASUREMENTS_STATE_GAL_E1C_2ND_CODE_LOCK_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_GAL_E1C_2ND_CODE_LOCK;
        if (in.measurements[i].stateMask & GNSS_MEASUREMENTS_STATE_GAL_E1B_PAGE_SYNC_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_GAL_E1B_PAGE_SYNC;
        if (in.measurements[i].stateMask &  GNSS_MEASUREMENTS_STATE_SBAS_SYNC_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_SBAS_SYNC;
        if (in.measurements[i].stateMask &  GNSS_MEASUREMENTS_STATE_TOW_KNOWN_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_TOW_KNOWN;
        if (in.measurements[i].stateMask &  GNSS_MEASUREMENTS_STATE_GLO_TOD_KNOWN_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_GLO_TOD_KNOWN;
        if (in.measurements[i].stateMask &  GNSS_MEASUREMENTS_STATE_2ND_CODE_LOCK_BIT)
            out.measurements[i].state |= IGnssMeasurementCallback::GnssMeasurementState::STATE_2ND_CODE_LOCK;
    }
    convertGnssClock(in.clock, out.clock);
}

static void convertGnssMeasurementsCodeType(GnssMeasurementsCodeType& in,
        ::android::hardware::hidl_string& out)
{
    switch(in) {
        case GNSS_MEASUREMENTS_CODE_TYPE_A:
            out = "A";
            break;
        case GNSS_MEASUREMENTS_CODE_TYPE_B:
            out = "B";
            break;
        case GNSS_MEASUREMENTS_CODE_TYPE_C:
            out = "C";
            break;
        case GNSS_MEASUREMENTS_CODE_TYPE_I:
            out = "I";
            break;
        case GNSS_MEASUREMENTS_CODE_TYPE_L:
            out = "L";
            break;
        case GNSS_MEASUREMENTS_CODE_TYPE_M:
            out = "M";
            break;
        case GNSS_MEASUREMENTS_CODE_TYPE_P:
            out = "P";
            break;
        case GNSS_MEASUREMENTS_CODE_TYPE_Q:
            out = "Q";
            break;
        case GNSS_MEASUREMENTS_CODE_TYPE_S:
            out = "S";
            break;
        case GNSS_MEASUREMENTS_CODE_TYPE_W:
            out = "W";
            break;
        case GNSS_MEASUREMENTS_CODE_TYPE_X:
            out = "X";
            break;
        case GNSS_MEASUREMENTS_CODE_TYPE_Y:
            out = "Y";
            break;
        case GNSS_MEASUREMENTS_CODE_TYPE_Z:
            out = "Z";
            break;
        case GNSS_MEASUREMENTS_CODE_TYPE_N:
            out = "N";
            break;
        default:
            out = "UNKNOWN";
    }
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace gnss
}  // namespace hardware
}  // namespace android
