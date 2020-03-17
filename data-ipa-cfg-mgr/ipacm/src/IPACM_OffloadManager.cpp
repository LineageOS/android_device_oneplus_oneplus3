/*
Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials provided
  with the distribution.
* Neither the name of The Linux Foundation nor the names of its
  contributors may be used to endorse or promote products derived
  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.Z
*/
/*!
  @file
  IPACM_OffloadManager.cpp

  @brief
  This file implements the basis Iface functionality.

  @Author
  Skylar Chang

*/
#include <IPACM_OffloadManager.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include "IPACM_ConntrackClient.h"
#include "IPACM_ConntrackListener.h"
#include "IPACM_Iface.h"
#include "IPACM_Config.h"
#include <unistd.h>

const char *IPACM_OffloadManager::DEVICE_NAME = "/dev/wwan_ioctl";

/* NatApp class Implementation */
IPACM_OffloadManager *IPACM_OffloadManager::pInstance = NULL;
int IPACM_OffloadManager::num_offload_v4_tethered_iface = 0;

IPACM_OffloadManager::IPACM_OffloadManager()
{
	default_gw_index = INVALID_IFACE;
	upstream_v4_up = false;
	upstream_v6_up = false;
	memset(event_cache, 0, MAX_EVENT_CACHE*sizeof(framework_event_cache));
	latest_cache_index = 0;
	elrInstance = NULL;
	touInstance = NULL;
	return ;
}

RET IPACM_OffloadManager::registerEventListener(IpaEventListener* eventlistener)
{
	RET result = SUCCESS;
	if (elrInstance == NULL) {
		IPACMDBG_H("get registerEventListener \n");
		elrInstance = eventlistener;
	} else {
		IPACMDBG_H("already have EventListener previously, override \n");
		elrInstance = eventlistener;
		result = FAIL_INPUT_CHECK;
	}
	return SUCCESS;
}

RET IPACM_OffloadManager::unregisterEventListener(IpaEventListener* )
{
	RET result = SUCCESS;
	if (elrInstance)
		elrInstance = NULL;
	else {
		IPACMDBG_H("already unregisterEventListener previously \n");
		result = SUCCESS_DUPLICATE_CONFIG;
	}
	return SUCCESS;
}

RET IPACM_OffloadManager::registerCtTimeoutUpdater(ConntrackTimeoutUpdater* timeoutupdater)
{
	RET result = SUCCESS;
	if (touInstance == NULL)
	{
		IPACMDBG_H("get ConntrackTimeoutUpdater \n");
		touInstance = timeoutupdater;
	} else {
		IPACMDBG_H("already have ConntrackTimeoutUpdater previously, override \n");
		touInstance = timeoutupdater;
		result = FAIL_INPUT_CHECK;
	}
	return SUCCESS;
}

RET IPACM_OffloadManager::unregisterCtTimeoutUpdater(ConntrackTimeoutUpdater* )
{
	RET result = SUCCESS;
	if (touInstance)
		touInstance = NULL;
	else {
		IPACMDBG_H("already unregisterCtTimeoutUpdater previously \n");
		result = SUCCESS_DUPLICATE_CONFIG;
	}
	return SUCCESS;
}

RET IPACM_OffloadManager::provideFd(int fd, unsigned int groups)
{
	IPACM_ConntrackClient *cc;
	int on = 1, rel;
	struct sockaddr_nl	local;
	unsigned int addr_len;

	cc = IPACM_ConntrackClient::GetInstance();

	if(!cc)
	{
		IPACMERR("Init failed: cc %p\n", cc);
		return FAIL_HARDWARE;
	}

	/* check socket name */
	memset(&local, 0, sizeof(struct sockaddr_nl));
	addr_len = sizeof(local);
	getsockname(fd, (struct sockaddr *)&local, (socklen_t *)&addr_len);
	IPACMDBG_H(" FD %d, nl_pad %d nl_pid %u\n", fd, local.nl_pad, local.nl_pid);

	/* add the check if getting FDs already or not */
	if(cc->fd_tcp > -1 && cc->fd_udp > -1) {
		IPACMDBG_H("has valid FDs fd_tcp %d, fd_udp %d, ignore fd %d.\n", cc->fd_tcp, cc->fd_udp, fd);
		return SUCCESS;
	}

	if (groups == cc->subscrips_tcp) {
		cc->fd_tcp = dup(fd);
		IPACMDBG_H("Received fd %d with groups %d.\n", fd, groups);
		/* set netlink buf */
		rel = setsockopt(cc->fd_tcp, SOL_NETLINK, NETLINK_NO_ENOBUFS, &on, sizeof(int) );
		if (rel == -1)
		{
			IPACMERR( "setsockopt returned error code %d ( %s )", errno, strerror( errno ) );
		}
	} else if (groups == cc->subscrips_udp) {
		cc->fd_udp = dup(fd);
		IPACMDBG_H("Received fd %d with groups %d.\n", fd, groups);
		/* set netlink buf */
		rel = setsockopt(cc->fd_tcp, SOL_NETLINK, NETLINK_NO_ENOBUFS, &on, sizeof(int) );
		if (rel == -1)
		{
			IPACMERR( "setsockopt returned error code %d ( %s )", errno, strerror( errno ) );
		}
	} else {
		IPACMERR("Received unexpected fd with groups %d.\n", groups);
	}
	if(cc->fd_tcp >0 && cc->fd_udp >0) {
		IPACMDBG_H(" Got both fds from framework, start conntrack listener thread.\n");
		CtList->CreateConnTrackThreads();
	}
	return SUCCESS;
}

RET IPACM_OffloadManager::clearAllFds()
{

	/* IPACM needs to kee old FDs, can't clear */
	IPACMDBG_H("Still use old Fds, can't clear \n");
	return SUCCESS;
}

bool IPACM_OffloadManager::isStaApSupported()
{
	return true;
}


RET IPACM_OffloadManager::setLocalPrefixes(std::vector<Prefix> &/* prefixes */)
{
	return SUCCESS;
}

RET IPACM_OffloadManager::addDownstream(const char * downstream_name, const Prefix &prefix)
{
	int index;
	ipacm_cmd_q_data evt;
	ipacm_event_ipahal_stream *evt_data;
	bool cache_need = false;

	IPACMDBG_H("addDownstream name(%s), ip-family(%d) \n", downstream_name, prefix.fam);

	if (prefix.fam == V4) {
		IPACMDBG_H("subnet info v4Addr (%x) v4Mask (%x)\n", prefix.v4Addr, prefix.v4Mask);
	} else {
		IPACMDBG_H("subnet info v6Addr: %08x:%08x:%08x:%08x \n",
							prefix.v6Addr[0], prefix.v6Addr[1], prefix.v6Addr[2], prefix.v6Addr[3]);
		IPACMDBG_H("subnet info v6Mask: %08x:%08x:%08x:%08x \n",
							prefix.v6Mask[0], prefix.v6Mask[1], prefix.v6Mask[2], prefix.v6Mask[3]);
	}

	/* check if netdev valid on device */
	if(ipa_get_if_index(downstream_name, &index))
	{
		IPACMERR("fail to get iface index.\n");
		return FAIL_INPUT_CHECK;
	}

	/* Iface is valid, add to list if not present */
	if (std::find(valid_ifaces.begin(), valid_ifaces.end(), std::string(downstream_name)) == valid_ifaces.end())
	{
		/* Iface is new, add it to the list */
		valid_ifaces.push_back(downstream_name);
		IPACMDBG_H("add iface(%s) to list\n", downstream_name);
	}

	/* check if upstream netdev driver finished its configuration on IPA-HW for ipv4 and ipv6 */
	if (prefix.fam == V4 && IPACM_Iface::ipacmcfg->CheckNatIfaces(downstream_name, IPA_IP_v4))
		cache_need = true;
	if (prefix.fam == V6 && IPACM_Iface::ipacmcfg->CheckNatIfaces(downstream_name, IPA_IP_v6))
		cache_need = true;

	if (cache_need)
	{
		IPACMDBG_H("addDownstream name(%s) currently not support in ipa \n", downstream_name);

		/* copy to the cache */
		for(int i = 0; i < MAX_EVENT_CACHE ;i++)
		{
			if (latest_cache_index >= 0)
			{
				if(event_cache[latest_cache_index].valid == false)
				{
					//do the copy
					event_cache[latest_cache_index].valid = true;
					event_cache[latest_cache_index].event = IPA_DOWNSTREAM_ADD;
					memcpy(event_cache[latest_cache_index].dev_name, downstream_name,
						sizeof(event_cache[latest_cache_index].dev_name));
					memcpy(&event_cache[latest_cache_index].prefix_cache, &prefix,
						sizeof(event_cache[latest_cache_index].prefix_cache));
					if (prefix.fam == V4) {
						IPACMDBG_H("cache event(%d) subnet info v4Addr (%x) v4Mask (%x) dev(%s) on entry (%d)\n",
							event_cache[latest_cache_index].event,
							event_cache[latest_cache_index].prefix_cache.v4Addr,
							event_cache[latest_cache_index].prefix_cache.v4Mask,
							event_cache[latest_cache_index].dev_name,
							latest_cache_index);
					} else {
						IPACMDBG_H("cache event (%d) v6Addr: %08x:%08x:%08x:%08x \n",
							event_cache[latest_cache_index].event,
							event_cache[latest_cache_index].prefix_cache.v6Addr[0],
							event_cache[latest_cache_index].prefix_cache.v6Addr[1],
							event_cache[latest_cache_index].prefix_cache.v6Addr[2],
							event_cache[latest_cache_index].prefix_cache.v6Addr[3]);
						IPACMDBG_H("subnet v6Mask: %08x:%08x:%08x:%08x dev(%s) on entry(%d), \n",
							event_cache[latest_cache_index].prefix_cache.v6Mask[0],
							event_cache[latest_cache_index].prefix_cache.v6Mask[1],
							event_cache[latest_cache_index].prefix_cache.v6Mask[2],
							event_cache[latest_cache_index].prefix_cache.v6Mask[3],
							event_cache[latest_cache_index].dev_name,
							latest_cache_index);
					}
					latest_cache_index = (latest_cache_index + 1)% MAX_EVENT_CACHE;
					break;
				}
				latest_cache_index = (latest_cache_index + 1)% MAX_EVENT_CACHE;
			}
			if(i == MAX_EVENT_CACHE - 1)
			{
				IPACMDBG_H(" run out of event cache (%d)\n", i);
				return FAIL_HARDWARE;
			}
		}

		return SUCCESS;
	}

	evt_data = (ipacm_event_ipahal_stream*)malloc(sizeof(ipacm_event_ipahal_stream));
	if(evt_data == NULL)
	{
		IPACMERR("Failed to allocate memory.\n");
		return FAIL_HARDWARE;
	}
	memset(evt_data, 0, sizeof(*evt_data));

	evt_data->if_index = index;
	memcpy(&evt_data->prefix, &prefix, sizeof(evt_data->prefix));

	memset(&evt, 0, sizeof(ipacm_cmd_q_data));
	evt.evt_data = (void*)evt_data;
	evt.event = IPA_DOWNSTREAM_ADD;

	IPACMDBG_H("Posting event IPA_DOWNSTREAM_ADD\n");
	IPACM_EvtDispatcher::PostEvt(&evt);

	return SUCCESS;
}

RET IPACM_OffloadManager::removeDownstream(const char * downstream_name, const Prefix &prefix)
{
	int index;
	ipacm_cmd_q_data evt;
	ipacm_event_ipahal_stream *evt_data;

	IPACMDBG_H("removeDownstream name(%s), ip-family(%d) \n", downstream_name, prefix.fam);
	if(strnlen(downstream_name, sizeof(downstream_name)) == 0)
	{
		IPACMERR("iface length is 0.\n");
		return FAIL_HARDWARE;
	}
	if (std::find(valid_ifaces.begin(), valid_ifaces.end(), std::string(downstream_name)) == valid_ifaces.end())
	{
		IPACMERR("iface is not present in list.\n");
		return FAIL_HARDWARE;
	}

	if(ipa_get_if_index(downstream_name, &index))
	{
		IPACMERR("netdev(%s) already removed, ignored\n", downstream_name);
		return SUCCESS;
	}

	evt_data = (ipacm_event_ipahal_stream*)malloc(sizeof(ipacm_event_ipahal_stream));
	if(evt_data == NULL)
	{
		IPACMERR("Failed to allocate memory.\n");
		return FAIL_HARDWARE;
	}
	memset(evt_data, 0, sizeof(*evt_data));

	evt_data->if_index = index;
	memcpy(&evt_data->prefix, &prefix, sizeof(evt_data->prefix));

	memset(&evt, 0, sizeof(ipacm_cmd_q_data));
	evt.evt_data = (void*)evt_data;
	evt.event = IPA_DOWNSTREAM_DEL;

	IPACMDBG_H("Posting event IPA_DOWNSTREAM_DEL\n");
	IPACM_EvtDispatcher::PostEvt(&evt);

	return SUCCESS;
}

RET IPACM_OffloadManager::setUpstream(const char *upstream_name, const Prefix& gw_addr_v4 , const Prefix& gw_addr_v6)
{
	int index;
	RET result = SUCCESS;
	bool cache_need = false;

	/* if interface name is NULL, default route is removed */
	if(upstream_name != NULL)
	{
		IPACMDBG_H("setUpstream upstream_name(%s), ipv4-fam(%d) ipv6-fam(%d)\n", upstream_name, gw_addr_v4.fam, gw_addr_v6.fam);
	}
	else
	{
		IPACMDBG_H("setUpstream clean upstream_name for ipv4-fam(%d) ipv6-fam(%d)\n", gw_addr_v4.fam, gw_addr_v6.fam);
	}
	if(upstream_name == NULL)
	{
		if (default_gw_index == INVALID_IFACE) {
			result = FAIL_INPUT_CHECK;
			for (index = 0; index < MAX_EVENT_CACHE; index++) {
				if (event_cache[index].valid == true &&
					event_cache[index ].event == IPA_WAN_UPSTREAM_ROUTE_ADD_EVENT) {
					event_cache[index].valid = false;
					result = SUCCESS;
				}
			}
			IPACMERR("no previous upstream set before\n");
			return result;
		}
		if (gw_addr_v4.fam == V4 && upstream_v4_up == true) {
			IPACMDBG_H("clean upstream for ipv4-fam(%d) upstream_v4_up(%d)\n", gw_addr_v4.fam, upstream_v4_up);
			post_route_evt(IPA_IP_v4, default_gw_index, IPA_WAN_UPSTREAM_ROUTE_DEL_EVENT, gw_addr_v4);
			upstream_v4_up = false;
		}
		if (gw_addr_v6.fam == V6 && upstream_v6_up == true) {
			IPACMDBG_H("clean upstream for ipv6-fam(%d) upstream_v6_up(%d)\n", gw_addr_v6.fam, upstream_v6_up);
			post_route_evt(IPA_IP_v6, default_gw_index, IPA_WAN_UPSTREAM_ROUTE_DEL_EVENT, gw_addr_v6);
			upstream_v6_up = false;
		}
		default_gw_index = INVALID_IFACE;
	}
	else
	{
		/* check if netdev valid on device */
		if(ipa_get_if_index(upstream_name, &index))
		{
			IPACMERR("fail to get iface index.\n");
			return FAIL_INPUT_CHECK;
		}

		/* check if upstream netdev driver finished its configuration on IPA-HW for ipv4 and ipv6 */
		if (gw_addr_v4.fam == V4 && IPACM_Iface::ipacmcfg->CheckNatIfaces(upstream_name, IPA_IP_v4))
			cache_need = true;
		if (gw_addr_v6.fam == V6 && IPACM_Iface::ipacmcfg->CheckNatIfaces(upstream_name, IPA_IP_v6))
			cache_need = true;

		if (cache_need)
		{
			IPACMDBG_H("setUpstream name(%s) currently not support in ipa \n", upstream_name);
#ifdef FEATURE_IPACM_RESTART
			/* add ipacm restart support */
			push_iface_up(upstream_name, true);
#endif
			/* copy to the cache */
			for(int i = 0; i < MAX_EVENT_CACHE ;i++)
			{
				if (latest_cache_index >= 0)
				{
					if(event_cache[latest_cache_index].valid == false)
					{
						//do the copy
						event_cache[latest_cache_index].valid = true;
						event_cache[latest_cache_index].event = IPA_WAN_UPSTREAM_ROUTE_ADD_EVENT;
						memcpy(event_cache[latest_cache_index].dev_name, upstream_name,
							sizeof(event_cache[latest_cache_index].dev_name));
						memcpy(&event_cache[latest_cache_index].prefix_cache, &gw_addr_v4,
							sizeof(event_cache[latest_cache_index].prefix_cache));
						memcpy(&event_cache[latest_cache_index].prefix_cache_v6, &gw_addr_v6,
							sizeof(event_cache[latest_cache_index].prefix_cache_v6));
						if (gw_addr_v4.fam == V4) {
							IPACMDBG_H("cache event(%d) ipv4 gateway: (%x) dev(%s) on entry (%d)\n",
								event_cache[latest_cache_index].event,
								event_cache[latest_cache_index].prefix_cache.v4Addr,
								event_cache[latest_cache_index].dev_name,
								latest_cache_index);
						}

						if (gw_addr_v6.fam == V6)
						{
							IPACMDBG_H("cache event (%d) ipv6 gateway: %08x:%08x:%08x:%08x dev(%s) on entry(%d)\n",
								event_cache[latest_cache_index].event,
								event_cache[latest_cache_index].prefix_cache_v6.v6Addr[0],
								event_cache[latest_cache_index].prefix_cache_v6.v6Addr[1],
								event_cache[latest_cache_index].prefix_cache_v6.v6Addr[2],
								event_cache[latest_cache_index].prefix_cache_v6.v6Addr[3],
								event_cache[latest_cache_index].dev_name,
								latest_cache_index);
						}
						latest_cache_index = (latest_cache_index + 1)% MAX_EVENT_CACHE;
						break;
					}
					latest_cache_index = (latest_cache_index + 1)% MAX_EVENT_CACHE;
				}
				if(i == MAX_EVENT_CACHE - 1)
				{
					IPACMDBG_H(" run out of event cache (%d) \n", i);
					return FAIL_HARDWARE;
				}
			}
			return SUCCESS;
		}

		/* reset the stats when switch from LTE->STA */
		if (index != default_gw_index) {
			IPACMDBG_H(" interface switched to %s\n", upstream_name);
			if (upstream_v4_up == true) {
				IPACMDBG_H("clean upstream for ipv4-fam(%d) upstream_v4_up(%d)\n", gw_addr_v4.fam, upstream_v4_up);
				post_route_evt(IPA_IP_v4, default_gw_index, IPA_WAN_UPSTREAM_ROUTE_DEL_EVENT, gw_addr_v4);
				upstream_v4_up = false;
			}
			if (upstream_v6_up == true) {
				IPACMDBG_H("clean upstream for ipv6-fam(%d) upstream_v6_up(%d)\n", gw_addr_v6.fam, upstream_v6_up);
				post_route_evt(IPA_IP_v6, default_gw_index, IPA_WAN_UPSTREAM_ROUTE_DEL_EVENT, gw_addr_v6);
				upstream_v6_up = false;
			}
			default_gw_index = INVALID_IFACE;
			if(memcmp(upstream_name, "wlan0", sizeof("wlan0")) == 0)
			{
				IPACMDBG_H("switch to STA mode, need reset wlan-fw stats\n");
				resetTetherStats(upstream_name);
			}
		}

		if (gw_addr_v4.fam == V4 && gw_addr_v6.fam == V6) {

			if (upstream_v4_up == false) {
				IPACMDBG_H("IPV4 gateway: 0x%x \n", gw_addr_v4.v4Addr);
				/* posting route add event for both IPv4 and IPv6 */
				post_route_evt(IPA_IP_v4, index, IPA_WAN_UPSTREAM_ROUTE_ADD_EVENT, gw_addr_v4);
				upstream_v4_up = true;
			} else {
				IPACMDBG_H("already setupstream iface(%s) ipv4 previously\n", upstream_name);
			}

			if (upstream_v6_up == false) {
				IPACMDBG_H("IPV6 gateway: %08x:%08x:%08x:%08x \n",
						gw_addr_v6.v6Addr[0], gw_addr_v6.v6Addr[1], gw_addr_v6.v6Addr[2], gw_addr_v6.v6Addr[3]);
				/* check v6-address valid or not */
				if((gw_addr_v6.v6Addr[0] == 0) && (gw_addr_v6.v6Addr[1] ==0) && (gw_addr_v6.v6Addr[2] == 0) && (gw_addr_v6.v6Addr[3] == 0))
				{
					IPACMDBG_H("Invliad ipv6-address, ignored v6-setupstream\n");
				} else {
					post_route_evt(IPA_IP_v6, index, IPA_WAN_UPSTREAM_ROUTE_ADD_EVENT, gw_addr_v6);
					upstream_v6_up = true;
				}
			} else {
				IPACMDBG_H("already setupstream iface(%s) ipv6 previously\n", upstream_name);
			}
		} else if (gw_addr_v4.fam == V4 ) {
			IPACMDBG_H("check upstream_v6_up (%d) v4_up (%d), default gw index (%d)\n", upstream_v6_up, upstream_v4_up, default_gw_index);
			if (upstream_v6_up == true && default_gw_index != INVALID_IFACE ) {
				/* clean up previous V6 upstream event */
				IPACMDBG_H(" Post clean-up v6 default gw on iface %d\n", default_gw_index);
				post_route_evt(IPA_IP_v6, default_gw_index, IPA_WAN_UPSTREAM_ROUTE_DEL_EVENT, gw_addr_v6);
				upstream_v6_up = false;
			}

			if (upstream_v4_up == false) {
				IPACMDBG_H("IPV4 gateway: %x \n", gw_addr_v4.v4Addr);
				/* posting route add event for both IPv4 and IPv6 */
				post_route_evt(IPA_IP_v4, index, IPA_WAN_UPSTREAM_ROUTE_ADD_EVENT, gw_addr_v4);
				upstream_v4_up = true;
			} else {
				IPACMDBG_H("already setupstream iface(%s) ipv4 previously\n", upstream_name);
				result = SUCCESS_DUPLICATE_CONFIG;
			}
		} else if (gw_addr_v6.fam == V6) {
			IPACMDBG_H("check upstream_v6_up (%d) v4_up (%d), default gw index (%d)\n", upstream_v6_up, upstream_v4_up, default_gw_index);
			if (upstream_v4_up == true && default_gw_index != INVALID_IFACE ) {
				/* clean up previous V4 upstream event */
				IPACMDBG_H(" Post clean-up v4 default gw on iface %d\n", default_gw_index);
				post_route_evt(IPA_IP_v4, default_gw_index, IPA_WAN_UPSTREAM_ROUTE_DEL_EVENT, gw_addr_v4);
				upstream_v4_up = false;
			}

			if (upstream_v6_up == false) {
				IPACMDBG_H("IPV6 gateway: %08x:%08x:%08x:%08x \n",
						gw_addr_v6.v6Addr[0], gw_addr_v6.v6Addr[1], gw_addr_v6.v6Addr[2], gw_addr_v6.v6Addr[3]);
				/* check v6-address valid or not */
				if((gw_addr_v6.v6Addr[0] == 0) && (gw_addr_v6.v6Addr[1] ==0) && (gw_addr_v6.v6Addr[2] == 0) && (gw_addr_v6.v6Addr[3] == 0))
				{
					IPACMDBG_H("Invliad ipv6-address, ignored v6-setupstream\n");
				} else {
					post_route_evt(IPA_IP_v6, index, IPA_WAN_UPSTREAM_ROUTE_ADD_EVENT, gw_addr_v6);
					upstream_v6_up = true;
				}
			} else {
				IPACMDBG_H("already setupstream iface(%s) ipv6 previously\n", upstream_name);
				result = SUCCESS_DUPLICATE_CONFIG;
			}
		}
		default_gw_index = index;
		IPACMDBG_H("Change degault_gw netdev to (%s)\n", upstream_name);
	}
	return result;
}

RET IPACM_OffloadManager::stopAllOffload()
{
	Prefix v4gw, v6gw;
	RET result = SUCCESS;

	memset(&v4gw, 0, sizeof(v4gw));
	memset(&v6gw, 0, sizeof(v6gw));
	v4gw.fam = V4;
	v6gw.fam = V6;
	IPACMDBG_H("posting setUpstream(NULL), ipv4-fam(%d) ipv6-fam(%d)\n", v4gw.fam, v6gw.fam);
	result = setUpstream(NULL, v4gw, v6gw);

	/* reset the event cache */
	default_gw_index = INVALID_IFACE;
	upstream_v4_up = false;
	upstream_v6_up = false;
	memset(event_cache, 0, MAX_EVENT_CACHE*sizeof(framework_event_cache));
	latest_cache_index = 0;
	valid_ifaces.clear();
	IPACM_OffloadManager::num_offload_v4_tethered_iface = 0;
	return result;
}

RET IPACM_OffloadManager::setQuota(const char * upstream_name /* upstream */, uint64_t mb/* limit */)
{
	wan_ioctl_set_data_quota quota;
	int fd = -1, rc = 0, err_type = 0;

	if ((fd = open(DEVICE_NAME, O_RDWR)) < 0)
	{
		IPACMERR("Failed opening %s.\n", DEVICE_NAME);
		return FAIL_HARDWARE;
	}

	quota.quota_mbytes = mb;
	quota.set_quota = true;

    memset(quota.interface_name, 0, IFNAMSIZ);
    if (strlcpy(quota.interface_name, upstream_name, IFNAMSIZ) >= IFNAMSIZ) {
		IPACMERR("String truncation occurred on upstream");
		close(fd);
		return FAIL_INPUT_CHECK;
	}

	IPACMDBG_H("SET_DATA_QUOTA %s %llu\n", quota.interface_name, (long long)mb);

	rc = ioctl(fd, WAN_IOC_SET_DATA_QUOTA, &quota);

	if(rc != 0)
	{
		err_type = errno;
		close(fd);
		IPACMERR("IOCTL WAN_IOCTL_SET_DATA_QUOTA call failed: %s err_type: %d\n", strerror(err_type), err_type);
		if (err_type == ENODEV) {
			IPACMDBG_H("Invalid argument.\n");
			return FAIL_UNSUPPORTED;
		}
		else {
			return FAIL_TRY_AGAIN;
		}
	}
	close(fd);
	return SUCCESS;
}

RET IPACM_OffloadManager::getStats(const char * upstream_name /* upstream */,
		bool reset /* reset */, OffloadStatistics& offload_stats/* ret */)
{
	int fd = -1;
	wan_ioctl_query_tether_stats_all stats;

	if ((fd = open(DEVICE_NAME, O_RDWR)) < 0) {
        IPACMERR("Failed opening %s.\n", DEVICE_NAME);
        return FAIL_HARDWARE;
    }

    memset(&stats, 0, sizeof(stats));
    if (strlcpy(stats.upstreamIface, upstream_name, IFNAMSIZ) >= IFNAMSIZ) {
		IPACMERR("String truncation occurred on upstream\n");
		close(fd);
		return FAIL_INPUT_CHECK;
	}
	stats.reset_stats = reset;
	stats.ipa_client = IPACM_CLIENT_MAX;

	if (ioctl(fd, WAN_IOC_QUERY_TETHER_STATS_ALL, &stats) < 0) {
        IPACMERR("IOCTL WAN_IOC_QUERY_TETHER_STATS_ALL call failed: %s \n", strerror(errno));
		close(fd);
		return FAIL_TRY_AGAIN;
	}
	/* feedback to IPAHAL*/
	offload_stats.tx = stats.tx_bytes;
	offload_stats.rx = stats.rx_bytes;

	IPACMDBG_H("send getStats tx:%llu rx:%llu \n", (long long)offload_stats.tx, (long long)offload_stats.rx);
	close(fd);
	return SUCCESS;
}

int IPACM_OffloadManager::post_route_evt(enum ipa_ip_type iptype, int index, ipa_cm_event_id event, const Prefix &gw_addr)
{
	ipacm_cmd_q_data evt;
	ipacm_event_data_iptype *evt_data_route;

	evt_data_route = (ipacm_event_data_iptype*)malloc(sizeof(ipacm_event_data_iptype));
	if(evt_data_route == NULL)
	{
		IPACMERR("Failed to allocate memory.\n");
		return -EFAULT;
	}
	memset(evt_data_route, 0, sizeof(*evt_data_route));

	evt_data_route->if_index = index;
	evt_data_route->if_index_tether = 0;
	evt_data_route->iptype = iptype;

	IPACMDBG_H("gw_addr.v4Addr: %d, gw_addr.v6Addr: %08x:%08x:%08x:%08x \n",
			gw_addr.v4Addr,gw_addr.v6Addr[0],gw_addr.v6Addr[1],gw_addr.v6Addr[2],gw_addr.v6Addr[3]);

#ifdef IPA_WAN_MSG_IPv6_ADDR_GW_LEN
	evt_data_route->ipv4_addr_gw = gw_addr.v4Addr;
	evt_data_route->ipv6_addr_gw[0] = gw_addr.v6Addr[0];
	evt_data_route->ipv6_addr_gw[1] = gw_addr.v6Addr[1];
	evt_data_route->ipv6_addr_gw[2] = gw_addr.v6Addr[2];
	evt_data_route->ipv6_addr_gw[3] = gw_addr.v6Addr[3];
	IPACMDBG_H("default gw ipv4 (%x)\n", evt_data_route->ipv4_addr_gw);
	IPACMDBG_H("IPV6 gateway: %08x:%08x:%08x:%08x \n",
					evt_data_route->ipv6_addr_gw[0], evt_data_route->ipv6_addr_gw[1], evt_data_route->ipv6_addr_gw[2], evt_data_route->ipv6_addr_gw[3]);
#endif
	if (event == IPA_WAN_UPSTREAM_ROUTE_ADD_EVENT) {
		IPACMDBG_H("Received WAN_UPSTREAM_ROUTE_ADD: fid(%d) tether_fid(%d) ip-type(%d)\n", evt_data_route->if_index,
			evt_data_route->if_index_tether, evt_data_route->iptype);
	}
	else if (event == IPA_WAN_UPSTREAM_ROUTE_DEL_EVENT) {
		IPACMDBG_H("Received WAN_UPSTREAM_ROUTE_DEL: fid(%d) tether_fid(%d) ip-type(%d)\n",
				evt_data_route->if_index,
				evt_data_route->if_index_tether, evt_data_route->iptype);
	}
	memset(&evt, 0, sizeof(evt));
	evt.evt_data = (void*)evt_data_route;
	evt.event = event;

	IPACM_EvtDispatcher::PostEvt(&evt);

	return 0;
}

int IPACM_OffloadManager::ipa_get_if_index(const char * if_name, int * if_index)
{
	int fd;
	struct ifreq ifr;

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		IPACMERR("get interface index socket create failed \n");
		return IPACM_FAILURE;
	}

	if(strnlen(if_name, sizeof(if_name)) >= sizeof(ifr.ifr_name)) {
		IPACMERR("interface name overflows: len %zu\n", strnlen(if_name, sizeof(if_name)));
		close(fd);
		return IPACM_FAILURE;
	}

	memset(&ifr, 0, sizeof(struct ifreq));
	(void)strlcpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name));
	IPACMDBG_H("interface name (%s)\n", if_name);

	if(ioctl(fd,SIOCGIFINDEX , &ifr) < 0)
	{
		IPACMERR("call_ioctl_on_dev: ioctl failed, interface name (%s):\n", ifr.ifr_name);
		close(fd);
		return IPACM_FAILURE;
	}

	*if_index = ifr.ifr_ifindex;
	IPACMDBG_H("Interface netdev index %d\n", *if_index);
	close(fd);
	return IPACM_SUCCESS;
}

int IPACM_OffloadManager::resetTetherStats(const char * upstream_name /* upstream */)
{
	int fd = -1;
	wan_ioctl_reset_tether_stats stats;

	if ((fd = open(DEVICE_NAME, O_RDWR)) < 0) {
        IPACMERR("Failed opening %s.\n", DEVICE_NAME);
        return FAIL_HARDWARE;
    }

    memset(stats.upstreamIface, 0, IFNAMSIZ);
    if (strlcpy(stats.upstreamIface, upstream_name, IFNAMSIZ) >= IFNAMSIZ) {
		IPACMERR("String truncation occurred on upstream\n");
		close(fd);
		return FAIL_INPUT_CHECK;
	}
	stats.reset_stats = true;
	if (ioctl(fd, WAN_IOC_RESET_TETHER_STATS, &stats) < 0) {
		IPACMERR("IOCTL WAN_IOC_RESET_TETHER_STATS call failed: %s", strerror(errno));
		close(fd);
		return FAIL_HARDWARE;
	}
	IPACMDBG_H("Reset Interface %s stats\n", upstream_name);
	close(fd);
	return IPACM_SUCCESS;
}

IPACM_OffloadManager* IPACM_OffloadManager::GetInstance()
{
	if(pInstance == NULL)
		pInstance = new IPACM_OffloadManager();

	return pInstance;
}

bool IPACM_OffloadManager::search_framwork_cache(char * interface_name)
{
	bool rel = false;
	bool cache_need = false;

	/* IPACM needs to kee old FDs, can't clear */
	IPACMDBG_H("check netdev(%s)\n", interface_name);

	for(int i = 0; i < MAX_EVENT_CACHE ;i++)
	{
		cache_need = false;
		if(event_cache[i].valid == true)
		{
			//do the compare
			if (strncmp(event_cache[i].dev_name,
					interface_name,
					sizeof(event_cache[i].dev_name)) == 0)
			{
				IPACMDBG_H("found netdev (%s) in entry (%d) with event (%d)\n", interface_name, i, event_cache[i].event);
				/* post event again */
				if (event_cache[i].event == IPA_DOWNSTREAM_ADD) {
					/* check if downsteam netdev driver finished its configuration on IPA-HW for ipv4 and ipv6 */
					if (event_cache[i].prefix_cache.fam == V4 && IPACM_Iface::ipacmcfg->CheckNatIfaces(event_cache[i].dev_name, IPA_IP_v4))
						cache_need = true;
					if (event_cache[i].prefix_cache.fam == V6 && IPACM_Iface::ipacmcfg->CheckNatIfaces(event_cache[i].dev_name, IPA_IP_v6))
						cache_need = true;
					if (cache_need) {
						IPACMDBG_H("still need cache (%d), index (%d) ip-family (%d)\n", cache_need, i, event_cache[i].prefix_cache.fam);
						break;
					} else {
						IPACMDBG_H("no need cache (%d), handling it event (%d)\n", cache_need, event_cache[i].event);
					addDownstream(interface_name, event_cache[i].prefix_cache);
					}
				} else if (event_cache[i].event == IPA_WAN_UPSTREAM_ROUTE_ADD_EVENT) {
					/* check if upstream netdev driver finished its configuration on IPA-HW for ipv4 and ipv6 */
					if (event_cache[i].prefix_cache.fam == V4 && IPACM_Iface::ipacmcfg->CheckNatIfaces(event_cache[i].dev_name, IPA_IP_v4))
						cache_need = true;
					if (event_cache[i].prefix_cache_v6.fam == V6 && IPACM_Iface::ipacmcfg->CheckNatIfaces(event_cache[i].dev_name, IPA_IP_v6))
						cache_need = true;
					if (cache_need) {
						IPACMDBG_H("still need cache (%d), index (%d)\n", cache_need, i);
						break;
					} else {
						IPACMDBG_H("no need cache (%d), handling it event (%d)\n", cache_need, event_cache[i].event);
					setUpstream(interface_name, event_cache[i].prefix_cache, event_cache[i].prefix_cache_v6);
					}
				} else {
						IPACMERR("wrong event cached (%d) index (%d)\n", event_cache[i].event, i);
				}

				/* reset entry */
				event_cache[i].valid = false;
				rel = true;
				IPACMDBG_H("reset entry (%d)", i);
			}
		}
	}
	IPACMDBG_H(" not found netdev (%s) has cached event\n", interface_name);
	return rel;
}

#ifdef FEATURE_IPACM_RESTART
int IPACM_OffloadManager::push_iface_up(const char * if_name, bool upstream)
{
	ipacm_cmd_q_data evt_data;
	ipacm_event_data_fid *data_fid = NULL;
	ipacm_event_data_mac *data = NULL;
	int index;

	IPACMDBG_H("name %s, upstream %d\n",
							 if_name, upstream);

	if(ipa_get_if_index(if_name, &index))
	{
		IPACMERR("netdev(%s) not registered ignored\n", if_name);
		return SUCCESS;
	}

	if(strncmp(if_name, "rmnet_data", 10) == 0 && upstream)
	{
		data_fid = (ipacm_event_data_fid *)malloc(sizeof(ipacm_event_data_fid));
		if(data_fid == NULL)
		{
			IPACMERR("unable to allocate memory for event data_fid\n");
			return FAIL_HARDWARE;
		}
		data_fid->if_index = index;
		evt_data.event = IPA_LINK_UP_EVENT;
		evt_data.evt_data = data_fid;
		IPACMDBG_H("Posting IPA_LINK_UP_EVENT with if index: %d\n",
							 data_fid->if_index);
		IPACM_EvtDispatcher::PostEvt(&evt_data);
	}

	if(strncmp(if_name, "rndis", 5) == 0 && !upstream)
	{
		data_fid = (ipacm_event_data_fid *)malloc(sizeof(ipacm_event_data_fid));
		if(data_fid == NULL)
		{
			IPACMERR("unable to allocate memory for event data_fid\n");
			return FAIL_HARDWARE;
		}
		data_fid->if_index = index;
		evt_data.event = IPA_USB_LINK_UP_EVENT;
		evt_data.evt_data = data_fid;
		IPACMDBG_H("Posting usb IPA_LINK_UP_EVENT with if index: %d\n",
				data_fid->if_index);
		IPACM_EvtDispatcher::PostEvt(&evt_data);
	}

	if((strncmp(if_name, "softap", 6) == 0 || strncmp(if_name, "wlan", 4) == 0 ) && !upstream)
	{
		data_fid = (ipacm_event_data_fid *)malloc(sizeof(ipacm_event_data_fid));
		if(data_fid == NULL)
		{
			IPACMERR("unable to allocate memory for event data_fid\n");
			return FAIL_HARDWARE;
		}
		data_fid->if_index = index;
		evt_data.event = IPA_WLAN_AP_LINK_UP_EVENT;
		evt_data.evt_data = data_fid;
		IPACMDBG_H("Posting IPA_WLAN_AP_LINK_UP_EVENT with if index: %d\n",
			data_fid->if_index);
		IPACM_EvtDispatcher::PostEvt(&evt_data);
	}

	if(strncmp(if_name, "wlan", 4) == 0 && upstream)
	{
		data = (ipacm_event_data_mac *)malloc(sizeof(ipacm_event_data_mac));
		if(data == NULL)
		{
			IPACMERR("unable to allocate memory for event_wlan data\n");
			return FAIL_HARDWARE;
		}
		data->if_index = index;
		evt_data.event = IPA_WLAN_STA_LINK_UP_EVENT;
		evt_data.evt_data = data;
		IPACMDBG_H("Posting IPA_WLAN_STA_LINK_UP_EVENT with if index: %d\n",
			data->if_index);
		IPACM_EvtDispatcher::PostEvt(&evt_data);
	}

	return IPACM_SUCCESS;
}
#endif


bool IPACM_OffloadManager::push_framework_event(const char * if_name, _ipacm_offload_prefix prefix)
{
	bool ret =  false;

	for(int i = 0; i < MAX_EVENT_CACHE ;i++)
	{
		if((latest_cache_index >= 0) && (latest_cache_index < MAX_EVENT_CACHE) &&
			(event_cache[latest_cache_index].valid == false))
		{
			//do the copy
			event_cache[latest_cache_index].valid = true;
			event_cache[latest_cache_index].event = IPA_DOWNSTREAM_ADD;
			memcpy(event_cache[latest_cache_index].dev_name, if_name,
				sizeof(event_cache[latest_cache_index].dev_name));
			memcpy(&event_cache[latest_cache_index].prefix_cache, &prefix,
				sizeof(event_cache[latest_cache_index].prefix_cache));

			if (prefix.iptype == IPA_IP_v4) {
				IPACMDBG_H("cache event(%d) subnet info v4Addr (%x) v4Mask (%x) dev(%s) on entry (%d)\n",
						event_cache[latest_cache_index].event,
						event_cache[latest_cache_index].prefix_cache.v4Addr,
						event_cache[latest_cache_index].prefix_cache.v4Mask,
						event_cache[latest_cache_index].dev_name,
						latest_cache_index);
			} else {
				IPACMDBG_H("cache event (%d) v6Addr: %08x:%08x:%08x:%08x \n",
						event_cache[latest_cache_index].event,
						event_cache[latest_cache_index].prefix_cache.v6Addr[0],
						event_cache[latest_cache_index].prefix_cache.v6Addr[1],
						event_cache[latest_cache_index].prefix_cache.v6Addr[2],
						event_cache[latest_cache_index].prefix_cache.v6Addr[3]);
				IPACMDBG_H("subnet v6Mask: %08x:%08x:%08x:%08x dev(%s) on entry(%d),\n",
						event_cache[latest_cache_index].prefix_cache.v6Mask[0],
						event_cache[latest_cache_index].prefix_cache.v6Mask[1],
						event_cache[latest_cache_index].prefix_cache.v6Mask[2],
						event_cache[latest_cache_index].prefix_cache.v6Mask[3],
						event_cache[latest_cache_index].dev_name,
						latest_cache_index);
			}
			latest_cache_index = (latest_cache_index + 1)% MAX_EVENT_CACHE;
			ret = true;
			break;
		}
		latest_cache_index = (latest_cache_index + 1)% MAX_EVENT_CACHE;
		if(i == MAX_EVENT_CACHE - 1)
		{
			IPACMDBG_H(" run out of event cache (%d)\n", i);
			ret = false;
		}
	}
	return ret;
}
