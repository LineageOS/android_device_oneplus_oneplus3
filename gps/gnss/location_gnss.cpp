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

#include "GnssAdapter.h"
#include "location_interface.h"

static GnssAdapter* gGnssAdapter = NULL;

static void initialize();
static void deinitialize();

static void addClient(LocationAPI* client, const LocationCallbacks& callbacks);
static void removeClient(LocationAPI* client, removeClientCompleteCallback rmClientCb);
static void requestCapabilities(LocationAPI* client);

static uint32_t startTracking(LocationAPI* client, TrackingOptions&);
static void updateTrackingOptions(LocationAPI* client, uint32_t id, TrackingOptions&);
static void stopTracking(LocationAPI* client, uint32_t id);

static void gnssNiResponse(LocationAPI* client, uint32_t id, GnssNiResponse response);
static uint32_t gnssDeleteAidingData(GnssAidingData& data);
static void gnssUpdateXtraThrottle(const bool enabled);

static void setControlCallbacks(LocationControlCallbacks& controlCallbacks);
static uint32_t enable(LocationTechnologyType techType);
static void disable(uint32_t id);
static uint32_t* gnssUpdateConfig(GnssConfig config);
static uint32_t* gnssGetConfig(GnssConfigFlagsMask mask);

static void gnssUpdateSvTypeConfig(GnssSvTypeConfig& config);
static void gnssGetSvTypeConfig(GnssSvTypeConfigCallback& callback);
static void gnssResetSvTypeConfig();

static void injectLocation(double latitude, double longitude, float accuracy);
static void injectLocationExt(const GnssLocationInfoNotification &locationInfo);
static void injectTime(int64_t time, int64_t timeReference, int32_t uncertainty);

static void agpsInit(const AgpsCbInfo& cbInfo);
static void agpsDataConnOpen(AGpsExtType agpsType, const char* apnName, int apnLen, int ipType);
static void agpsDataConnClosed(AGpsExtType agpsType);
static void agpsDataConnFailed(AGpsExtType agpsType);
static void getDebugReport(GnssDebugReport& report);
static void updateConnectionStatus(bool connected, int8_t type, bool roaming = false,
                                   NetworkHandle networkHandle = NETWORK_HANDLE_UNKNOWN);
static void getGnssEnergyConsumed(GnssEnergyConsumedCallback energyConsumedCb);
static void enableNfwLocationAccess(bool enable);
static void nfwInit(const NfwCbInfo& cbInfo);
static void getPowerStateChanges(void* powerStateCb);

static void odcpiInit(const OdcpiRequestCallback& callback);
static void odcpiInject(const Location& location);

static void blockCPI(double latitude, double longitude, float accuracy,
                     int blockDurationMsec, double latLonDiffThreshold);
static void updateBatteryStatus(bool charging);

static const GnssInterface gGnssInterface = {
    sizeof(GnssInterface),
    initialize,
    deinitialize,
    addClient,
    removeClient,
    requestCapabilities,
    startTracking,
    updateTrackingOptions,
    stopTracking,
    gnssNiResponse,
    setControlCallbacks,
    enable,
    disable,
    gnssUpdateConfig,
    gnssGetConfig,
    gnssUpdateSvTypeConfig,
    gnssGetSvTypeConfig,
    gnssResetSvTypeConfig,
    gnssDeleteAidingData,
    gnssUpdateXtraThrottle,
    injectLocation,
    injectTime,
    agpsInit,
    agpsDataConnOpen,
    agpsDataConnClosed,
    agpsDataConnFailed,
    getDebugReport,
    updateConnectionStatus,
    odcpiInit,
    odcpiInject,
    blockCPI,
    getGnssEnergyConsumed,
    enableNfwLocationAccess,
    nfwInit,
    getPowerStateChanges,
    injectLocationExt,
    updateBatteryStatus
};

#ifndef DEBUG_X86
extern "C" const GnssInterface* getGnssInterface()
#else
const GnssInterface* getGnssInterface()
#endif // DEBUG_X86
{
   return &gGnssInterface;
}

static void initialize()
{
    if (NULL == gGnssAdapter) {
        gGnssAdapter = new GnssAdapter();
    }
}

static void deinitialize()
{
    if (NULL != gGnssAdapter) {
        delete gGnssAdapter;
        gGnssAdapter = NULL;
    }
}

static void addClient(LocationAPI* client, const LocationCallbacks& callbacks)
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->addClientCommand(client, callbacks);
    }
}

static void removeClient(LocationAPI* client, removeClientCompleteCallback rmClientCb)
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->removeClientCommand(client, rmClientCb);
    }
}

static void requestCapabilities(LocationAPI* client)
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->requestCapabilitiesCommand(client);
    }
}

static uint32_t startTracking(
        LocationAPI* client, TrackingOptions& trackingOptions)
{
    if (NULL != gGnssAdapter) {
        return gGnssAdapter->startTrackingCommand(client, trackingOptions);
    } else {
        return 0;
    }
}

static void updateTrackingOptions(
        LocationAPI* client, uint32_t id, TrackingOptions& trackingOptions)
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->updateTrackingOptionsCommand(
                client, id, trackingOptions);
    }
}

static void stopTracking(LocationAPI* client, uint32_t id)
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->stopTrackingCommand(client, id);
    }
}

static void gnssNiResponse(LocationAPI* client, uint32_t id, GnssNiResponse response)
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->gnssNiResponseCommand(client, id, response);
    }
}

static void setControlCallbacks(LocationControlCallbacks& controlCallbacks)
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->setControlCallbacksCommand(controlCallbacks);
    }
}

static uint32_t enable(LocationTechnologyType techType)
{
    if (NULL != gGnssAdapter) {
        return gGnssAdapter->enableCommand(techType);
    } else {
        return 0;
    }
}

static void disable(uint32_t id)
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->disableCommand(id);
    }
}

static uint32_t* gnssUpdateConfig(GnssConfig config)
{
    if (NULL != gGnssAdapter) {
        return gGnssAdapter->gnssUpdateConfigCommand(config);
    } else {
        return NULL;
    }
}

static uint32_t* gnssGetConfig(GnssConfigFlagsMask mask)
{
    if (NULL != gGnssAdapter) {
        return gGnssAdapter->gnssGetConfigCommand(mask);
    } else {
        return NULL;
    }
}

static void gnssUpdateSvTypeConfig(GnssSvTypeConfig& config)
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->gnssUpdateSvTypeConfigCommand(config);
    }
}

static void gnssGetSvTypeConfig(GnssSvTypeConfigCallback& callback)
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->gnssGetSvTypeConfigCommand(callback);
    }
}

static void gnssResetSvTypeConfig()
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->gnssResetSvTypeConfigCommand();
    }
}

static uint32_t gnssDeleteAidingData(GnssAidingData& data)
{
    if (NULL != gGnssAdapter) {
        return gGnssAdapter->gnssDeleteAidingDataCommand(data);
    } else {
        return 0;
    }
}

static void gnssUpdateXtraThrottle(const bool enabled)
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->gnssUpdateXtraThrottleCommand(enabled);
    }
}

static void injectLocation(double latitude, double longitude, float accuracy)
{
   if (NULL != gGnssAdapter) {
       gGnssAdapter->injectLocationCommand(latitude, longitude, accuracy);
   }
}

static void injectTime(int64_t time, int64_t timeReference, int32_t uncertainty)
{
   if (NULL != gGnssAdapter) {
       gGnssAdapter->injectTimeCommand(time, timeReference, uncertainty);
   }
}

static void agpsInit(const AgpsCbInfo& cbInfo) {

    if (NULL != gGnssAdapter) {
        gGnssAdapter->initAgpsCommand(cbInfo);
    }
}
static void agpsDataConnOpen(
        AGpsExtType agpsType, const char* apnName, int apnLen, int ipType) {

    if (NULL != gGnssAdapter) {
        gGnssAdapter->dataConnOpenCommand(
                agpsType, apnName, apnLen, (AGpsBearerType)ipType);
    }
}
static void agpsDataConnClosed(AGpsExtType agpsType) {

    if (NULL != gGnssAdapter) {
        gGnssAdapter->dataConnClosedCommand(agpsType);
    }
}
static void agpsDataConnFailed(AGpsExtType agpsType) {

    if (NULL != gGnssAdapter) {
        gGnssAdapter->dataConnFailedCommand(agpsType);
    }
}

static void getDebugReport(GnssDebugReport& report) {

    if (NULL != gGnssAdapter) {
        gGnssAdapter->getDebugReport(report);
    }
}

static void updateConnectionStatus(bool connected, int8_t type,
                                   bool roaming, NetworkHandle networkHandle) {
    if (NULL != gGnssAdapter) {
        gGnssAdapter->getSystemStatus()->eventConnectionStatus(
                connected, type, roaming, networkHandle);
    }
}

static void odcpiInit(const OdcpiRequestCallback& callback)
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->initOdcpiCommand(callback);
    }
}

static void odcpiInject(const Location& location)
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->injectOdcpiCommand(location);
    }
}

static void blockCPI(double latitude, double longitude, float accuracy,
                     int blockDurationMsec, double latLonDiffThreshold) {
    if (NULL != gGnssAdapter) {
        gGnssAdapter->blockCPICommand(latitude, longitude, accuracy,
                                      blockDurationMsec, latLonDiffThreshold);
    }
}

static void getGnssEnergyConsumed(GnssEnergyConsumedCallback energyConsumedCb) {
    if (NULL != gGnssAdapter) {
        gGnssAdapter->getGnssEnergyConsumedCommand(energyConsumedCb);
    }
}

static void enableNfwLocationAccess(bool enable) {
    if (NULL != gGnssAdapter) {
        gGnssAdapter->nfwControlCommand(enable);
    }
}

static void nfwInit(const NfwCbInfo& cbInfo) {
    if (NULL != gGnssAdapter) {
        gGnssAdapter->initNfwCommand(cbInfo);
    }
}
static void getPowerStateChanges(void* powerStateCb)
{
    if (NULL != gGnssAdapter) {
        gGnssAdapter->getPowerStateChangesCommand(powerStateCb);
    }
}

static void injectLocationExt(const GnssLocationInfoNotification &locationInfo)
{
   if (NULL != gGnssAdapter) {
       gGnssAdapter->injectLocationExtCommand(locationInfo);
   }
}

static void updateBatteryStatus(bool charging) {
    if (NULL != gGnssAdapter) {
        gGnssAdapter->getSystemStatus()->updatePowerConnectState(charging);
    }
}
