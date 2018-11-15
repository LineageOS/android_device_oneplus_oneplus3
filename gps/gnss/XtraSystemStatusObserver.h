/* Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
#ifndef XTRA_SYSTEM_STATUS_OBS_H
#define XTRA_SYSTEM_STATUS_OBS_H

#include <cinttypes>
#include <MsgTask.h>
#include <LocIpc.h>
#include <LocTimer.h>

using namespace std;
using loc_core::IOsObserver;
using loc_core::IDataItemObserver;
using loc_core::IDataItemCore;
using loc_util::LocIpc;

class XtraSystemStatusObserver : public IDataItemObserver, public LocIpc{
public :
    // constructor & destructor
    inline XtraSystemStatusObserver(IOsObserver* sysStatObs, const MsgTask* msgTask):
            mSystemStatusObsrvr(sysStatObs), mMsgTask(msgTask),
            mGpsLock(-1), mConnections(0), mXtraThrottle(true), mReqStatusReceived(false),
            mDelayLocTimer(*this), mIsConnectivityStatusKnown (false) {
        subscribe(true);
        startListeningNonBlocking(LOC_IPC_HAL);
        mDelayLocTimer.start(100 /*.1 sec*/,  false);
    }
    inline virtual ~XtraSystemStatusObserver() {
        subscribe(false);
        stopListening();
    }

    // IDataItemObserver overrides
    inline virtual void getName(string& name);
    virtual void notify(const list<IDataItemCore*>& dlist);

    bool updateLockStatus(uint32_t lock);
    bool updateConnections(uint64_t allConnections);
    bool updateTac(const string& tac);
    bool updateMccMnc(const string& mccmnc);
    inline const MsgTask* getMsgTask() { return mMsgTask; }
    void subscribe(bool yes);

    void onReceive(const std::string& data) override;

private:
    IOsObserver*    mSystemStatusObsrvr;
    const MsgTask* mMsgTask;
    int32_t mGpsLock;
    uint64_t mConnections;
    string mTac;
    string mMccmnc;
    bool mXtraThrottle;
    bool mReqStatusReceived;
    bool mIsConnectivityStatusKnown;

    class DelayLocTimer : public LocTimer {
        XtraSystemStatusObserver& mXSSO;
    public:
        DelayLocTimer(XtraSystemStatusObserver& xsso) : mXSSO(xsso) {}
        void timeOutCallback() override {
            mXSSO.send(LOC_IPC_XTRA, "halinit");
        }
    } mDelayLocTimer;

    bool onStatusRequested(int32_t xtraStatusUpdated);
};

#endif
