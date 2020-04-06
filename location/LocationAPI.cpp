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
#define LOG_NDEBUG 0
#define LOG_TAG "LocSvc_LocationAPI"

#include <location_interface.h>
#include <dlfcn.h>
#include <loc_pla.h>
#include <log_util.h>
#include <pthread.h>
#include <map>
#include <loc_misc_utils.h>

typedef const GnssInterface* (getGnssInterface)();
typedef const GeofenceInterface* (getGeofenceInterface)();
typedef const BatchingInterface* (getBatchingInterface)();

typedef struct {
    // bit mask of the adpaters that we need to wait for the removeClientCompleteCallback
    // before we invoke the registered locationApiDestroyCompleteCallback
    LocationAdapterTypeMask waitAdapterMask;
    locationApiDestroyCompleteCallback destroyCompleteCb;
} LocationAPIDestroyCbData;

// This is the map for the client that has requested destroy with
// destroy callback provided.
typedef std::map<LocationAPI*, LocationAPIDestroyCbData>
    LocationClientDestroyCbMap;

typedef std::map<LocationAPI*, LocationCallbacks> LocationClientMap;
typedef struct {
    LocationClientMap clientData;
    LocationClientDestroyCbMap destroyClientData;
    LocationControlAPI* controlAPI;
    LocationControlCallbacks controlCallbacks;
    GnssInterface* gnssInterface;
    GeofenceInterface* geofenceInterface;
    BatchingInterface* batchingInterface;
} LocationAPIData;

static LocationAPIData gData = {};
static pthread_mutex_t gDataMutex = PTHREAD_MUTEX_INITIALIZER;
static bool gGnssLoadFailed = false;
static bool gBatchingLoadFailed = false;
static bool gGeofenceLoadFailed = false;

template <typename T1, typename T2>
static const T1* loadLocationInterface(const char* library, const char* name) {
    void* libhandle = nullptr;
    T2* getter = (T2*)dlGetSymFromLib(libhandle, library, name);
    if (nullptr == getter) {
        return (const T1*) getter;
    }else {
        return (*getter)();
    }
}

static bool isGnssClient(LocationCallbacks& locationCallbacks)
{
    return (locationCallbacks.gnssNiCb != nullptr ||
            locationCallbacks.trackingCb != nullptr ||
            locationCallbacks.gnssLocationInfoCb != nullptr ||
            locationCallbacks.engineLocationsInfoCb != nullptr ||
            locationCallbacks.gnssMeasurementsCb != nullptr);
}

static bool isBatchingClient(LocationCallbacks& locationCallbacks)
{
    return (locationCallbacks.batchingCb != nullptr);
}

static bool isGeofenceClient(LocationCallbacks& locationCallbacks)
{
    return (locationCallbacks.geofenceBreachCb != nullptr ||
            locationCallbacks.geofenceStatusCb != nullptr);
}


void LocationAPI::onRemoveClientCompleteCb (LocationAdapterTypeMask adapterType)
{
    bool invokeCallback = false;
    locationApiDestroyCompleteCallback destroyCompleteCb;
    LOC_LOGd("adatper type %x", adapterType);
    pthread_mutex_lock(&gDataMutex);
    auto it = gData.destroyClientData.find(this);
    if (it != gData.destroyClientData.end()) {
        it->second.waitAdapterMask &= ~adapterType;
        if (it->second.waitAdapterMask == 0) {
            invokeCallback = true;
            destroyCompleteCb = it->second.destroyCompleteCb;
            gData.destroyClientData.erase(it);
        }
    }
    pthread_mutex_unlock(&gDataMutex);

    if ((true == invokeCallback) && (nullptr != destroyCompleteCb)) {
        LOC_LOGd("invoke client destroy cb");
        (destroyCompleteCb) ();
        LOC_LOGd("finish invoke client destroy cb");

        delete this;
    }
}

void onGnssRemoveClientCompleteCb (LocationAPI* client)
{
    client->onRemoveClientCompleteCb (LOCATION_ADAPTER_GNSS_TYPE_BIT);
}

void onBatchingRemoveClientCompleteCb (LocationAPI* client)
{
    client->onRemoveClientCompleteCb (LOCATION_ADAPTER_BATCHING_TYPE_BIT);
}

void onGeofenceRemoveClientCompleteCb (LocationAPI* client)
{
    client->onRemoveClientCompleteCb (LOCATION_ADAPTER_GEOFENCE_TYPE_BIT);
}

LocationAPI*
LocationAPI::createInstance(LocationCallbacks& locationCallbacks)
{
    if (nullptr == locationCallbacks.capabilitiesCb ||
        nullptr == locationCallbacks.responseCb ||
        nullptr == locationCallbacks.collectiveResponseCb) {
        return NULL;
    }

    LocationAPI* newLocationAPI = new LocationAPI();
    bool requestedCapabilities = false;

    pthread_mutex_lock(&gDataMutex);

    if (isGnssClient(locationCallbacks)) {
        if (NULL == gData.gnssInterface && !gGnssLoadFailed) {
            gData.gnssInterface =
                (GnssInterface*)loadLocationInterface<GnssInterface,
                    getGnssInterface>("libgnss.so", "getGnssInterface");
            if (NULL == gData.gnssInterface) {
                gGnssLoadFailed = true;
                LOC_LOGW("%s:%d]: No gnss interface available", __func__, __LINE__);
            } else {
                gData.gnssInterface->initialize();
            }
        }
        if (NULL != gData.gnssInterface) {
            gData.gnssInterface->addClient(newLocationAPI, locationCallbacks);
            if (!requestedCapabilities) {
                gData.gnssInterface->requestCapabilities(newLocationAPI);
                requestedCapabilities = true;
            }
        }
    }

    if (isBatchingClient(locationCallbacks)) {
        if (NULL == gData.batchingInterface && !gBatchingLoadFailed) {
            gData.batchingInterface =
                (BatchingInterface*)loadLocationInterface<BatchingInterface,
                 getBatchingInterface>("libbatching.so", "getBatchingInterface");
            if (NULL == gData.batchingInterface) {
                gBatchingLoadFailed = true;
                LOC_LOGW("%s:%d]: No batching interface available", __func__, __LINE__);
            } else {
                gData.batchingInterface->initialize();
            }
        }
        if (NULL != gData.batchingInterface) {
            gData.batchingInterface->addClient(newLocationAPI, locationCallbacks);
            if (!requestedCapabilities) {
                gData.batchingInterface->requestCapabilities(newLocationAPI);
                requestedCapabilities = true;
            }
        }
    }

    if (isGeofenceClient(locationCallbacks)) {
        if (NULL == gData.geofenceInterface && !gGeofenceLoadFailed) {
            gData.geofenceInterface =
               (GeofenceInterface*)loadLocationInterface<GeofenceInterface,
               getGeofenceInterface>("libgeofencing.so", "getGeofenceInterface");
            if (NULL == gData.geofenceInterface) {
                gGeofenceLoadFailed = true;
                LOC_LOGW("%s:%d]: No geofence interface available", __func__, __LINE__);
            } else {
                gData.geofenceInterface->initialize();
            }
        }
        if (NULL != gData.geofenceInterface) {
            gData.geofenceInterface->addClient(newLocationAPI, locationCallbacks);
            if (!requestedCapabilities) {
                gData.geofenceInterface->requestCapabilities(newLocationAPI);
                requestedCapabilities = true;
            }
        }
    }

    gData.clientData[newLocationAPI] = locationCallbacks;

    pthread_mutex_unlock(&gDataMutex);

    return newLocationAPI;
}

void
LocationAPI::destroy(locationApiDestroyCompleteCallback destroyCompleteCb)
{
    bool invokeDestroyCb = false;

    pthread_mutex_lock(&gDataMutex);
    auto it = gData.clientData.find(this);
    if (it != gData.clientData.end()) {
        bool removeFromGnssInf =
                (isGnssClient(it->second) && NULL != gData.gnssInterface);
        bool removeFromBatchingInf =
                (isBatchingClient(it->second) && NULL != gData.batchingInterface);
        bool removeFromGeofenceInf =
                (isGeofenceClient(it->second) && NULL != gData.geofenceInterface);
        bool needToWait = (removeFromGnssInf || removeFromBatchingInf || removeFromGeofenceInf);
        LOC_LOGe("removeFromGnssInf: %d, removeFromBatchingInf: %d, removeFromGeofenceInf: %d,"
                 "need %d", removeFromGnssInf, removeFromBatchingInf, removeFromGeofenceInf,
                 needToWait);

        if ((NULL != destroyCompleteCb) && (true == needToWait)) {
            LocationAPIDestroyCbData destroyCbData = {};
            destroyCbData.destroyCompleteCb = destroyCompleteCb;
            // record down from which adapter we need to wait for the destroy complete callback
            // only when we have received all the needed callbacks from all the associated stacks,
            // we shall notify the client.
            destroyCbData.waitAdapterMask =
                    (removeFromGnssInf ? LOCATION_ADAPTER_GNSS_TYPE_BIT : 0);
            destroyCbData.waitAdapterMask |=
                    (removeFromBatchingInf ? LOCATION_ADAPTER_BATCHING_TYPE_BIT : 0);
            destroyCbData.waitAdapterMask |=
                    (removeFromGeofenceInf ? LOCATION_ADAPTER_GEOFENCE_TYPE_BIT : 0);
            gData.destroyClientData[this] = destroyCbData;
            LOC_LOGe("destroy data stored in the map: 0x%x", destroyCbData.waitAdapterMask);
        }

        if (removeFromGnssInf) {
            gData.gnssInterface->removeClient(it->first,
                                              onGnssRemoveClientCompleteCb);
        }
        if (removeFromBatchingInf) {
            gData.batchingInterface->removeClient(it->first,
                                             onBatchingRemoveClientCompleteCb);
        }
        if (removeFromGeofenceInf) {
            gData.geofenceInterface->removeClient(it->first,
                                                  onGeofenceRemoveClientCompleteCb);
        }

        gData.clientData.erase(it);

        if ((NULL != destroyCompleteCb) && (false == needToWait)) {
            invokeDestroyCb = true;
        }
    } else {
        LOC_LOGE("%s:%d]: Location API client %p not found in client data",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
    if (invokeDestroyCb == true) {
        (destroyCompleteCb) ();
        delete this;
    }
}

LocationAPI::LocationAPI()
{
    LOC_LOGD("LOCATION API CONSTRUCTOR");
}

// private destructor
LocationAPI::~LocationAPI()
{
    LOC_LOGD("LOCATION API DESTRUCTOR");
}

void
LocationAPI::updateCallbacks(LocationCallbacks& locationCallbacks)
{
    if (nullptr == locationCallbacks.capabilitiesCb ||
        nullptr == locationCallbacks.responseCb ||
        nullptr == locationCallbacks.collectiveResponseCb) {
        return;
    }

    pthread_mutex_lock(&gDataMutex);

    if (isGnssClient(locationCallbacks)) {
        if (NULL == gData.gnssInterface && !gGnssLoadFailed) {
            gData.gnssInterface =
                (GnssInterface*)loadLocationInterface<GnssInterface,
                    getGnssInterface>("libgnss.so", "getGnssInterface");
            if (NULL == gData.gnssInterface) {
                gGnssLoadFailed = true;
                LOC_LOGW("%s:%d]: No gnss interface available", __func__, __LINE__);
            } else {
                gData.gnssInterface->initialize();
            }
        }
        if (NULL != gData.gnssInterface) {
            // either adds new Client or updates existing Client
            gData.gnssInterface->addClient(this, locationCallbacks);
        }
    }

    if (isBatchingClient(locationCallbacks)) {
        if (NULL == gData.batchingInterface && !gBatchingLoadFailed) {
            gData.batchingInterface =
                (BatchingInterface*)loadLocationInterface<BatchingInterface,
                 getBatchingInterface>("libbatching.so", "getBatchingInterface");
            if (NULL == gData.batchingInterface) {
                gBatchingLoadFailed = true;
                LOC_LOGW("%s:%d]: No batching interface available", __func__, __LINE__);
            } else {
                gData.batchingInterface->initialize();
            }
        }
        if (NULL != gData.batchingInterface) {
            // either adds new Client or updates existing Client
            gData.batchingInterface->addClient(this, locationCallbacks);
        }
    }

    if (isGeofenceClient(locationCallbacks)) {
        if (NULL == gData.geofenceInterface && !gGeofenceLoadFailed) {
            gData.geofenceInterface =
                (GeofenceInterface*)loadLocationInterface<GeofenceInterface,
                 getGeofenceInterface>("libgeofencing.so", "getGeofenceInterface");
            if (NULL == gData.geofenceInterface) {
                gGeofenceLoadFailed = true;
                LOC_LOGW("%s:%d]: No geofence interface available", __func__, __LINE__);
            } else {
                gData.geofenceInterface->initialize();
            }
        }
        if (NULL != gData.geofenceInterface) {
            // either adds new Client or updates existing Client
            gData.geofenceInterface->addClient(this, locationCallbacks);
        }
    }

    gData.clientData[this] = locationCallbacks;

    pthread_mutex_unlock(&gDataMutex);
}

uint32_t
LocationAPI::startTracking(TrackingOptions& trackingOptions)
{
    uint32_t id = 0;
    pthread_mutex_lock(&gDataMutex);

    auto it = gData.clientData.find(this);
    if (it != gData.clientData.end()) {
        if (NULL != gData.gnssInterface) {
            id = gData.gnssInterface->startTracking(this, trackingOptions);
        } else {
            LOC_LOGE("%s:%d]: No gnss interface available for Location API client %p ",
                     __func__, __LINE__, this);
        }
    } else {
        LOC_LOGE("%s:%d]: Location API client %p not found in client data",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
    return id;
}

void
LocationAPI::stopTracking(uint32_t id)
{
    pthread_mutex_lock(&gDataMutex);

    auto it = gData.clientData.find(this);
    if (it != gData.clientData.end()) {
        if (gData.gnssInterface != NULL) {
            gData.gnssInterface->stopTracking(this, id);
        } else {
            LOC_LOGE("%s:%d]: No gnss interface available for Location API client %p ",
                     __func__, __LINE__, this);
        }
    } else {
        LOC_LOGE("%s:%d]: Location API client %p not found in client data",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
}

void
LocationAPI::updateTrackingOptions(
        uint32_t id, TrackingOptions& trackingOptions)
{
    pthread_mutex_lock(&gDataMutex);

    auto it = gData.clientData.find(this);
    if (it != gData.clientData.end()) {
        if (gData.gnssInterface != NULL) {
            gData.gnssInterface->updateTrackingOptions(this, id, trackingOptions);
        } else {
            LOC_LOGE("%s:%d]: No gnss interface available for Location API client %p ",
                     __func__, __LINE__, this);
        }
    } else {
        LOC_LOGE("%s:%d]: Location API client %p not found in client data",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
}

uint32_t
LocationAPI::startBatching(BatchingOptions &batchingOptions)
{
    uint32_t id = 0;
    pthread_mutex_lock(&gDataMutex);

    if (NULL != gData.batchingInterface) {
        id = gData.batchingInterface->startBatching(this, batchingOptions);
    } else {
        LOC_LOGE("%s:%d]: No batching interface available for Location API client %p ",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
    return id;
}

void
LocationAPI::stopBatching(uint32_t id)
{
    pthread_mutex_lock(&gDataMutex);

    if (NULL != gData.batchingInterface) {
        gData.batchingInterface->stopBatching(this, id);
    } else {
        LOC_LOGE("%s:%d]: No batching interface available for Location API client %p ",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
}

void
LocationAPI::updateBatchingOptions(uint32_t id, BatchingOptions& batchOptions)
{
    pthread_mutex_lock(&gDataMutex);

    if (NULL != gData.batchingInterface) {
        gData.batchingInterface->updateBatchingOptions(this, id, batchOptions);
    } else {
        LOC_LOGE("%s:%d]: No batching interface available for Location API client %p ",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
}

void
LocationAPI::getBatchedLocations(uint32_t id, size_t count)
{
    pthread_mutex_lock(&gDataMutex);

    if (gData.batchingInterface != NULL) {
        gData.batchingInterface->getBatchedLocations(this, id, count);
    } else {
        LOC_LOGE("%s:%d]: No batching interface available for Location API client %p ",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
}

uint32_t*
LocationAPI::addGeofences(size_t count, GeofenceOption* options, GeofenceInfo* info)
{
    uint32_t* ids = NULL;
    pthread_mutex_lock(&gDataMutex);

    if (gData.geofenceInterface != NULL) {
        ids = gData.geofenceInterface->addGeofences(this, count, options, info);
    } else {
        LOC_LOGE("%s:%d]: No geofence interface available for Location API client %p ",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
    return ids;
}

void
LocationAPI::removeGeofences(size_t count, uint32_t* ids)
{
    pthread_mutex_lock(&gDataMutex);

    if (gData.geofenceInterface != NULL) {
        gData.geofenceInterface->removeGeofences(this, count, ids);
    } else {
        LOC_LOGE("%s:%d]: No geofence interface available for Location API client %p ",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
}

void
LocationAPI::modifyGeofences(size_t count, uint32_t* ids, GeofenceOption* options)
{
    pthread_mutex_lock(&gDataMutex);

    if (gData.geofenceInterface != NULL) {
        gData.geofenceInterface->modifyGeofences(this, count, ids, options);
    } else {
        LOC_LOGE("%s:%d]: No geofence interface available for Location API client %p ",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
}

void
LocationAPI::pauseGeofences(size_t count, uint32_t* ids)
{
    pthread_mutex_lock(&gDataMutex);

    if (gData.geofenceInterface != NULL) {
        gData.geofenceInterface->pauseGeofences(this, count, ids);
    } else {
        LOC_LOGE("%s:%d]: No geofence interface available for Location API client %p ",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
}

void
LocationAPI::resumeGeofences(size_t count, uint32_t* ids)
{
    pthread_mutex_lock(&gDataMutex);

    if (gData.geofenceInterface != NULL) {
        gData.geofenceInterface->resumeGeofences(this, count, ids);
    } else {
        LOC_LOGE("%s:%d]: No geofence interface available for Location API client %p ",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
}

void
LocationAPI::gnssNiResponse(uint32_t id, GnssNiResponse response)
{
    pthread_mutex_lock(&gDataMutex);

    if (gData.gnssInterface != NULL) {
        gData.gnssInterface->gnssNiResponse(this, id, response);
    } else {
        LOC_LOGE("%s:%d]: No gnss interface available for Location API client %p ",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
}

LocationControlAPI*
LocationControlAPI::createInstance(LocationControlCallbacks& locationControlCallbacks)
{
    LocationControlAPI* controlAPI = NULL;
    pthread_mutex_lock(&gDataMutex);

    if (nullptr != locationControlCallbacks.responseCb && NULL == gData.controlAPI) {
        if (NULL == gData.gnssInterface && !gGnssLoadFailed) {
            gData.gnssInterface =
                (GnssInterface*)loadLocationInterface<GnssInterface,
                    getGnssInterface>("libgnss.so", "getGnssInterface");
            if (NULL == gData.gnssInterface) {
                gGnssLoadFailed = true;
                LOC_LOGW("%s:%d]: No gnss interface available", __func__, __LINE__);
            } else {
                gData.gnssInterface->initialize();
            }
        }
        if (NULL != gData.gnssInterface) {
            gData.controlAPI = new LocationControlAPI();
            gData.controlCallbacks = locationControlCallbacks;
            gData.gnssInterface->setControlCallbacks(locationControlCallbacks);
            controlAPI = gData.controlAPI;
        }
    }

    pthread_mutex_unlock(&gDataMutex);
    return controlAPI;
}

void
LocationControlAPI::destroy()
{
    delete this;
}

LocationControlAPI::LocationControlAPI()
{
    LOC_LOGD("LOCATION CONTROL API CONSTRUCTOR");
}

LocationControlAPI::~LocationControlAPI()
{
    LOC_LOGD("LOCATION CONTROL API DESTRUCTOR");
    pthread_mutex_lock(&gDataMutex);

    gData.controlAPI = NULL;

    pthread_mutex_unlock(&gDataMutex);
}

uint32_t
LocationControlAPI::enable(LocationTechnologyType techType)
{
    uint32_t id = 0;
    pthread_mutex_lock(&gDataMutex);

    if (gData.gnssInterface != NULL) {
        id = gData.gnssInterface->enable(techType);
    } else {
        LOC_LOGE("%s:%d]: No gnss interface available for Location Control API client %p ",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
    return id;
}

void
LocationControlAPI::disable(uint32_t id)
{
    pthread_mutex_lock(&gDataMutex);

    if (gData.gnssInterface != NULL) {
        gData.gnssInterface->disable(id);
    } else {
        LOC_LOGE("%s:%d]: No gnss interface available for Location Control API client %p ",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
}

uint32_t*
LocationControlAPI::gnssUpdateConfig(GnssConfig config)
{
    uint32_t* ids = NULL;
    pthread_mutex_lock(&gDataMutex);

    if (gData.gnssInterface != NULL) {
        ids = gData.gnssInterface->gnssUpdateConfig(config);
    } else {
        LOC_LOGE("%s:%d]: No gnss interface available for Location Control API client %p ",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
    return ids;
}

uint32_t* LocationControlAPI::gnssGetConfig(GnssConfigFlagsMask mask) {

    uint32_t* ids = NULL;
    pthread_mutex_lock(&gDataMutex);

    if (NULL != gData.gnssInterface) {
        ids = gData.gnssInterface->gnssGetConfig(mask);
    } else {
        LOC_LOGe("No gnss interface available for Control API client %p", this);
    }

    pthread_mutex_unlock(&gDataMutex);
    return ids;
}

uint32_t
LocationControlAPI::gnssDeleteAidingData(GnssAidingData& data)
{
    uint32_t id = 0;
    pthread_mutex_lock(&gDataMutex);

    if (gData.gnssInterface != NULL) {
        id = gData.gnssInterface->gnssDeleteAidingData(data);
    } else {
        LOC_LOGE("%s:%d]: No gnss interface available for Location Control API client %p ",
                 __func__, __LINE__, this);
    }

    pthread_mutex_unlock(&gDataMutex);
    return id;
}
