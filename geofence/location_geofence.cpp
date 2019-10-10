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
#include "GeofenceAdapter.h"
#include "location_interface.h"

static GeofenceAdapter* gGeofenceAdapter = NULL;

static void initialize();
static void deinitialize();

static void addClient(LocationAPI* client, const LocationCallbacks& callbacks);
static void removeClient(LocationAPI* client, removeClientCompleteCallback rmClientCb);
static void requestCapabilities(LocationAPI* client);

static uint32_t* addGeofences(LocationAPI* client, size_t count, GeofenceOption*, GeofenceInfo*);
static void removeGeofences(LocationAPI* client, size_t count, uint32_t* ids);
static void modifyGeofences(LocationAPI* client, size_t count, uint32_t* ids,
                               GeofenceOption* options);
static void pauseGeofences(LocationAPI* client, size_t count, uint32_t* ids);
static void resumeGeofences(LocationAPI* client, size_t count, uint32_t* ids);

static const GeofenceInterface gGeofenceInterface = {
    sizeof(GeofenceInterface),
    initialize,
    deinitialize,
    addClient,
    removeClient,
    requestCapabilities,
    addGeofences,
    removeGeofences,
    modifyGeofences,
    pauseGeofences,
    resumeGeofences
};

#ifndef DEBUG_X86
extern "C" const GeofenceInterface* getGeofenceInterface()
#else
const GeofenceInterface* getGeofenceInterface()
#endif // DEBUG_X86
{
   return &gGeofenceInterface;
}

static void initialize()
{
    if (NULL == gGeofenceAdapter) {
        gGeofenceAdapter = new GeofenceAdapter();
    }
}

static void deinitialize()
{
    if (NULL != gGeofenceAdapter) {
        delete gGeofenceAdapter;
        gGeofenceAdapter = NULL;
    }
}

static void addClient(LocationAPI* client, const LocationCallbacks& callbacks)
{
    if (NULL != gGeofenceAdapter) {
        gGeofenceAdapter->addClientCommand(client, callbacks);
    }
}

static void removeClient(LocationAPI* client, removeClientCompleteCallback rmClientCb)
{
    if (NULL != gGeofenceAdapter) {
        gGeofenceAdapter->removeClientCommand(client, rmClientCb);
    }
}

static void requestCapabilities(LocationAPI* client)
{
    if (NULL != gGeofenceAdapter) {
        gGeofenceAdapter->requestCapabilitiesCommand(client);
    }
}

static uint32_t* addGeofences(LocationAPI* client, size_t count,
                              GeofenceOption* options, GeofenceInfo* info)
{
    if (NULL != gGeofenceAdapter) {
        return gGeofenceAdapter->addGeofencesCommand(client, count, options, info);
    } else {
        return NULL;
    }
}

static void removeGeofences(LocationAPI* client, size_t count, uint32_t* ids)
{
    if (NULL != gGeofenceAdapter) {
        return gGeofenceAdapter->removeGeofencesCommand(client, count, ids);
    }
}

static void modifyGeofences(LocationAPI* client, size_t count, uint32_t* ids,
                            GeofenceOption* options)
{
    if (NULL != gGeofenceAdapter) {
        return gGeofenceAdapter->modifyGeofencesCommand(client, count, ids, options);
    }
}

static void pauseGeofences(LocationAPI* client, size_t count, uint32_t* ids)
{
    if (NULL != gGeofenceAdapter) {
        return gGeofenceAdapter->pauseGeofencesCommand(client, count, ids);
    }
}

static void resumeGeofences(LocationAPI* client, size_t count, uint32_t* ids)
{
    if (NULL != gGeofenceAdapter) {
        return gGeofenceAdapter->resumeGeofencesCommand(client, count, ids);
    }
}

