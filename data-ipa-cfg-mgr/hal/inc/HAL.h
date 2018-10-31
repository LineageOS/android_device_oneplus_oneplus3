/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
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
#ifndef _HAL_H_
#define _HAL_H_

/* HIDL Includes */
#include <android/hardware/tetheroffload/config/1.0/IOffloadConfig.h>
#include <android/hardware/tetheroffload/control/1.0/IOffloadControl.h>
#include <hidl/HidlTransportSupport.h>

/* External Includes */
#include <string>
#include <vector>

/* Internal Includes */
#include "CtUpdateAmbassador.h"
#include "IOffloadManager.h"
#include "IpaEventRelay.h"
#include "LocalLogBuffer.h"

/* Avoid the namespace litering everywhere */
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;
using ::android::hardware::Return;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;

using RET = ::IOffloadManager::RET;
using Prefix = ::IOffloadManager::Prefix;

using ::std::map;
using ::std::string;
using ::std::vector;

using ::android::hardware::tetheroffload::config::V1_0::IOffloadConfig;
using ::android::hardware::tetheroffload::control::V1_0::IOffloadControl;

using ::android::hardware::tetheroffload::control::V1_0::ITetheringOffloadCallback;

#define KERNEL_PAGE 4096

class HAL : public IOffloadControl, IOffloadConfig {
public:
    /* Static Const Definitions */
    static const uint32_t UDP_SUBSCRIPTIONS =
            NF_NETLINK_CONNTRACK_NEW | NF_NETLINK_CONNTRACK_DESTROY;
    static const uint32_t TCP_SUBSCRIPTIONS =
            NF_NETLINK_CONNTRACK_UPDATE | NF_NETLINK_CONNTRACK_DESTROY;

    /* Interface to IPACM */
    /**
     * @TODO This will likely need to be extended into a proper FactoryPattern
     * when version bumps are needed.
     *
     * This makeIPAHAL function would move to a HALFactory class.  Each HAL could
     * then be versioned (class HAL_V1, class HAL_V2, etc) and inherit from a base class HAL.
     * Then the version number in this function could be used to decide which one to return
     * (if any).
     *
     * IPACM does not need to talk directly back to the returned HAL class.  The other methods that
     * IPACM needs to call are covered by registering the event listeners.  If IPACM did need to
     * talk directly back to the HAL object, without HAL registering a callback, these methods would
     * need to be defined in the HAL base class.
     *
     * This would slightly break backwards compatibility so it should be discouraged; however, the
     * base class could define a sane default implementation and not require that the child class
     * implement this new method.  This "sane default implementation" might only be possible in the
     * case of listening to async events; if IPACM needs to query something, then this would not
     * be backwards compatible and should be done via registering a callback so that IPACM knows
     * this version of HAL supports that functionality.
     *
     * The above statements assume that only one version of the HAL will be instantiated at a time.
     * Yet, it seems possible that a HAL_V1 and HAL_V2 service could both be registered, extending
     * support to both old and new client implementations.  It would be difficult to multiplex
     * information from both versions.  Additionally, IPACM would be responsible for instantiating
     * two HALs (makeIPAHAL(1, ...); makeIPAHAL(2, ...)) which makes signaling between HAL versions
     * (see next paragraph) slightly more difficult but not impossible.
     *
     * If concurrent versions of HAL are required, there will likely need to only be one master.
     * Whichever version of HAL receives a client first may be allowed to take over control while
     * other versions would be required to return failures (ETRYAGAIN: another version in use) until
     * that version of the client relinquishes control.  This should work seemlessly because we
     * currently have an assumption that only one client will be present in system image.
     * Logically, that client will have only a single version (or if it supports multiple, it will
     * always attempt the newest version of HAL before falling back) and therefore no version
     * collisions could possibly occur.
     *
     * Dislaimer:
     * ==========
     * Supporting multiple versions of an interface, in the same code base, at runtime, comes with a
     * significant carrying cost and overhead in the form of developer headaches.  This should not
     * be done lightly and should be extensively scoped before committing to the effort.
     *
     * Perhaps the notion of minor version could be introduced to bridge the gaps created above.
     * For example, 1.x and 1.y could be ran concurrently and supported from the same IPACM code.
     * Yet, a major version update, would not be backwards compatible.  This means that a 2.x HAL
     * could not linked into the same IPACM code base as a 1.x HAL.
     */
    static HAL* makeIPAHAL(int /* version */, IOffloadManager* /* mgr */);

    /* IOffloadConfig */
    Return<void> setHandles(
            const hidl_handle& /* fd1 */,
            const hidl_handle& /* fd2 */,
            setHandles_cb /* hidl_cb */);

    /* IOffloadControl */
    Return<void> initOffload(
            const ::android::sp<ITetheringOffloadCallback>& /* cb */,
            initOffload_cb /* hidl_cb */);
    Return<void> stopOffload(
            stopOffload_cb /* hidl_cb */);
    Return<void> setLocalPrefixes(
            const hidl_vec<hidl_string>& /* prefixes */,
            setLocalPrefixes_cb /* hidl_cb */);
    Return<void> getForwardedStats(
            const hidl_string& /* upstream */,
            getForwardedStats_cb /* hidl_cb */);
    Return<void> setDataLimit(
            const hidl_string& /* upstream */,
            uint64_t /* limit */,
            setDataLimit_cb /* hidl_cb */);
    Return<void> setUpstreamParameters(
            const hidl_string& /* iface */,
            const hidl_string& /* v4Addr */,
            const hidl_string& /* v4Gw */,
            const hidl_vec<hidl_string>& /* v6Gws */,
            setUpstreamParameters_cb /* hidl_cb */);
    Return<void> addDownstream(
            const hidl_string& /* iface */,
            const hidl_string& /* prefix */,
            addDownstream_cb /* hidl_cb */);
    Return<void> removeDownstream(
            const hidl_string& /* iface */,
            const hidl_string& /* prefix */,
            removeDownstream_cb /* hidl_cb */);

private:
    typedef struct BoolResult {
        bool success;
        string errMsg;
    } boolResult_t;

    HAL(IOffloadManager* /* mgr */);
    void registerAsSystemService(const char* /* name */);

    void doLogcatDump();

    static BoolResult makeInputCheckFailure(string /* customErr */);
    static BoolResult ipaResultToBoolResult(RET /* in */);

    static vector<string> convertHidlStrToStdStr(hidl_vec<hidl_string> /* in */);

    void registerEventListeners();
    void registerIpaCb();
    void registerCtCb();
    void unregisterEventListeners();
    void unregisterIpaCb();
    void unregisterCtCb();

    void clearHandles();

    bool isInitialized();

    IOffloadManager* mIPA;
    hidl_handle mHandle1;
    hidl_handle mHandle2;
    LocalLogBuffer mLogs;
    ::android::sp<ITetheringOffloadCallback> mCb;
    IpaEventRelay *mCbIpa;
    CtUpdateAmbassador *mCbCt;
}; /* HAL */
#endif /* _HAL_H_ */
