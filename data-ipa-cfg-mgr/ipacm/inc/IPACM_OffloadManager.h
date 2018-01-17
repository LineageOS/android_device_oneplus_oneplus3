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
#ifndef _IPACM_OFFLOAD_MANAGER_H_
#define _IPACM_OFFLOAD_MANAGER_H_

#include <list>
#include <stdint.h>
#include <IOffloadManager.h>
#include "IPACM_Defs.h"

using RET = ::IOffloadManager::RET;
using Prefix = ::IOffloadManager::Prefix;
using IP_FAM = ::IOffloadManager::IP_FAM;
using L4Protocol = ::IOffloadManager::ConntrackTimeoutUpdater::L4Protocol;
using natTimeoutUpdate_t = ::IOffloadManager::ConntrackTimeoutUpdater::natTimeoutUpdate_t;
//using ipAddrPortPair_t = ::IOffloadManager::ConntrackTimeoutUpdater::ipAddrPortPair_t;
//using UDP = ::IOffloadManager::ConntrackTimeoutUpdater::UDP;
//using TCP = ::IOffloadManager::ConntrackTimeoutUpdater::TCP;

#define MAX_EVENT_CACHE  10

typedef struct _framework_event_cache
{
	/* IPACM interface name */
	ipa_cm_event_id event;
	char dev_name[IF_NAME_LEN];
	Prefix prefix_cache;
	Prefix prefix_cache_v6; //for setupstream use
	bool valid;
}framework_event_cache;

class IPACM_OffloadManager : public IOffloadManager
{

public:

	IPACM_OffloadManager();
	static IPACM_OffloadManager* GetInstance();

	virtual RET registerEventListener(IpaEventListener* /* listener */);
	virtual RET unregisterEventListener(IpaEventListener* /* listener */);
	virtual RET registerCtTimeoutUpdater(ConntrackTimeoutUpdater* /* cb */);
	virtual RET unregisterCtTimeoutUpdater(ConntrackTimeoutUpdater* /* cb */);

	virtual RET provideFd(int /* fd */, unsigned int /* group */);
	virtual RET clearAllFds();
	virtual bool isStaApSupported();

    /* ---------------------------- ROUTE ------------------------------- */
    virtual RET setLocalPrefixes(std::vector<Prefix> &/* prefixes */);
    virtual RET addDownstream(const char * /* downstream */,
            const Prefix & /* prefix */);
    virtual RET removeDownstream(const char * /* downstream */,
            const Prefix &/* prefix */);
	virtual RET setUpstream(const char* /* iface */, const Prefix& /* v4Gw */, const Prefix& /* v6Gw */);
    virtual RET stopAllOffload();

    /* ------------------------- STATS/POLICY --------------------------- */
    virtual RET setQuota(const char * /* upstream */, uint64_t /* limit */);
    virtual RET getStats(const char * /* upstream */, bool /* reset */,
		OffloadStatistics& /* ret */);

	static IPACM_OffloadManager *pInstance; //sky

	IpaEventListener *elrInstance;

	ConntrackTimeoutUpdater *touInstance;

	bool search_framwork_cache(char * interface_name);

private:

	std::list<std::string> valid_ifaces;

	bool upstream_v4_up;

	bool upstream_v6_up;

	int default_gw_index;

	int post_route_evt(enum ipa_ip_type iptype, int index, ipa_cm_event_id event, const Prefix &gw_addr);

	int ipa_get_if_index(const char *if_name, int *if_index);

	int resetTetherStats(const char *upstream_name);

	static const char *DEVICE_NAME;

	/* cache the add_downstream events if netdev is not ready */
	framework_event_cache event_cache[MAX_EVENT_CACHE];

	/* latest update cache entry */
	int latest_cache_index;

}; /* IPACM_OffloadManager */

#endif /* _IPACM_OFFLOAD_MANAGER_H_ */
