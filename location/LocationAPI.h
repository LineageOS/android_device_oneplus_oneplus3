/* Copyright (c) 2017-2018 The Linux Foundation. All rights reserved.
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

#ifndef LOCATIONAPI_H
#define LOCATIONAPI_H

#include "ILocationAPI.h"

class LocationAPI : public ILocationAPI
{
private:
    LocationAPI();
    ~LocationAPI();

public:
    /* creates an instance to LocationAPI object.
       Will return NULL if mandatory parameters are invalid or if the maximum number
       of instances have been reached */
    static LocationAPI* createInstance(LocationCallbacks&);

    /* destroy/cleans up the instance, which should be called when LocationControlAPI object is
       no longer needed. LocationControlAPI* returned from createInstance will no longer valid
       after destroy is called.
       If the caller allocates the memory for LocationControlCallbacks used in
       LocationControlAPI::createInstance, then the caller must ensure that the memory still remains
       valid until destroyCompleteCb is invoked.
    */
    void destroy(locationApiDestroyCompleteCallback destroyCompleteCb=nullptr);

    void onRemoveClientCompleteCb (LocationAdapterTypeMask adapterType);

    /* updates/changes the callbacks that will be called.
        mandatory callbacks must be present for callbacks to be successfully updated
        no return value */
    virtual void updateCallbacks(LocationCallbacks&) override;

    /* ================================== TRACKING ================================== */

    /* startTracking starts a tracking session, which returns a session id that will be
       used by the other tracking APIs and also in the responseCallback to match command
       with response. locations are reported on the trackingCallback passed in createInstance
       periodically according to LocationOptions.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successfully started
                LOCATION_ERROR_ALREADY_STARTED if a startTracking session is already in progress
                LOCATION_ERROR_CALLBACK_MISSING if no trackingCallback was passed in createInstance
                LOCATION_ERROR_INVALID_PARAMETER if TrackingOptions parameter is invalid */
    virtual uint32_t startTracking(TrackingOptions&) override;

    /* stopTracking stops a tracking session associated with id parameter.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a tracking session */
    virtual void stopTracking(uint32_t id) override;

    /* updateTrackingOptions changes the TrackingOptions of a tracking session associated with id
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_INVALID_PARAMETER if TrackingOptions parameters are invalid
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a tracking session */
    virtual void updateTrackingOptions(uint32_t id, TrackingOptions&) override;

    /* ================================== BATCHING ================================== */

    /* startBatching starts a batching session, which returns a session id that will be
       used by the other batching APIs and also in the responseCallback to match command
       with response. locations are reported on the batchingCallback passed in createInstance
       periodically according to LocationOptions. A batching session starts tracking on
       the low power processor and delivers them in batches by the batchingCallback when
       the batch is full or when getBatchedLocations is called. This allows for the processor
       that calls this API to sleep when the low power processor can batch locations in the
       backgroup and wake up the processor calling the API only when the batch is full, thus
       saving power
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successful
                LOCATION_ERROR_ALREADY_STARTED if a startBatching session is already in progress
                LOCATION_ERROR_CALLBACK_MISSING if no batchingCallback was passed in createInstance
                LOCATION_ERROR_INVALID_PARAMETER if a parameter is invalid
                LOCATION_ERROR_NOT_SUPPORTED if batching is not supported */
    virtual uint32_t startBatching(BatchingOptions&) override;

    /* stopBatching stops a batching session associated with id parameter.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with batching session */
    virtual void stopBatching(uint32_t id) override;

    /* updateBatchingOptions changes the BatchingOptions of a batching session associated with id
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_INVALID_PARAMETER if BatchingOptions parameters are invalid
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a batching session */
    virtual void updateBatchingOptions(uint32_t id, BatchingOptions&) override;

    /* getBatchedLocations gets a number of locations that are currently stored/batched
       on the low power processor, delivered by the batchingCallback passed in createInstance.
       Location are then deleted from the batch stored on the low power processor.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful, will be followed by batchingCallback call
                LOCATION_ERROR_CALLBACK_MISSING if no batchingCallback was passed in createInstance
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a batching session */
    virtual void getBatchedLocations(uint32_t id, size_t count) override;

    /* ================================== GEOFENCE ================================== */

    /* addGeofences adds any number of geofences and returns an array of geofence ids that
       will be used by the other geofence APIs and also in the collectiveResponseCallback to
       match command with response. The geofenceBreachCallback will deliver the status of each
       geofence according to the GeofenceOption for each. The geofence id array returned will
       be valid until the collectiveResponseCallback is called and has returned.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successful
                LOCATION_ERROR_CALLBACK_MISSING if no geofenceBreachCallback
                LOCATION_ERROR_INVALID_PARAMETER if any parameters are invalid
                LOCATION_ERROR_NOT_SUPPORTED if geofence is not supported */
    virtual uint32_t* addGeofences(size_t count, GeofenceOption*, GeofenceInfo*) override;

    /* removeGeofences removes any number of geofences. Caller should delete ids array after
       removeGeofences returneds.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a geofence session */
    virtual void removeGeofences(size_t count, uint32_t* ids) override;

    /* modifyGeofences modifies any number of geofences. Caller should delete ids array after
       modifyGeofences returns.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a geofence session
                LOCATION_ERROR_INVALID_PARAMETER if any parameters are invalid */
    virtual void modifyGeofences(size_t count, uint32_t* ids, GeofenceOption* options) override;

    /* pauseGeofences pauses any number of geofences, which is similar to removeGeofences,
       only that they can be resumed at any time. Caller should delete ids array after
       pauseGeofences returns.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a geofence session */
    virtual void pauseGeofences(size_t count, uint32_t* ids) override;

    /* resumeGeofences resumes any number of geofences that are currently paused. Caller should
       delete ids array after resumeGeofences returns.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a geofence session */
    virtual void resumeGeofences(size_t count, uint32_t* ids) override;

    /* ================================== GNSS ====================================== */

     /* gnssNiResponse is called in response to a gnssNiCallback.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successful
                LOCATION_ERROR_INVALID_PARAMETER if any parameters in GnssNiResponse are invalid
                LOCATION_ERROR_ID_UNKNOWN if id does not match a gnssNiCallback */
    virtual void gnssNiResponse(uint32_t id, GnssNiResponse response) override;
};

typedef struct {
    size_t size; // set to sizeof(LocationControlCallbacks)
    responseCallback responseCb;                     // mandatory
    collectiveResponseCallback collectiveResponseCb; // mandatory
    gnssConfigCallback gnssConfigCb;                 // optional
} LocationControlCallbacks;

class LocationControlAPI : public ILocationControlAPI
{
private:
    LocationControlAPI();
    ~LocationControlAPI();

public:
    /* creates an instance to LocationControlAPI object.
       Will return NULL if mandatory parameters are invalid or if the maximum number
       of instances have been reached. Only once instance allowed */
    static LocationControlAPI* createInstance(LocationControlCallbacks&);

    /* destroy/cleans up the instance, which should be called when LocationControlAPI object is
       no longer needed. LocationControlAPI* returned from createInstance will no longer valid
       after destroy is called */
    void destroy();

    /* enable will enable specific location technology to be used for calculation locations and
       will effectively start a control session if call is successful, which returns a session id
       that will be returned in responseCallback to match command with response. The session id is
       also needed to call the disable command.
       This effect is global for all clients of LocationAPI
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ALREADY_STARTED if an enable was already called for this techType
                LOCATION_ERROR_INVALID_PARAMETER if any parameters are invalid
                LOCATION_ERROR_GENERAL_FAILURE if failure for any other reason */
    uint32_t enable(LocationTechnologyType techType);

    /* disable will disable specific location technology to be used for calculation locations and
       effectively ends the control session if call is successful.
       id parameter is the session id that was returned in enable responseCallback for techType.
       The session id is no longer valid after disable's responseCallback returns success.
       This effect is global for all clients of LocationAPI
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id was not returned from responseCallback from enable
                LOCATION_ERROR_GENERAL_FAILURE if failure for any other reason */
    void disable(uint32_t id);

    /* gnssUpdateConfig updates the gnss specific configuration, which returns a session id array
       with an id for each of the bits set in GnssConfig.flags, order from low bits to high bits.
       The response for each config that is set will be returned in collectiveResponseCallback.
       The session id array returned will be valid until the collectiveResponseCallback is called
       and has returned. This effect is global for all clients of LocationAPI
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successful
                LOCATION_ERROR_INVALID_PARAMETER if any other parameters are invalid
                LOCATION_ERROR_GENERAL_FAILURE if failure for any other reason */
    virtual uint32_t* gnssUpdateConfig(GnssConfig config) override;

    /* gnssGetConfig fetches the current constellation and SV configuration
       on the GNSS engine.
       Returns a session id array with an id for each of the bits set in
       the mask parameter, order from low bits to high bits.
       Response is sent via the registered gnssConfigCallback.
       This effect is global for all clients of LocationAPI
       collectiveResponseCallback returns:
           LOCATION_ERROR_SUCCESS if session was successful
           LOCATION_ERROR_INVALID_PARAMETER if any parameter is invalid
           LOCATION_ERROR_CALLBACK_MISSING If no gnssConfigCallback
                                           was passed in createInstance
           LOCATION_ERROR_NOT_SUPPORTED If read of requested configuration
                                        is not supported */
    uint32_t* gnssGetConfig(GnssConfigFlagsMask mask);

    /* delete specific gnss aiding data for testing, which returns a session id
       that will be returned in responseCallback to match command with response.
       Only allowed in userdebug builds. This effect is global for all clients of LocationAPI
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_INVALID_PARAMETER if any parameters are invalid
                LOCATION_ERROR_NOT_SUPPORTED if build is not userdebug */
    virtual uint32_t gnssDeleteAidingData(GnssAidingData& data) override;
};

#endif /* LOCATIONAPI_H */
