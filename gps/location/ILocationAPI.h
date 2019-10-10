/* Copyright (c) 2018 The Linux Foundation. All rights reserved.
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

#ifndef ILOCATIONAPI_H
#define ILOCATIONAPI_H

#include "LocationDataTypes.h"

class ILocationAPI
{
public:
    virtual ~ILocationAPI(){};

    /** @brief Updates/changes the callbacks that will be called.
        mandatory callbacks must be present for callbacks to be successfully updated
        no return value */
    virtual void updateCallbacks(LocationCallbacks&) = 0;

    /* ================================== TRACKING ================================== */

    /** @brief Starts a tracking session, which returns a session id that will be
       used by the other tracking APIs and also in the responseCallback to match command
       with response. locations are reported on the registered trackingCallback
       periodically according to LocationOptions.
       @return session id
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successfully started
                LOCATION_ERROR_ALREADY_STARTED if a startTracking session is already in progress
                LOCATION_ERROR_CALLBACK_MISSING if no trackingCallback was passed
                LOCATION_ERROR_INVALID_PARAMETER if LocationOptions parameter is invalid */
    virtual uint32_t startTracking(TrackingOptions&) = 0;

    /** @brief Stops a tracking session associated with id parameter.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a tracking session */
    virtual void stopTracking(uint32_t id) = 0;

    /** @brief Changes the LocationOptions of a tracking session associated with id.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_INVALID_PARAMETER if LocationOptions parameters are invalid
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a tracking session */
    virtual void updateTrackingOptions(uint32_t id, TrackingOptions&) = 0;

    /* ================================== BATCHING ================================== */

    /** @brief starts a batching session, which returns a session id that will be
       used by the other batching APIs and also in the responseCallback to match command
       with response. locations are reported on the batchingCallback passed in createInstance
       periodically according to LocationOptions. A batching session starts tracking on
       the low power processor and delivers them in batches by the batchingCallback when
       the batch is full or when getBatchedLocations is called. This allows for the processor
       that calls this API to sleep when the low power processor can batch locations in the
       backgroup and wake up the processor calling the API only when the batch is full, thus
       saving power.
       @return session id
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successful
                LOCATION_ERROR_ALREADY_STARTED if a startBatching session is already in progress
                LOCATION_ERROR_CALLBACK_MISSING if no batchingCallback
                LOCATION_ERROR_INVALID_PARAMETER if a parameter is invalid
                LOCATION_ERROR_NOT_SUPPORTED if batching is not supported */
    virtual uint32_t startBatching(BatchingOptions&) = 0;

    /** @brief Stops a batching session associated with id parameter.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with batching session */
    virtual void stopBatching(uint32_t id) = 0;

    /** @brief Changes the LocationOptions of a batching session associated with id.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_INVALID_PARAMETER if LocationOptions parameters are invalid
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a batching session */
    virtual void updateBatchingOptions(uint32_t id, BatchingOptions&) = 0;

    /** @brief Gets a number of locations that are currently stored/batched
       on the low power processor, delivered by the batchingCallback passed in createInstance.
       Location are then deleted from the batch stored on the low power processor.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful, will be followed by batchingCallback call
                LOCATION_ERROR_CALLBACK_MISSING if no batchingCallback
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a batching session */
    virtual void getBatchedLocations(uint32_t id, size_t count) = 0;

    /* ================================== GEOFENCE ================================== */

    /** @brief Adds any number of geofences and returns an array of geofence ids that
       will be used by the other geofence APIs and also in the collectiveResponseCallback to
       match command with response. The geofenceBreachCallback will deliver the status of each
       geofence according to the GeofenceOption for each. The geofence id array returned will
       be valid until the collectiveResponseCallback is called and has returned.
       @return id array
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successful
                LOCATION_ERROR_CALLBACK_MISSING if no geofenceBreachCallback
                LOCATION_ERROR_INVALID_PARAMETER if any parameters are invalid
                LOCATION_ERROR_NOT_SUPPORTED if geofence is not supported */
    virtual uint32_t* addGeofences(size_t count, GeofenceOption*, GeofenceInfo*) = 0;

    /** @brief Removes any number of geofences. Caller should delete ids array after
       removeGeofences returneds.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a geofence session */
    virtual void removeGeofences(size_t count, uint32_t* ids) = 0;

    /** @brief Modifies any number of geofences. Caller should delete ids array after
       modifyGeofences returns.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a geofence session
                LOCATION_ERROR_INVALID_PARAMETER if any parameters are invalid */
    virtual void modifyGeofences(size_t count, uint32_t* ids, GeofenceOption* options) = 0;

    /** @brief Pauses any number of geofences, which is similar to removeGeofences,
       only that they can be resumed at any time. Caller should delete ids array after
       pauseGeofences returns.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a geofence session */
    virtual void pauseGeofences(size_t count, uint32_t* ids) = 0;

    /** @brief Resumes any number of geofences that are currently paused. Caller should
       delete ids array after resumeGeofences returns.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_ID_UNKNOWN if id is not associated with a geofence session */
    virtual void resumeGeofences(size_t count, uint32_t* ids) = 0;

    /* ================================== GNSS ====================================== */

     /** @brief gnssNiResponse is called in response to a gnssNiCallback.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successful
                LOCATION_ERROR_INVALID_PARAMETER if any parameters in GnssNiResponse are invalid
                LOCATION_ERROR_ID_UNKNOWN if id does not match a gnssNiCallback */
    virtual void gnssNiResponse(uint32_t id, GnssNiResponse response) = 0;
};

class ILocationControlAPI
{
public:
    virtual ~ILocationControlAPI(){};

    /** @brief Updates the gnss specific configuration, which returns a session id array
       with an id for each of the bits set in GnssConfig.flags, order from low bits to high bits.
       The response for each config that is set will be returned in collectiveResponseCallback.
       The session id array returned will be valid until the collectiveResponseCallback is called
       and has returned. This effect is global for all clients of ILocationAPI.
        collectiveResponseCallback returns:
                LOCATION_ERROR_SUCCESS if session was successful
                LOCATION_ERROR_INVALID_PARAMETER if any other parameters are invalid
                LOCATION_ERROR_GENERAL_FAILURE if failure for any other reason */
    virtual uint32_t* gnssUpdateConfig(GnssConfig config) = 0;

    /** @brief Delete specific gnss aiding data for testing, which returns a session id
       that will be returned in responseCallback to match command with response.
       Only allowed in userdebug builds. This effect is global for all clients of ILocationAPI.
        responseCallback returns:
                LOCATION_ERROR_SUCCESS if successful
                LOCATION_ERROR_INVALID_PARAMETER if any parameters are invalid
                LOCATION_ERROR_NOT_SUPPORTED if build is not userdebug */
    virtual uint32_t gnssDeleteAidingData(GnssAidingData& data) = 0;
};

#endif /* ILOCATIONAPI_H */
