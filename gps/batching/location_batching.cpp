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
#include "BatchingAdapter.h"
#include "location_interface.h"

static BatchingAdapter* gBatchingAdapter = NULL;

static void initialize();
static void deinitialize();

static void addClient(LocationAPI* client, const LocationCallbacks& callbacks);
static void removeClient(LocationAPI* client, removeClientCompleteCallback rmClientCb);
static void requestCapabilities(LocationAPI* client);

static uint32_t startBatching(LocationAPI* client, BatchingOptions&);
static void stopBatching(LocationAPI* client, uint32_t id);
static void updateBatchingOptions(LocationAPI* client, uint32_t id, BatchingOptions&);
static void getBatchedLocations(LocationAPI* client, uint32_t id, size_t count);

static const BatchingInterface gBatchingInterface = {
    sizeof(BatchingInterface),
    initialize,
    deinitialize,
    addClient,
    removeClient,
    requestCapabilities,
    startBatching,
    stopBatching,
    updateBatchingOptions,
    getBatchedLocations
};

#ifndef DEBUG_X86
extern "C" const BatchingInterface* getBatchingInterface()
#else
const BatchingInterface* getBatchingInterface()
#endif // DEBUG_X86
{
   return &gBatchingInterface;
}

static void initialize()
{
    if (NULL == gBatchingAdapter) {
        gBatchingAdapter = new BatchingAdapter();
    }
}

static void deinitialize()
{
    if (NULL != gBatchingAdapter) {
        delete gBatchingAdapter;
        gBatchingAdapter = NULL;
    }
}

static void addClient(LocationAPI* client, const LocationCallbacks& callbacks)
{
    if (NULL != gBatchingAdapter) {
        gBatchingAdapter->addClientCommand(client, callbacks);
    }
}

static void removeClient(LocationAPI* client, removeClientCompleteCallback rmClientCb)
{
    if (NULL != gBatchingAdapter) {
        gBatchingAdapter->removeClientCommand(client, rmClientCb);
    }
}

static void requestCapabilities(LocationAPI* client)
{
    if (NULL != gBatchingAdapter) {
        gBatchingAdapter->requestCapabilitiesCommand(client);
    }
}

static uint32_t startBatching(LocationAPI* client, BatchingOptions &batchOptions)
{
    if (NULL != gBatchingAdapter) {
        return gBatchingAdapter->startBatchingCommand(client, batchOptions);
    } else {
        return 0;
    }
}

static void stopBatching(LocationAPI* client, uint32_t id)
{
    if (NULL != gBatchingAdapter) {
        gBatchingAdapter->stopBatchingCommand(client, id);
    }
}

static void updateBatchingOptions(
        LocationAPI* client, uint32_t id, BatchingOptions& batchOptions)
{
    if (NULL != gBatchingAdapter) {
        gBatchingAdapter->updateBatchingOptionsCommand(client, id, batchOptions);
    }
}

static void getBatchedLocations(LocationAPI* client, uint32_t id, size_t count)
{
    if (NULL != gBatchingAdapter) {
        gBatchingAdapter->getBatchedLocationsCommand(client, id, count);
    }
}

