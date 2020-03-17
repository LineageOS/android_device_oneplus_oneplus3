/*
 * Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
#ifndef DBG
    #define DBG true
#endif /* DBG */
#define LOG_TAG "IPAHALService"

/* HIDL Includes */
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>

/* Kernel Includes */
#include <linux/netfilter/nfnetlink_compat.h>

/* External Includes */
#include <cutils/log.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

/* Internal Includes */
#include "HAL.h"
#include "LocalLogBuffer.h"
#include "PrefixParser.h"

/* Namespace pollution avoidance */
using ::android::hardware::Void;
using ::android::status_t;

using RET = ::IOffloadManager::RET;
using Prefix = ::IOffloadManager::Prefix;

using ::std::map;
using ::std::vector;


/* ------------------------------ PUBLIC ------------------------------------ */
HAL* HAL::makeIPAHAL(int version, IOffloadManager* mgr) {
    android::hardware::ProcessState::initWithMmapSize((size_t)(2 * KERNEL_PAGE));

    if (DBG)
        ALOGI("makeIPAHAL(%d, %s)", version,
                (mgr != nullptr) ? "provided" : "null");
    if (nullptr == mgr) return NULL;
    else if (version != 1) return NULL;
    HAL* ret = new HAL(mgr);
    if (nullptr == ret) return NULL;
    configureRpcThreadpool(1, false);
    ret->registerAsSystemService("ipacm");
    return ret;
} /* makeIPAHAL */


/* ------------------------------ PRIVATE ----------------------------------- */
HAL::HAL(IOffloadManager* mgr) : mLogs("HAL Function Calls", 50) {
    mIPA = mgr;
    mCb.clear();
    mCbIpa = nullptr;
    mCbCt = nullptr;
} /* HAL */

void HAL::registerAsSystemService(const char* name) {
    status_t ret = 0;

    ret = IOffloadControl::registerAsService();
    if (ret != 0) ALOGE("Failed to register IOffloadControl (%d) name(%s)", ret, name);
    else if (DBG) {
        ALOGI("Successfully registered IOffloadControl");
    }

    ret = IOffloadConfig::registerAsService();
    if (ret != 0) ALOGE("Failed to register IOffloadConfig (%d)", ret);
    else if (DBG) {
        ALOGI("Successfully registered IOffloadConfig");
    }
} /* registerAsSystemService */

void HAL::doLogcatDump() {
    ALOGD("mHandles");
    ALOGD("========");
    /* @TODO This will segfault if they aren't initialized and I don't currently
     * care to check for initialization in a function that isn't used anyways
     * ALOGD("fd1->%d", mHandle1->data[0]);
     * ALOGD("fd2->%d", mHandle2->data[0]);
     */
    ALOGD("========");
} /* doLogcatDump */

HAL::BoolResult HAL::makeInputCheckFailure(string customErr) {
    BoolResult ret;
    ret.success = false;
    ret.errMsg = "Failed Input Checks: " + customErr;
    return ret;
} /* makeInputCheckFailure */

HAL::BoolResult HAL::ipaResultToBoolResult(RET in) {
    BoolResult ret;
    ret.success = (in >= RET::SUCCESS);
    switch (in) {
        case RET::FAIL_TOO_MANY_PREFIXES:
            ret.errMsg = "Too Many Prefixes Provided";
            break;
        case RET::FAIL_UNSUPPORTED:
            ret.errMsg = "Unsupported by Hardware";
            break;
        case RET::FAIL_INPUT_CHECK:
            ret.errMsg = "Failed Input Checks";
            break;
        case RET::FAIL_HARDWARE:
            ret.errMsg = "Hardware did not accept";
            break;
        case RET::FAIL_TRY_AGAIN:
            ret.errMsg = "Try Again";
            break;
        case RET::SUCCESS:
            ret.errMsg = "Successful";
            break;
        case RET::SUCCESS_DUPLICATE_CONFIG:
            ret.errMsg = "Successful: Was a duplicate configuration";
            break;
        case RET::SUCCESS_NO_OP:
            ret.errMsg = "Successful: No action needed";
            break;
        case RET::SUCCESS_OPTIMIZED:
            ret.errMsg = "Successful: Performed optimized version of action";
            break;
        default:
            ret.errMsg = "Unknown Error";
            break;
    }
    return ret;
} /* ipaResultToBoolResult */

/* This will likely always result in doubling the number of loops the execution
 * goes through.  Obviously that is suboptimal.  But if we first translate
 * away from all HIDL specific code, then we can avoid sprinkling HIDL
 * dependencies everywhere.
 */
vector<string> HAL::convertHidlStrToStdStr(hidl_vec<hidl_string> in) {
    vector<string> ret;
    for (size_t i = 0; i < in.size(); i++) {
        string add = in[i];
        ret.push_back(add);
    }
    return ret;
} /* convertHidlStrToStdStr */

void HAL::registerEventListeners() {
    registerIpaCb();
    registerCtCb();
} /* registerEventListeners */

void HAL::registerIpaCb() {
    if (isInitialized() && mCbIpa == nullptr) {
        LocalLogBuffer::FunctionLog fl("registerEventListener");
        mCbIpa = new IpaEventRelay(mCb);
        mIPA->registerEventListener(mCbIpa);
        mLogs.addLog(fl);
    } else {
        ALOGE("Failed to registerIpaCb (isInitialized()=%s, (mCbIpa == nullptr)=%s)",
                isInitialized() ? "true" : "false",
                (mCbIpa == nullptr) ? "true" : "false");
    }
} /* registerIpaCb */

void HAL::registerCtCb() {
    if (isInitialized() && mCbCt == nullptr) {
        LocalLogBuffer::FunctionLog fl("registerCtTimeoutUpdater");
        mCbCt = new CtUpdateAmbassador(mCb);
        mIPA->registerCtTimeoutUpdater(mCbCt);
        mLogs.addLog(fl);
    } else {
        ALOGE("Failed to registerCtCb (isInitialized()=%s, (mCbCt == nullptr)=%s)",
                isInitialized() ? "true" : "false",
                (mCbCt == nullptr) ? "true" : "false");
    }
} /* registerCtCb */

void HAL::unregisterEventListeners() {
    unregisterIpaCb();
    unregisterCtCb();
} /* unregisterEventListeners */

void HAL::unregisterIpaCb() {
    if (mCbIpa != nullptr) {
        LocalLogBuffer::FunctionLog fl("unregisterEventListener");
        mIPA->unregisterEventListener(mCbIpa);
        mCbIpa = nullptr;
        mLogs.addLog(fl);
    } else {
        ALOGE("Failed to unregisterIpaCb");
    }
} /* unregisterIpaCb */

void HAL::unregisterCtCb() {
    if (mCbCt != nullptr) {
        LocalLogBuffer::FunctionLog fl("unregisterCtTimeoutUpdater");
        mIPA->unregisterCtTimeoutUpdater(mCbCt);
        mCbCt = nullptr;
        mLogs.addLog(fl);
    } else {
        ALOGE("Failed to unregisterCtCb");
    }
} /* unregisterCtCb */

void HAL::clearHandles() {
    ALOGI("clearHandles()");
    /* @TODO handle this more gracefully... also remove the log
     *
     * Things that would be nice, but I can't do:
     * [1] Destroy the object (it's on the stack)
     * [2] Call freeHandle (it's private)
     *
     * Things I can do but are hacks:
     * [1] Look at code and notice that setTo immediately calls freeHandle
     */
    mHandle1.setTo(nullptr, true);
    mHandle2.setTo(nullptr, true);
} /* clearHandles */

bool HAL::isInitialized() {
    return mCb.get() != nullptr;
} /* isInitialized */


/* -------------------------- IOffloadConfig -------------------------------- */
Return<void> HAL::setHandles(
    const hidl_handle &fd1,
    const hidl_handle &fd2,
    setHandles_cb hidl_cb
) {
    LocalLogBuffer::FunctionLog fl(__func__);

    if (fd1->numFds != 1) {
        BoolResult res = makeInputCheckFailure("Must provide exactly one FD per handle (fd1)");
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);

        mLogs.addLog(fl);
        return Void();
    }

    if (fd2->numFds != 1) {
        BoolResult res = makeInputCheckFailure("Must provide exactly one FD per handle (fd2)");
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);

        mLogs.addLog(fl);
        return Void();
    }

    /* The = operator calls freeHandle internally.  Therefore, if we were using
     * these handles previously, they're now gone... forever.  But hopefully the
     * new ones kick in very quickly.
     *
     * After freeing anything previously held, it will dup the FD so we have our
     * own copy.
     */
    mHandle1 = fd1;
    mHandle2 = fd2;

    /* Log the DUPed FD instead of the actual input FD so that we can lookup
     * this value in ls -l /proc/<pid>/<fd>
     */
    fl.addArg("fd1", mHandle1->data[0]);
    fl.addArg("fd2", mHandle2->data[0]);

    /* Try to provide each handle to IPACM.  Destroy our DUPed hidl_handles if
     * IPACM does not like either input.  This keeps us from leaking FDs or
     * providing half solutions.
     *
     * @TODO unfortunately, this does not cover duplicate configs where IPACM
     * thinks it is still holding on to a handle that we would have freed above.
     * It also probably means that IPACM would not know about the first FD being
     * freed if it rejects the second FD.
     */
    RET ipaReturn = mIPA->provideFd(mHandle1->data[0], UDP_SUBSCRIPTIONS);
    if (ipaReturn == RET::SUCCESS) {
        ipaReturn = mIPA->provideFd(mHandle2->data[0], TCP_SUBSCRIPTIONS);
    }

    if (ipaReturn != RET::SUCCESS) {
        ALOGE("IPACM failed to accept the FDs (%d %d)", mHandle1->data[0],
                mHandle2->data[0]);
        clearHandles();
    } else {
        /* @TODO remove logs after stabilization */
        ALOGI("IPACM was provided two FDs (%d, %d)", mHandle1->data[0],
                mHandle2->data[0]);
    }

    BoolResult res = ipaResultToBoolResult(ipaReturn);
    hidl_cb(res.success, res.errMsg);

    fl.setResult(res.success, res.errMsg);
    mLogs.addLog(fl);
    return Void();
} /* setHandles */


/* -------------------------- IOffloadControl ------------------------------- */
Return<void> HAL::initOffload
(
    const ::android::sp<ITetheringOffloadCallback>& cb,
    initOffload_cb hidl_cb
) {
    LocalLogBuffer::FunctionLog fl(__func__);

    if (isInitialized()) {
        BoolResult res = makeInputCheckFailure("Already initialized");
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
        mLogs.addLog(fl);
    } else {
        /* Should storing the CB be a function? */
        mCb = cb;
        registerEventListeners();
        BoolResult res = ipaResultToBoolResult(RET::SUCCESS);
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
        mLogs.addLog(fl);
    }

    return Void();
} /* initOffload */

Return<void> HAL::stopOffload
(
    stopOffload_cb hidl_cb
) {
    LocalLogBuffer::FunctionLog fl(__func__);

    if (!isInitialized()) {
        BoolResult res = makeInputCheckFailure("Was never initialized");
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
        mLogs.addLog(fl);
    } else {
        /* Should removing the CB be a function? */
        mCb.clear();
        unregisterEventListeners();

        RET ipaReturn = mIPA->stopAllOffload();
        if (ipaReturn != RET::SUCCESS) {
            /* Ignore IPAs return value here and provide why stopAllOffload
             * failed.  However, if IPA failed to clearAllFds, then we can't
             * clear our map because they may still be in use.
             */
            RET ret = mIPA->clearAllFds();
            if (ret == RET::SUCCESS) {
                clearHandles();
            }
        } else {
            ipaReturn = mIPA->clearAllFds();
            /* If IPA fails, they may still be using these for some reason. */
            if (ipaReturn == RET::SUCCESS) {
                clearHandles();
            } else {
                ALOGE("IPACM failed to return success for clearAllFds so they will not be released...");
            }
        }

        BoolResult res = ipaResultToBoolResult(ipaReturn);
        hidl_cb(res.success, res.errMsg);

        fl.setResult(res.success, res.errMsg);
        mLogs.addLog(fl);
    }

    return Void();
} /* stopOffload */

Return<void> HAL::setLocalPrefixes
(
    const hidl_vec<hidl_string>& prefixes,
    setLocalPrefixes_cb hidl_cb
) {
    BoolResult res;
    PrefixParser parser;
    vector<string> prefixesStr = convertHidlStrToStdStr(prefixes);

    LocalLogBuffer::FunctionLog fl(__func__);
    fl.addArg("prefixes", prefixesStr);

    memset(&res,0,sizeof(BoolResult));

    if (!isInitialized()) {
        BoolResult res = makeInputCheckFailure("Not initialized");
    } else if(prefixesStr.size() < 1) {
        res = ipaResultToBoolResult(RET::FAIL_INPUT_CHECK);
    } else if (!parser.add(prefixesStr)) {
        res = makeInputCheckFailure(parser.getLastErrAsStr());
    } else {
        res = ipaResultToBoolResult(RET::SUCCESS);
    }

    hidl_cb(res.success, res.errMsg);
    fl.setResult(res.success, res.errMsg);
    mLogs.addLog(fl);
    return Void();
} /* setLocalPrefixes */

Return<void> HAL::getForwardedStats
(
    const hidl_string& upstream,
    getForwardedStats_cb hidl_cb
) {
    LocalLogBuffer::FunctionLog fl(__func__);
    fl.addArg("upstream", upstream);

    OffloadStatistics ret;
    RET ipaReturn = mIPA->getStats(upstream.c_str(), true, ret);
    if (ipaReturn == RET::SUCCESS) {
        hidl_cb(ret.getTotalRxBytes(), ret.getTotalTxBytes());
        fl.setResult(ret.getTotalRxBytes(), ret.getTotalTxBytes());
    } else {
        /* @TODO Ensure the output is zeroed, but this is probably not enough to
         * tell Framework that an error has occurred.  If, for example, they had
         * not yet polled for statistics previously, they may incorrectly assume
         * that simply no statistics have transpired on hardware path.
         *
         * Maybe ITetheringOffloadCallback:onEvent(OFFLOAD_STOPPED_ERROR) is
         * enough to handle this case, time will tell.
         */
        hidl_cb(0, 0);
        fl.setResult(0, 0);
    }

    mLogs.addLog(fl);
    return Void();
} /* getForwardedStats */

Return<void> HAL::setDataLimit
(
    const hidl_string& upstream,
    uint64_t limit,
    setDataLimit_cb hidl_cb
) {
    LocalLogBuffer::FunctionLog fl(__func__);
    fl.addArg("upstream", upstream);
    fl.addArg("limit", limit);

    if (!isInitialized()) {
        BoolResult res = makeInputCheckFailure("Not initialized (setDataLimit)");
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
    } else {
        RET ipaReturn = mIPA->setQuota(upstream.c_str(), limit);
        if(ipaReturn == RET::FAIL_TRY_AGAIN) {
            ipaReturn = RET::SUCCESS;
        }
        BoolResult res = ipaResultToBoolResult(ipaReturn);
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
    }

    mLogs.addLog(fl);
    return Void();
} /* setDataLimit */

Return<void> HAL::setUpstreamParameters
(
    const hidl_string& iface,
    const hidl_string& v4Addr,
    const hidl_string& v4Gw,
    const hidl_vec<hidl_string>& v6Gws,
    setUpstreamParameters_cb hidl_cb
) {
    vector<string> v6GwStrs = convertHidlStrToStdStr(v6Gws);

    LocalLogBuffer::FunctionLog fl(__func__);
    fl.addArg("iface", iface);
    fl.addArg("v4Addr", v4Addr);
    fl.addArg("v4Gw", v4Gw);
    fl.addArg("v6Gws", v6GwStrs);

    PrefixParser v4AddrParser;
    PrefixParser v4GwParser;
    PrefixParser v6GwParser;

    /* @TODO maybe we should enforce that these addresses and gateways are fully
     * qualified here.  But then, how do we allow them to be empty/null as well
     * while still preserving a sane API on PrefixParser?
     */
    if (!isInitialized()) {
        BoolResult res = makeInputCheckFailure("Not initialized (setUpstreamParameters)");
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
    } else if (!v4AddrParser.addV4(v4Addr) && !v4Addr.empty()) {
        BoolResult res = makeInputCheckFailure(v4AddrParser.getLastErrAsStr());
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
    } else if (!v4GwParser.addV4(v4Gw) && !v4Gw.empty()) {
        BoolResult res = makeInputCheckFailure(v4GwParser.getLastErrAsStr());
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
    } else if (v6GwStrs.size() >= 1 && !v6GwParser.addV6(v6GwStrs)) {
        BoolResult res = makeInputCheckFailure(v6GwParser.getLastErrAsStr());
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
    } else if (iface.size()>= 1) {
        RET ipaReturn = mIPA->setUpstream(
                iface.c_str(),
                v4GwParser.getFirstPrefix(),
                v6GwParser.getFirstPrefix());
        BoolResult res = ipaResultToBoolResult(ipaReturn);
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
    } else {
	/* send NULL iface string when upstream down */
        RET ipaReturn = mIPA->setUpstream(
                NULL,
                v4GwParser.getFirstPrefix(IP_FAM::V4),
                v6GwParser.getFirstPrefix(IP_FAM::V6));
        BoolResult res = ipaResultToBoolResult(ipaReturn);
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
    }

    mLogs.addLog(fl);
    return Void();
} /* setUpstreamParameters */

Return<void> HAL::addDownstream
(
    const hidl_string& iface,
    const hidl_string& prefix,
    addDownstream_cb hidl_cb
) {
    LocalLogBuffer::FunctionLog fl(__func__);
    fl.addArg("iface", iface);
    fl.addArg("prefix", prefix);

    PrefixParser prefixParser;

    if (!isInitialized()) {
        BoolResult res = makeInputCheckFailure("Not initialized (setUpstreamParameters)");
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
    }
    else if (!prefixParser.add(prefix)) {
        BoolResult res = makeInputCheckFailure(prefixParser.getLastErrAsStr());
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
    } else {
        RET ipaReturn = mIPA->addDownstream(
                iface.c_str(),
                prefixParser.getFirstPrefix());
        BoolResult res = ipaResultToBoolResult(ipaReturn);
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
    }

    mLogs.addLog(fl);
    return Void();
} /* addDownstream */

Return<void> HAL::removeDownstream
(
    const hidl_string& iface,
    const hidl_string& prefix,
    removeDownstream_cb hidl_cb
) {
    LocalLogBuffer::FunctionLog fl(__func__);
    fl.addArg("iface", iface);
    fl.addArg("prefix", prefix);

    PrefixParser prefixParser;

    if (!isInitialized()) {
        BoolResult res = makeInputCheckFailure("Not initialized (setUpstreamParameters)");
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
    }
    else if (!prefixParser.add(prefix)) {
        BoolResult res = makeInputCheckFailure(prefixParser.getLastErrAsStr());
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
    } else {
        RET ipaReturn = mIPA->removeDownstream(
                iface.c_str(),
                prefixParser.getFirstPrefix());
        BoolResult res = ipaResultToBoolResult(ipaReturn);
        hidl_cb(res.success, res.errMsg);
        fl.setResult(res.success, res.errMsg);
    }

    mLogs.addLog(fl);
    return Void();
} /* removeDownstream */
