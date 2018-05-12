/* Copyright (c) 2017 The Linux Foundation. All rights reserved.
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

#define LOG_NDDEBUG 0
#define LOG_TAG "LocSvc_APIClientBase"

#include <platform_lib_log_util.h>
#include <inttypes.h>
#include <loc_cfg.h>
#include "LocationAPIClientBase.h"

#define FLP_CONF_FILE "/etc/flp.conf"
#define GEOFENCE_SESSION_ID 0xFFFFFFFF
#define CONFIG_SESSION_ID 0xFFFFFFFF

// LocationAPIControlClient
LocationAPIControlClient::LocationAPIControlClient() :
    mEnabled(false)
{
    pthread_mutex_init(&mMutex, nullptr);

    for (int i = 0; i < CTRL_REQUEST_MAX; i++) {
        mRequestQueues[i].reset(0);
    }

    memset(&mConfig, 0, sizeof(GnssConfig));

    LocationControlCallbacks locationControlCallbacks;
    locationControlCallbacks.size = sizeof(LocationControlCallbacks);

    locationControlCallbacks.responseCb =
        [this](LocationError error, uint32_t id) {
            onCtrlResponseCb(error, id);
        };
    locationControlCallbacks.collectiveResponseCb =
        [this](size_t count, LocationError* errors, uint32_t* ids) {
            onCtrlCollectiveResponseCb(count, errors, ids);
        };

    mLocationControlAPI = LocationControlAPI::createInstance(locationControlCallbacks);
}

LocationAPIControlClient::~LocationAPIControlClient()
{
    pthread_mutex_lock(&mMutex);

    if (mLocationControlAPI) {
        mLocationControlAPI->destroy();
        mLocationControlAPI = nullptr;
    }

    for (int i = 0; i < CTRL_REQUEST_MAX; i++) {
        mRequestQueues[i].reset(0);
    }

    pthread_mutex_unlock(&mMutex);

    pthread_mutex_destroy(&mMutex);
}

uint32_t LocationAPIControlClient::locAPIGnssDeleteAidingData(GnssAidingData& data)
{
    uint32_t retVal = LOCATION_ERROR_GENERAL_FAILURE;
    pthread_mutex_lock(&mMutex);
    if (mLocationControlAPI) {
        uint32_t session = mLocationControlAPI->gnssDeleteAidingData(data);
        LOC_LOGI("%s:%d] start new session: %d", __FUNCTION__, __LINE__, session);
        mRequestQueues[CTRL_REQUEST_DELETEAIDINGDATA].reset(session);
        mRequestQueues[CTRL_REQUEST_DELETEAIDINGDATA].push(new GnssDeleteAidingDataRequest(*this));

        retVal = LOCATION_ERROR_SUCCESS;
    }
    pthread_mutex_unlock(&mMutex);

    return retVal;
}

uint32_t LocationAPIControlClient::locAPIEnable(LocationTechnologyType techType)
{
    uint32_t retVal = LOCATION_ERROR_GENERAL_FAILURE;
    pthread_mutex_lock(&mMutex);
    if (mEnabled) {
        // just return success if already enabled
        retVal = LOCATION_ERROR_SUCCESS;
    } else if (mLocationControlAPI) {
        uint32_t session = mLocationControlAPI->enable(techType);
        LOC_LOGI("%s:%d] start new session: %d", __FUNCTION__, __LINE__, session);
        mRequestQueues[CTRL_REQUEST_CONTROL].reset(session);
        mRequestQueues[CTRL_REQUEST_CONTROL].push(new EnableRequest(*this));
        retVal = LOCATION_ERROR_SUCCESS;
        mEnabled = true;
    } else {
        LOC_LOGE("%s:%d] failed.", __FUNCTION__, __LINE__);
    }
    pthread_mutex_unlock(&mMutex);

    return retVal;
}

void LocationAPIControlClient::locAPIDisable()
{
    pthread_mutex_lock(&mMutex);
    if (mEnabled && mLocationControlAPI) {
        uint32_t session = 0;
        session = mRequestQueues[CTRL_REQUEST_CONTROL].getSession();
        if (session > 0) {
            mRequestQueues[CTRL_REQUEST_CONTROL].push(new DisableRequest(*this));
            mLocationControlAPI->disable(session);
            mEnabled = false;
        } else {
            LOC_LOGE("%s:%d] invalid session: %d.", __FUNCTION__, __LINE__, session);
        }
    }
    pthread_mutex_unlock(&mMutex);
}

uint32_t LocationAPIControlClient::locAPIGnssUpdateConfig(GnssConfig config)
{
    uint32_t retVal = LOCATION_ERROR_GENERAL_FAILURE;
    if (memcmp(&mConfig, &config, sizeof(GnssConfig)) == 0) {
        LOC_LOGV("%s:%d] GnssConfig is identical to previous call", __FUNCTION__, __LINE__);
        retVal = LOCATION_ERROR_SUCCESS;
        return retVal;
    }

    pthread_mutex_lock(&mMutex);
    if (mLocationControlAPI) {

        memcpy(&mConfig, &config, sizeof(GnssConfig));

        uint32_t session = 0;
        uint32_t* idArray = mLocationControlAPI->gnssUpdateConfig(config);
        LOC_LOGV("%s:%d] gnssUpdateConfig return array: %p", __FUNCTION__, __LINE__, idArray);
        if (idArray != nullptr) {
            if (mRequestQueues[CTRL_REQUEST_CONFIG].getSession() != CONFIG_SESSION_ID) {
                mRequestQueues[CTRL_REQUEST_CONFIG].reset(CONFIG_SESSION_ID);
            }
            mRequestQueues[CTRL_REQUEST_CONFIG].push(new GnssUpdateConfigRequest(*this));
            retVal = LOCATION_ERROR_SUCCESS;
        }
    }
    pthread_mutex_unlock(&mMutex);
    return retVal;
}

void LocationAPIControlClient::onCtrlResponseCb(LocationError error, uint32_t id)
{
    if (error != LOCATION_ERROR_SUCCESS) {
        LOC_LOGE("%s:%d] ERROR: %d ID: %d", __FUNCTION__, __LINE__, error, id);
    } else {
        LOC_LOGV("%s:%d] SUCCESS: %d id: %d", __FUNCTION__, __LINE__, error, id);
    }
    LocationAPIRequest* request = getRequestBySession(id);
    if (request) {
        request->onResponse(error, id);
        delete request;
    }
}

void LocationAPIControlClient::onCtrlCollectiveResponseCb(
        size_t count, LocationError* errors, uint32_t* ids)
{
    for (size_t i = 0; i < count; i++) {
        if (errors[i] != LOCATION_ERROR_SUCCESS) {
            LOC_LOGE("%s:%d] ERROR: %d ID: %d", __FUNCTION__, __LINE__, errors[i], ids[i]);
        } else {
            LOC_LOGV("%s:%d] SUCCESS: %d id: %d", __FUNCTION__, __LINE__, errors[i], ids[i]);
        }
    }
    LocationAPIRequest* request = nullptr;
    pthread_mutex_lock(&mMutex);
    if (mRequestQueues[CTRL_REQUEST_CONFIG].getSession() == CONFIG_SESSION_ID) {
        request = mRequestQueues[CTRL_REQUEST_CONFIG].pop();
    }
    pthread_mutex_unlock(&mMutex);
    if (request) {
        request->onCollectiveResponse(count, errors, ids);
        delete request;
    }
}

LocationAPIRequest* LocationAPIControlClient::getRequestBySession(uint32_t session)
{
    pthread_mutex_lock(&mMutex);
    LocationAPIRequest* request = nullptr;
    for (int i = 0; i < CTRL_REQUEST_MAX; i++) {
        if (i != CTRL_REQUEST_CONFIG &&
                mRequestQueues[i].getSession() == session) {
            request = mRequestQueues[i].pop();
            break;
        }
    }
    pthread_mutex_unlock(&mMutex);
    return request;
}

// LocationAPIClientBase
LocationAPIClientBase::LocationAPIClientBase() :
    mGeofenceBreachCallback(nullptr),
    mBatchingStatusCallback(nullptr),
    mLocationAPI(nullptr),
    mBatchSize(-1),
    mTracking(false)
{

    // use recursive mutex, in case callback come from the same thread
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mMutex, &attr);

    for (int i = 0; i < REQUEST_MAX; i++) {
        mRequestQueues[i].reset(0);
    }
}

void LocationAPIClientBase::locAPISetCallbacks(LocationCallbacks& locationCallbacks)
{
    pthread_mutex_lock(&mMutex);

    if (locationCallbacks.geofenceBreachCb != nullptr) {
        mGeofenceBreachCallback = locationCallbacks.geofenceBreachCb;
        locationCallbacks.geofenceBreachCb =
            [this](GeofenceBreachNotification geofenceBreachNotification) {
                beforeGeofenceBreachCb(geofenceBreachNotification);
            };
    }

    locationCallbacks.capabilitiesCb =
        [this](LocationCapabilitiesMask capabilitiesMask) {
            onCapabilitiesCb(capabilitiesMask);
        };
    locationCallbacks.responseCb = [this](LocationError error, uint32_t id) {
        onResponseCb(error, id);
    };
    locationCallbacks.collectiveResponseCb =
        [this](size_t count, LocationError* errors, uint32_t* ids) {
            onCollectiveResponseCb(count, errors, ids);
        };

    if (locationCallbacks.batchingStatusCb != nullptr) {
        mBatchingStatusCallback = locationCallbacks.batchingStatusCb;
        locationCallbacks.batchingStatusCb =
            [this](BatchingStatusInfo batchStatus, std::list<uint32_t> & tripCompletedList) {
            beforeBatchingStatusCb(batchStatus, tripCompletedList);
        };
    }

    if (mLocationAPI == nullptr ) {
        mLocationAPI = LocationAPI::createInstance(locationCallbacks);
    } else {
        mLocationAPI->updateCallbacks(locationCallbacks);
    }

    pthread_mutex_unlock(&mMutex);
}

LocationAPIClientBase::~LocationAPIClientBase()
{
    pthread_mutex_lock(&mMutex);

    mGeofenceBreachCallback = nullptr;

    if (mLocationAPI) {
        mLocationAPI->destroy();
        mLocationAPI = nullptr;
    }

    for (int i = 0; i < REQUEST_MAX; i++) {
        mRequestQueues[i].reset(0);
    }

    pthread_mutex_unlock(&mMutex);

    pthread_mutex_destroy(&mMutex);
}

uint32_t LocationAPIClientBase::locAPIStartTracking(LocationOptions& options)
{
    uint32_t retVal = LOCATION_ERROR_GENERAL_FAILURE;
    pthread_mutex_lock(&mMutex);
    if (mLocationAPI) {
        if (mTracking) {
            LOC_LOGW("%s:%d] Existing tracking session present", __FUNCTION__, __LINE__);
        } else {
            uint32_t session = mLocationAPI->startTracking(options);
            LOC_LOGI("%s:%d] start new session: %d", __FUNCTION__, __LINE__, session);
            // onResponseCb might be called from other thread immediately after
            // startTracking returns, so we are not going to unlock mutex
            // until StartTrackingRequest is pushed into mRequestQueues[REQUEST_TRACKING]
            mRequestQueues[REQUEST_TRACKING].reset(session);
            mRequestQueues[REQUEST_TRACKING].push(new StartTrackingRequest(*this));
            mTracking = true;
        }

        retVal = LOCATION_ERROR_SUCCESS;
    }
    pthread_mutex_unlock(&mMutex);

    return retVal;
}

void LocationAPIClientBase::locAPIStopTracking()
{
    pthread_mutex_lock(&mMutex);
    if (mLocationAPI) {
        uint32_t session = 0;
        session = mRequestQueues[REQUEST_TRACKING].getSession();
        if (session > 0) {
            mRequestQueues[REQUEST_TRACKING].push(new StopTrackingRequest(*this));
            mLocationAPI->stopTracking(session);
            mTracking = false;
        } else {
            LOC_LOGE("%s:%d] invalid session: %d.", __FUNCTION__, __LINE__, session);
        }
    }
    pthread_mutex_unlock(&mMutex);
}

void LocationAPIClientBase::locAPIUpdateTrackingOptions(LocationOptions& options)
{
    pthread_mutex_lock(&mMutex);
    if (mLocationAPI) {
        uint32_t session = 0;
        session = mRequestQueues[REQUEST_TRACKING].getSession();
        if (session > 0) {
            mRequestQueues[REQUEST_TRACKING].push(new UpdateTrackingOptionsRequest(*this));
            mLocationAPI->updateTrackingOptions(session, options);
        } else {
            LOC_LOGE("%s:%d] invalid session: %d.", __FUNCTION__, __LINE__, session);
        }
    }
    pthread_mutex_unlock(&mMutex);
}

int32_t LocationAPIClientBase::locAPIGetBatchSize()
{
    if (mBatchSize == -1) {
        const loc_param_s_type flp_conf_param_table[] =
        {
            {"BATCH_SIZE", &mBatchSize, nullptr, 'n'},
        };
        UTIL_READ_CONF(FLP_CONF_FILE, flp_conf_param_table);
        if (mBatchSize < 0) {
            // set mBatchSize to 0 if we got an illegal value from config file
            mBatchSize = 0;
        }
    }
    return mBatchSize;
}


uint32_t LocationAPIClientBase::locAPIStartSession(uint32_t id, uint32_t sessionMode,
        LocationOptions& locationOptions)
{
    uint32_t retVal = LOCATION_ERROR_GENERAL_FAILURE;
    pthread_mutex_lock(&mMutex);
    if (mLocationAPI) {

        if (mSessionBiDict.hasId(id)) {
            LOC_LOGE("%s:%d] session %d has already started.", __FUNCTION__, __LINE__, id);
            retVal = LOCATION_ERROR_ALREADY_STARTED;
        } else {
            uint32_t trackingSession = 0;
            uint32_t batchingSession = 0;

            if (sessionMode == SESSION_MODE_ON_FIX) {
                trackingSession = mLocationAPI->startTracking(locationOptions);
                LOC_LOGI("%s:%d] start new session: %d", __FUNCTION__, __LINE__, trackingSession);
                mRequestQueues[REQUEST_SESSION].push(new StartTrackingRequest(*this));
            } else {
                // Fill in the batch mode
                BatchingOptions batchOptions = {};
                batchOptions.size = sizeof(BatchingOptions);
                switch (sessionMode) {
                case SESSION_MODE_ON_FULL:
                    batchOptions.batchingMode = BATCHING_MODE_ROUTINE;
                    break;
                case SESSION_MODE_ON_TRIP_COMPLETED:
                    batchOptions.batchingMode = BATCHING_MODE_TRIP;
                    break;
                default:
                    batchOptions.batchingMode = BATCHING_MODE_NO_AUTO_REPORT;
                    break;
                }

                batchingSession = mLocationAPI->startBatching(locationOptions, batchOptions);
                LOC_LOGI("%s:%d] start new session: %d", __FUNCTION__, __LINE__, batchingSession);
                mRequestQueues[REQUEST_SESSION].setSession(batchingSession);
                mRequestQueues[REQUEST_SESSION].push(new StartBatchingRequest(*this));
            }

            uint32_t session = ((sessionMode != SESSION_MODE_ON_FIX) ?
                    batchingSession : trackingSession);

            SessionEntity entity;
            entity.id = id;
            entity.trackingSession = trackingSession;
            entity.batchingSession = batchingSession;
            entity.sessionMode = sessionMode;
            mSessionBiDict.set(id, session, entity);

            retVal = LOCATION_ERROR_SUCCESS;
        }

    }
    pthread_mutex_unlock(&mMutex);

    return retVal;
}

uint32_t LocationAPIClientBase::locAPIStopSession(uint32_t id)
{
    uint32_t retVal = LOCATION_ERROR_GENERAL_FAILURE;
    pthread_mutex_lock(&mMutex);
    if (mLocationAPI) {

        if (mSessionBiDict.hasId(id)) {
            SessionEntity entity = mSessionBiDict.getExtById(id);

            uint32_t trackingSession = entity.trackingSession;
            uint32_t batchingSession = entity.batchingSession;
            uint32_t sMode = entity.sessionMode;

            if (sMode == SESSION_MODE_ON_FIX) {
                mRequestQueues[REQUEST_SESSION].push(new StopTrackingRequest(*this));
                mLocationAPI->stopTracking(trackingSession);
            } else {
                mRequestQueues[REQUEST_SESSION].push(new StopBatchingRequest(*this));
                mLocationAPI->stopBatching(batchingSession);
            }

            retVal = LOCATION_ERROR_SUCCESS;
        } else {
            retVal = LOCATION_ERROR_ID_UNKNOWN;
            LOC_LOGE("%s:%d] session %d is not exist.", __FUNCTION__, __LINE__, id);
        }

    }
    pthread_mutex_unlock(&mMutex);
    return retVal;
}

uint32_t LocationAPIClientBase::locAPIUpdateSessionOptions(uint32_t id, uint32_t sessionMode,
        LocationOptions& options)
{
    uint32_t retVal = LOCATION_ERROR_GENERAL_FAILURE;
    pthread_mutex_lock(&mMutex);
    if (mLocationAPI) {

        if (mSessionBiDict.hasId(id)) {
            SessionEntity entity = mSessionBiDict.getExtById(id);

            uint32_t trackingSession = entity.trackingSession;
            uint32_t batchingSession = entity.batchingSession;
            uint32_t sMode = entity.sessionMode;

            if (sessionMode == SESSION_MODE_ON_FIX) {
                // we only add an UpdateTrackingOptionsRequest to mRequestQueues[REQUEST_SESSION],
                // even if this update request will stop batching and then start tracking.
                mRequestQueues[REQUEST_SESSION].push(new UpdateTrackingOptionsRequest(*this));
                if (sMode == SESSION_MODE_ON_FIX) {
                    mLocationAPI->updateTrackingOptions(trackingSession, options);
                } else  {
                    // stop batching
                    // batchingSession will be removed from mSessionBiDict soon,
                    // so we don't need to add a new request to mRequestQueues[REQUEST_SESSION].
                    mLocationAPI->stopBatching(batchingSession);
                    batchingSession = 0;
                    mRequestQueues[REQUEST_SESSION].setSession(batchingSession);

                    // start tracking
                    trackingSession = mLocationAPI->startTracking(options);
                    LOC_LOGI("%s:%d] start new session: %d",
                            __FUNCTION__, __LINE__, trackingSession);
                }
            } else {
                // we only add an UpdateBatchingOptionsRequest to mRequestQueues[REQUEST_SESSION],
                // even if this update request will stop tracking and then start batching.
                mRequestQueues[REQUEST_SESSION].push(new UpdateBatchingOptionsRequest(*this));
                BatchingOptions batchOptions = {};
                batchOptions.size = sizeof(BatchingOptions);
                switch (sessionMode) {
                case SESSION_MODE_ON_FULL:
                    batchOptions.batchingMode = BATCHING_MODE_ROUTINE;
                    break;
                case SESSION_MODE_ON_TRIP_COMPLETED:
                    batchOptions.batchingMode = BATCHING_MODE_TRIP;
                    break;
                default:
                    batchOptions.batchingMode = BATCHING_MODE_NO_AUTO_REPORT;
                    break;
                }

                if (sMode == SESSION_MODE_ON_FIX) {
                    // stop tracking
                    // trackingSession will be removed from mSessionBiDict soon,
                    // so we don't need to add a new request to mRequestQueues[REQUEST_SESSION].
                    mLocationAPI->stopTracking(trackingSession);
                    trackingSession = 0;

                    // start batching
                    batchingSession = mLocationAPI->startBatching(options, batchOptions);
                    LOC_LOGI("%s:%d] start new session: %d",
                            __FUNCTION__, __LINE__, batchingSession);
                    mRequestQueues[REQUEST_SESSION].setSession(batchingSession);
                } else {
                    mLocationAPI->updateBatchingOptions(batchingSession, options, batchOptions);
                }

            }

            uint32_t session = ((sessionMode != SESSION_MODE_ON_FIX) ?
                    batchingSession : trackingSession);

            entity.trackingSession = trackingSession;
            entity.batchingSession = batchingSession;
            entity.sessionMode = sessionMode;
            // remove the old values from mSessionBiDict before we add a new one.
            mSessionBiDict.rmById(id);
            mSessionBiDict.set(id, session, entity);

            retVal = LOCATION_ERROR_SUCCESS;
        } else {
            retVal = LOCATION_ERROR_ID_UNKNOWN;
            LOC_LOGE("%s:%d] session %d is not exist.", __FUNCTION__, __LINE__, id);
        }
    }
    pthread_mutex_unlock(&mMutex);
    return retVal;
}

uint32_t LocationAPIClientBase::locAPIGetBatchedLocations(uint32_t id, size_t count)
{
    uint32_t retVal = LOCATION_ERROR_GENERAL_FAILURE;
    pthread_mutex_lock(&mMutex);
    if (mLocationAPI) {
        if (mSessionBiDict.hasId(id)) {
            SessionEntity entity = mSessionBiDict.getExtById(id);
            uint32_t batchingSession = entity.batchingSession;
            mRequestQueues[REQUEST_SESSION].push(new GetBatchedLocationsRequest(*this));
            mLocationAPI->getBatchedLocations(batchingSession, count);
            retVal = LOCATION_ERROR_SUCCESS;
        }  else {
            retVal = LOCATION_ERROR_ID_UNKNOWN;
            LOC_LOGE("%s:%d] invalid session: %d.", __FUNCTION__, __LINE__, id);
        }
    }
    pthread_mutex_unlock(&mMutex);

    return retVal;
}

uint32_t LocationAPIClientBase::locAPIAddGeofences(
        size_t count, uint32_t* ids, GeofenceOption* options, GeofenceInfo* data)
{
    uint32_t retVal = LOCATION_ERROR_GENERAL_FAILURE;
    pthread_mutex_lock(&mMutex);
    if (mLocationAPI) {
        if (mRequestQueues[REQUEST_GEOFENCE].getSession() != GEOFENCE_SESSION_ID) {
            mRequestQueues[REQUEST_GEOFENCE].reset(GEOFENCE_SESSION_ID);
        }
        uint32_t* sessions = mLocationAPI->addGeofences(count, options, data);
        if (sessions) {
            LOC_LOGI("%s:%d] start new sessions: %p", __FUNCTION__, __LINE__, sessions);
            mRequestQueues[REQUEST_GEOFENCE].push(new AddGeofencesRequest(*this));

            for (size_t i = 0; i < count; i++) {
                mGeofenceBiDict.set(ids[i], sessions[i], options[i].breachTypeMask);
            }
            retVal = LOCATION_ERROR_SUCCESS;
        }
    }
    pthread_mutex_unlock(&mMutex);

    return retVal;
}

void LocationAPIClientBase::locAPIRemoveGeofences(size_t count, uint32_t* ids)
{
    pthread_mutex_lock(&mMutex);
    if (mLocationAPI) {
        uint32_t* sessions = (uint32_t*)malloc(sizeof(uint32_t) * count);
        if (sessions == NULL) {
            LOC_LOGE("%s:%d] Failed to allocate %zu bytes !",
                    __FUNCTION__, __LINE__, sizeof(uint32_t) * count);
            pthread_mutex_unlock(&mMutex);
            return;
        }

        if (mRequestQueues[REQUEST_GEOFENCE].getSession() == GEOFENCE_SESSION_ID) {
            size_t j = 0;
            uint32_t id_cb;
            LocationError err;
            for (size_t i = 0; i < count; i++) {
                sessions[j] = mGeofenceBiDict.getSession(ids[i]);
                id_cb = ids[i];
                if (sessions[j] > 0) {
                    mGeofenceBiDict.rmBySession(sessions[j]);
                    err = LOCATION_ERROR_SUCCESS;
                    onRemoveGeofencesCb(1, &err, &id_cb);
                    j++;
                } else {
                    err = LOCATION_ERROR_ID_UNKNOWN;
                    onRemoveGeofencesCb(1, &err, &id_cb);
                }
            }

            if (j > 0) {
                mRequestQueues[REQUEST_GEOFENCE].push(new RemoveGeofencesRequest(*this));
                mLocationAPI->removeGeofences(j, sessions);
            }
        } else {
            LOC_LOGE("%s:%d] invalid session: %d.", __FUNCTION__, __LINE__,
                    mRequestQueues[REQUEST_GEOFENCE].getSession());
        }

        free(sessions);
    }
    pthread_mutex_unlock(&mMutex);
}

void LocationAPIClientBase::locAPIModifyGeofences(
        size_t count, uint32_t* ids, GeofenceOption* options)
{
    pthread_mutex_lock(&mMutex);
    if (mLocationAPI) {
        uint32_t* sessions = (uint32_t*)malloc(sizeof(uint32_t) * count);
        if (sessions == NULL) {
            LOC_LOGE("%s:%d] Failed to allocate %zu bytes !",
                    __FUNCTION__, __LINE__, sizeof(uint32_t) * count);
            pthread_mutex_unlock(&mMutex);
            return;
        }

        if (mRequestQueues[REQUEST_GEOFENCE].getSession() == GEOFENCE_SESSION_ID) {
            size_t j = 0;
            for (size_t i = 0; i < count; i++) {
                sessions[j] = mGeofenceBiDict.getSession(ids[i]);
                if (sessions[j] > 0) {
                    mGeofenceBiDict.set(ids[i], sessions[j], options[i].breachTypeMask);
                    j++;
                }
            }
            if (j > 0) {
                mRequestQueues[REQUEST_GEOFENCE].push(new ModifyGeofencesRequest(*this));
                mLocationAPI->modifyGeofences(j, sessions, options);
            }
        } else {
            LOC_LOGE("%s:%d] invalid session: %d.", __FUNCTION__, __LINE__,
                    mRequestQueues[REQUEST_GEOFENCE].getSession());
        }

        free(sessions);
    }
    pthread_mutex_unlock(&mMutex);
}

void LocationAPIClientBase::locAPIPauseGeofences(size_t count, uint32_t* ids)
{
    pthread_mutex_lock(&mMutex);
    if (mLocationAPI) {
        uint32_t* sessions = (uint32_t*)malloc(sizeof(uint32_t) * count);
        if (sessions == NULL) {
            LOC_LOGE("%s:%d] Failed to allocate %zu bytes !",
                    __FUNCTION__, __LINE__, sizeof(uint32_t) * count);
            pthread_mutex_unlock(&mMutex);
            return;
        }

        if (mRequestQueues[REQUEST_GEOFENCE].getSession() == GEOFENCE_SESSION_ID) {
            size_t j = 0;
            for (size_t i = 0; i < count; i++) {
                sessions[j] = mGeofenceBiDict.getSession(ids[i]);
                if (sessions[j] > 0) {
                    j++;
                }
            }
            if (j > 0) {
                mRequestQueues[REQUEST_GEOFENCE].push(new PauseGeofencesRequest(*this));
                mLocationAPI->pauseGeofences(j, sessions);
            }
        } else {
            LOC_LOGE("%s:%d] invalid session: %d.", __FUNCTION__, __LINE__,
                    mRequestQueues[REQUEST_GEOFENCE].getSession());
        }

        free(sessions);
    }
    pthread_mutex_unlock(&mMutex);
}

void LocationAPIClientBase::locAPIResumeGeofences(
        size_t count, uint32_t* ids, GeofenceBreachTypeMask* mask)
{
    pthread_mutex_lock(&mMutex);
    if (mLocationAPI) {
        uint32_t* sessions = (uint32_t*)malloc(sizeof(uint32_t) * count);
        if (sessions == NULL) {
            LOC_LOGE("%s:%d] Failed to allocate %zu bytes !",
                    __FUNCTION__, __LINE__, sizeof(uint32_t) * count);
            pthread_mutex_unlock(&mMutex);
            return;
        }

        if (mRequestQueues[REQUEST_GEOFENCE].getSession() == GEOFENCE_SESSION_ID) {
            size_t j = 0;
            for (size_t i = 0; i < count; i++) {
                sessions[j] = mGeofenceBiDict.getSession(ids[i]);
                if (sessions[j] > 0) {
                    if (mask) {
                        mGeofenceBiDict.set(ids[i], sessions[j], mask[i]);
                    }
                    j++;
                }
            }
            if (j > 0) {
                mRequestQueues[REQUEST_GEOFENCE].push(new ResumeGeofencesRequest(*this));
                mLocationAPI->resumeGeofences(j, sessions);
            }
        } else {
            LOC_LOGE("%s:%d] invalid session: %d.", __FUNCTION__, __LINE__,
                    mRequestQueues[REQUEST_GEOFENCE].getSession());
        }

        free(sessions);
    }
    pthread_mutex_unlock(&mMutex);
}

void LocationAPIClientBase::locAPIRemoveAllGeofences()
{
    std::vector<uint32_t> sessionsVec = mGeofenceBiDict.getAllSessions();
    locAPIRemoveGeofences(sessionsVec.size(), &sessionsVec[0]);
}

void LocationAPIClientBase::locAPIGnssNiResponse(uint32_t id, GnssNiResponse response)
{
    pthread_mutex_lock(&mMutex);
    if (mLocationAPI) {
        uint32_t session = id;
        mLocationAPI->gnssNiResponse(id, response);
        LOC_LOGI("%s:%d] start new session: %d", __FUNCTION__, __LINE__, session);
        mRequestQueues[REQUEST_NIRESPONSE].reset(session);
        mRequestQueues[REQUEST_NIRESPONSE].push(new GnssNiResponseRequest(*this));
    }
    pthread_mutex_unlock(&mMutex);
}

void LocationAPIClientBase::beforeGeofenceBreachCb(
        GeofenceBreachNotification geofenceBreachNotification)
{
    uint32_t* ids = (uint32_t*)malloc(sizeof(uint32_t) * geofenceBreachNotification.count);
    uint32_t* backup = geofenceBreachNotification.ids;
    size_t n = geofenceBreachNotification.count;
    geofenceBreachCallback genfenceCallback = nullptr;

    if (ids == NULL) {
        LOC_LOGE("%s:%d] Failed to alloc %zu bytes",
                __FUNCTION__, __LINE__,
                sizeof(uint32_t) * geofenceBreachNotification.count);
        return;
    }

    pthread_mutex_lock(&mMutex);
    if (mGeofenceBreachCallback != nullptr) {
        size_t count = 0;
        for (size_t i = 0; i < n; i++) {
            uint32_t id = mGeofenceBiDict.getId(geofenceBreachNotification.ids[i]);
            GeofenceBreachTypeMask type =
                mGeofenceBiDict.getExtBySession(geofenceBreachNotification.ids[i]);
            // if type == 0, we will not head into the fllowing block anyway.
            // so we don't need to check id and type
            if ((geofenceBreachNotification.type == GEOFENCE_BREACH_ENTER &&
                        (type & GEOFENCE_BREACH_ENTER_BIT)) ||
                    (geofenceBreachNotification.type == GEOFENCE_BREACH_EXIT &&
                     (type & GEOFENCE_BREACH_EXIT_BIT))
               ) {
                ids[count] = id;
                count++;
            }
        }
        geofenceBreachNotification.count = count;
        geofenceBreachNotification.ids = ids;

        genfenceCallback = mGeofenceBreachCallback;
    }
    pthread_mutex_unlock(&mMutex);

    if (genfenceCallback != nullptr) {
        genfenceCallback(geofenceBreachNotification);
    }

    // restore ids
    geofenceBreachNotification.ids = backup;
    geofenceBreachNotification.count = n;
    free(ids);
}

void LocationAPIClientBase::beforeBatchingStatusCb(BatchingStatusInfo batchStatus,
        std::list<uint32_t> & tripCompletedList) {

    // map the trip ids to the client ids
    std::list<uint32_t> tripCompletedClientIdList;
    tripCompletedClientIdList.clear();

    if (batchStatus.batchingStatus == BATCHING_STATUS_TRIP_COMPLETED) {
        for (auto itt = tripCompletedList.begin(); itt != tripCompletedList.end(); itt++) {
            if (mSessionBiDict.hasSession(*itt)) {
                SessionEntity sessEntity = mSessionBiDict.getExtBySession(*itt);

                if (sessEntity.sessionMode == SESSION_MODE_ON_TRIP_COMPLETED) {
                    tripCompletedClientIdList.push_back(sessEntity.id);
                    mSessionBiDict.rmBySession(*itt);
                }
            }
        }
    }

    mBatchingStatusCallback(batchStatus, tripCompletedClientIdList);
}

void LocationAPIClientBase::onResponseCb(LocationError error, uint32_t id)
{
    if (error != LOCATION_ERROR_SUCCESS) {
        LOC_LOGE("%s:%d] ERROR: %d ID: %d", __FUNCTION__, __LINE__, error, id);
    } else {
        LOC_LOGV("%s:%d] SUCCESS: %d id: %d", __FUNCTION__, __LINE__, error, id);
    }
    LocationAPIRequest* request = getRequestBySession(id);
    if (request) {
        request->onResponse(error, id);
        delete request;
    }
}

void LocationAPIClientBase::onCollectiveResponseCb(
        size_t count, LocationError* errors, uint32_t* ids)
{
    for (size_t i = 0; i < count; i++) {
        if (errors[i] != LOCATION_ERROR_SUCCESS) {
            LOC_LOGE("%s:%d] ERROR: %d ID: %d", __FUNCTION__, __LINE__, errors[i], ids[i]);
        } else {
            LOC_LOGV("%s:%d] SUCCESS: %d id: %d", __FUNCTION__, __LINE__, errors[i], ids[i]);
        }
    }
    LocationAPIRequest* request = nullptr;
    pthread_mutex_lock(&mMutex);
    if (mRequestQueues[REQUEST_GEOFENCE].getSession() == GEOFENCE_SESSION_ID) {
        request = mRequestQueues[REQUEST_GEOFENCE].pop();
    }
    pthread_mutex_unlock(&mMutex);
    if (request) {
        request->onCollectiveResponse(count, errors, ids);
        delete request;
    }
}

void LocationAPIClientBase::removeSession(uint32_t session) {
    if (mSessionBiDict.hasSession(session)) {
        mSessionBiDict.rmBySession(session);
    }
}

LocationAPIRequest* LocationAPIClientBase::getRequestBySession(uint32_t session)
{
    pthread_mutex_lock(&mMutex);
    LocationAPIRequest* request = nullptr;
    for (int i = 0; i < REQUEST_MAX; i++) {
        if (i != REQUEST_GEOFENCE &&
                i != REQUEST_SESSION &&
                mRequestQueues[i].getSession() == session) {
            request = mRequestQueues[i].pop();
            break;
        }
    }
    if (request == nullptr) {
        // Can't find a request with correct session,
        // try to find it from mSessionBiDict
        if (mSessionBiDict.hasSession(session)) {
            request = mRequestQueues[REQUEST_SESSION].pop();
        }
    }
    pthread_mutex_unlock(&mMutex);
    return request;
}
