/* Copyright (c) 2013-2019, The Linux Foundation. All rights reserved.
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
#define LOG_TAG "LocSvc_GeofenceAdapter"

#include <GeofenceAdapter.h>
#include "loc_log.h"
#include <log_util.h>
#include <string>

using namespace loc_core;

GeofenceAdapter::GeofenceAdapter() :
    LocAdapterBase(0,
                    LocContext::getLocContext(
                        NULL,
                        NULL,
                        LocContext::mLocationHalName,
                        false),
                    true /*isMaster*/)
{
    LOC_LOGD("%s]: Constructor", __func__);
}

void
GeofenceAdapter::stopClientSessions(LocationAPI* client)
{
    LOC_LOGD("%s]: client %p", __func__, client);


    for (auto it = mGeofenceIds.begin(); it != mGeofenceIds.end();) {
        uint32_t hwId = it->second;
        GeofenceKey key(it->first);
        if (client == key.client) {
            it = mGeofenceIds.erase(it);
            mLocApi->removeGeofence(hwId, key.id,
                    new LocApiResponse(*getContext(),
                    [this, hwId] (LocationError err) {
                if (LOCATION_ERROR_SUCCESS == err) {
                    auto it2 = mGeofences.find(hwId);
                    if (it2 != mGeofences.end()) {
                        mGeofences.erase(it2);
                    } else {
                        LOC_LOGE("%s]:geofence item to erase not found. hwId %u", __func__, hwId);
                    }
                }
            }));
            continue;
        }
        ++it; // increment only when not erasing an iterator
    }

}

void
GeofenceAdapter::updateClientsEventMask()
{
    LOC_API_ADAPTER_EVENT_MASK_T mask = 0;
    for (auto it=mClientData.begin(); it != mClientData.end(); ++it) {
        if (it->second.geofenceBreachCb != nullptr) {
            mask |= LOC_API_ADAPTER_BIT_BATCHED_GENFENCE_BREACH_REPORT;
            mask |= LOC_API_ADAPTER_BIT_REPORT_GENFENCE_DWELL;
        }
        if (it->second.geofenceStatusCb != nullptr) {
            mask |= LOC_API_ADAPTER_BIT_GEOFENCE_GEN_ALERT;
        }
    }
    updateEvtMask(mask, LOC_REGISTRATION_MASK_SET);
}

LocationError
GeofenceAdapter::getHwIdFromClient(LocationAPI* client, uint32_t clientId, uint32_t& hwId)
{
    GeofenceKey key(client, clientId);
    auto it = mGeofenceIds.find(key);
    if (it != mGeofenceIds.end()) {
        hwId = it->second;
        return LOCATION_ERROR_SUCCESS;
    }
    return LOCATION_ERROR_ID_UNKNOWN;
}

LocationError
GeofenceAdapter::getGeofenceKeyFromHwId(uint32_t hwId, GeofenceKey& key)
{
    auto it = mGeofences.find(hwId);
    if (it != mGeofences.end()) {
        key = it->second.key;
        return LOCATION_ERROR_SUCCESS;
    }
    return LOCATION_ERROR_ID_UNKNOWN;
}

void
GeofenceAdapter::handleEngineUpEvent()
{
    struct MsgSSREvent : public LocMsg {
        GeofenceAdapter& mAdapter;
        inline MsgSSREvent(GeofenceAdapter& adapter) :
            LocMsg(),
            mAdapter(adapter) {}
        virtual void proc() const {
            mAdapter.setEngineCapabilitiesKnown(true);
            mAdapter.broadcastCapabilities(mAdapter.getCapabilities());
            mAdapter.restartGeofences();
            for (auto msg: mAdapter.mPendingMsgs) {
                mAdapter.sendMsg(msg);
            }
            mAdapter.mPendingMsgs.clear();
        }
    };

    sendMsg(new MsgSSREvent(*this));
}

void
GeofenceAdapter::restartGeofences()
{
    if (mGeofences.empty()) {
        return;
    }

    GeofencesMap oldGeofences(mGeofences);
    mGeofences.clear();
    mGeofenceIds.clear();

    for (auto it = oldGeofences.begin(); it != oldGeofences.end(); it++) {
        GeofenceObject object = it->second;
        GeofenceOption options = {sizeof(GeofenceOption),
                                   object.breachMask,
                                   object.responsiveness,
                                   object.dwellTime};
        GeofenceInfo info = {sizeof(GeofenceInfo),
                             object.latitude,
                             object.longitude,
                             object.radius};
        mLocApi->addGeofence(object.key.id,
                              options,
                              info,
                              new LocApiResponseData<LocApiGeofenceData>(*getContext(),
                [this, object, options, info] (LocationError err, LocApiGeofenceData data) {
            if (LOCATION_ERROR_SUCCESS == err) {
                if (true == object.paused) {
                    mLocApi->pauseGeofence(data.hwId, object.key.id,
                            new LocApiResponse(*getContext(), [] (LocationError err ) {}));
                }
                saveGeofenceItem(object.key.client, object.key.id, data.hwId, options, info);
            }
        }));
    }
}

void
GeofenceAdapter::reportResponse(LocationAPI* client, size_t count, LocationError* errs,
        uint32_t* ids)
{
    IF_LOC_LOGD {
        std::string idsString = "[";
        std::string errsString = "[";
        if (NULL != ids && NULL != errs) {
            for (size_t i=0; i < count; ++i) {
                idsString += std::to_string(ids[i]) + " ";
                errsString += std::to_string(errs[i]) + " ";
            }
        }
        idsString += "]";
        errsString += "]";

        LOC_LOGD("%s]: client %p ids %s errs %s",
                 __func__, client, idsString.c_str(), errsString.c_str());
    }

    auto it = mClientData.find(client);
    if (it != mClientData.end() && it->second.collectiveResponseCb != nullptr) {
        it->second.collectiveResponseCb(count, errs, ids);
    } else {
        LOC_LOGE("%s]: client %p response not found in info", __func__, client);
    }
}

uint32_t*
GeofenceAdapter::addGeofencesCommand(LocationAPI* client, size_t count, GeofenceOption* options,
        GeofenceInfo* infos)
{
    LOC_LOGD("%s]: client %p count %zu", __func__, client, count);

    struct MsgAddGeofences : public LocMsg {
        GeofenceAdapter& mAdapter;
        LocApiBase& mApi;
        LocationAPI* mClient;
        size_t mCount;
        uint32_t* mIds;
        GeofenceOption* mOptions;
        GeofenceInfo* mInfos;
        inline MsgAddGeofences(GeofenceAdapter& adapter,
                               LocApiBase& api,
                               LocationAPI* client,
                               size_t count,
                               uint32_t* ids,
                               GeofenceOption* options,
                               GeofenceInfo* infos) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api),
            mClient(client),
            mCount(count),
            mIds(ids),
            mOptions(options),
            mInfos(infos) {}
        inline virtual void proc() const {
            LocationError* errs = new LocationError[mCount];
            if (nullptr == errs) {
                LOC_LOGE("%s]: new failed to allocate errs", __func__);
                return;
            }
            for (size_t i=0; i < mCount; ++i) {
                if (NULL == mIds || NULL == mOptions || NULL == mInfos) {
                    errs[i] = LOCATION_ERROR_INVALID_PARAMETER;
                } else {
                    mApi.addToCallQueue(new LocApiResponse(*mAdapter.getContext(),
                            [&mAdapter = mAdapter, mCount = mCount, mClient = mClient,
                            mOptions = mOptions, mInfos = mInfos, mIds = mIds, &mApi = mApi,
                            errs, i] (LocationError err ) {
                        mApi.addGeofence(mIds[i], mOptions[i], mInfos[i],
                        new LocApiResponseData<LocApiGeofenceData>(*mAdapter.getContext(),
                        [&mAdapter = mAdapter, mOptions = mOptions, mClient = mClient,
                        mCount = mCount, mIds = mIds, mInfos = mInfos, errs, i]
                        (LocationError err, LocApiGeofenceData data) {
                            if (LOCATION_ERROR_SUCCESS == err) {
                                mAdapter.saveGeofenceItem(mClient,
                                mIds[i],
                                data.hwId,
                                mOptions[i],
                                mInfos[i]);
                            }
                            errs[i] = err;

                            // Send aggregated response on last item and cleanup
                            if (i == mCount-1) {
                                mAdapter.reportResponse(mClient, mCount, errs, mIds);
                                delete[] errs;
                                delete[] mIds;
                                delete[] mOptions;
                                delete[] mInfos;
                            }
                        }));
                    }));
                }
            }
        }
    };

    if (0 == count) {
        return NULL;
    }
    uint32_t* ids = new uint32_t[count];
    if (nullptr == ids) {
        LOC_LOGE("%s]: new failed to allocate ids", __func__);
        return NULL;
    }
    if (NULL != ids) {
        for (size_t i=0; i < count; ++i) {
            ids[i] = generateSessionId();
        }
    }
    GeofenceOption* optionsCopy;
    if (options == NULL) {
        optionsCopy = NULL;
    } else {
        optionsCopy = new GeofenceOption[count];
        if (nullptr == optionsCopy) {
            LOC_LOGE("%s]: new failed to allocate optionsCopy", __func__);
            return NULL;
        }
        COPY_IF_NOT_NULL(optionsCopy, options, count);
    }
    GeofenceInfo* infosCopy;
    if (infos == NULL) {
        infosCopy = NULL;
    } else {
        infosCopy = new GeofenceInfo[count];
        if (nullptr == infosCopy) {
            LOC_LOGE("%s]: new failed to allocate infosCopy", __func__);
            return NULL;
        }
        COPY_IF_NOT_NULL(infosCopy, infos, count);
    }

    sendMsg(new MsgAddGeofences(*this, *mLocApi, client, count, ids, optionsCopy, infosCopy));
    return ids;
}

void
GeofenceAdapter::removeGeofencesCommand(LocationAPI* client, size_t count, uint32_t* ids)
{
    LOC_LOGD("%s]: client %p count %zu", __func__, client, count);

    struct MsgRemoveGeofences : public LocMsg {
        GeofenceAdapter& mAdapter;
        LocApiBase& mApi;
        LocationAPI* mClient;
        size_t mCount;
        uint32_t* mIds;
        inline MsgRemoveGeofences(GeofenceAdapter& adapter,
                                  LocApiBase& api,
                                  LocationAPI* client,
                                  size_t count,
                                  uint32_t* ids) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api),
            mClient(client),
            mCount(count),
            mIds(ids) {}
        inline virtual void proc() const  {
            LocationError* errs = new LocationError[mCount];
            if (nullptr == errs) {
                LOC_LOGE("%s]: new failed to allocate errs", __func__);
                return;
            }
            for (size_t i=0; i < mCount; ++i) {
                mApi.addToCallQueue(new LocApiResponse(*mAdapter.getContext(),
                        [&mAdapter = mAdapter, mCount = mCount, mClient = mClient, mIds = mIds,
                        &mApi = mApi, errs, i] (LocationError err ) {
                    uint32_t hwId = 0;
                    errs[i] = mAdapter.getHwIdFromClient(mClient, mIds[i], hwId);
                    if (LOCATION_ERROR_SUCCESS == errs[i]) {
                        mApi.removeGeofence(hwId, mIds[i],
                        new LocApiResponse(*mAdapter.getContext(),
                        [&mAdapter = mAdapter, mCount = mCount, mClient = mClient, mIds = mIds,
                        hwId, errs, i] (LocationError err ) {
                            if (LOCATION_ERROR_SUCCESS == err) {
                                mAdapter.removeGeofenceItem(hwId);
                            }
                            errs[i] = err;

                            // Send aggregated response on last item and cleanup
                            if (i == mCount-1) {
                                mAdapter.reportResponse(mClient, mCount, errs, mIds);
                                delete[] errs;
                                delete[] mIds;
                            }
                        }));
                    } else {
                        // Send aggregated response on last item and cleanup
                        if (i == mCount-1) {
                            mAdapter.reportResponse(mClient, mCount, errs, mIds);
                            delete[] errs;
                            delete[] mIds;
                        }
                    }
                }));
            }
        }
    };

    if (0 == count) {
        return;
    }
    uint32_t* idsCopy = new uint32_t[count];
    if (nullptr == idsCopy) {
        LOC_LOGE("%s]: new failed to allocate idsCopy", __func__);
        return;
    }
    COPY_IF_NOT_NULL(idsCopy, ids, count);
    sendMsg(new MsgRemoveGeofences(*this, *mLocApi, client, count, idsCopy));
}

void
GeofenceAdapter::pauseGeofencesCommand(LocationAPI* client, size_t count, uint32_t* ids)
{
    LOC_LOGD("%s]: client %p count %zu", __func__, client, count);

    struct MsgPauseGeofences : public LocMsg {
        GeofenceAdapter& mAdapter;
        LocApiBase& mApi;
        LocationAPI* mClient;
        size_t mCount;
        uint32_t* mIds;
        inline MsgPauseGeofences(GeofenceAdapter& adapter,
                                 LocApiBase& api,
                                 LocationAPI* client,
                                 size_t count,
                                 uint32_t* ids) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api),
            mClient(client),
            mCount(count),
            mIds(ids) {}
        inline virtual void proc() const  {
            LocationError* errs = new LocationError[mCount];
            if (nullptr == errs) {
                LOC_LOGE("%s]: new failed to allocate errs", __func__);
                return;
            }
            for (size_t i=0; i < mCount; ++i) {
                mApi.addToCallQueue(new LocApiResponse(*mAdapter.getContext(),
                        [&mAdapter = mAdapter, mCount = mCount, mClient = mClient, mIds = mIds,
                        &mApi = mApi, errs, i] (LocationError err ) {
                    uint32_t hwId = 0;
                    errs[i] = mAdapter.getHwIdFromClient(mClient, mIds[i], hwId);
                    if (LOCATION_ERROR_SUCCESS == errs[i]) {
                        mApi.pauseGeofence(hwId, mIds[i], new LocApiResponse(*mAdapter.getContext(),
                        [&mAdapter = mAdapter, mCount = mCount, mClient = mClient, mIds = mIds,
                        hwId, errs, i] (LocationError err ) {
                            if (LOCATION_ERROR_SUCCESS == err) {
                                mAdapter.pauseGeofenceItem(hwId);
                            }
                            errs[i] = err;

                            // Send aggregated response on last item and cleanup
                            if (i == mCount-1) {
                                mAdapter.reportResponse(mClient, mCount, errs, mIds);
                                delete[] errs;
                                delete[] mIds;
                            }
                        }));
                    } else {
                        // Send aggregated response on last item and cleanup
                        if (i == mCount-1) {
                            mAdapter.reportResponse(mClient, mCount, errs, mIds);
                            delete[] errs;
                            delete[] mIds;
                        }
                    }
                }));
            }
        }
    };

    if (0 == count) {
        return;
    }
    uint32_t* idsCopy = new uint32_t[count];
    if (nullptr == idsCopy) {
        LOC_LOGE("%s]: new failed to allocate idsCopy", __func__);
        return;
    }
    COPY_IF_NOT_NULL(idsCopy, ids, count);
    sendMsg(new MsgPauseGeofences(*this, *mLocApi, client, count, idsCopy));
}

void
GeofenceAdapter::resumeGeofencesCommand(LocationAPI* client, size_t count, uint32_t* ids)
{
    LOC_LOGD("%s]: client %p count %zu", __func__, client, count);

    struct MsgResumeGeofences : public LocMsg {
        GeofenceAdapter& mAdapter;
        LocApiBase& mApi;
        LocationAPI* mClient;
        size_t mCount;
        uint32_t* mIds;
        inline MsgResumeGeofences(GeofenceAdapter& adapter,
                                  LocApiBase& api,
                                  LocationAPI* client,
                                  size_t count,
                                  uint32_t* ids) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api),
            mClient(client),
            mCount(count),
            mIds(ids) {}
        inline virtual void proc() const  {
            LocationError* errs = new LocationError[mCount];
            if (nullptr == errs) {
                LOC_LOGE("%s]: new failed to allocate errs", __func__);
                return;
            }
            for (size_t i=0; i < mCount; ++i) {
                mApi.addToCallQueue(new LocApiResponse(*mAdapter.getContext(),
                        [&mAdapter = mAdapter, mCount = mCount, mClient = mClient, mIds = mIds,
                        &mApi = mApi, errs, i] (LocationError err ) {
                    uint32_t hwId = 0;
                    errs[i] = mAdapter.getHwIdFromClient(mClient, mIds[i], hwId);
                    if (LOCATION_ERROR_SUCCESS == errs[i]) {
                        mApi.resumeGeofence(hwId, mIds[i],
                                new LocApiResponse(*mAdapter.getContext(),
                                [&mAdapter = mAdapter, mCount = mCount, mClient = mClient, hwId,
                                errs, mIds = mIds, i] (LocationError err ) {
                            if (LOCATION_ERROR_SUCCESS == err) {
                                errs[i] = err;

                                mAdapter.resumeGeofenceItem(hwId);
                                // Send aggregated response on last item and cleanup
                                if (i == mCount-1) {
                                    mAdapter.reportResponse(mClient, mCount, errs, mIds);
                                    delete[] errs;
                                    delete[] mIds;
                                }
                            }
                        }));
                    } else {
                        // Send aggregated response on last item and cleanup
                        if (i == mCount-1) {
                            mAdapter.reportResponse(mClient, mCount, errs, mIds);
                            delete[] errs;
                            delete[] mIds;
                        }
                    }
                }));
            }
        }
    };

    if (0 == count) {
        return;
    }
    uint32_t* idsCopy = new uint32_t[count];
    if (nullptr == idsCopy) {
        LOC_LOGE("%s]: new failed to allocate idsCopy", __func__);
        return;
    }
    COPY_IF_NOT_NULL(idsCopy, ids, count);
    sendMsg(new MsgResumeGeofences(*this, *mLocApi, client, count, idsCopy));
}

void
GeofenceAdapter::modifyGeofencesCommand(LocationAPI* client, size_t count, uint32_t* ids,
        GeofenceOption* options)
{
    LOC_LOGD("%s]: client %p count %zu", __func__, client, count);

    struct MsgModifyGeofences : public LocMsg {
        GeofenceAdapter& mAdapter;
        LocApiBase& mApi;
        LocationAPI* mClient;
        size_t mCount;
        uint32_t* mIds;
        GeofenceOption* mOptions;
        inline MsgModifyGeofences(GeofenceAdapter& adapter,
                                  LocApiBase& api,
                                  LocationAPI* client,
                                  size_t count,
                                  uint32_t* ids,
                                  GeofenceOption* options) :
            LocMsg(),
            mAdapter(adapter),
            mApi(api),
            mClient(client),
            mCount(count),
            mIds(ids),
            mOptions(options) {}
        inline virtual void proc() const  {
            LocationError* errs = new LocationError[mCount];
            if (nullptr == errs) {
                LOC_LOGE("%s]: new failed to allocate errs", __func__);
                return;
            }
            for (size_t i=0; i < mCount; ++i) {
                if (NULL == mIds || NULL == mOptions) {
                    errs[i] = LOCATION_ERROR_INVALID_PARAMETER;
                } else {
                    mApi.addToCallQueue(new LocApiResponse(*mAdapter.getContext(),
                            [&mAdapter = mAdapter, mCount = mCount, mClient = mClient, mIds = mIds,
                            &mApi = mApi, mOptions = mOptions, errs, i] (LocationError err ) {
                        uint32_t hwId = 0;
                        errs[i] = mAdapter.getHwIdFromClient(mClient, mIds[i], hwId);
                        if (LOCATION_ERROR_SUCCESS == errs[i]) {
                            mApi.modifyGeofence(hwId, mIds[i], mOptions[i],
                                    new LocApiResponse(*mAdapter.getContext(),
                                    [&mAdapter = mAdapter, mCount = mCount, mClient = mClient,
                                    mIds = mIds, mOptions = mOptions, hwId, errs, i]
                                    (LocationError err ) {
                                if (LOCATION_ERROR_SUCCESS == err) {
                                    errs[i] = err;

                                    mAdapter.modifyGeofenceItem(hwId, mOptions[i]);
                                }
                                // Send aggregated response on last item and cleanup
                                if (i == mCount-1) {
                                    mAdapter.reportResponse(mClient, mCount, errs, mIds);
                                    delete[] errs;
                                    delete[] mIds;
                                    delete[] mOptions;
                                }
                            }));
                        } else {
                            // Send aggregated response on last item and cleanup
                            if (i == mCount-1) {
                                mAdapter.reportResponse(mClient, mCount, errs, mIds);
                                delete[] errs;
                                delete[] mIds;
                                delete[] mOptions;
                            }
                        }
                    }));
                }
            }
        }
    };

    if (0 == count) {
        return;
    }
    uint32_t* idsCopy = new uint32_t[count];
    if (nullptr == idsCopy) {
        LOC_LOGE("%s]: new failed to allocate idsCopy", __func__);
        return;
    }
    COPY_IF_NOT_NULL(idsCopy, ids, count);
    GeofenceOption* optionsCopy;
    if (options == NULL) {
        optionsCopy = NULL;
    } else {
        optionsCopy = new GeofenceOption[count];
        if (nullptr == optionsCopy) {
            LOC_LOGE("%s]: new failed to allocate optionsCopy", __func__);
            return;
        }
        COPY_IF_NOT_NULL(optionsCopy, options, count);
    }

    sendMsg(new MsgModifyGeofences(*this, *mLocApi, client, count, idsCopy, optionsCopy));
}

void
GeofenceAdapter::saveGeofenceItem(LocationAPI* client, uint32_t clientId, uint32_t hwId,
        const GeofenceOption& options, const GeofenceInfo& info)
{
    LOC_LOGD("%s]: hwId %u client %p clientId %u", __func__, hwId, client, clientId);
    GeofenceKey key(client, clientId);
    GeofenceObject object = {key,
                             options.breachTypeMask,
                             options.responsiveness,
                             options.dwellTime,
                             info.latitude,
                             info.longitude,
                             info.radius,
                             false};
    mGeofences[hwId] = object;
    mGeofenceIds[key] = hwId;
    dump();
}

void
GeofenceAdapter::removeGeofenceItem(uint32_t hwId)
{
    GeofenceKey key;
    LocationError err = getGeofenceKeyFromHwId(hwId, key);
    if (LOCATION_ERROR_SUCCESS != err) {
        LOC_LOGE("%s]: can not find the key for hwId %u", __func__, hwId);
    } else {
        auto it1 = mGeofenceIds.find(key);
        if (it1 != mGeofenceIds.end()) {
            mGeofenceIds.erase(it1);

            auto it2 = mGeofences.find(hwId);
            if (it2 != mGeofences.end()) {
                mGeofences.erase(it2);
                dump();
            } else {
                LOC_LOGE("%s]:geofence item to erase not found. hwId %u", __func__, hwId);
            }
        } else {
            LOC_LOGE("%s]: geofence item to erase not found. hwId %u", __func__, hwId);
        }
    }
}

void
GeofenceAdapter::pauseGeofenceItem(uint32_t hwId)
{
    auto it = mGeofences.find(hwId);
    if (it != mGeofences.end()) {
        it->second.paused = true;
        dump();
    } else {
        LOC_LOGE("%s]: geofence item to pause not found. hwId %u", __func__, hwId);
    }
}

void
GeofenceAdapter::resumeGeofenceItem(uint32_t hwId)
{
    auto it = mGeofences.find(hwId);
    if (it != mGeofences.end()) {
        it->second.paused = false;
        dump();
    } else {
        LOC_LOGE("%s]: geofence item to resume not found. hwId %u", __func__, hwId);
    }
}

void
GeofenceAdapter::modifyGeofenceItem(uint32_t hwId, const GeofenceOption& options)
{
    auto it = mGeofences.find(hwId);
    if (it != mGeofences.end()) {
        it->second.breachMask = options.breachTypeMask;
        it->second.responsiveness = options.responsiveness;
        it->second.dwellTime = options.dwellTime;
        dump();
    } else {
        LOC_LOGE("%s]: geofence item to modify not found. hwId %u", __func__, hwId);
    }
}


void
GeofenceAdapter::geofenceBreachEvent(size_t count, uint32_t* hwIds, Location& location,
        GeofenceBreachType breachType, uint64_t timestamp)
{

    IF_LOC_LOGD {
        std::string idsString = "[";
        if (NULL != hwIds) {
            for (size_t i=0; i < count; ++i) {
                idsString += std::to_string(hwIds[i]) + " ";
            }
        }
        idsString += "]";
        LOC_LOGD("%s]: breachType %u count %zu ids %s",
                 __func__, breachType, count, idsString.c_str());
    }

    if (0 == count || NULL == hwIds)
        return;

    struct MsgGeofenceBreach : public LocMsg {
        GeofenceAdapter& mAdapter;
        size_t mCount;
        uint32_t* mHwIds;
        Location mLocation;
        GeofenceBreachType mBreachType;
        uint64_t mTimestamp;
        inline MsgGeofenceBreach(GeofenceAdapter& adapter,
                                 size_t count,
                                 uint32_t* hwIds,
                                 Location& location,
                                 GeofenceBreachType breachType,
                                 uint64_t timestamp) :
            LocMsg(),
            mAdapter(adapter),
            mCount(count),
            mHwIds(new uint32_t[count]),
            mLocation(location),
            mBreachType(breachType),
            mTimestamp(timestamp)
        {
            if (nullptr == mHwIds) {
                LOC_LOGE("%s]: new failed to allocate mHwIds", __func__);
                return;
            }
            COPY_IF_NOT_NULL(mHwIds, hwIds, mCount);
        }
        inline virtual ~MsgGeofenceBreach() {
            delete[] mHwIds;
        }
        inline virtual void proc() const {
            mAdapter.geofenceBreach(mCount, mHwIds, mLocation, mBreachType, mTimestamp);
        }
    };

    sendMsg(new MsgGeofenceBreach(*this, count, hwIds, location, breachType, timestamp));

}

void
GeofenceAdapter::geofenceBreach(size_t count, uint32_t* hwIds, const Location& location,
        GeofenceBreachType breachType, uint64_t timestamp)
{

    for (auto it = mClientData.begin(); it != mClientData.end(); ++it) {
        uint32_t* clientIds = new uint32_t[count];
        if (nullptr == clientIds) {
            return;
        }
        uint32_t index = 0;
        for (size_t i=0; i < count; ++i) {
            GeofenceKey key;
            LocationError err = getGeofenceKeyFromHwId(hwIds[i], key);
            if (LOCATION_ERROR_SUCCESS == err) {
                if (key.client == it->first) {
                    clientIds[index++] = key.id;
                }
            }
        }
        if (index > 0 && it->second.geofenceBreachCb != nullptr) {
            GeofenceBreachNotification notify = {sizeof(GeofenceBreachNotification),
                                                 index,
                                                 clientIds,
                                                 location,
                                                 breachType,
                                                 timestamp};

            it->second.geofenceBreachCb(notify);
        }
        delete[] clientIds;
    }
}

void
GeofenceAdapter::geofenceStatusEvent(GeofenceStatusAvailable available)
{
    LOC_LOGD("%s]: available %u ", __func__, available);

    struct MsgGeofenceStatus : public LocMsg {
        GeofenceAdapter& mAdapter;
        GeofenceStatusAvailable mAvailable;
        inline MsgGeofenceStatus(GeofenceAdapter& adapter,
                                 GeofenceStatusAvailable available) :
            LocMsg(),
            mAdapter(adapter),
            mAvailable(available) {}
        inline virtual void proc() const {
            mAdapter.geofenceStatus(mAvailable);
        }
    };

    sendMsg(new MsgGeofenceStatus(*this, available));
}

void
GeofenceAdapter::geofenceStatus(GeofenceStatusAvailable available)
{
    for (auto it = mClientData.begin(); it != mClientData.end(); ++it) {
        if (it->second.geofenceStatusCb != nullptr) {
            GeofenceStatusNotification notify = {sizeof(GeofenceStatusNotification),
                                                 available,
                                                 LOCATION_TECHNOLOGY_TYPE_GNSS};
            it->second.geofenceStatusCb(notify);
        }
    }
}

void
GeofenceAdapter::dump()
{
    IF_LOC_LOGV {
        LOC_LOGV(
            "HAL | hwId  | mask | respon | latitude | longitude | radius | paused |  Id  | client");
        for (auto it = mGeofences.begin(); it != mGeofences.end(); ++it) {
            uint32_t hwId = it->first;
            GeofenceObject object = it->second;
            LOC_LOGV("    | %5u | %4u | %6u | %8.2f | %9.2f | %6.2f | %6u | %04x | %p ",
                    hwId, object.breachMask, object.responsiveness,
                    object.latitude, object.longitude, object.radius,
                    object.paused, object.key.id, object.key.client);
        }
    }
}

