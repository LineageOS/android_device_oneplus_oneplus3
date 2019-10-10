/* Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
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

#ifndef BATCHING_API_CLINET_H
#define BATCHING_API_CLINET_H

#include <android/hardware/gnss/1.0/IGnssBatching.h>
#include <android/hardware/gnss/1.0/IGnssBatchingCallback.h>
#include <pthread.h>

#include <LocationAPIClientBase.h>

namespace android {
namespace hardware {
namespace gnss {
namespace V1_1 {
namespace implementation {

class BatchingAPIClient : public LocationAPIClientBase
{
public:
    BatchingAPIClient(const sp<V1_0::IGnssBatchingCallback>& callback);
    ~BatchingAPIClient();
    int getBatchSize();
    int startSession(const V1_0::IGnssBatching::Options& options);
    int updateSessionOptions(const V1_0::IGnssBatching::Options& options);
    int stopSession();
    void getBatchedLocation(int last_n_locations);
    void flushBatchedLocations();

    inline LocationCapabilitiesMask getCapabilities() { return mLocationCapabilitiesMask; }

    // callbacks
    void onCapabilitiesCb(LocationCapabilitiesMask capabilitiesMask) final;
    void onBatchingCb(size_t count, Location* location, BatchingOptions batchOptions) final;

private:
    sp<V1_0::IGnssBatchingCallback> mGnssBatchingCbIface;
    uint32_t mDefaultId;
    LocationCapabilitiesMask mLocationCapabilitiesMask;
};

}  // namespace implementation
}  // namespace V1_1
}  // namespace gnss
}  // namespace hardware
}  // namespace android
#endif // BATCHING_API_CLINET_H
