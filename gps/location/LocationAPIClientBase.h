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

#ifndef LOCATION_API_CLINET_BASE_H
#define LOCATION_API_CLINET_BASE_H

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <queue>
#include <map>

#include "LocationAPI.h"

enum SESSION_MODE {
    SESSION_MODE_NONE = 0,
    SESSION_MODE_ON_FULL,
    SESSION_MODE_ON_FIX,
    SESSION_MODE_ON_TRIP_COMPLETED
};

enum REQUEST_TYPE {
    REQUEST_TRACKING = 0,
    REQUEST_SESSION,
    REQUEST_GEOFENCE,
    REQUEST_NIRESPONSE,
    REQUEST_MAX,
};

enum CTRL_REQUEST_TYPE {
    CTRL_REQUEST_DELETEAIDINGDATA = 0,
    CTRL_REQUEST_CONTROL,
    CTRL_REQUEST_CONFIG,
    CTRL_REQUEST_MAX,
};

class LocationAPIClientBase;

class LocationAPIRequest {
public:
    LocationAPIRequest() {}
    virtual ~LocationAPIRequest() {}
    virtual void onResponse(LocationError /*error*/, uint32_t /*id*/) {}
    virtual void onCollectiveResponse(
            size_t /*count*/, LocationError* /*errors*/, uint32_t* /*ids*/) {}
};

class RequestQueue {
public:
    RequestQueue(): mSession(0) {
    }
    virtual ~RequestQueue() {
        reset(0);
    }
    void inline setSession(uint32_t session) { mSession = session; }
    void reset(uint32_t session) {
        LocationAPIRequest* request = nullptr;
        while (!mQueue.empty()) {
            request = mQueue.front();
            mQueue.pop();
            delete request;
        }
        mSession = session;
    }
    void push(LocationAPIRequest* request) {
        mQueue.push(request);
    }
    LocationAPIRequest* pop() {
        LocationAPIRequest* request = nullptr;
        if (!mQueue.empty()) {
            request = mQueue.front();
            mQueue.pop();
        }
        return request;
    }
    uint32_t getSession() { return mSession; }
private:
    uint32_t mSession;
    std::queue<LocationAPIRequest*> mQueue;
};

class LocationAPIControlClient {
public:
    LocationAPIControlClient();
    virtual ~LocationAPIControlClient();
    LocationAPIControlClient(const LocationAPIControlClient&) = delete;
    LocationAPIControlClient& operator=(const LocationAPIControlClient&) = delete;

    LocationAPIRequest* getRequestBySession(uint32_t session);

    // LocationControlAPI
    uint32_t locAPIGnssDeleteAidingData(GnssAidingData& data);
    uint32_t locAPIEnable(LocationTechnologyType techType);
    void locAPIDisable();
    uint32_t locAPIGnssUpdateConfig(GnssConfig config);

    // callbacks
    void onCtrlResponseCb(LocationError error, uint32_t id);
    void onCtrlCollectiveResponseCb(size_t count, LocationError* errors, uint32_t* ids);

    inline virtual void onGnssDeleteAidingDataCb(LocationError /*error*/) {}
    inline virtual void onEnableCb(LocationError /*error*/) {}
    inline virtual void onDisableCb(LocationError /*error*/) {}
    inline virtual void onGnssUpdateConfigCb(
            size_t /*count*/, LocationError* /*errors*/, uint32_t* /*ids*/) {}

    class GnssDeleteAidingDataRequest : public LocationAPIRequest {
    public:
        GnssDeleteAidingDataRequest(LocationAPIControlClient& API) : mAPI(API) {}
        inline void onResponse(LocationError error, uint32_t /*id*/) {
            mAPI.onGnssDeleteAidingDataCb(error);
        }
        LocationAPIControlClient& mAPI;
    };

    class EnableRequest : public LocationAPIRequest {
    public:
        EnableRequest(LocationAPIControlClient& API) : mAPI(API) {}
        inline void onResponse(LocationError error, uint32_t /*id*/) {
            mAPI.onEnableCb(error);
        }
        LocationAPIControlClient& mAPI;
    };

    class DisableRequest : public LocationAPIRequest {
    public:
        DisableRequest(LocationAPIControlClient& API) : mAPI(API) {}
        inline void onResponse(LocationError error, uint32_t /*id*/) {
            mAPI.onDisableCb(error);
        }
        LocationAPIControlClient& mAPI;
    };

    class GnssUpdateConfigRequest : public LocationAPIRequest {
    public:
        GnssUpdateConfigRequest(LocationAPIControlClient& API) : mAPI(API) {}
        inline void onCollectiveResponse(size_t count, LocationError* errors, uint32_t* ids) {
            mAPI.onGnssUpdateConfigCb(count, errors, ids);
        }
        LocationAPIControlClient& mAPI;
    };

private:
    pthread_mutex_t mMutex;
    LocationControlAPI* mLocationControlAPI;
    RequestQueue mRequestQueues[CTRL_REQUEST_MAX];
    bool mEnabled;
    GnssConfig mConfig;
};

class LocationAPIClientBase {
public:
    LocationAPIClientBase();
    virtual ~LocationAPIClientBase();
    LocationAPIClientBase(const LocationAPIClientBase&) = delete;
    LocationAPIClientBase& operator=(const LocationAPIClientBase&) = delete;

    void locAPISetCallbacks(LocationCallbacks& locationCallbacks);
    void removeSession(uint32_t session);
    LocationAPIRequest* getRequestBySession(uint32_t session);

    // LocationAPI
    uint32_t locAPIStartTracking(LocationOptions& options);
    void locAPIStopTracking();
    void locAPIUpdateTrackingOptions(LocationOptions& options);

    int32_t locAPIGetBatchSize();
    uint32_t locAPIStartSession(uint32_t id, uint32_t sessionMode,
            LocationOptions& options);
    uint32_t locAPIStopSession(uint32_t id);
    uint32_t locAPIUpdateSessionOptions(uint32_t id, uint32_t sessionMode,
            LocationOptions& options);
    uint32_t locAPIGetBatchedLocations(uint32_t id, size_t count);

    uint32_t locAPIAddGeofences(size_t count, uint32_t* ids,
            GeofenceOption* options, GeofenceInfo* data);
    void locAPIRemoveGeofences(size_t count, uint32_t* ids);
    void locAPIModifyGeofences(size_t count, uint32_t* ids, GeofenceOption* options);
    void locAPIPauseGeofences(size_t count, uint32_t* ids);
    void locAPIResumeGeofences(size_t count, uint32_t* ids, GeofenceBreachTypeMask* mask);
    void locAPIRemoveAllGeofences();

    void locAPIGnssNiResponse(uint32_t id, GnssNiResponse response);

    // callbacks
    void onResponseCb(LocationError error, uint32_t id);
    void onCollectiveResponseCb(size_t count, LocationError* errors, uint32_t* ids);

    void beforeGeofenceBreachCb(GeofenceBreachNotification geofenceBreachNotification);

    inline virtual void onCapabilitiesCb(LocationCapabilitiesMask /*capabilitiesMask*/) {}
    inline virtual void onGnssNmeaCb(GnssNmeaNotification /*gnssNmeaNotification*/) {}
    inline virtual void onGnssMeasurementsCb(
            GnssMeasurementsNotification /*gnssMeasurementsNotification*/) {}

    inline virtual void onTrackingCb(Location /*location*/) {}
    inline virtual void onGnssSvCb(GnssSvNotification /*gnssSvNotification*/) {}
    inline virtual void onStartTrackingCb(LocationError /*error*/) {}
    inline virtual void onStopTrackingCb(LocationError /*error*/) {}
    inline virtual void onUpdateTrackingOptionsCb(LocationError /*error*/) {}

    inline virtual void onGnssLocationInfoCb(
            GnssLocationInfoNotification /*gnssLocationInfoNotification*/) {}

    inline virtual void onBatchingCb(size_t /*count*/, Location* /*location*/,
            BatchingOptions /*batchingOptions*/) {}
    inline virtual void onBatchingStatusCb(BatchingStatusInfo /*batchingStatus*/,
            std::list<uint32_t> &/*listOfCompletedTrips*/) {}
    void beforeBatchingStatusCb(BatchingStatusInfo batchStatus,
            std::list<uint32_t> & tripCompletedList);
    inline virtual void onStartBatchingCb(LocationError /*error*/) {}
    inline virtual void onStopBatchingCb(LocationError /*error*/) {}
    inline virtual void onUpdateBatchingOptionsCb(LocationError /*error*/) {}
    inline virtual void onGetBatchedLocationsCb(LocationError /*error*/) {}

    inline virtual void onGeofenceBreachCb(
            GeofenceBreachNotification /*geofenceBreachNotification*/) {}
    inline virtual void onGeofenceStatusCb(
            GeofenceStatusNotification /*geofenceStatusNotification*/) {}
    inline virtual void onAddGeofencesCb(
            size_t /*count*/, LocationError* /*errors*/, uint32_t* /*ids*/) {}
    inline virtual void onRemoveGeofencesCb(
            size_t /*count*/, LocationError* /*errors*/, uint32_t* /*ids*/) {}
    inline virtual void onModifyGeofencesCb(
            size_t /*count*/, LocationError* /*errors*/, uint32_t* /*ids*/) {}
    inline virtual void onPauseGeofencesCb(
            size_t /*count*/, LocationError* /*errors*/, uint32_t* /*ids*/) {}
    inline virtual void onResumeGeofencesCb(
            size_t /*count*/, LocationError* /*errors*/, uint32_t* /*ids*/) {}

    inline virtual void onGnssNiCb(uint32_t /*id*/, GnssNiNotification /*gnssNiNotification*/) {}
    inline virtual void onGnssNiResponseCb(LocationError /*error*/) {}

private:
    // private inner classes
    typedef struct {
        uint32_t id;
        uint32_t trackingSession;
        uint32_t batchingSession;
        uint32_t sessionMode;
    } SessionEntity;

    template<typename T>
    class BiDict {
    public:
        BiDict() {
            pthread_mutex_init(&mBiDictMutex, nullptr);
        }
        virtual ~BiDict() {
            pthread_mutex_destroy(&mBiDictMutex);
        }
        bool hasId(uint32_t id) {
            pthread_mutex_lock(&mBiDictMutex);
            bool ret = (mForwardMap.find(id) != mForwardMap.end());
            pthread_mutex_unlock(&mBiDictMutex);
            return ret;
        }
        bool hasSession(uint32_t session) {
            pthread_mutex_lock(&mBiDictMutex);
            bool ret = (mBackwardMap.find(session) != mBackwardMap.end());
            pthread_mutex_unlock(&mBiDictMutex);
            return ret;
        }
        void set(uint32_t id, uint32_t session, T& ext) {
            pthread_mutex_lock(&mBiDictMutex);
            mForwardMap[id] = session;
            mBackwardMap[session] = id;
            mExtMap[session] = ext;
            pthread_mutex_unlock(&mBiDictMutex);
        }
        void clear() {
            pthread_mutex_lock(&mBiDictMutex);
            mForwardMap.clear();
            mBackwardMap.clear();
            mExtMap.clear();
            pthread_mutex_unlock(&mBiDictMutex);
        }
        void rmById(uint32_t id) {
            pthread_mutex_lock(&mBiDictMutex);
            mBackwardMap.erase(mForwardMap[id]);
            mExtMap.erase(mForwardMap[id]);
            mForwardMap.erase(id);
            pthread_mutex_unlock(&mBiDictMutex);
        }
        void rmBySession(uint32_t session) {
            pthread_mutex_lock(&mBiDictMutex);
            mForwardMap.erase(mBackwardMap[session]);
            mBackwardMap.erase(session);
            mExtMap.erase(session);
            pthread_mutex_unlock(&mBiDictMutex);
        }
        uint32_t getId(uint32_t session) {
            pthread_mutex_lock(&mBiDictMutex);
            uint32_t ret = 0;
            auto it = mBackwardMap.find(session);
            if (it != mBackwardMap.end()) {
                ret = it->second;
            }
            pthread_mutex_unlock(&mBiDictMutex);
            return ret;
        }
        uint32_t getSession(uint32_t id) {
            pthread_mutex_lock(&mBiDictMutex);
            uint32_t ret = 0;
            auto it = mForwardMap.find(id);
            if (it != mForwardMap.end()) {
                ret = it->second;
            }
            pthread_mutex_unlock(&mBiDictMutex);
            return ret;
        }
        T getExtById(uint32_t id) {
            pthread_mutex_lock(&mBiDictMutex);
            T ret;
            memset(&ret, 0, sizeof(T));
            uint32_t session = mForwardMap[id];
            if (session > 0) {
                auto it = mExtMap.find(session);
                if (it != mExtMap.end()) {
                    ret = it->second;
                }
            }
            pthread_mutex_unlock(&mBiDictMutex);
            return ret;
        }
        T getExtBySession(uint32_t session) {
            pthread_mutex_lock(&mBiDictMutex);
            T ret;
            memset(&ret, 0, sizeof(T));
            auto it = mExtMap.find(session);
            if (it != mExtMap.end()) {
                ret = it->second;
            }
            pthread_mutex_unlock(&mBiDictMutex);
            return ret;
        }
        std::vector<uint32_t> getAllSessions() {
            std::vector<uint32_t> ret;
            pthread_mutex_lock(&mBiDictMutex);
            for (auto it = mBackwardMap.begin(); it != mBackwardMap.end(); it++) {
                ret.push_back(it->first);
            }
            pthread_mutex_unlock(&mBiDictMutex);
            return ret;
        }
    private:
        pthread_mutex_t mBiDictMutex;
        // mForwarMap mapping id->session
        std::map<uint32_t, uint32_t> mForwardMap;
        // mBackwardMap mapping session->id
        std::map<uint32_t, uint32_t> mBackwardMap;
        // mExtMap mapping session->ext
        std::map<uint32_t, T> mExtMap;
    };

    class StartTrackingRequest : public LocationAPIRequest {
    public:
        StartTrackingRequest(LocationAPIClientBase& API) : mAPI(API) {}
        inline void onResponse(LocationError error, uint32_t id) {
            if (error != LOCATION_ERROR_SUCCESS) {
                mAPI.removeSession(id);
            }
            mAPI.onStartTrackingCb(error);
        }
        LocationAPIClientBase& mAPI;
    };

    class StopTrackingRequest : public LocationAPIRequest {
    public:
        StopTrackingRequest(LocationAPIClientBase& API) : mAPI(API) {}
        inline void onResponse(LocationError error, uint32_t id) {
            mAPI.onStopTrackingCb(error);
            if (error == LOCATION_ERROR_SUCCESS) {
                mAPI.removeSession(id);
            }
        }
        LocationAPIClientBase& mAPI;
    };

    class UpdateTrackingOptionsRequest : public LocationAPIRequest {
    public:
        UpdateTrackingOptionsRequest(LocationAPIClientBase& API) : mAPI(API) {}
        inline void onResponse(LocationError error, uint32_t /*id*/) {
            mAPI.onUpdateTrackingOptionsCb(error);
        }
        LocationAPIClientBase& mAPI;
    };

    class StartBatchingRequest : public LocationAPIRequest {
    public:
        StartBatchingRequest(LocationAPIClientBase& API) : mAPI(API) {}
        inline void onResponse(LocationError error, uint32_t id) {
            if (error != LOCATION_ERROR_SUCCESS) {
                mAPI.removeSession(id);
            }
            mAPI.onStartBatchingCb(error);
        }
        LocationAPIClientBase& mAPI;
    };

    class StopBatchingRequest : public LocationAPIRequest {
    public:
        StopBatchingRequest(LocationAPIClientBase& API) : mAPI(API) {}
        inline void onResponse(LocationError error, uint32_t id) {
            mAPI.onStopBatchingCb(error);
            if (error == LOCATION_ERROR_SUCCESS) {
                mAPI.removeSession(id);
            }
        }
        LocationAPIClientBase& mAPI;
    };

    class UpdateBatchingOptionsRequest : public LocationAPIRequest {
    public:
        UpdateBatchingOptionsRequest(LocationAPIClientBase& API) : mAPI(API) {}
        inline void onResponse(LocationError error, uint32_t /*id*/) {
            mAPI.onUpdateBatchingOptionsCb(error);
        }
        LocationAPIClientBase& mAPI;
    };

    class GetBatchedLocationsRequest : public LocationAPIRequest {
    public:
        GetBatchedLocationsRequest(LocationAPIClientBase& API) : mAPI(API) {}
        inline void onResponse(LocationError error, uint32_t /*id*/) {
            mAPI.onGetBatchedLocationsCb(error);
        }
        LocationAPIClientBase& mAPI;
    };

    class AddGeofencesRequest : public LocationAPIRequest {
    public:
        AddGeofencesRequest(LocationAPIClientBase& API) : mAPI(API) {}
        inline void onCollectiveResponse(size_t count, LocationError* errors, uint32_t* sessions) {
            uint32_t *ids = (uint32_t*)malloc(sizeof(uint32_t) * count);
            for (size_t i = 0; i < count; i++) {
                ids[i] = mAPI.mGeofenceBiDict.getId(sessions[i]);
            }
            mAPI.onAddGeofencesCb(count, errors, ids);
            free(ids);
        }
        LocationAPIClientBase& mAPI;
    };

    class RemoveGeofencesRequest : public LocationAPIRequest {
    public:
        RemoveGeofencesRequest(LocationAPIClientBase& API) : mAPI(API) {}
        inline void onCollectiveResponse(size_t count, LocationError* errors, uint32_t* sessions) {
            // No need to handle collectiveResponse, cbs already notified
        }
        LocationAPIClientBase& mAPI;
    };

    class ModifyGeofencesRequest : public LocationAPIRequest {
    public:
        ModifyGeofencesRequest(LocationAPIClientBase& API) : mAPI(API) {}
        inline void onCollectiveResponse(size_t count, LocationError* errors, uint32_t* sessions) {
            uint32_t *ids = (uint32_t*)malloc(sizeof(uint32_t) * count);
            for (size_t i = 0; i < count; i++) {
                ids[i] = mAPI.mGeofenceBiDict.getId(sessions[i]);
            }
            mAPI.onModifyGeofencesCb(count, errors, ids);
            free(ids);
        }
        LocationAPIClientBase& mAPI;
    };

    class PauseGeofencesRequest : public LocationAPIRequest {
    public:
        PauseGeofencesRequest(LocationAPIClientBase& API) : mAPI(API) {}
        inline void onCollectiveResponse(size_t count, LocationError* errors, uint32_t* sessions) {
            uint32_t *ids = (uint32_t*)malloc(sizeof(uint32_t) * count);
            for (size_t i = 0; i < count; i++) {
                ids[i] = mAPI.mGeofenceBiDict.getId(sessions[i]);
            }
            mAPI.onPauseGeofencesCb(count, errors, ids);
            free(ids);
        }
        LocationAPIClientBase& mAPI;
    };

    class ResumeGeofencesRequest : public LocationAPIRequest {
    public:
        ResumeGeofencesRequest(LocationAPIClientBase& API) : mAPI(API) {}
        inline void onCollectiveResponse(size_t count, LocationError* errors, uint32_t* sessions) {
            uint32_t *ids = (uint32_t*)malloc(sizeof(uint32_t) * count);
            for (size_t i = 0; i < count; i++) {
                ids[i] = mAPI.mGeofenceBiDict.getId(sessions[i]);
            }
            mAPI.onResumeGeofencesCb(count, errors, ids);
            free(ids);
        }
        LocationAPIClientBase& mAPI;
    };

    class GnssNiResponseRequest : public LocationAPIRequest {
    public:
        GnssNiResponseRequest(LocationAPIClientBase& API) : mAPI(API) {}
        inline void onResponse(LocationError error, uint32_t /*id*/) {
            mAPI.onGnssNiResponseCb(error);
        }
        LocationAPIClientBase& mAPI;
    };

private:
    pthread_mutex_t mMutex;

    geofenceBreachCallback mGeofenceBreachCallback;
    batchingStatusCallback mBatchingStatusCallback;

    LocationAPI* mLocationAPI;

    RequestQueue mRequestQueues[REQUEST_MAX];
    BiDict<GeofenceBreachTypeMask> mGeofenceBiDict;
    BiDict<SessionEntity> mSessionBiDict;
    int32_t mBatchSize;
    bool mTracking;
};

#endif /* LOCATION_API_CLINET_BASE_H */
