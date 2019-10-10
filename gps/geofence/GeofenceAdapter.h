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
#ifndef GEOFENCE_ADAPTER_H
#define GEOFENCE_ADAPTER_H

#include <LocAdapterBase.h>
#include <LocContext.h>
#include <LocationAPI.h>
#include <map>

using namespace loc_core;

#define COPY_IF_NOT_NULL(dest, src, len) do { \
    if (NULL!=dest && NULL!=src) { \
        for (size_t i=0; i<len; ++i) { \
            dest[i] = src[i]; \
        } \
    } \
} while (0)

typedef struct GeofenceKey {
    LocationAPI* client;
    uint32_t id;
    inline GeofenceKey() :
        client(NULL), id(0) {}
    inline GeofenceKey(LocationAPI* _client, uint32_t _id) :
        client(_client), id(_id) {}
} GeofenceKey;
inline bool operator <(GeofenceKey const& left, GeofenceKey const& right) {
    return left.id < right.id || (left.id == right.id && left.client < right.client);
}
inline bool operator ==(GeofenceKey const& left, GeofenceKey const& right) {
    return left.id == right.id && left.client == right.client;
}
inline bool operator !=(GeofenceKey const& left, GeofenceKey const& right) {
    return left.id != right.id || left.client != right.client;
}
typedef struct {
    GeofenceKey key;
    GeofenceBreachTypeMask breachMask;
    uint32_t responsiveness;
    uint32_t dwellTime;
    double latitude;
    double longitude;
    double radius;
    bool paused;
} GeofenceObject;
typedef std::map<uint32_t, GeofenceObject> GeofencesMap; //map of hwId to GeofenceObject
typedef std::map<GeofenceKey, uint32_t> GeofenceIdMap; //map of GeofenceKey to hwId

class GeofenceAdapter : public LocAdapterBase {

    /* ==== GEOFENCES ====================================================================== */
    GeofencesMap mGeofences; //map hwId to GeofenceObject
    GeofenceIdMap mGeofenceIds; //map of GeofenceKey to hwId

protected:

    /* ==== CLIENT ========================================================================= */
    virtual void updateClientsEventMask();
    virtual void stopClientSessions(LocationAPI* client);

public:

    GeofenceAdapter();
    virtual ~GeofenceAdapter() {}

    /* ==== SSR ============================================================================ */
    /* ======== EVENTS ====(Called from QMI Thread)========================================= */
    virtual void handleEngineUpEvent();
    /* ======== UTILITIES ================================================================== */
    void restartGeofences();

    /* ==== GEOFENCES ====================================================================== */
    /* ======== COMMANDS ====(Called from Client Thread)==================================== */
    uint32_t* addGeofencesCommand(LocationAPI* client, size_t count,
                                  GeofenceOption* options, GeofenceInfo* info);
    void removeGeofencesCommand(LocationAPI* client, size_t count, uint32_t* ids);
    void pauseGeofencesCommand(LocationAPI* client, size_t count, uint32_t* ids);
    void resumeGeofencesCommand(LocationAPI* client, size_t count, uint32_t* ids);
    void modifyGeofencesCommand(LocationAPI* client, size_t count, uint32_t* ids,
                                GeofenceOption* options);
    /* ======== RESPONSES ================================================================== */
    void reportResponse(LocationAPI* client, size_t count, LocationError* errs, uint32_t* ids);
    /* ======== UTILITIES ================================================================== */
    void saveGeofenceItem(LocationAPI* client,
                          uint32_t clientId,
                          uint32_t hwId,
                          const GeofenceOption& options,
                          const GeofenceInfo& info);
    void removeGeofenceItem(uint32_t hwId);
    void pauseGeofenceItem(uint32_t hwId);
    void resumeGeofenceItem(uint32_t hwId);
    void modifyGeofenceItem(uint32_t hwId, const GeofenceOption& options);
    LocationError getHwIdFromClient(LocationAPI* client, uint32_t clientId, uint32_t& hwId);
    LocationError getGeofenceKeyFromHwId(uint32_t hwId, GeofenceKey& key);
    void dump();

    /* ==== REPORTS ======================================================================== */
    /* ======== EVENTS ====(Called from QMI Thread)========================================= */
    void geofenceBreachEvent(size_t count, uint32_t* hwIds, Location& location,
                             GeofenceBreachType breachType, uint64_t timestamp);
    void geofenceStatusEvent(GeofenceStatusAvailable available);
    /* ======== UTILITIES ================================================================== */
    void geofenceBreach(size_t count, uint32_t* hwIds, const Location& location,
                        GeofenceBreachType breachType, uint64_t timestamp);
    void geofenceStatus(GeofenceStatusAvailable available);
};

#endif /* GEOFENCE_ADAPTER_H */
