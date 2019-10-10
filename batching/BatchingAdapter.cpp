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
#define LOG_TAG "LocSvc_BatchingAdapter"

#include <loc_pla.h>
#include <log_util.h>
#include <LocContext.h>
#include <BatchingAdapter.h>

using namespace loc_core;

BatchingAdapter::BatchingAdapter() :
    LocAdapterBase(0,
                    LocContext::getLocContext(
                        NULL,
                        NULL,
                        LocContext::mLocationHalName,
                        false)),
    mOngoingTripDistance(0),
    mOngoingTripTBFInterval(0),
    mTripWithOngoingTBFDropped(false),
    mTripWithOngoingTripDistanceDropped(false),
    mBatchingTimeout(0),
    mBatchingAccuracy(1),
    mBatchSize(0),
    mTripBatchSize(0)
{
    LOC_LOGD("%s]: Constructor", __func__);
    readConfigCommand();
    setConfigCommand();
}

void
BatchingAdapter::readConfigCommand()
{
    LOC_LOGD("%s]: ", __func__);

    struct MsgReadConfig : public LocMsg {
        BatchingAdapter& mAdapter;
        inline MsgReadConfig(BatchingAdapter& adapter) :
            LocMsg(),
            mAdapter(adapter) {}
        inline virtual void proc() const {
            uint32_t batchingTimeout = 0;
            uint32_t batchingAccuracy = 0;
            uint32_t batchSize = 0;
            uint32_t tripBatchSize = 0;
            static const loc_param_s_type flp_conf_param_table[] =
            {
                {"BATCH_SIZE", &batchSize, NULL, 'n'},
                {"OUTDOOR_TRIP_BATCH_SIZE", &tripBatchSize, NULL, 'n'},
                {"BATCH_SESSION_TIMEOUT", &batchingTimeout, NULL, 'n'},
                {"ACCURACY", &batchingAccuracy, NULL, 'n'},
            };
            UTIL_READ_CONF(LOC_PATH_FLP_CONF, flp_conf_param_table);

            LOC_LOGD("%s]: batchSize %u tripBatchSize %u batchingAccuracy %u batchingTimeout %u ",
                     __func__, batchSize, tripBatchSize, batchingAccuracy, batchingTimeout);

             mAdapter.setBatchSize(batchSize);
             mAdapter.setTripBatchSize(tripBatchSize);
             mAdapter.setBatchingTimeout(batchingTimeout);
             mAdapter.setBatchingAccuracy(batchingAccuracy);
        }
    };

    sendMsg(new MsgReadConfig(*this));

}

void
BatchingAdapter::setConfigCommand()
{
    LOC_LOGD("%s]: ", __func__);

    struct MsgSetConfig : public LocMsg {
        BatchingAdapter& mAdapter;
        LocApiBase& mApi;
        inline MsgSetConfig(BatchingAdapter& adapter,
                            LocApiBase& api) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api) {}
        inline virtual void proc() const {
            mApi.setBatchSize(mAdapter.getBatchSize());
            mApi.setTripBatchSize(mAdapter.getTripBatchSize());
        }
    };

    sendMsg(new MsgSetConfig(*this, *mLocApi));
}

void
BatchingAdapter::stopClientSessions(LocationAPI* client)
{
    LOC_LOGD("%s]: client %p", __func__, client);

    typedef struct pairKeyBatchMode {
        LocationAPI* client;
        uint32_t id;
        BatchingMode batchingMode;
        inline pairKeyBatchMode(LocationAPI* _client, uint32_t _id, BatchingMode _bMode) :
            client(_client), id(_id), batchingMode(_bMode) {}
    } pairKeyBatchMode;
    std::vector<pairKeyBatchMode> vBatchingClient;
    for (auto it : mBatchingSessions) {
        if (client == it.first.client) {
            vBatchingClient.emplace_back(it.first.client, it.first.id, it.second.batchingMode);
        }
    }
    for (auto keyBatchingMode : vBatchingClient) {
        if (keyBatchingMode.batchingMode != BATCHING_MODE_TRIP) {
            stopBatching(keyBatchingMode.client, keyBatchingMode.id);
        } else {
            stopTripBatchingMultiplex(keyBatchingMode.client, keyBatchingMode.id);
        }
    }
}

void
BatchingAdapter::updateClientsEventMask()
{
    LOC_API_ADAPTER_EVENT_MASK_T mask = 0;
    for (auto it=mClientData.begin(); it != mClientData.end(); ++it) {
        // we don't register LOC_API_ADAPTER_BIT_BATCH_FULL until we
        // start batching with ROUTINE or TRIP option
        if (it->second.batchingCb != nullptr) {
            mask |= LOC_API_ADAPTER_BIT_BATCH_STATUS;
        }
    }
    if (autoReportBatchingSessionsCount() > 0) {
        mask |= LOC_API_ADAPTER_BIT_BATCH_FULL;
    }
    updateEvtMask(mask, LOC_REGISTRATION_MASK_SET);
}

void
BatchingAdapter::handleEngineUpEvent()
{
    struct MsgSSREvent : public LocMsg {
        BatchingAdapter& mAdapter;
        LocApiBase& mApi;
        inline MsgSSREvent(BatchingAdapter& adapter,
                           LocApiBase& api) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api) {}
        virtual void proc() const {
            mAdapter.setEngineCapabilitiesKnown(true);
            mAdapter.broadcastCapabilities(mAdapter.getCapabilities());
            mApi.setBatchSize(mAdapter.getBatchSize());
            mApi.setTripBatchSize(mAdapter.getTripBatchSize());
            mAdapter.restartSessions();
            for (auto msg: mAdapter.mPendingMsgs) {
                mAdapter.sendMsg(msg);
            }
            mAdapter.mPendingMsgs.clear();
        }
    };

    sendMsg(new MsgSSREvent(*this, *mLocApi));
}

void
BatchingAdapter::restartSessions()
{
    LOC_LOGD("%s]: ", __func__);

    if (autoReportBatchingSessionsCount() > 0) {
        updateEvtMask(LOC_API_ADAPTER_BIT_BATCH_FULL,
                      LOC_REGISTRATION_MASK_ENABLED);
    }
    for (auto it = mBatchingSessions.begin();
              it != mBatchingSessions.end(); ++it) {
        if (it->second.batchingMode != BATCHING_MODE_TRIP) {
            mLocApi->startBatching(it->first.id, it->second,
                                    getBatchingAccuracy(), getBatchingTimeout(),
                                    new LocApiResponse(*getContext(),
                                    [] (LocationError /*err*/) {}));
        }
    }

    if (mTripSessions.size() > 0) {
        // restart outdoor trip batching session if any.
        mOngoingTripDistance = 0;
        mOngoingTripTBFInterval = 0;

        // record the min trip distance and min tbf interval of all ongoing sessions
        for (auto tripSession : mTripSessions) {

            TripSessionStatus &tripSessStatus = tripSession.second;

            if ((0 == mOngoingTripDistance) ||
                (mOngoingTripDistance >
                 (tripSessStatus.tripDistance - tripSessStatus.accumulatedDistanceThisTrip))) {
                mOngoingTripDistance = tripSessStatus.tripDistance -
                    tripSessStatus.accumulatedDistanceThisTrip;
            }

            if ((0 == mOngoingTripTBFInterval) ||
                (mOngoingTripTBFInterval > tripSessStatus.tripTBFInterval)) {
                mOngoingTripTBFInterval = tripSessStatus.tripTBFInterval;
            }

            // reset the accumulatedDistanceOngoingBatch for each session
            tripSessStatus.accumulatedDistanceOngoingBatch = 0;

        }

        mLocApi->startOutdoorTripBatching(mOngoingTripDistance, mOngoingTripTBFInterval,
                getBatchingTimeout(), new LocApiResponse(*getContext(), [this] (LocationError err) {
            if (LOCATION_ERROR_SUCCESS != err) {
                mOngoingTripDistance = 0;
                mOngoingTripTBFInterval = 0;
            }
            printTripReport();
        }));
    }
}

bool
BatchingAdapter::hasBatchingCallback(LocationAPI* client)
{
    auto it = mClientData.find(client);
    return (it != mClientData.end() && it->second.batchingCb);
}

bool
BatchingAdapter::isBatchingSession(LocationAPI* client, uint32_t sessionId)
{
    LocationSessionKey key(client, sessionId);
    return (mBatchingSessions.find(key) != mBatchingSessions.end());
}

bool
BatchingAdapter::isTripSession(uint32_t sessionId) {
    return (mTripSessions.find(sessionId) != mTripSessions.end());
}

void
BatchingAdapter::saveBatchingSession(LocationAPI* client, uint32_t sessionId,
        const BatchingOptions& batchingOptions)
{
    LocationSessionKey key(client, sessionId);
    mBatchingSessions[key] = batchingOptions;
}

void
BatchingAdapter::eraseBatchingSession(LocationAPI* client, uint32_t sessionId)
{
    LocationSessionKey key(client, sessionId);
    auto it = mBatchingSessions.find(key);
    if (it != mBatchingSessions.end()) {
        mBatchingSessions.erase(it);
    }
}

void
BatchingAdapter::reportResponse(LocationAPI* client, LocationError err, uint32_t sessionId)
{
    LOC_LOGD("%s]: client %p id %u err %u", __func__, client, sessionId, err);

    auto it = mClientData.find(client);
    if (it != mClientData.end() &&
        it->second.responseCb != nullptr) {
        it->second.responseCb(err, sessionId);
    } else {
        LOC_LOGE("%s]: client %p id %u not found in data", __func__, client, sessionId);
    }
}

uint32_t
BatchingAdapter::autoReportBatchingSessionsCount()
{
    uint32_t count = 0;
    for (auto batchingSession: mBatchingSessions) {
        if (batchingSession.second.batchingMode != BATCHING_MODE_NO_AUTO_REPORT) {
            count++;
        }
    }
    count += mTripSessions.size();
    return count;
}

uint32_t
BatchingAdapter::startBatchingCommand(
        LocationAPI* client, BatchingOptions& batchOptions)
{
    uint32_t sessionId = generateSessionId();
    LOC_LOGD("%s]: client %p id %u minInterval %u minDistance %u mode %u Batching Mode %d",
             __func__, client, sessionId, batchOptions.minInterval, batchOptions.minDistance,
             batchOptions.mode,batchOptions.batchingMode);

    struct MsgStartBatching : public LocMsg {
        BatchingAdapter& mAdapter;
        LocApiBase& mApi;
        LocationAPI* mClient;
        uint32_t mSessionId;
        BatchingOptions mBatchingOptions;
        inline MsgStartBatching(BatchingAdapter& adapter,
                               LocApiBase& api,
                               LocationAPI* client,
                               uint32_t sessionId,
                               BatchingOptions batchOptions) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api),
            mClient(client),
            mSessionId(sessionId),
            mBatchingOptions(batchOptions) {}
        inline virtual void proc() const {
            if (!mAdapter.isEngineCapabilitiesKnown()) {
                mAdapter.mPendingMsgs.push_back(new MsgStartBatching(*this));
                return;
            }
            LocationError err = LOCATION_ERROR_SUCCESS;

            if (!mAdapter.hasBatchingCallback(mClient)) {
                err = LOCATION_ERROR_CALLBACK_MISSING;
            } else if (0 == mBatchingOptions.size) {
                err = LOCATION_ERROR_INVALID_PARAMETER;
            } else if (!ContextBase::isMessageSupported(
                       LOC_API_ADAPTER_MESSAGE_DISTANCE_BASE_LOCATION_BATCHING)) {
                err = LOCATION_ERROR_NOT_SUPPORTED;
            }
            if (LOCATION_ERROR_SUCCESS == err) {
                if (mBatchingOptions.batchingMode == BATCHING_MODE_ROUTINE ||
                    mBatchingOptions.batchingMode == BATCHING_MODE_NO_AUTO_REPORT) {
                    mAdapter.startBatching(mClient, mSessionId, mBatchingOptions);
                } else if (mBatchingOptions.batchingMode == BATCHING_MODE_TRIP) {
                    mAdapter.startTripBatchingMultiplex(mClient, mSessionId, mBatchingOptions);
                } else {
                    mAdapter.reportResponse(mClient, LOCATION_ERROR_INVALID_PARAMETER, mSessionId);
                }
            }
        }
    };

    sendMsg(new MsgStartBatching(*this, *mLocApi, client, sessionId, batchOptions));

    return sessionId;
}

void
BatchingAdapter::startBatching(LocationAPI* client, uint32_t sessionId,
        const BatchingOptions& batchingOptions)
{
    if (batchingOptions.batchingMode != BATCHING_MODE_NO_AUTO_REPORT &&
        0 == autoReportBatchingSessionsCount()) {
        // if there is currenty no batching sessions interested in batch full event, then this
        // new session will need to register for batch full event
        updateEvtMask(LOC_API_ADAPTER_BIT_BATCH_FULL,
                      LOC_REGISTRATION_MASK_ENABLED);
    }

    // Assume start will be OK, remove session if not
    saveBatchingSession(client, sessionId, batchingOptions);
    mLocApi->startBatching(sessionId, batchingOptions, getBatchingAccuracy(), getBatchingTimeout(),
            new LocApiResponse(*getContext(),
            [this, client, sessionId, batchingOptions] (LocationError err) {
        if (LOCATION_ERROR_SUCCESS != err) {
            eraseBatchingSession(client, sessionId);
        }

        if (LOCATION_ERROR_SUCCESS != err &&
            batchingOptions.batchingMode != BATCHING_MODE_NO_AUTO_REPORT &&
            0 == autoReportBatchingSessionsCount()) {
            // if we fail to start batching and we have already registered batch full event
            // we need to undo that since no sessions are now interested in batch full event
            updateEvtMask(LOC_API_ADAPTER_BIT_BATCH_FULL,
                          LOC_REGISTRATION_MASK_DISABLED);
        }

        reportResponse(client, err, sessionId);
    }));
}

void
BatchingAdapter::updateBatchingOptionsCommand(LocationAPI* client, uint32_t id,
        BatchingOptions& batchOptions)
{
    LOC_LOGD("%s]: client %p id %u minInterval %u minDistance %u mode %u batchMode %u",
             __func__, client, id, batchOptions.minInterval,
             batchOptions.minDistance, batchOptions.mode,
             batchOptions.batchingMode);

    struct MsgUpdateBatching : public LocMsg {
        BatchingAdapter& mAdapter;
        LocApiBase& mApi;
        LocationAPI* mClient;
        uint32_t mSessionId;
        BatchingOptions mBatchOptions;
        inline MsgUpdateBatching(BatchingAdapter& adapter,
                                LocApiBase& api,
                                LocationAPI* client,
                                uint32_t sessionId,
                                BatchingOptions batchOptions) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api),
            mClient(client),
            mSessionId(sessionId),
            mBatchOptions(batchOptions) {}
        inline virtual void proc() const {
            if (!mAdapter.isEngineCapabilitiesKnown()) {
                mAdapter.mPendingMsgs.push_back(new MsgUpdateBatching(*this));
                return;
            }
            LocationError err = LOCATION_ERROR_SUCCESS;
            if (!mAdapter.isBatchingSession(mClient, mSessionId)) {
                err = LOCATION_ERROR_ID_UNKNOWN;
            } else if ((0 == mBatchOptions.size) ||
                       (mBatchOptions.batchingMode > BATCHING_MODE_NO_AUTO_REPORT)) {
                err = LOCATION_ERROR_INVALID_PARAMETER;
            }
            if (LOCATION_ERROR_SUCCESS == err) {
                if (!mAdapter.isTripSession(mSessionId)) {
                    mAdapter.stopBatching(mClient, mSessionId, true, mBatchOptions);
                } else {
                    mAdapter.stopTripBatchingMultiplex(mClient, mSessionId, true, mBatchOptions);
                }
           }
        }
    };

    sendMsg(new MsgUpdateBatching(*this, *mLocApi, client, id, batchOptions));
}

void
BatchingAdapter::stopBatchingCommand(LocationAPI* client, uint32_t id)
{
    LOC_LOGD("%s]: client %p id %u", __func__, client, id);

    struct MsgStopBatching : public LocMsg {
        BatchingAdapter& mAdapter;
        LocApiBase& mApi;
        LocationAPI* mClient;
        uint32_t mSessionId;
        inline MsgStopBatching(BatchingAdapter& adapter,
                               LocApiBase& api,
                               LocationAPI* client,
                               uint32_t sessionId) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api),
            mClient(client),
            mSessionId(sessionId) {}
        inline virtual void proc() const {
            if (!mAdapter.isEngineCapabilitiesKnown()) {
                mAdapter.mPendingMsgs.push_back(new MsgStopBatching(*this));
                return;
            }
            LocationError err = LOCATION_ERROR_SUCCESS;
            if (!mAdapter.isBatchingSession(mClient, mSessionId)) {
                err = LOCATION_ERROR_ID_UNKNOWN;
            }
            if (LOCATION_ERROR_SUCCESS == err) {
                if (mAdapter.isTripSession(mSessionId)) {
                    mAdapter.stopTripBatchingMultiplex(mClient, mSessionId);
                } else {
                    mAdapter.stopBatching(mClient, mSessionId);
                }
            }
        }
    };

    sendMsg(new MsgStopBatching(*this, *mLocApi, client, id));
}

void
BatchingAdapter::stopBatching(LocationAPI* client, uint32_t sessionId, bool restartNeeded,
        const BatchingOptions& batchOptions)
{
    LocationSessionKey key(client, sessionId);
    auto it = mBatchingSessions.find(key);
    if (it != mBatchingSessions.end()) {
        auto flpOptions = it->second;
        // Assume stop will be OK, restore session if not
        eraseBatchingSession(client, sessionId);
        mLocApi->stopBatching(sessionId,
                new LocApiResponse(*getContext(),
                [this, client, sessionId, flpOptions, restartNeeded, batchOptions]
                (LocationError err) {
            if (LOCATION_ERROR_SUCCESS != err) {
                saveBatchingSession(client, sessionId, batchOptions);
            } else {
                // if stopBatching is success, unregister for batch full event if this was the last
                // batching session that is interested in batch full event
                if (0 == autoReportBatchingSessionsCount() &&
                    flpOptions.batchingMode != BATCHING_MODE_NO_AUTO_REPORT) {
                    updateEvtMask(LOC_API_ADAPTER_BIT_BATCH_FULL,
                                  LOC_REGISTRATION_MASK_DISABLED);
                }

                if (restartNeeded) {
                    if (batchOptions.batchingMode == BATCHING_MODE_ROUTINE ||
                            batchOptions.batchingMode == BATCHING_MODE_NO_AUTO_REPORT) {
                        startBatching(client, sessionId, batchOptions);
                    } else if (batchOptions.batchingMode == BATCHING_MODE_TRIP) {
                        startTripBatchingMultiplex(client, sessionId, batchOptions);
                    }
                }
            }
            reportResponse(client, err, sessionId);
        }));
    }
}

void
BatchingAdapter::getBatchedLocationsCommand(LocationAPI* client, uint32_t id, size_t count)
{
    LOC_LOGD("%s]: client %p id %u count %zu", __func__, client, id, count);

    struct MsgGetBatchedLocations : public LocMsg {
        BatchingAdapter& mAdapter;
        LocApiBase& mApi;
        LocationAPI* mClient;
        uint32_t mSessionId;
        size_t mCount;
        inline MsgGetBatchedLocations(BatchingAdapter& adapter,
                                     LocApiBase& api,
                                     LocationAPI* client,
                                     uint32_t sessionId,
                                     size_t count) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api),
            mClient(client),
            mSessionId(sessionId),
            mCount(count) {}
        inline virtual void proc() const {
            if (!mAdapter.isEngineCapabilitiesKnown()) {
                mAdapter.mPendingMsgs.push_back(new MsgGetBatchedLocations(*this));
                return;
            }
            LocationError err = LOCATION_ERROR_SUCCESS;
            if (!mAdapter.hasBatchingCallback(mClient)) {
                err = LOCATION_ERROR_CALLBACK_MISSING;
            } else if (!mAdapter.isBatchingSession(mClient, mSessionId)) {
                err = LOCATION_ERROR_ID_UNKNOWN;
            }
            if (LOCATION_ERROR_SUCCESS == err) {
                if (mAdapter.isTripSession(mSessionId)) {
                    mApi.getBatchedTripLocations(mCount, 0,
                            new LocApiResponse(*mAdapter.getContext(),
                            [&mAdapter = mAdapter, mSessionId = mSessionId,
                            mClient = mClient] (LocationError err) {
                        mAdapter.reportResponse(mClient, err, mSessionId);
                    }));
                } else {
                    mApi.getBatchedLocations(mCount, new LocApiResponse(*mAdapter.getContext(),
                            [&mAdapter = mAdapter, mSessionId = mSessionId,
                            mClient = mClient] (LocationError err) {
                        mAdapter.reportResponse(mClient, err, mSessionId);
                    }));
                }
            } else {
                mAdapter.reportResponse(mClient, err, mSessionId);
            }
        }
    };

    sendMsg(new MsgGetBatchedLocations(*this, *mLocApi, client, id, count));
}

void
BatchingAdapter::reportLocationsEvent(const Location* locations, size_t count,
        BatchingMode batchingMode)
{
    LOC_LOGD("%s]: count %zu batchMode %d", __func__, count, batchingMode);

    struct MsgReportLocations : public LocMsg {
        BatchingAdapter& mAdapter;
        Location* mLocations;
        size_t mCount;
        BatchingMode mBatchingMode;
        inline MsgReportLocations(BatchingAdapter& adapter,
                                  const Location* locations,
                                  size_t count,
                                  BatchingMode batchingMode) :
            LocMsg(),
            mAdapter(adapter),
            mLocations(new Location[count]),
            mCount(count),
            mBatchingMode(batchingMode)
        {
            if (nullptr == mLocations) {
                LOC_LOGE("%s]: new failed to allocate mLocations", __func__);
                return;
            }
            for (size_t i=0; i < mCount; ++i) {
                mLocations[i] = locations[i];
            }
        }
        inline virtual ~MsgReportLocations() {
            if (nullptr != mLocations)
                delete[] mLocations;
        }
        inline virtual void proc() const {
            mAdapter.reportLocations(mLocations, mCount, mBatchingMode);
        }
    };

    sendMsg(new MsgReportLocations(*this, locations, count, batchingMode));
}

void
BatchingAdapter::reportLocations(Location* locations, size_t count, BatchingMode batchingMode)
{
    BatchingOptions batchOptions = {sizeof(BatchingOptions), batchingMode};

    for (auto it=mClientData.begin(); it != mClientData.end(); ++it) {
        if (nullptr != it->second.batchingCb) {
            it->second.batchingCb(count, locations, batchOptions);
        }
    }
}

void
BatchingAdapter::reportCompletedTripsEvent(uint32_t accumulated_distance)
{
    struct MsgReportCompletedTrips : public LocMsg {
        BatchingAdapter& mAdapter;
        uint32_t mAccumulatedDistance;
        inline MsgReportCompletedTrips(BatchingAdapter& adapter,
                                  uint32_t accumulated_distance) :
            LocMsg(),
            mAdapter(adapter),
            mAccumulatedDistance(accumulated_distance)
        {
        }
        inline virtual ~MsgReportCompletedTrips() {
        }
        inline virtual void proc() const {

            // Check if any trips are completed
            std::list<uint32_t> completedTripsList;
            completedTripsList.clear();

            for(auto itt = mAdapter.mTripSessions.begin(); itt != mAdapter.mTripSessions.end();)
            {
                TripSessionStatus &tripSession = itt->second;

                tripSession.accumulatedDistanceThisTrip =
                        tripSession.accumulatedDistanceOnTripRestart
                        + (mAccumulatedDistance - tripSession.accumulatedDistanceOngoingBatch);
                if (tripSession.tripDistance <= tripSession.accumulatedDistanceThisTrip) {
                    // trip is completed
                    completedTripsList.push_back(itt->first);
                    itt = mAdapter.mTripSessions.erase(itt);

                    if (tripSession.tripTBFInterval == mAdapter.mOngoingTripTBFInterval) {
                        // trip with ongoing TBF interval is completed
                        mAdapter.mTripWithOngoingTBFDropped = true;
                    }

                    if (tripSession.tripDistance == mAdapter.mOngoingTripDistance) {
                        // trip with ongoing trip distance is completed
                        mAdapter.mTripWithOngoingTripDistanceDropped = true;
                    }
                } else {
                    itt++;
                }
            }

            if (completedTripsList.size() > 0) {
                mAdapter.reportBatchStatusChange(BATCHING_STATUS_TRIP_COMPLETED,
                        completedTripsList);
                mAdapter.restartTripBatching(false, mAccumulatedDistance, 0);
            } else {
                mAdapter.printTripReport();
            }
        }
    };

    LOC_LOGD("%s]: Accumulated Distance so far: %u",
               __func__,  accumulated_distance);

    sendMsg(new MsgReportCompletedTrips(*this, accumulated_distance));
}

void
BatchingAdapter::reportBatchStatusChange(BatchingStatus batchStatus,
        std::list<uint32_t> & completedTripsList)
{
    BatchingStatusInfo batchStatusInfo =
            {sizeof(BatchingStatusInfo), batchStatus};

    for (auto it=mClientData.begin(); it != mClientData.end(); ++it) {
        if (nullptr != it->second.batchingStatusCb) {
            it->second.batchingStatusCb(batchStatusInfo, completedTripsList);
        }
    }
}

void
BatchingAdapter::reportBatchStatusChangeEvent(BatchingStatus batchStatus)
{
    struct MsgReportBatchStatus : public LocMsg {
        BatchingAdapter& mAdapter;
        BatchingStatus mBatchStatus;
        inline MsgReportBatchStatus(BatchingAdapter& adapter,
                BatchingStatus batchStatus) :
            LocMsg(),
            mAdapter(adapter),
            mBatchStatus(batchStatus)
        {
        }
        inline virtual ~MsgReportBatchStatus() {
        }
        inline virtual void proc() const {
            std::list<uint32_t> tempList;
            tempList.clear();
            mAdapter.reportBatchStatusChange(mBatchStatus, tempList);
        }
    };

    sendMsg(new MsgReportBatchStatus(*this, batchStatus));
}

void
BatchingAdapter::startTripBatchingMultiplex(LocationAPI* client, uint32_t sessionId,
        const BatchingOptions& batchingOptions)
{
    if (mTripSessions.size() == 0) {
        // if there is currenty no batching sessions interested in batch full event, then this
        // new session will need to register for batch full event
        if (0 == autoReportBatchingSessionsCount()) {
            updateEvtMask(LOC_API_ADAPTER_BIT_BATCH_FULL,
                          LOC_REGISTRATION_MASK_ENABLED);
        }

        // Assume start will be OK, remove session if not
        saveBatchingSession(client, sessionId, batchingOptions);

        mTripSessions[sessionId] = { 0, 0, 0, batchingOptions.minDistance,
                batchingOptions.minInterval};
        mLocApi->startOutdoorTripBatching(batchingOptions.minDistance,
                batchingOptions.minInterval, getBatchingTimeout(), new LocApiResponse(*getContext(),
                [this, client, sessionId, batchingOptions] (LocationError err) {
            if (err == LOCATION_ERROR_SUCCESS) {
                mOngoingTripDistance = batchingOptions.minDistance;
                mOngoingTripTBFInterval = batchingOptions.minInterval;
                LOC_LOGD("%s] New Trip started ...", __func__);
                printTripReport();
            } else {
                eraseBatchingSession(client, sessionId);
                mTripSessions.erase(sessionId);
                // if we fail to start batching and we have already registered batch full event
                // we need to undo that since no sessions are now interested in batch full event
                if (0 == autoReportBatchingSessionsCount()) {
                    updateEvtMask(LOC_API_ADAPTER_BIT_BATCH_FULL,
                                  LOC_REGISTRATION_MASK_DISABLED);
                }
            }
            reportResponse(client, err, sessionId);
        }));
    } else {
        // query accumulated distance
        mLocApi->queryAccumulatedTripDistance(
                new LocApiResponseData<LocApiBatchData>(*getContext(),
                [this, batchingOptions, sessionId, client]
                (LocationError err, LocApiBatchData data) {
            uint32_t accumulatedDistanceOngoingBatch = 0;
            uint32_t numOfBatchedPositions = 0;
            uint32_t ongoingTripDistance = mOngoingTripDistance;
            uint32_t ongoingTripInterval = mOngoingTripTBFInterval;
            bool needsRestart = false;

            // check if TBF of new session is lesser than ongoing TBF interval
            if (ongoingTripInterval > batchingOptions.minInterval) {
                ongoingTripInterval = batchingOptions.minInterval;
                needsRestart = true;
            }
            accumulatedDistanceOngoingBatch = data.accumulatedDistance;
            numOfBatchedPositions = data.numOfBatchedPositions;
            TripSessionStatus newTripSession = { accumulatedDistanceOngoingBatch, 0, 0,
                                                 batchingOptions.minDistance,
                                                 batchingOptions.minInterval};
            if (err != LOCATION_ERROR_SUCCESS) {
                // unable to query accumulated distance, assume remaining distance in
                // ongoing batch is mongoingTripDistance.
                if (batchingOptions.minDistance < ongoingTripDistance) {
                    ongoingTripDistance = batchingOptions.minDistance;
                    needsRestart = true;
                }
            } else {
                // compute the remaining distance
                uint32_t ongoing_trip_remaining_distance = ongoingTripDistance -
                        accumulatedDistanceOngoingBatch;

                // check if new trip distance is lesser than the ongoing batch remaining distance
                if (batchingOptions.minDistance < ongoing_trip_remaining_distance) {
                    ongoingTripDistance = batchingOptions.minDistance;
                    needsRestart = true;
                } else if (needsRestart == true) {
                    // needsRestart is anyways true , may be because of lesser TBF of new session.
                    ongoingTripDistance = ongoing_trip_remaining_distance;
                }
                mTripSessions[sessionId] = newTripSession;
                LOC_LOGD("%s] New Trip started ...", __func__);
                printTripReport();
            }

            if (needsRestart) {
                mOngoingTripDistance = ongoingTripDistance;
                mOngoingTripTBFInterval = ongoingTripInterval;

                // reset the accumulatedDistanceOngoingBatch for each session,
                // and record the total accumulated distance so far for the session.
                for (auto itt = mTripSessions.begin(); itt != mTripSessions.end(); itt++) {
                    TripSessionStatus &tripSessStatus = itt->second;
                    tripSessStatus.accumulatedDistanceOngoingBatch = 0;
                    tripSessStatus.accumulatedDistanceOnTripRestart =
                            tripSessStatus.accumulatedDistanceThisTrip;
                }
                mLocApi->reStartOutdoorTripBatching(ongoingTripDistance, ongoingTripInterval,
                        getBatchingTimeout(), new LocApiResponse(*getContext(),
                        [this, client, sessionId] (LocationError err) {
                    if (err != LOCATION_ERROR_SUCCESS) {
                        LOC_LOGE("%s] New Trip restart failed!", __func__);
                    }
                    reportResponse(client, err, sessionId);
                }));
            } else {
                reportResponse(client, LOCATION_ERROR_SUCCESS, sessionId);
            }
        }));
    }
}

void
BatchingAdapter::stopTripBatchingMultiplex(LocationAPI* client, uint32_t sessionId,
        bool restartNeeded, const BatchingOptions& batchOptions)
{
    LocationError err = LOCATION_ERROR_SUCCESS;

    if (mTripSessions.size() == 1) {
        mLocApi->stopOutdoorTripBatching(true, new LocApiResponse(*getContext(),
                [this, restartNeeded, client, sessionId, batchOptions]
                (LocationError err) {
            if (LOCATION_ERROR_SUCCESS == err) {
                // if stopOutdoorTripBatching is success, unregister for batch full event if this
                // was the last batching session that is interested in batch full event
                if (1 == autoReportBatchingSessionsCount()) {
                    updateEvtMask(LOC_API_ADAPTER_BIT_BATCH_FULL,
                                  LOC_REGISTRATION_MASK_DISABLED);
                }
            }
            stopTripBatchingMultiplexCommon(err, client, sessionId, restartNeeded, batchOptions);
        }));
        return;
    }

    stopTripBatchingMultiplexCommon(err, client, sessionId, restartNeeded, batchOptions);
}

void
BatchingAdapter::stopTripBatchingMultiplexCommon(LocationError err, LocationAPI* client,
        uint32_t sessionId, bool restartNeeded, const BatchingOptions& batchOptions)
{
    auto itt = mTripSessions.find(sessionId);
    TripSessionStatus tripSess = itt->second;
    if (tripSess.tripTBFInterval == mOngoingTripTBFInterval) {
        // trip with ongoing trip interval is stopped
        mTripWithOngoingTBFDropped = true;
    }

    if (tripSess.tripDistance == mOngoingTripDistance) {
        // trip with ongoing trip distance is stopped
        mTripWithOngoingTripDistanceDropped = true;
    }

    mTripSessions.erase(sessionId);

    if (mTripSessions.size() == 0) {
        mOngoingTripDistance = 0;
        mOngoingTripTBFInterval = 0;
    } else {
        restartTripBatching(true);
    }

    if (restartNeeded) {
        eraseBatchingSession(client, sessionId);
        if (batchOptions.batchingMode == BATCHING_MODE_ROUTINE ||
                batchOptions.batchingMode == BATCHING_MODE_NO_AUTO_REPORT) {
            startBatching(client, sessionId, batchOptions);
        } else if (batchOptions.batchingMode == BATCHING_MODE_TRIP) {
            startTripBatchingMultiplex(client, sessionId, batchOptions);
        }
    }
    reportResponse(client, err, sessionId);
}


void
BatchingAdapter::restartTripBatching(bool queryAccumulatedDistance, uint32_t accDist,
        uint32_t numbatchedPos)
{
    // does batch need restart with new trip distance / TBF interval
    uint32_t minRemainingDistance = 0;
    uint32_t minTBFInterval = 0;

    // if no more trips left, stop the ongoing trip
    if (mTripSessions.size() == 0) {
        mLocApi->stopOutdoorTripBatching(true, new LocApiResponse(*getContext(),
                                               [] (LocationError /*err*/) {}));
        mOngoingTripDistance = 0;
        mOngoingTripTBFInterval = 0;
        // unregister for batch full event if there are no more
        // batching session that is interested in batch full event
        if (0 == autoReportBatchingSessionsCount()) {
                updateEvtMask(LOC_API_ADAPTER_BIT_BATCH_FULL,
                              LOC_REGISTRATION_MASK_DISABLED);
        }
        return;
    }

    // record the min trip distance and min tbf interval of all ongoing sessions
    for (auto itt = mTripSessions.begin(); itt != mTripSessions.end(); itt++) {

        TripSessionStatus tripSessStatus = itt->second;

        if ((minRemainingDistance == 0) ||
                (minRemainingDistance > (tripSessStatus.tripDistance
                - tripSessStatus.accumulatedDistanceThisTrip))) {
            minRemainingDistance = tripSessStatus.tripDistance -
                    tripSessStatus.accumulatedDistanceThisTrip;
        }

        if ((minTBFInterval == 0) ||
            (minTBFInterval > tripSessStatus.tripTBFInterval)) {
            minTBFInterval = tripSessStatus.tripTBFInterval;
        }
    }

    mLocApi->queryAccumulatedTripDistance(
            new LocApiResponseData<LocApiBatchData>(*getContext(),
            [this, queryAccumulatedDistance, minRemainingDistance, minTBFInterval, accDist,
            numbatchedPos] (LocationError /*err*/, LocApiBatchData data) {
        bool needsRestart = false;

        uint32_t ongoingTripDistance = mOngoingTripDistance;
        uint32_t ongoingTripInterval = mOngoingTripTBFInterval;
        uint32_t accumulatedDistance = accDist;
        uint32_t numOfBatchedPositions = numbatchedPos;

        if (queryAccumulatedDistance) {
            accumulatedDistance = data.accumulatedDistance;
            numOfBatchedPositions = data.numOfBatchedPositions;
        }

        if ((!mTripWithOngoingTripDistanceDropped) &&
                (ongoingTripDistance - accumulatedDistance != 0)) {
            // if ongoing trip is already not completed still,
            // check the min distance against the remaining distance
            if (minRemainingDistance <
                    (ongoingTripDistance - accumulatedDistance)) {
                ongoingTripDistance = minRemainingDistance;
                needsRestart = true;
            }
        } else if (minRemainingDistance != 0) {
            // else if ongoing trip is already completed / dropped,
            // use the minRemainingDistance of ongoing sessions
            ongoingTripDistance = minRemainingDistance;
            needsRestart = true;
        }

         if ((minTBFInterval < ongoingTripInterval) ||
                    ((minTBFInterval != ongoingTripInterval) &&
                    (mTripWithOngoingTBFDropped))) {
            ongoingTripInterval = minTBFInterval;
            needsRestart = true;
        }

        if (needsRestart) {
            mLocApi->reStartOutdoorTripBatching(ongoingTripDistance, ongoingTripInterval,
                    getBatchingTimeout(), new LocApiResponse(*getContext(),
                    [this, accumulatedDistance, ongoingTripDistance, ongoingTripInterval]
                    (LocationError err) {

                if (err == LOCATION_ERROR_SUCCESS) {
                    for(auto itt = mTripSessions.begin(); itt != mTripSessions.end(); itt++) {
                        TripSessionStatus &tripSessStatus = itt->second;
                        tripSessStatus.accumulatedDistanceThisTrip =
                                tripSessStatus.accumulatedDistanceOnTripRestart +
                                (accumulatedDistance -
                                 tripSessStatus.accumulatedDistanceOngoingBatch);

                        tripSessStatus.accumulatedDistanceOngoingBatch = 0;
                        tripSessStatus.accumulatedDistanceOnTripRestart =
                                tripSessStatus.accumulatedDistanceThisTrip;
                    }

                    mOngoingTripDistance = ongoingTripDistance;
                    mOngoingTripTBFInterval = ongoingTripInterval;
                }
            }));
        }
    }));
}

void
BatchingAdapter::printTripReport()
{
    IF_LOC_LOGD {
        LOC_LOGD("Ongoing Trip Distance = %u, Ongoing Trip TBF Interval = %u",
                mOngoingTripDistance, mOngoingTripTBFInterval);

        for (auto itt = mTripSessions.begin(); itt != mTripSessions.end(); itt++) {
            TripSessionStatus tripSessStatus = itt->second;

            LOC_LOGD("tripDistance:%u tripTBFInterval:%u"
                    " trip accumulated Distance:%u"
                    " trip accumualted distance ongoing batch:%u"
                    " trip accumulated distance on trip restart %u \r\n",
                    tripSessStatus.tripDistance, tripSessStatus.tripTBFInterval,
                    tripSessStatus.accumulatedDistanceThisTrip,
                    tripSessStatus.accumulatedDistanceOngoingBatch,
                    tripSessStatus.accumulatedDistanceOnTripRestart);
        }
    }
}
