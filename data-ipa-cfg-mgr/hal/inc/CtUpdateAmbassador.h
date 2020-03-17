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
#ifndef _CT_UPDATE_AMBASSADOR_H_
#define _CT_UPDATE_AMBASSADOR_H_
/* External Includes */
#include <hidl/HidlTransportSupport.h>

/* HIDL Includes */
#include <android/hardware/tetheroffload/control/1.0/ITetheringOffloadCallback.h>

/* Internal Includes */
#include "IOffloadManager.h"

/* Namespace pollution avoidance */
using ::android::hardware::tetheroffload::control::V1_0::ITetheringOffloadCallback;
using ::android::hardware::tetheroffload::control::V1_0::NetworkProtocol;
using HALIpAddrPortPair = ::android::hardware::tetheroffload::control::V1_0::IPv4AddrPortPair;
using HALNatTimeoutUpdate = ::android::hardware::tetheroffload::control::V1_0::NatTimeoutUpdate;

using IpaIpAddrPortPair = ::IOffloadManager::ConntrackTimeoutUpdater::IpAddrPortPair;
using IpaNatTimeoutUpdate = ::IOffloadManager::ConntrackTimeoutUpdater::NatTimeoutUpdate;
using IpaL4Protocol = ::IOffloadManager::ConntrackTimeoutUpdater::L4Protocol;


class CtUpdateAmbassador : public IOffloadManager::ConntrackTimeoutUpdater {
public:
    CtUpdateAmbassador(const ::android::sp<ITetheringOffloadCallback>& /* cb */);
    /* ------------------- CONNTRACK TIMEOUT UPDATER ------------------------ */
    void updateTimeout(IpaNatTimeoutUpdate /* update */);
private:
    static bool translate(IpaNatTimeoutUpdate /* in */, HALNatTimeoutUpdate& /* out */);
    static bool translate(IpaIpAddrPortPair /* in */, HALIpAddrPortPair& /* out */);
    static bool L4ToNetwork(IpaL4Protocol /* in */, NetworkProtocol& /* out */);
    const ::android::sp<ITetheringOffloadCallback>& mFramework;
}; /* CtUpdateAmbassador */
#endif /* _CT_UPDATE_AMBASSADOR_H_ */