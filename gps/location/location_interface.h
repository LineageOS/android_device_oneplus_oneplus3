/* Copyright (c) 2017-2019 The Linux Foundation. All rights reserved.
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

#ifndef LOCATION_INTERFACE_H
#define LOCATION_INTERFACE_H

#include <LocationAPI.h>
#include <gps_extended_c.h>

/* Used for callback to deliver GNSS energy consumed */
/** @fn
    @brief Used by query API that retrieves energy consumed by
           modem GNSS engine.

    @param gnssEnergyConsumedFromFirstBoot:
            Energy consumed by the GNSS engine since the first bootup
            in units of 0.1 milli watt seconds.
            A value of 0xffffffffffffffff indicates an invalid reading.
*/
typedef std::function<void(
    uint64_t gnssEnergyConsumedFromFirstBoot
)> GnssEnergyConsumedCallback;

typedef void (*removeClientCompleteCallback)(LocationAPI* client);

struct GnssInterface {
    size_t size;
    void (*initialize)(void);
    void (*deinitialize)(void);
    void (*addClient)(LocationAPI* client, const LocationCallbacks& callbacks);
    void (*removeClient)(LocationAPI* client, removeClientCompleteCallback rmClientCb);
    void (*requestCapabilities)(LocationAPI* client);
    uint32_t (*startTracking)(LocationAPI* client, TrackingOptions&);
    void (*updateTrackingOptions)(LocationAPI* client, uint32_t id, TrackingOptions&);
    void (*stopTracking)(LocationAPI* client, uint32_t id);
    void (*gnssNiResponse)(LocationAPI* client, uint32_t id, GnssNiResponse response);
    void (*setControlCallbacks)(LocationControlCallbacks& controlCallbacks);
    uint32_t (*enable)(LocationTechnologyType techType);
    void (*disable)(uint32_t id);
    uint32_t* (*gnssUpdateConfig)(GnssConfig config);
    uint32_t* (*gnssGetConfig)(GnssConfigFlagsMask config);
    void (*gnssUpdateSvTypeConfig)(GnssSvTypeConfig& config);
    void (*gnssGetSvTypeConfig)(GnssSvTypeConfigCallback& callback);
    void (*gnssResetSvTypeConfig)();
    uint32_t (*gnssDeleteAidingData)(GnssAidingData& data);
    void (*gnssUpdateXtraThrottle)(const bool enabled);
    void (*injectLocation)(double latitude, double longitude, float accuracy);
    void (*injectTime)(int64_t time, int64_t timeReference, int32_t uncertainty);
    void (*agpsInit)(const AgpsCbInfo& cbInfo);
    void (*agpsDataConnOpen)(AGpsExtType agpsType, const char* apnName, int apnLen, int ipType);
    void (*agpsDataConnClosed)(AGpsExtType agpsType);
    void (*agpsDataConnFailed)(AGpsExtType agpsType);
    void (*getDebugReport)(GnssDebugReport& report);
    void (*updateConnectionStatus)(bool connected, int8_t type, bool roaming,
                                   NetworkHandle networkHandle);
    void (*odcpiInit)(const OdcpiRequestCallback& callback);
    void (*odcpiInject)(const Location& location);
    void (*blockCPI)(double latitude, double longitude, float accuracy,
                     int blockDurationMsec, double latLonDiffThreshold);
    void (*getGnssEnergyConsumed)(GnssEnergyConsumedCallback energyConsumedCb);
    void (*enableNfwLocationAccess)(bool enable);
    void (*nfwInit)(const NfwCbInfo& cbInfo);
    void (*getPowerStateChanges)(void* powerStateCb);
    void (*injectLocationExt)(const GnssLocationInfoNotification &locationInfo);
    void (*updateBatteryStatus)(bool charging);
    void (*updateSystemPowerState)(PowerStateType systemPowerState);
    uint32_t (*setConstrainedTunc) (bool enable, float tuncConstraint, uint32_t energyBudget);
    uint32_t (*setPositionAssistedClockEstimator) (bool enable);
    uint32_t (*gnssUpdateSvConfig)(const GnssSvTypeConfig& svTypeConfig,
                                   const GnssSvIdConfig& svIdConfig);
    uint32_t (*gnssResetSvConfig)();
    uint32_t (*configLeverArm)(const LeverArmConfigInfo& configInfo);
};

struct BatchingInterface {
    size_t size;
    void (*initialize)(void);
    void (*deinitialize)(void);
    void (*addClient)(LocationAPI* client, const LocationCallbacks& callbacks);
    void (*removeClient)(LocationAPI* client, removeClientCompleteCallback rmClientCb);
    void (*requestCapabilities)(LocationAPI* client);
    uint32_t (*startBatching)(LocationAPI* client, BatchingOptions&);
    void (*stopBatching)(LocationAPI* client, uint32_t id);
    void (*updateBatchingOptions)(LocationAPI* client, uint32_t id, BatchingOptions&);
    void (*getBatchedLocations)(LocationAPI* client, uint32_t id, size_t count);
};

struct GeofenceInterface {
    size_t size;
    void (*initialize)(void);
    void (*deinitialize)(void);
    void (*addClient)(LocationAPI* client, const LocationCallbacks& callbacks);
    void (*removeClient)(LocationAPI* client, removeClientCompleteCallback rmClientCb);
    void (*requestCapabilities)(LocationAPI* client);
    uint32_t* (*addGeofences)(LocationAPI* client, size_t count, GeofenceOption*, GeofenceInfo*);
    void (*removeGeofences)(LocationAPI* client, size_t count, uint32_t* ids);
    void (*modifyGeofences)(LocationAPI* client, size_t count, uint32_t* ids,
                            GeofenceOption* options);
    void (*pauseGeofences)(LocationAPI* client, size_t count, uint32_t* ids);
    void (*resumeGeofences)(LocationAPI* client, size_t count, uint32_t* ids);
};

#endif /* LOCATION_INTERFACE_H */
