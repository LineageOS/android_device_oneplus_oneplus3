/*
Copyright (c) 2013-2019, The Linux Foundation. All rights reserved.

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
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*!
@file
IPACM_Wlan.cpp

@brief
This file implements the WLAN iface functionality.

@Author
Skylar Chang

*/

#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <IPACM_Wlan.h>
#include <IPACM_Netlink.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <IPACM_Wan.h>
#include <IPACM_Lan.h>
#include <IPACM_IfaceManager.h>
#include <IPACM_ConntrackListener.h>
#ifdef FEATURE_IPACM_HAL
#include "IPACM_OffloadManager.h"
#endif

/* static member to store the number of total wifi clients within all APs*/
int IPACM_Wlan::total_num_wifi_clients = 0;

int IPACM_Wlan::num_wlan_ap_iface = 0;

IPACM_Wlan::IPACM_Wlan(int iface_index) : IPACM_Lan(iface_index)
{
#define WLAN_AMPDU_DEFAULT_FILTER_RULES 3

	wlan_ap_index = IPACM_Wlan::num_wlan_ap_iface;
	if(wlan_ap_index < 0 || wlan_ap_index > 1)
	{
		IPACMERR("Wlan_ap_index is not correct: %d, not creating instance.\n", wlan_ap_index);
		if (tx_prop != NULL)
		{
			free(tx_prop);
		}
		if (rx_prop != NULL)
		{
			free(rx_prop);
		}
		if (iface_query != NULL)
		{
			free(iface_query);
		}
		delete this;
		return;
	}

	num_wifi_client = 0;
	header_name_count = 0;
	wlan_client = NULL;
	wlan_client_len = 0;

	if(iface_query != NULL)
	{
		wlan_client_len = (sizeof(ipa_wlan_client)) + (iface_query->num_tx_props * sizeof(wlan_client_rt_hdl));
		wlan_client = (ipa_wlan_client *)calloc(IPA_MAX_NUM_WIFI_CLIENTS, wlan_client_len);
		if (wlan_client == NULL)
		{
			IPACMERR("unable to allocate memory\n");
			return;
		}
		IPACMDBG_H("index:%d constructor: Tx properties:%d\n", iface_index, iface_query->num_tx_props);
	}
	Nat_App = NatApp::GetInstance();
	if (Nat_App == NULL)
	{
		IPACMERR("unable to get Nat App instance \n");
		return;
	}

	IPACM_Wlan::num_wlan_ap_iface++;
	IPACMDBG_H("Now the number of wlan AP iface is %d\n", IPACM_Wlan::num_wlan_ap_iface);

	m_is_guest_ap = false;
	if (IPACM_Iface::ipacmcfg->iface_table[ipa_if_num].wlan_mode == INTERNET)
	{
		m_is_guest_ap = true;
	}
	IPACMDBG_H("%s: guest ap enable: %d \n",
		IPACM_Iface::ipacmcfg->iface_table[ipa_if_num].iface_name, m_is_guest_ap);

#ifdef FEATURE_IPA_ANDROID
	/* set the IPA-client pipe enum */
	if(ipa_if_cate == WLAN_IF)
	{
#ifdef FEATURE_IPACM_HAL
		handle_tethering_client(false, IPACM_CLIENT_MAX);
#else
		handle_tethering_client(false, IPACM_CLIENT_WLAN);
#endif
	}
#endif
	return;
}


IPACM_Wlan::~IPACM_Wlan()
{
	IPACM_EvtDispatcher::deregistr(this);
	IPACM_IfaceManager::deregistr(this);
	IPACM_Wlan::num_wlan_ap_iface--;
	return;
}

void IPACM_Wlan::event_callback(ipa_cm_event_id event, void *param)
{
	if(is_active == false && event != IPA_LAN_DELETE_SELF)
	{
		IPACMDBG_H("The interface is no longer active, return.\n");
		return;
	}

	int ipa_interface_index;
	int wlan_index;
	ipacm_ext_prop* ext_prop;
	ipacm_event_iface_up_tehter* data_wan_tether;
#ifdef FEATURE_IPACM_HAL
	IPACM_OffloadManager* OffloadMng;
#endif

	switch (event)
	{

	case IPA_WLAN_LINK_DOWN_EVENT:
		{
			ipacm_event_data_fid *data = (ipacm_event_data_fid *)param;
			ipa_interface_index = iface_ipa_index_query(data->if_index);
			if (ipa_interface_index == ipa_if_num)
			{
				IPACMDBG_H("Received IPA_WLAN_LINK_DOWN_EVENT\n");
				handle_down_evt();
				/* reset the AP-iface category to unknown */
				IPACM_Iface::ipacmcfg->iface_table[ipa_if_num].if_cat = UNKNOWN_IF;
				IPACM_Iface::ipacmcfg->DelNatIfaces(dev_name); // delete NAT-iface
				IPACM_Wlan::total_num_wifi_clients = (IPACM_Wlan::total_num_wifi_clients) - \
                                                                     (num_wifi_client);
				return;
			}
		}
		break;

	case IPA_PRIVATE_SUBNET_CHANGE_EVENT:
		{
			ipacm_event_data_fid *data = (ipacm_event_data_fid *)param;
			/* internel event: data->if_index is ipa_if_index */
			if (data->if_index == ipa_if_num)
			{
				IPACMDBG_H("Received IPA_PRIVATE_SUBNET_CHANGE_EVENT from itself posting, ignore\n");
				return;
			}
			else
			{
				IPACMDBG_H("Received IPA_PRIVATE_SUBNET_CHANGE_EVENT from other LAN iface \n");
#ifdef FEATURE_IPA_ANDROID
				handle_private_subnet_android(IPA_IP_v4);
#endif
				IPACMDBG_H(" delete old private subnet rules, use new sets \n");
				return;
			}
		}
		break;

	case IPA_LAN_DELETE_SELF:
	{
		ipacm_event_data_fid *data = (ipacm_event_data_fid *)param;
		if(data->if_index == ipa_if_num)
		{
			IPACMDBG_H("Now the number of wlan AP iface is %d\n", IPACM_Wlan::num_wlan_ap_iface);

			IPACMDBG_H("Received IPA_LAN_DELETE_SELF event.\n");
			IPACMDBG_H("ipa_WLAN (%s):ipa_index (%d) instance close \n", IPACM_Iface::ipacmcfg->iface_table[ipa_if_num].iface_name, ipa_if_num);
#ifdef FEATURE_ETH_BRIDGE_LE
			if(rx_prop != NULL)
			{
				free(rx_prop);
			}
			if(tx_prop != NULL)
			{
				free(tx_prop);
			}
			if(iface_query != NULL)
			{
				free(iface_query);
			}
#endif
			delete this;
		}
		break;
	}

	case IPA_ADDR_ADD_EVENT:
		{
			ipacm_event_data_addr *data = (ipacm_event_data_addr *)param;
			ipa_interface_index = iface_ipa_index_query(data->if_index);

			if ( (data->iptype == IPA_IP_v4 && data->ipv4_addr == 0) ||
					 (data->iptype == IPA_IP_v6 &&
						data->ipv6_addr[0] == 0 && data->ipv6_addr[1] == 0 &&
					  data->ipv6_addr[2] == 0 && data->ipv6_addr[3] == 0) )
			{
				IPACMDBG_H("Invalid address, ignore IPA_ADDR_ADD_EVENT event\n");
				return;
			}

			if (ipa_interface_index == ipa_if_num)
			{
				/* check v4 not setup before, v6 can have 2 iface ip */
				if( ((data->iptype != ip_type) && (ip_type != IPA_IP_MAX))
				    || ((data->iptype==IPA_IP_v6) && (num_dft_rt_v6!=MAX_DEFAULT_v6_ROUTE_RULES)))
				{
					IPACMDBG_H("Got IPA_ADDR_ADD_EVENT ip-family:%d, v6 num %d: \n",data->iptype,num_dft_rt_v6);
					/* Post event to NAT */
					if (data->iptype == IPA_IP_v4)
					{
						ipacm_cmd_q_data evt_data;
						ipacm_event_iface_up *info;

						info = (ipacm_event_iface_up *)
							 malloc(sizeof(ipacm_event_iface_up));
						if (info == NULL)
						{
							IPACMERR("Unable to allocate memory\n");
							return;
						}

						memcpy(info->ifname, dev_name, IF_NAME_LEN);
						info->ipv4_addr = data->ipv4_addr;
						info->addr_mask = IPACM_Iface::ipacmcfg->private_subnet_table[0].subnet_mask;

						evt_data.event = IPA_HANDLE_WLAN_UP;
						evt_data.evt_data = (void *)info;

						/* Insert IPA_HANDLE_WLAN_UP to command queue */
						IPACMDBG_H("posting IPA_HANDLE_WLAN_UP for IPv4 with below information\n");
						IPACMDBG_H("IPv4 address:0x%x, IPv4 address mask:0x%x\n",
										 info->ipv4_addr, info->addr_mask);
						IPACM_EvtDispatcher::PostEvt(&evt_data);
#ifdef FEATURE_IPACM_RESTART
						/* Query wlan-clients */
						ipa_query_wlan_client();
#endif
					}

					if(handle_addr_evt(data) == IPACM_FAILURE)
					{
						return;
					}

#ifdef FEATURE_IPA_ANDROID
					add_dummy_private_subnet_flt_rule(data->iptype);
					handle_private_subnet_android(data->iptype);
#else
					handle_private_subnet(data->iptype);
#endif

#ifndef FEATURE_IPACM_HAL
					if (IPACM_Wan::isWanUP(ipa_if_num))
					{
						if(data->iptype == IPA_IP_v4 || data->iptype == IPA_IP_MAX)
						{
							if(IPACM_Wan::backhaul_mode == Q6_WAN)
							{
								ext_prop = IPACM_Iface::ipacmcfg->GetExtProp(IPA_IP_v4);
								IPACM_Lan::handle_wan_up_ex(ext_prop, IPA_IP_v4,
												IPACM_Wan::getXlat_Mux_Id());
							}
							else
							{
								IPACM_Lan::handle_wan_up(IPA_IP_v4);
							}
						}
						IPACMDBG_H("Finished checking wan_up\n");
					} else {
						IPACMDBG_H("Wan_V4 haven't up yet \n");
					}

					if(IPACM_Wan::isWanUP_V6(ipa_if_num))
					{
						if((data->iptype == IPA_IP_v6 || data->iptype == IPA_IP_MAX) && num_dft_rt_v6 == 1)
						{
							memcpy(ipv6_prefix, IPACM_Wan::backhaul_ipv6_prefix, sizeof(ipv6_prefix));
							install_ipv6_prefix_flt_rule(IPACM_Wan::backhaul_ipv6_prefix);

							if(IPACM_Wan::backhaul_mode == Q6_WAN)
							{
								ext_prop = IPACM_Iface::ipacmcfg->GetExtProp(IPA_IP_v6);
								IPACM_Lan::handle_wan_up_ex(ext_prop, IPA_IP_v6, 0);
							}
							else
							{
								IPACM_Lan::handle_wan_up(IPA_IP_v6);
							}
						}
						IPACMDBG_H("Finished checking wan_up_v6\n");
					} else {
						IPACMDBG_H("Wan_V6 haven't up yet \n");
					}
#else
					/* check if Upstream was set before */
					if (IPACM_Wan::isWanUP(ipa_if_num))
					{
						IPACMDBG_H("Upstream was set previously for ipv4, change is_upstream_set flag\n");
						is_upstream_set[IPA_IP_v4] = true;
					}
					if (IPACM_Wan::isWanUP_V6(ipa_if_num))
					{
						IPACMDBG_H("Upstream was set previously for ipv6, change is_upstream_set flag\n");
						is_upstream_set[IPA_IP_v6] = true;
					}
#endif
					/* checking if SW-RT_enable */
					if (IPACM_Iface::ipacmcfg->ipa_sw_rt_enable == true)
					{
						/* handle software routing enable event*/
						IPACMDBG_H("IPA_SW_ROUTING_ENABLE for iface: %s \n",IPACM_Iface::ipacmcfg->iface_table[ipa_if_num].iface_name);
						handle_software_routing_enable(false);
					}
				}
			}
		}
		break;
#ifdef FEATURE_IPA_ANDROID
	case IPA_HANDLE_WAN_UP_TETHER:
		IPACMDBG_H("Received IPA_HANDLE_WAN_UP_TETHER event\n");

		data_wan_tether = (ipacm_event_iface_up_tehter*)param;
		if(data_wan_tether == NULL)
		{
			IPACMERR("No event data is found.\n");
			return;
		}
		IPACMDBG_H("Backhaul is sta mode?%d, if_index_tether:%d tether_if_name:%s xlat_mux_id: %d\n", data_wan_tether->backhaul_type,
					data_wan_tether->if_index_tether,
					IPACM_Iface::ipacmcfg->iface_table[data_wan_tether->if_index_tether].iface_name,
					data_wan_tether->xlat_mux_id);
#ifndef FEATURE_IPACM_HAL
		if (data_wan_tether->if_index_tether != ipa_if_num)
		{
			IPACMERR("IPA_HANDLE_WAN_UP_TETHER tether_if(%d), not valid (%d) ignore\n", data_wan_tether->if_index_tether, ipa_if_num);
			return;
		}
#endif
		if(ip_type == IPA_IP_v4 || ip_type == IPA_IP_MAX)
		{
#ifdef FEATURE_IPACM_HAL
			if(is_upstream_set[IPA_IP_v4] == false)
			{
				IPACMDBG_H("Add upstream for IPv4.\n");
				is_upstream_set[IPA_IP_v4] = true;
				if(is_downstream_set[IPA_IP_v4] == true)
				{
					IPACMDBG_H("Downstream was set before, adding UL rules.\n");
					if(data_wan_tether->backhaul_type == Q6_WAN)
					{
						ext_prop = IPACM_Iface::ipacmcfg->GetExtProp(IPA_IP_v4);
						handle_wan_up_ex(ext_prop, IPA_IP_v4,
							data_wan_tether->xlat_mux_id);
					} else {
						handle_wan_up(IPA_IP_v4);
					}
				}
			}
#else
			if(data_wan_tether->backhaul_type == Q6_WAN)
			{
				ext_prop = IPACM_Iface::ipacmcfg->GetExtProp(IPA_IP_v4);
				handle_wan_up_ex(ext_prop, IPA_IP_v4, 0);
			} else {
				handle_wan_up(IPA_IP_v4);
			}
#endif
		}
		break;

	case IPA_HANDLE_WAN_UP_V6_TETHER:
		IPACMDBG_H("Received IPA_HANDLE_WAN_UP_V6_TETHER event\n");

		data_wan_tether = (ipacm_event_iface_up_tehter*)param;
		if(data_wan_tether == NULL)
		{
			IPACMERR("No event data is found.\n");
			return;
		}
		IPACMDBG_H("Backhaul is sta mode?%d, if_index_tether:%d tether_if_name:%s\n", data_wan_tether->backhaul_type,
					data_wan_tether->if_index_tether,
					IPACM_Iface::ipacmcfg->iface_table[data_wan_tether->if_index_tether].iface_name);
#ifndef FEATURE_IPACM_HAL
		if (data_wan_tether->if_index_tether != ipa_if_num)
		{
			IPACMERR("IPA_HANDLE_WAN_UP_V6_TETHER tether_if(%d), not valid (%d) ignore\n", data_wan_tether->if_index_tether, ipa_if_num);
			return;
		}
#endif
		if(ip_type == IPA_IP_v6 || ip_type == IPA_IP_MAX)
		{
#ifdef FEATURE_IPACM_HAL
			if(is_upstream_set[IPA_IP_v6] == false)
			{
				IPACMDBG_H("Add upstream for IPv6.\n");
				is_upstream_set[IPA_IP_v6] = true;

				if(is_downstream_set[IPA_IP_v6] == true)
				{
					IPACMDBG_H("Downstream was set before, adding UL rules.\n");
					memcpy(ipv6_prefix, data_wan_tether->ipv6_prefix, sizeof(ipv6_prefix));
					install_ipv6_prefix_flt_rule(data_wan_tether->ipv6_prefix);
					if(data_wan_tether->backhaul_type == Q6_WAN)
					{
						ext_prop = IPACM_Iface::ipacmcfg->GetExtProp(IPA_IP_v6);
						handle_wan_up_ex(ext_prop, IPA_IP_v6, 0);
					}
					else
					{
						handle_wan_up(IPA_IP_v6);
					}
				}
			}
#else
			if(data_wan_tether->backhaul_type == Q6_WAN)
			{
				ext_prop = IPACM_Iface::ipacmcfg->GetExtProp(IPA_IP_v6);
				handle_wan_up_ex(ext_prop, IPA_IP_v6, 0);
			}
			else
			{
				handle_wan_up(IPA_IP_v6);
			}
#endif
		}
		break;

	case IPA_HANDLE_WAN_DOWN_TETHER:
		IPACMDBG_H("Received IPA_HANDLE_WAN_DOWN_TETHER event\n");
		data_wan_tether = (ipacm_event_iface_up_tehter*)param;
		if(data_wan_tether == NULL)
		{
			IPACMERR("No event data is found.\n");
			return;
		}
		if(rx_prop == NULL)
		{
			IPACMERR("No rx prop.\n");
			return;
		}
		IPACMDBG_H("Backhaul is sta mode?%d, if_index_tether:%d tether_if_name:%s\n", data_wan_tether->backhaul_type,
					data_wan_tether->if_index_tether,
					IPACM_Iface::ipacmcfg->iface_table[data_wan_tether->if_index_tether].iface_name);
#ifndef FEATURE_IPACM_HAL
		if (data_wan_tether->if_index_tether != ipa_if_num)
		{
			IPACMERR("IPA_HANDLE_WAN_DOWN_TETHER tether_if(%d), not valid (%d) ignore\n", data_wan_tether->if_index_tether, ipa_if_num);
			return;
		}
#endif
		if(ip_type == IPA_IP_v4 || ip_type == IPA_IP_MAX)
		{
#ifdef FEATURE_IPACM_HAL
			if(is_upstream_set[IPA_IP_v4] == true)
			{
				IPACMDBG_H("Del upstream for IPv4.\n");
				is_upstream_set[IPA_IP_v4] = false;
				if(is_downstream_set[IPA_IP_v4] == true)
				{
					IPACMDBG_H("Downstream was set before, deleting UL rules.\n");
					handle_wan_down(data_wan_tether->backhaul_type);
				}
			}
#else
			handle_wan_down(data_wan_tether->backhaul_type);
#endif
		}
		break;

	case IPA_HANDLE_WAN_DOWN_V6_TETHER:
		IPACMDBG_H("Received IPA_HANDLE_WAN_DOWN_V6_TETHER event\n");
		data_wan_tether = (ipacm_event_iface_up_tehter*)param;
		if(data_wan_tether == NULL)
		{
			IPACMERR("No event data is found.\n");
			return;
		}
		if(rx_prop == NULL)
		{
			IPACMERR("No rx prop.\n");
			return;
		}
		IPACMDBG_H("Backhaul is sta mode?%d, if_index_tether:%d tether_if_name:%s\n", data_wan_tether->backhaul_type,
					data_wan_tether->if_index_tether,
					IPACM_Iface::ipacmcfg->iface_table[data_wan_tether->if_index_tether].iface_name);
#ifndef FEATURE_IPACM_HAL
		if (data_wan_tether->if_index_tether != ipa_if_num)
		{
			IPACMERR("IPA_HANDLE_WAN_DOWN_V6_TETHER tether_if(%d), not valid (%d) ignore\n", data_wan_tether->if_index_tether, ipa_if_num);
			return;
		}
#endif
		if(ip_type == IPA_IP_v6 || ip_type == IPA_IP_MAX)
		{
#ifdef FEATURE_IPACM_HAL
			if(is_upstream_set[IPA_IP_v6] == true)
			{
				IPACMDBG_H("Del upstream for IPv6.\n");
				is_upstream_set[IPA_IP_v6] = false;
				if(is_downstream_set[IPA_IP_v6] == true)
				{
					IPACMDBG_H("Downstream was set before, deleting UL rules.\n");
					/* reset usb-client ipv6 rt-rules */
					handle_wlan_client_reset_rt(IPA_IP_v6);
					handle_wan_down_v6(data_wan_tether->backhaul_type);
				}
			}
#else
			/* reset usb-client ipv6 rt-rules */
			handle_wlan_client_reset_rt(IPA_IP_v6);
			handle_wan_down_v6(data_wan_tether->backhaul_type);
#endif
		}
		break;

	case IPA_DOWNSTREAM_ADD:
	{
		ipacm_event_ipahal_stream *data = (ipacm_event_ipahal_stream *)param;
		ipa_interface_index = iface_ipa_index_query(data->if_index);
		if(ipa_interface_index == ipa_if_num)
		{
			IPACMDBG_H("Received IPA_DOWNSTREAM_ADD event.\n");
#ifdef FEATURE_IPA_ANDROID
			/* indicate v4-offload */
			IPACM_OffloadManager::num_offload_v4_tethered_iface++;

			/* xlat not support for 2st tethered iface sky */
			if (IPACM_Wan::isXlat() && (data->prefix.iptype == IPA_IP_v4) && (IPACM_OffloadManager::num_offload_v4_tethered_iface > 1))
			{
				IPACMDBG_H("Not support 2st downstream iface %s for xlat, cur: %d\n", dev_name,
					IPACM_OffloadManager::num_offload_v4_tethered_iface);
				return;
			}

			IPACMDBG_H(" support downstream iface %s, cur %d\n", dev_name,
				IPACM_OffloadManager::num_offload_v4_tethered_iface);
#endif
			if(data->prefix.iptype < IPA_IP_MAX && is_downstream_set[data->prefix.iptype] == false)
			{
				IPACMDBG_H("Add downstream for IP iptype %d.\n", data->prefix.iptype);
				is_downstream_set[data->prefix.iptype] = true;
				memcpy(&prefix[data->prefix.iptype], &data->prefix,
					sizeof(prefix[data->prefix.iptype]));

				if (is_upstream_set[data->prefix.iptype] == true)
				{
					IPACMDBG_H("Upstream was set before, adding modem UL rules.\n");
					if(ip_type == IPA_IP_MAX || ip_type == data->prefix.iptype)
					{
						if (data->prefix.iptype == IPA_IP_v6) /* ipv6 only */
						{
							/* Only offload clients has same prefix as Android gave */
							ipv6_prefix[0] = data->prefix.v6Addr[0];
							ipv6_prefix[1] = data->prefix.v6Addr[1];
							IPACMDBG_H("ipv6_prefix0x%x:%x\n", ipv6_prefix[0], ipv6_prefix[1]);
							install_ipv6_prefix_flt_rule(ipv6_prefix);
						}

						if (IPACM_Wan::backhaul_mode == Q6_WAN) /* LTE */
						{
							ext_prop = IPACM_Iface::ipacmcfg->GetExtProp(data->prefix.iptype);
							if (data->prefix.iptype == IPA_IP_v4)
							{
								handle_wan_up_ex(ext_prop, data->prefix.iptype,
									IPACM_Wan::getXlat_Mux_Id());
							}
							else {
								handle_wan_up_ex(ext_prop, data->prefix.iptype, 0);
							}
						} else {
							handle_wan_up(data->prefix.iptype); /* STA */
						}
					}
				}
			}
		}
		break;
	}

	case IPA_DOWNSTREAM_DEL:
	{
		ipacm_event_ipahal_stream *data = (ipacm_event_ipahal_stream *)param;
		ipa_interface_index = iface_ipa_index_query(data->if_index);
		if(ipa_interface_index == ipa_if_num)
		{
			IPACMDBG_H("Received IPA_DOWNSTREAM_DEL event.\n");
			if(is_downstream_set[data->prefix.iptype] == true)
			{
				IPACMDBG_H("Del downstream for IP iptype %d.\n", data->prefix.iptype);
				is_downstream_set[data->prefix.iptype] = false;

				if(is_upstream_set[data->prefix.iptype] == true)
				{
					IPACMDBG_H("Upstream was set before, deleting UL rules.\n");
					if (data->prefix.iptype == IPA_IP_v4)
					{
						handle_wan_down(IPACM_Wan::backhaul_mode); /* LTE STA */
					} else {
						handle_wlan_client_reset_rt(IPA_IP_v6);
						handle_wan_down_v6(IPACM_Wan::backhaul_mode); /* LTE STA */
					}
				}
			}
		}
		break;
	}
#else
	case IPA_HANDLE_WAN_UP:
		IPACMDBG_H("Received IPA_HANDLE_WAN_UP event\n");

		ipacm_event_iface_up* data_wan = (ipacm_event_iface_up*)param;
		if(data_wan == NULL)
		{
			IPACMERR("No event data is found.\n");
			return;
		}
		IPACMDBG_H("Backhaul is sta mode?%d\n", data_wan->backhaul_type);
		if(ip_type == IPA_IP_v4 || ip_type == IPA_IP_MAX)
		{
			if(data_wan->backhaul_type == Q6_WAN)
			{
				ext_prop = IPACM_Iface::ipacmcfg->GetExtProp(IPA_IP_v4);
				IPACM_Lan::handle_wan_up_ex(ext_prop, IPA_IP_v4, data_wan->xlat_mux_id);
			}
			else
			{
				IPACM_Lan::handle_wan_up(IPA_IP_v4);
			}
		}
		break;

	case IPA_HANDLE_WAN_UP_V6:
		IPACMDBG_H("Received IPA_HANDLE_WAN_UP_V6 event\n");

		data_wan = (ipacm_event_iface_up*)param;
		if(data_wan == NULL)
		{
			IPACMERR("No event data is found.\n");
			return;
		}
		IPACMDBG_H("Backhaul is sta mode?%d\n", data_wan->backhaul_type);
		if(ip_type == IPA_IP_v6 || ip_type == IPA_IP_MAX)
		{
			memcpy(ipv6_prefix, data_wan->ipv6_prefix, sizeof(ipv6_prefix));
			install_ipv6_prefix_flt_rule(data_wan->ipv6_prefix);

			if(data_wan->backhaul_type == Q6_WAN)
			{
				ext_prop = IPACM_Iface::ipacmcfg->GetExtProp(IPA_IP_v6);
				IPACM_Lan::handle_wan_up_ex(ext_prop, IPA_IP_v6, 0);
			}
			else
			{
				IPACM_Lan::handle_wan_up(IPA_IP_v6);
			}
		}
		break;

	case IPA_HANDLE_WAN_DOWN:
		IPACMDBG_H("Received IPA_HANDLE_WAN_DOWN event\n");
		data_wan = (ipacm_event_iface_up*)param;
		if(data_wan == NULL)
		{
			IPACMERR("No event data is found.\n");
			return;
		}
		IPACMDBG_H("Backhaul is sta mode?%d\n", data_wan->backhaul_type);
		if (rx_prop != NULL)
		{
			if(ip_type == IPA_IP_v4 || ip_type == IPA_IP_MAX)
			{
				handle_wan_down(data_wan->backhaul_type);
			}
		}
		break;

	case IPA_HANDLE_WAN_DOWN_V6:
		IPACMDBG_H("Received IPA_HANDLE_WAN_DOWN_V6 event\n");
		data_wan = (ipacm_event_iface_up*)param;
		if(data_wan == NULL)
		{
			IPACMERR("No event data is found.\n");
			return;
		}
		/* clean up v6 RT rules*/
		IPACMDBG_H("Received IPA_WAN_V6_DOWN in WLAN-instance and need clean up client IPv6 address \n");
		/* reset wifi-client ipv6 rt-rules */
		handle_wlan_client_reset_rt(IPA_IP_v6);
		IPACMDBG_H("Backhaul is sta mode ? %d\n", data_wan->backhaul_type);
		if (rx_prop != NULL)
		{
			if(ip_type == IPA_IP_v6 || ip_type == IPA_IP_MAX)
			{
				handle_wan_down_v6(data_wan->backhaul_type);
			}
		}
		break;
#endif

	case IPA_WLAN_CLIENT_ADD_EVENT_EX:
		{
			ipacm_event_data_wlan_ex *data = (ipacm_event_data_wlan_ex *)param;
			ipa_interface_index = iface_ipa_index_query(data->if_index);
			if (ipa_interface_index == ipa_if_num)
			{
				int i;
				for(i=0; i<data->num_of_attribs; i++)
				{
					if(data->attribs[i].attrib_type == WLAN_HDR_ATTRIB_MAC_ADDR)
					{
						eth_bridge_post_event(IPA_ETH_BRIDGE_CLIENT_ADD, IPA_IP_MAX, data->attribs[i].u.mac_addr, NULL, NULL,
							IPA_CLIENT_MAX);
						break;
					}
				}
				IPACMDBG_H("Received IPA_WLAN_CLIENT_ADD_EVENT\n");
				handle_wlan_client_init_ex(data);
			}
		}
		break;

	case IPA_WIGIG_CLIENT_ADD_EVENT:
	{
		ipacm_event_data_mac_ep *data = (ipacm_event_data_mac_ep *)param;
		ipa_interface_index = iface_ipa_index_query(data->if_index);
		if(ipa_interface_index == ipa_if_num)
		{
			IPACMDBG_H("Received IPA_WIGIG_CLIENT_ADD_EVENT\n");
			handle_wigig_client_add(data);
		}
	}
	break;

	case IPA_WLAN_CLIENT_DEL_EVENT:
		{
			ipacm_event_data_mac *data = (ipacm_event_data_mac *)param;
			ipa_interface_index = iface_ipa_index_query(data->if_index);
			if (ipa_interface_index == ipa_if_num)
			{
				IPACMDBG_H("Received IPA_WLAN_CLIENT_DEL_EVENT\n");
				eth_bridge_post_event(IPA_ETH_BRIDGE_CLIENT_DEL, IPA_IP_MAX, data->mac_addr, NULL, NULL, IPA_CLIENT_MAX);
				handle_wlan_client_down_evt(data->mac_addr);
			}
		}
		break;

	case IPA_WLAN_CLIENT_POWER_SAVE_EVENT:
		{
			ipacm_event_data_mac *data = (ipacm_event_data_mac *)param;
			ipa_interface_index = iface_ipa_index_query(data->if_index);
			if (ipa_interface_index == ipa_if_num)
			{
				IPACMDBG_H("Received IPA_WLAN_CLIENT_POWER_SAVE_EVENT\n");
				handle_wlan_client_pwrsave(data->mac_addr);
			}
		}
		break;

	case IPA_WLAN_CLIENT_RECOVER_EVENT:
		{
			ipacm_event_data_mac *data = (ipacm_event_data_mac *)param;
			ipa_interface_index = iface_ipa_index_query(data->if_index);
			if (ipa_interface_index == ipa_if_num)
			{
				IPACMDBG_H("Received IPA_WLAN_CLIENT_RECOVER_EVENT\n");

				wlan_index = get_wlan_client_index(data->mac_addr);
				if ((wlan_index != IPACM_INVALID_INDEX) &&
						(get_client_memptr(wlan_client, wlan_index)->power_save_set == true))
				{

					IPACMDBG_H("change wlan client out of  power safe mode \n");
					get_client_memptr(wlan_client, wlan_index)->power_save_set = false;

					/* First add route rules and then nat rules */
					if(get_client_memptr(wlan_client, wlan_index)->ipv4_set == true) /* for ipv4 */
					{
						     IPACMDBG_H("recover client index(%d):ipv4 address: 0x%x\n",
										 wlan_index,
										 get_client_memptr(wlan_client, wlan_index)->v4_addr);

						IPACMDBG_H("Adding Route Rules\n");
						handle_wlan_client_route_rule(data->mac_addr, IPA_IP_v4);
						IPACMDBG_H("Adding Nat Rules\n");
						Nat_App->ResetPwrSaveIf(get_client_memptr(wlan_client, wlan_index)->v4_addr);
					}

					if(get_client_memptr(wlan_client, wlan_index)->ipv6_set != 0) /* for ipv6 */
					{
						handle_wlan_client_route_rule(data->mac_addr, IPA_IP_v6);
					}
				}
			}
		}
		break;

	case IPA_NEIGH_CLIENT_IP_ADDR_ADD_EVENT:
		{
			ipacm_event_data_all *data = (ipacm_event_data_all *)param;
			ipa_interface_index = iface_ipa_index_query(data->if_index);
			if (ipa_interface_index == ipa_if_num)
			{
				IPACMDBG_H("Received IPA_NEIGH_CLIENT_IP_ADDR_ADD_EVENT\n");
				if (handle_wlan_client_ipaddr(data) == IPACM_FAILURE)
				{
					return;
				}

				handle_wlan_client_route_rule(data->mac_addr, data->iptype);
				if(data->iptype == IPA_IP_v4)
				{
					/* Add NAT rules after ipv4 RT rules are set */
					CtList->HandleNeighIpAddrAddEvt(data);
					//Nat_App->ResetPwrSaveIf(data->ipv4_addr);
				}
			}
		}
		break;
		/* handle software routing enable event, iface will update softwarerouting_act to true*/
	case IPA_SW_ROUTING_ENABLE:
		IPACMDBG_H("Received IPA_SW_ROUTING_ENABLE\n");
		IPACM_Iface::handle_software_routing_enable(false);
		break;

		/* handle software routing disable event, iface will update softwarerouting_act to false*/
	case IPA_SW_ROUTING_DISABLE:
		IPACMDBG_H("Received IPA_SW_ROUTING_DISABLE\n");
		IPACM_Iface::handle_software_routing_disable(false);
		break;

	case IPA_WLAN_SWITCH_TO_SCC:
		IPACMDBG_H("Received IPA_WLAN_SWITCH_TO_SCC\n");
		if(ip_type == IPA_IP_MAX)
		{
			handle_SCC_MCC_switch(IPA_IP_v4);
			handle_SCC_MCC_switch(IPA_IP_v6);
		}
		else
		{
			handle_SCC_MCC_switch(ip_type);
		}
		eth_bridge_post_event(IPA_ETH_BRIDGE_WLAN_SCC_MCC_SWITCH, IPA_IP_MAX, NULL, NULL, NULL, IPA_CLIENT_MAX);
		break;

	case IPA_WLAN_SWITCH_TO_MCC:
		IPACMDBG_H("Received IPA_WLAN_SWITCH_TO_MCC\n");
		if(ip_type == IPA_IP_MAX)
		{
			handle_SCC_MCC_switch(IPA_IP_v4);
			handle_SCC_MCC_switch(IPA_IP_v6);
		}
		else
		{
			handle_SCC_MCC_switch(ip_type);
		}
		eth_bridge_post_event(IPA_ETH_BRIDGE_WLAN_SCC_MCC_SWITCH, IPA_IP_MAX, NULL, NULL, NULL, IPA_CLIENT_MAX);
		break;

	case IPA_CRADLE_WAN_MODE_SWITCH:
	{
		IPACMDBG_H("Received IPA_CRADLE_WAN_MODE_SWITCH event.\n");
		ipacm_event_cradle_wan_mode* wan_mode = (ipacm_event_cradle_wan_mode*)param;
		if(wan_mode == NULL)
		{
			IPACMERR("Event data is empty.\n");
			return;
		}

		if(wan_mode->cradle_wan_mode == BRIDGE)
		{
			handle_cradle_wan_mode_switch(true);
		}
		else
		{
			handle_cradle_wan_mode_switch(false);
		}
	}
	break;
	case IPA_CFG_CHANGE_EVENT:
	{
		IPACMDBG_H("Received IPA_CFG_CHANGE_EVENT event for %s with new wlan-mode: %s old wlan-mode: %s\n",
				IPACM_Iface::ipacmcfg->iface_table[ipa_if_num].iface_name,
				(IPACM_Iface::ipacmcfg->iface_table[ipa_if_num].wlan_mode == 0) ? "full" : "internet",
				(m_is_guest_ap == true) ? "internet" : "full");
		/* Add Natting iface to IPACM_Config if there is  Rx/Tx property */
		if (rx_prop != NULL || tx_prop != NULL)
		{
			IPACMDBG_H(" Has rx/tx properties registered for iface %s, add for NATTING \n", dev_name);
			IPACM_Iface::ipacmcfg->AddNatIfaces(dev_name, IPA_IP_MAX);
		}

		if (m_is_guest_ap == true && (IPACM_Iface::ipacmcfg->iface_table[ipa_if_num].wlan_mode == FULL))
		{
			m_is_guest_ap = false;
			IPACMDBG_H("wlan mode is switched to full access mode. \n");
			eth_bridge_handle_wlan_mode_switch();
		}
		else if (m_is_guest_ap == false && (IPACM_Iface::ipacmcfg->iface_table[ipa_if_num].wlan_mode == INTERNET))
		{
			m_is_guest_ap = true;
			IPACMDBG_H("wlan mode is switched to internet only access mode. \n");
			eth_bridge_handle_wlan_mode_switch();
		}
		else
		{
			IPACMDBG_H("No change in %s access mode. \n",
					IPACM_Iface::ipacmcfg->iface_table[ipa_if_num].iface_name);
		}
	}
	break;
	case IPA_TETHERING_STATS_UPDATE_EVENT:
	{
		IPACMDBG_H("Received IPA_TETHERING_STATS_UPDATE_EVENT event.\n");
		if (IPACM_Wan::isWanUP(ipa_if_num) || IPACM_Wan::isWanUP_V6(ipa_if_num))
		{
			if(IPACM_Wan::backhaul_mode == Q6_WAN) /* LTE */
			{
				ipa_get_data_stats_resp_msg_v01 *data = (ipa_get_data_stats_resp_msg_v01 *)param;
				if (data->ipa_stats_type != QMI_IPA_STATS_TYPE_PIPE_V01)
				{
					IPACMERR("not valid pipe stats\n");
					return;
				}
				handle_tethering_stats_event(data);
			};
		}
	}
	break;
#ifdef FEATURE_IPACM_HAL
	/* WA for WLAN to clean up NAT instance during SSR */
	case IPA_SSR_NOTICE:
	{
		IPACMDBG_H("Received IPA_SSR_NOTICE event.\n");
		IPACM_Iface::ipacmcfg->DelNatIfaces(dev_name); // delete NAT-iface
	}
	break;
	case IPA_WLAN_FWR_SSR_BEFORE_SHUTDOWN_NOTICE:
        {
                IPACMDBG_H("Received IPA_WLAN_FWR_SSR_BEFORE_SHUTDOWN_NOTICE.\n");

                /* internal push add_downstream event in cache */
                OffloadMng = IPACM_OffloadManager::GetInstance();
                if (OffloadMng == NULL) {
                        IPACMERR("failed to get IPACM_OffloadManager instance !\n");
                } else {
                        IPACMDBG_H("Update iface %s add_downstream cache events\n", dev_name);
			if(ip_type == IPA_IP_v4 || ip_type == IPA_IP_MAX)
			{
				OffloadMng->push_framework_event(dev_name, prefix[IPA_IP_v4]);
			}
			else if(ip_type == IPA_IP_v6 || ip_type == IPA_IP_MAX)
			{
				OffloadMng->push_framework_event(dev_name, prefix[IPA_IP_v6]);
			}
                }
                IPACM_Iface::ipacmcfg->DelNatIfaces(dev_name); // delete NAT-iface
        }
        break;
#endif
	default:
		break;
	}
	return;
}


int IPACM_Wlan::handle_wigig_client_add(ipacm_event_data_mac_ep *data)
{
	ipacm_event_data_wlan_ex *wlan_data;
	int ret = IPACM_SUCCESS;

	wlan_data = (ipacm_event_data_wlan_ex *)malloc(sizeof(ipa_wlan_msg_ex) + sizeof(ipa_wlan_hdr_attrib_val));
	if(wlan_data == NULL)
	{
		IPACMERR("Unable to allocate memory\n");
		return IPACM_FAILURE;
	}

	wlan_data->num_of_attribs = 1;
	wlan_data->if_index = data->if_index;
	wlan_data->attribs[0].attrib_type = WLAN_HDR_ATTRIB_MAC_ADDR;
	wlan_data->attribs[0].offset = 0;
	memcpy(wlan_data->attribs[0].u.mac_addr, data->mac_addr, sizeof(wlan_data->attribs[0].u.mac_addr));

	eth_bridge_post_event(IPA_ETH_BRIDGE_CLIENT_ADD, IPA_IP_MAX, data->mac_addr, NULL, NULL, data->client);
	handle_wlan_client_init_ex(wlan_data);

	int idx = get_wlan_client_index(data->mac_addr);

	if(idx == IPACM_INVALID_INDEX)
	{
		IPACMERR("wlan client not attached\n");
		ret = IPACM_FAILURE;
		goto fail;
	}

	/* store client pipe hdl for the time we add the DL header */
	get_client_memptr(wlan_client, idx)->wigig_ipa_client = data->client;
fail:
	free(wlan_data);
	return ret;
}

/* handle wifi client initial,copy all partial headers (tx property) */
int IPACM_Wlan::handle_wlan_client_init_ex(ipacm_event_data_wlan_ex *data)
{

#define WLAN_IFACE_INDEX_LEN 2

	int res = IPACM_SUCCESS, len = 0, i, evt_size;
	char index[WLAN_IFACE_INDEX_LEN];
	struct ipa_ioc_copy_hdr sCopyHeader;
	struct ipa_ioc_add_hdr *pHeaderDescriptor = NULL;
        uint32_t cnt;

	/* start of adding header */
	IPACMDBG_H("Wifi client number for this iface: %d & total number of wlan clients: %d\n",
                 num_wifi_client,IPACM_Wlan::total_num_wifi_clients);

	if ((num_wifi_client >= IPA_MAX_NUM_WIFI_CLIENTS) ||
			(IPACM_Wlan::total_num_wifi_clients >= IPA_MAX_NUM_WIFI_CLIENTS))
	{
		IPACMERR("Reached maximum number of wlan clients\n");
		return IPACM_FAILURE;
	}

	IPACMDBG_H("Wifi client number: %d\n", num_wifi_client);

	/* add header to IPA */
	if(tx_prop != NULL)
	{
		len = sizeof(struct ipa_ioc_add_hdr) + (1 * sizeof(struct ipa_hdr_add));
		pHeaderDescriptor = (struct ipa_ioc_add_hdr *)calloc(1, len);
		if (pHeaderDescriptor == NULL)
		{
			IPACMERR("calloc failed to allocate pHeaderDescriptor\n");
			return IPACM_FAILURE;
		}

		evt_size = sizeof(ipacm_event_data_wlan_ex) + data->num_of_attribs * sizeof(struct ipa_wlan_hdr_attrib_val);
		get_client_memptr(wlan_client, num_wifi_client)->p_hdr_info = (ipacm_event_data_wlan_ex*)malloc(evt_size);
		memcpy(get_client_memptr(wlan_client, num_wifi_client)->p_hdr_info, data, evt_size);

		/* copy partial header for v4*/
		for (cnt=0; cnt<tx_prop->num_tx_props; cnt++)
		{
			if(tx_prop->tx[cnt].ip==IPA_IP_v4)
			{
				IPACMDBG_H("Got partial v4-header name from %d tx props\n", cnt);
				memset(&sCopyHeader, 0, sizeof(sCopyHeader));
				memcpy(sCopyHeader.name,
							 tx_prop->tx[cnt].hdr_name,
							 sizeof(sCopyHeader.name));

				IPACMDBG_H("header name: %s in tx:%d\n", sCopyHeader.name,cnt);
				if (m_header.CopyHeader(&sCopyHeader) == false)
				{
					PERROR("ioctl copy header failed");
					res = IPACM_FAILURE;
					goto fail;
				}

				IPACMDBG_H("header length: %d, paritial: %d\n", sCopyHeader.hdr_len, sCopyHeader.is_partial);
				if (sCopyHeader.hdr_len > IPA_HDR_MAX_SIZE)
				{
					IPACMERR("header oversize\n");
					res = IPACM_FAILURE;
					goto fail;
				}
				else
				{
					memcpy(pHeaderDescriptor->hdr[0].hdr,
								 sCopyHeader.hdr,
								 sCopyHeader.hdr_len);
				}

				for(i = 0; i < data->num_of_attribs; i++)
				{
					if(data->attribs[i].attrib_type == WLAN_HDR_ATTRIB_MAC_ADDR)
					{
						memcpy(get_client_memptr(wlan_client, num_wifi_client)->mac,
								data->attribs[i].u.mac_addr,
								sizeof(get_client_memptr(wlan_client, num_wifi_client)->mac));

						/* copy client mac_addr to partial header */
						memcpy(&pHeaderDescriptor->hdr[0].hdr[data->attribs[i].offset],
									 get_client_memptr(wlan_client, num_wifi_client)->mac,
									 IPA_MAC_ADDR_SIZE);
						/* replace src mac to bridge mac_addr if any  */
						if (IPACM_Iface::ipacmcfg->ipa_bridge_enable)
						{
							memcpy(&pHeaderDescriptor->hdr[0].hdr[data->attribs[i].offset+IPA_MAC_ADDR_SIZE],
									 IPACM_Iface::ipacmcfg->bridge_mac,
									 IPA_MAC_ADDR_SIZE);
							IPACMDBG_H("device is in bridge mode \n");
						}

					}
					else if(data->attribs[i].attrib_type == WLAN_HDR_ATTRIB_STA_ID)
					{
						/* copy client id to header */
						memcpy(&pHeaderDescriptor->hdr[0].hdr[data->attribs[i].offset],
									&data->attribs[i].u.sta_id, sizeof(data->attribs[i].u.sta_id));
					}
					else
					{
						IPACMDBG_H("The attribute type is not expected!\n");
					}
				}

				pHeaderDescriptor->commit = true;
				pHeaderDescriptor->num_hdrs = 1;

				memset(pHeaderDescriptor->hdr[0].name, 0,
							 sizeof(pHeaderDescriptor->hdr[0].name));

				snprintf(index,sizeof(index), "%d", ipa_if_num);
				strlcpy(pHeaderDescriptor->hdr[0].name, index, sizeof(pHeaderDescriptor->hdr[0].name));
				pHeaderDescriptor->hdr[0].name[IPA_RESOURCE_NAME_MAX-1] = '\0';

				if (strlcat(pHeaderDescriptor->hdr[0].name, IPA_WLAN_PARTIAL_HDR_NAME_v4, sizeof(pHeaderDescriptor->hdr[0].name)) > IPA_RESOURCE_NAME_MAX)
				{
					IPACMERR(" header name construction failed exceed length (%zu)\n", strlen(pHeaderDescriptor->hdr[0].name));
					res = IPACM_FAILURE;
					goto fail;
				}
				snprintf(index,sizeof(index), "%d", header_name_count);
				if (strlcat(pHeaderDescriptor->hdr[0].name, index, sizeof(pHeaderDescriptor->hdr[0].name)) > IPA_RESOURCE_NAME_MAX)
				{
					IPACMERR(" header name construction failed exceed length (%zu)\n", strlen(pHeaderDescriptor->hdr[0].name));
					res = IPACM_FAILURE;
					goto fail;
				}


				pHeaderDescriptor->hdr[0].hdr_len = sCopyHeader.hdr_len;
				pHeaderDescriptor->hdr[0].hdr_hdl = -1;
				pHeaderDescriptor->hdr[0].is_partial = 0;
				pHeaderDescriptor->hdr[0].status = -1;

				if (m_header.AddHeader(pHeaderDescriptor) == false ||
						pHeaderDescriptor->hdr[0].status != 0)
				{
					IPACMERR("ioctl IPA_IOC_ADD_HDR failed: %d\n", pHeaderDescriptor->hdr[0].status);
					res = IPACM_FAILURE;
					goto fail;
				}

				get_client_memptr(wlan_client, num_wifi_client)->hdr_hdl_v4 = pHeaderDescriptor->hdr[0].hdr_hdl;
				IPACMDBG_H("client(%d) v4 full header name:%s header handle:(0x%x)\n",
								 num_wifi_client,
								 pHeaderDescriptor->hdr[0].name,
								 get_client_memptr(wlan_client, num_wifi_client)->hdr_hdl_v4);
				get_client_memptr(wlan_client, num_wifi_client)->ipv4_header_set=true;
				break;
			}
		}

		/* copy partial header for v6*/
		for (cnt=0; cnt<tx_prop->num_tx_props; cnt++)
		{
			if(tx_prop->tx[cnt].ip==IPA_IP_v6)
			{
				IPACMDBG_H("Got partial v6-header name from %d tx props\n", cnt);
				memset(&sCopyHeader, 0, sizeof(sCopyHeader));
				memcpy(sCopyHeader.name,
							 tx_prop->tx[cnt].hdr_name,
							 sizeof(sCopyHeader.name));

				IPACMDBG_H("header name: %s in tx:%d\n", sCopyHeader.name,cnt);
				if (m_header.CopyHeader(&sCopyHeader) == false)
				{
					PERROR("ioctl copy header failed");
					res = IPACM_FAILURE;
					goto fail;
				}

				IPACMDBG_H("header length: %d, paritial: %d\n", sCopyHeader.hdr_len, sCopyHeader.is_partial);
				if (sCopyHeader.hdr_len > IPA_HDR_MAX_SIZE)
				{
					IPACMERR("header oversize\n");
					res = IPACM_FAILURE;
					goto fail;
				}
				else
				{
					memcpy(pHeaderDescriptor->hdr[0].hdr,
								 sCopyHeader.hdr,
								 sCopyHeader.hdr_len);
				}

				for(i = 0; i < data->num_of_attribs; i++)
				{
					if(data->attribs[i].attrib_type == WLAN_HDR_ATTRIB_MAC_ADDR)
					{
						memcpy(get_client_memptr(wlan_client, num_wifi_client)->mac,
								data->attribs[i].u.mac_addr,
								sizeof(get_client_memptr(wlan_client, num_wifi_client)->mac));

						/* copy client mac_addr to partial header */
						memcpy(&pHeaderDescriptor->hdr[0].hdr[data->attribs[i].offset],
								get_client_memptr(wlan_client, num_wifi_client)->mac,
								IPA_MAC_ADDR_SIZE);

						/* replace src mac to bridge mac_addr if any  */
						if (IPACM_Iface::ipacmcfg->ipa_bridge_enable)
						{
							memcpy(&pHeaderDescriptor->hdr[0].hdr[data->attribs[i].offset+IPA_MAC_ADDR_SIZE],
									 IPACM_Iface::ipacmcfg->bridge_mac,
									 IPA_MAC_ADDR_SIZE);
							IPACMDBG_H("device is in bridge mode \n");
						}
					}
					else if (data->attribs[i].attrib_type == WLAN_HDR_ATTRIB_STA_ID)
					{
						/* copy client id to header */
						memcpy(&pHeaderDescriptor->hdr[0].hdr[data->attribs[i].offset],
								&data->attribs[i].u.sta_id, sizeof(data->attribs[i].u.sta_id));
					}
					else
					{
						IPACMDBG_H("The attribute type is not expected!\n");
					}
				}

				pHeaderDescriptor->commit = true;
				pHeaderDescriptor->num_hdrs = 1;

				memset(pHeaderDescriptor->hdr[0].name, 0,
							 sizeof(pHeaderDescriptor->hdr[0].name));

				snprintf(index,sizeof(index), "%d", ipa_if_num);
				strlcpy(pHeaderDescriptor->hdr[0].name, index, sizeof(pHeaderDescriptor->hdr[0].name));
				pHeaderDescriptor->hdr[0].name[IPA_RESOURCE_NAME_MAX-1] = '\0';
				if (strlcat(pHeaderDescriptor->hdr[0].name, IPA_WLAN_PARTIAL_HDR_NAME_v6, sizeof(pHeaderDescriptor->hdr[0].name)) > IPA_RESOURCE_NAME_MAX)
				{
					IPACMERR(" header name construction failed exceed length (%zu)\n", strlen(pHeaderDescriptor->hdr[0].name));
					res = IPACM_FAILURE;
					goto fail;
				}

				snprintf(index,sizeof(index), "%d", header_name_count);
				if (strlcat(pHeaderDescriptor->hdr[0].name, index, sizeof(pHeaderDescriptor->hdr[0].name)) > IPA_RESOURCE_NAME_MAX)
				{
					IPACMERR(" header name construction failed exceed length (%zu)\n", strlen(pHeaderDescriptor->hdr[0].name));
					res = IPACM_FAILURE;
					goto fail;
				}

				pHeaderDescriptor->hdr[0].hdr_len = sCopyHeader.hdr_len;
				pHeaderDescriptor->hdr[0].hdr_hdl = -1;
				pHeaderDescriptor->hdr[0].is_partial = 0;
				pHeaderDescriptor->hdr[0].status = -1;

				if (m_header.AddHeader(pHeaderDescriptor) == false ||
						pHeaderDescriptor->hdr[0].status != 0)
				{
					IPACMERR("ioctl IPA_IOC_ADD_HDR failed: %d\n", pHeaderDescriptor->hdr[0].status);
					res = IPACM_FAILURE;
					goto fail;
				}

				get_client_memptr(wlan_client, num_wifi_client)->hdr_hdl_v6 = pHeaderDescriptor->hdr[0].hdr_hdl;
				IPACMDBG_H("client(%d) v6 full header name:%s header handle:(0x%x)\n",
								 num_wifi_client,
								 pHeaderDescriptor->hdr[0].name,
											 get_client_memptr(wlan_client, num_wifi_client)->hdr_hdl_v6);

				get_client_memptr(wlan_client, num_wifi_client)->ipv6_header_set=true;
				break;
			}
		}

		/* initialize wifi client*/
		get_client_memptr(wlan_client, num_wifi_client)->route_rule_set_v4 = false;
		get_client_memptr(wlan_client, num_wifi_client)->route_rule_set_v6 = 0;
		get_client_memptr(wlan_client, num_wifi_client)->ipv4_set = false;
		get_client_memptr(wlan_client, num_wifi_client)->ipv6_set = 0;
		get_client_memptr(wlan_client, num_wifi_client)->power_save_set=false;
		num_wifi_client++;
		header_name_count++; //keep increasing header_name_count
		IPACM_Wlan::total_num_wifi_clients++;
		res = IPACM_SUCCESS;
		IPACMDBG_H("Wifi client number: %d\n", num_wifi_client);
	}
	else
	{
		return res;
	}

fail:
	free(pHeaderDescriptor);
	return res;
}

/*handle wifi client */
int IPACM_Wlan::handle_wlan_client_ipaddr(ipacm_event_data_all *data)
{
	int clnt_indx;
	int v6_num;
	uint32_t ipv6_link_local_prefix = 0xFE800000;
	uint32_t ipv6_link_local_prefix_mask = 0xFFC00000;

	IPACMDBG_H("number of wifi clients: %d\n", num_wifi_client);
	IPACMDBG_H(" event MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
					 data->mac_addr[0],
					 data->mac_addr[1],
					 data->mac_addr[2],
					 data->mac_addr[3],
					 data->mac_addr[4],
					 data->mac_addr[5]);

	clnt_indx = get_wlan_client_index(data->mac_addr);

		if (clnt_indx == IPACM_INVALID_INDEX)
		{
			IPACMERR("wlan client not found/attached \n");
			return IPACM_FAILURE;
		}

	IPACMDBG_H("Ip-type received %d\n", data->iptype);
	if (data->iptype == IPA_IP_v4)
	{
		IPACMDBG_H("ipv4 address: 0x%x\n", data->ipv4_addr);
		if (data->ipv4_addr != 0) /* not 0.0.0.0 */
		{
			if (get_client_memptr(wlan_client, clnt_indx)->ipv4_set == false)
			{
				get_client_memptr(wlan_client, clnt_indx)->v4_addr = data->ipv4_addr;
				get_client_memptr(wlan_client, clnt_indx)->ipv4_set = true;
			}
			else
			{
				/* check if client got new IPv4 address*/
				if(data->ipv4_addr == get_client_memptr(wlan_client, clnt_indx)->v4_addr)
				{
					IPACMDBG_H("Already setup ipv4 addr for client:%d, ipv4 address didn't change\n", clnt_indx);
					return IPACM_FAILURE;
				}
				else
				{
					IPACMDBG_H("ipv4 addr for client:%d is changed \n", clnt_indx);
					/* delete NAT rules first */
					CtList->HandleNeighIpAddrDelEvt(get_client_memptr(wlan_client, clnt_indx)->v4_addr);
					delete_default_qos_rtrules(clnt_indx, IPA_IP_v4);
					get_client_memptr(wlan_client, clnt_indx)->route_rule_set_v4 = false;
					get_client_memptr(wlan_client, clnt_indx)->v4_addr = data->ipv4_addr;
				}
			}
		}
		else
		{
			IPACMDBG_H("Invalid client IPv4 address \n");
			return IPACM_FAILURE;
		}
	}
	else
	{
		if ((data->ipv6_addr[0] != 0) || (data->ipv6_addr[1] != 0) ||
				(data->ipv6_addr[2] != 0) || (data->ipv6_addr[3] != 0)) /* check if all 0 not valid ipv6 address */
		{
			IPACMDBG_H("ipv6 address: 0x%x:%x:%x:%x\n", data->ipv6_addr[0], data->ipv6_addr[1], data->ipv6_addr[2], data->ipv6_addr[3]);
			if( (data->ipv6_addr[0] & ipv6_link_local_prefix_mask) != (ipv6_link_local_prefix & ipv6_link_local_prefix_mask) &&
				memcmp(ipv6_prefix, data->ipv6_addr, sizeof(ipv6_prefix)) != 0)
			{
				IPACMDBG_H("This IPv6 address is not global IPv6 address with correct prefix, ignore.\n");
				IPACMDBG_H("ipv6 address: 0x%x:%x ipv6_prefix0x%x:%x\n", data->ipv6_addr[0], data->ipv6_addr[1], ipv6_prefix[0], ipv6_prefix[1]);
				return IPACM_FAILURE;
			}

			if(get_client_memptr(wlan_client, clnt_indx)->ipv6_set < IPV6_NUM_ADDR)
			{

		       for(v6_num=0;v6_num < get_client_memptr(wlan_client, clnt_indx)->ipv6_set;v6_num++)
				{
					if( data->ipv6_addr[0] == get_client_memptr(wlan_client, clnt_indx)->v6_addr[v6_num][0] &&
			           data->ipv6_addr[1] == get_client_memptr(wlan_client, clnt_indx)->v6_addr[v6_num][1] &&
			  	        data->ipv6_addr[2]== get_client_memptr(wlan_client, clnt_indx)->v6_addr[v6_num][2] &&
			  	         data->ipv6_addr[3] == get_client_memptr(wlan_client, clnt_indx)->v6_addr[v6_num][3])
					{
			  	    IPACMDBG_H("Already see this ipv6 addr for client:%d\n", clnt_indx);
			  	    return IPACM_FAILURE; /* not setup the RT rules*/
			  		break;
					}
				}

		       /* not see this ipv6 before for wifi client*/
			   get_client_memptr(wlan_client, clnt_indx)->v6_addr[get_client_memptr(wlan_client, clnt_indx)->ipv6_set][0] = data->ipv6_addr[0];
			   get_client_memptr(wlan_client, clnt_indx)->v6_addr[get_client_memptr(wlan_client, clnt_indx)->ipv6_set][1] = data->ipv6_addr[1];
			   get_client_memptr(wlan_client, clnt_indx)->v6_addr[get_client_memptr(wlan_client, clnt_indx)->ipv6_set][2] = data->ipv6_addr[2];
			   get_client_memptr(wlan_client, clnt_indx)->v6_addr[get_client_memptr(wlan_client, clnt_indx)->ipv6_set][3] = data->ipv6_addr[3];
			   get_client_memptr(wlan_client, clnt_indx)->ipv6_set++;
		    }
		    else
		    {
				IPACMDBG_H("Already got %d ipv6 addr for client:%d\n", IPV6_NUM_ADDR, clnt_indx);
				return IPACM_FAILURE; /* not setup the RT rules*/
		    }
		}
		else
		{
			IPACMDBG_H("IPV6 address is invalid\n");
			return IPACM_FAILURE;
		}
	}

	return IPACM_SUCCESS;
}

/*handle wifi client routing rule*/
int IPACM_Wlan::handle_wlan_client_route_rule(uint8_t *mac_addr, ipa_ip_type iptype)
{
	struct ipa_ioc_add_rt_rule *rt_rule;
	struct ipa_rt_rule_add *rt_rule_entry;
	uint32_t tx_index;
	int wlan_index,v6_num;
	const int NUM = 1;
	bool result;

	if(tx_prop == NULL)
	{
		IPACMDBG_H("No rx properties registered for iface %s\n", dev_name);
		return IPACM_SUCCESS;
	}

	IPACMDBG_H("Received mac_addr MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
			mac_addr[0], mac_addr[1], mac_addr[2],
			mac_addr[3], mac_addr[4], mac_addr[5]);

	wlan_index = get_wlan_client_index(mac_addr);
	if (wlan_index == IPACM_INVALID_INDEX)
	{
		IPACMDBG_H("wlan client not found/attached \n");
		return IPACM_SUCCESS;
	}

	/* during power_save mode, even receive IP_ADDR_ADD, not setting RT rules*/
	if (get_client_memptr(wlan_client, wlan_index)->power_save_set == true)
	{
		IPACMDBG_H("wlan client is in power safe mode \n");
		return IPACM_SUCCESS;
	}

	if (iptype==IPA_IP_v4)
	{
		IPACMDBG_H("wlan client index: %d, ip-type: %d, ipv4_set:%d, ipv4_rule_set:%d \n", wlan_index, iptype,
				get_client_memptr(wlan_client, wlan_index)->ipv4_set,
				get_client_memptr(wlan_client, wlan_index)->route_rule_set_v4);
	}
	else
	{
		IPACMDBG_H("wlan client index: %d, ip-type: %d, ipv6_set:%d, ipv6_rule_num:%d \n", wlan_index, iptype,
				get_client_memptr(wlan_client, wlan_index)->ipv6_set,
				get_client_memptr(wlan_client, wlan_index)->route_rule_set_v6);
	}


	/* Add default  Qos routing rules if not set yet */
	if ((iptype == IPA_IP_v4
				&& get_client_memptr(wlan_client, wlan_index)->route_rule_set_v4 == false
				&& get_client_memptr(wlan_client, wlan_index)->ipv4_set == true)
			|| (iptype == IPA_IP_v6
				&& get_client_memptr(wlan_client, wlan_index)->route_rule_set_v6 < get_client_memptr(wlan_client, wlan_index)->ipv6_set
			   ))
	{
		rt_rule = (struct ipa_ioc_add_rt_rule *)
			calloc(1, sizeof(struct ipa_ioc_add_rt_rule) +
					NUM * sizeof(struct ipa_rt_rule_add));

		if (rt_rule == NULL)
		{
			PERROR("Error Locate ipa_ioc_add_rt_rule memory...\n");
			return IPACM_FAILURE;
		}

		rt_rule->commit = 1;
		rt_rule->num_rules = (uint8_t)NUM;
		rt_rule->ip = iptype;


		for (tx_index = 0; tx_index < iface_query->num_tx_props; tx_index++)
		{

			if(iptype != tx_prop->tx[tx_index].ip)
			{
				IPACMDBG_H("Tx:%d, ip-type: %d conflict ip-type: %d no RT-rule added\n",
						tx_index, tx_prop->tx[tx_index].ip,iptype);
				continue;
			}

			rt_rule_entry = &rt_rule->rules[0];
			rt_rule_entry->at_rear = 0;

			if (iptype == IPA_IP_v4)
			{
				IPACMDBG_H("client index(%d):ipv4 address: 0x%x\n", wlan_index,
						get_client_memptr(wlan_client, wlan_index)->v4_addr);

				IPACMDBG_H("client(%d): v4 header handle:(0x%x)\n",
						wlan_index,
						get_client_memptr(wlan_client, wlan_index)->hdr_hdl_v4);
				strlcpy(rt_rule->rt_tbl_name,
						IPACM_Iface::ipacmcfg->rt_tbl_lan_v4.name,
						sizeof(rt_rule->rt_tbl_name));
				rt_rule->rt_tbl_name[IPA_RESOURCE_NAME_MAX-1] = '\0';

				if(!strcmp("wigig0", dev_name))
				{
					IPACMDBG_H("for WIGIG client use relevant pipe %d\n",
						get_client_memptr(wlan_client, wlan_index)->wigig_ipa_client);
					rt_rule_entry->rule.dst = get_client_memptr(wlan_client, wlan_index)->wigig_ipa_client;
				}
				else
				{
					if(IPACM_Iface::ipacmcfg->isMCC_Mode)
					{
						IPACMDBG_H("In MCC mode, use alt dst pipe: %d\n",
							tx_prop->tx[tx_index].alt_dst_pipe);
						rt_rule_entry->rule.dst = tx_prop->tx[tx_index].alt_dst_pipe;
					}
					else
					{
						rt_rule_entry->rule.dst = tx_prop->tx[tx_index].dst_pipe;
					}
				}

				memcpy(&rt_rule_entry->rule.attrib,
						&tx_prop->tx[tx_index].attrib,
						sizeof(rt_rule_entry->rule.attrib));
				rt_rule_entry->rule.attrib.attrib_mask |= IPA_FLT_DST_ADDR;
				rt_rule_entry->rule.hdr_hdl = get_client_memptr(wlan_client, wlan_index)->hdr_hdl_v4;
				rt_rule_entry->rule.attrib.u.v4.dst_addr = get_client_memptr(wlan_client, wlan_index)->v4_addr;
				rt_rule_entry->rule.attrib.u.v4.dst_addr_mask = 0xFFFFFFFF;

				if(IPACM_Iface::ipacmcfg->GetIPAVer() >= IPA_HW_v4_0)
				{
					rt_rule_entry->rule.hashable = true;
				}
#ifdef IPA_IOCTL_SET_FNR_COUNTER_INFO
				/* use index hw-counter */
				if(IPACM_Iface::ipacmcfg->hw_fnr_stats_support)
				{
					IPACMDBG_H("hw-index-enable %d, counter %d\n", IPACM_Iface::ipacmcfg->hw_fnr_stats_support, IPACM_Iface::ipacmcfg->hw_counter_offset + DL_HW);
					result = m_routing.AddRoutingRule_hw_index(rt_rule, IPACM_Iface::ipacmcfg->hw_counter_offset + DL_HW);
				} else {
					result = m_routing.AddRoutingRule(rt_rule);
				}
#else
				result = m_routing.AddRoutingRule(rt_rule);
#endif
				if (result == false)
				{
					IPACMERR("Routing rule addition failed!\n");
					free(rt_rule);
					return IPACM_FAILURE;
				}

				/* copy ipv4 RT hdl */
				get_client_memptr(wlan_client, wlan_index)->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v4 =
					rt_rule->rules[0].rt_rule_hdl;
				IPACMDBG_H("tx:%d, rt rule hdl=%x ip-type: %d\n", tx_index,
						get_client_memptr(wlan_client, wlan_index)->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v4, iptype);
			}
			else
			{
				for(v6_num = get_client_memptr(wlan_client, wlan_index)->route_rule_set_v6;v6_num < get_client_memptr(wlan_client, wlan_index)->ipv6_set;v6_num++)
				{
					IPACMDBG_H("client(%d): v6 header handle:(0x%x)\n",
							wlan_index,
							get_client_memptr(wlan_client, wlan_index)->hdr_hdl_v6);

					/* v6 LAN_RT_TBL */
					strlcpy(rt_rule->rt_tbl_name,
							IPACM_Iface::ipacmcfg->rt_tbl_v6.name,
							sizeof(rt_rule->rt_tbl_name));
					rt_rule->rt_tbl_name[IPA_RESOURCE_NAME_MAX-1] = '\0';
					/* Support QCMAP LAN traffic feature, send to A5 */
					rt_rule_entry->rule.dst = iface_query->excp_pipe;
					memset(&rt_rule_entry->rule.attrib, 0, sizeof(rt_rule_entry->rule.attrib));
					rt_rule_entry->rule.hdr_hdl = 0;
					rt_rule_entry->rule.attrib.attrib_mask |= IPA_FLT_DST_ADDR;
					rt_rule_entry->rule.attrib.u.v6.dst_addr[0] = get_client_memptr(wlan_client, wlan_index)->v6_addr[v6_num][0];
					rt_rule_entry->rule.attrib.u.v6.dst_addr[1] = get_client_memptr(wlan_client, wlan_index)->v6_addr[v6_num][1];
					rt_rule_entry->rule.attrib.u.v6.dst_addr[2] = get_client_memptr(wlan_client, wlan_index)->v6_addr[v6_num][2];
					rt_rule_entry->rule.attrib.u.v6.dst_addr[3] = get_client_memptr(wlan_client, wlan_index)->v6_addr[v6_num][3];
					rt_rule_entry->rule.attrib.u.v6.dst_addr_mask[0] = 0xFFFFFFFF;
					rt_rule_entry->rule.attrib.u.v6.dst_addr_mask[1] = 0xFFFFFFFF;
					rt_rule_entry->rule.attrib.u.v6.dst_addr_mask[2] = 0xFFFFFFFF;
					rt_rule_entry->rule.attrib.u.v6.dst_addr_mask[3] = 0xFFFFFFFF;
#ifdef FEATURE_IPA_V3
					rt_rule_entry->rule.hashable = true;
#endif
					if (false == m_routing.AddRoutingRule(rt_rule))
					{
						IPACMERR("Routing rule addition failed!\n");
						free(rt_rule);
						return IPACM_FAILURE;
					}

					get_client_memptr(wlan_client, wlan_index)->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v6[v6_num] = rt_rule->rules[0].rt_rule_hdl;
					IPACMDBG_H("tx:%d, rt rule hdl=%x ip-type: %d\n", tx_index,
							get_client_memptr(wlan_client, wlan_index)->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v6[v6_num], iptype);

					/*Copy same rule to v6 WAN RT TBL*/
					strlcpy(rt_rule->rt_tbl_name,
							IPACM_Iface::ipacmcfg->rt_tbl_wan_v6.name,
							sizeof(rt_rule->rt_tbl_name));
					rt_rule->rt_tbl_name[IPA_RESOURCE_NAME_MAX-1] = '\0';
					/* Downlink traffic from Wan iface, directly through IPA */
					if(!strcmp("wigig0", dev_name))
					{
						IPACMDBG_H("for WIGIG client use relevant pipe %d\n",
							get_client_memptr(wlan_client, wlan_index)->wigig_ipa_client);
						rt_rule_entry->rule.dst = get_client_memptr(wlan_client, wlan_index)->wigig_ipa_client;
					}
					else
					{
						if(IPACM_Iface::ipacmcfg->isMCC_Mode)
						{
							IPACMDBG_H("In MCC mode, use alt dst pipe: %d\n",
								tx_prop->tx[tx_index].alt_dst_pipe);
							rt_rule_entry->rule.dst = tx_prop->tx[tx_index].alt_dst_pipe;
						}
						else
						{
							rt_rule_entry->rule.dst = tx_prop->tx[tx_index].dst_pipe;
						}
					}
					memcpy(&rt_rule_entry->rule.attrib,
							&tx_prop->tx[tx_index].attrib,
							sizeof(rt_rule_entry->rule.attrib));
					rt_rule_entry->rule.hdr_hdl = get_client_memptr(wlan_client, wlan_index)->hdr_hdl_v6;
					rt_rule_entry->rule.attrib.attrib_mask |= IPA_FLT_DST_ADDR;
					rt_rule_entry->rule.attrib.u.v6.dst_addr[0] = get_client_memptr(wlan_client, wlan_index)->v6_addr[v6_num][0];
					rt_rule_entry->rule.attrib.u.v6.dst_addr[1] = get_client_memptr(wlan_client, wlan_index)->v6_addr[v6_num][1];
					rt_rule_entry->rule.attrib.u.v6.dst_addr[2] = get_client_memptr(wlan_client, wlan_index)->v6_addr[v6_num][2];
					rt_rule_entry->rule.attrib.u.v6.dst_addr[3] = get_client_memptr(wlan_client, wlan_index)->v6_addr[v6_num][3];
					rt_rule_entry->rule.attrib.u.v6.dst_addr_mask[0] = 0xFFFFFFFF;
					rt_rule_entry->rule.attrib.u.v6.dst_addr_mask[1] = 0xFFFFFFFF;
					rt_rule_entry->rule.attrib.u.v6.dst_addr_mask[2] = 0xFFFFFFFF;
					rt_rule_entry->rule.attrib.u.v6.dst_addr_mask[3] = 0xFFFFFFFF;
#ifdef FEATURE_IPA_V3
					rt_rule_entry->rule.hashable = true;
#endif
#ifdef IPA_IOCTL_SET_FNR_COUNTER_INFO
					/* use index hw-counter */
					if(IPACM_Iface::ipacmcfg->hw_fnr_stats_support)
					{
						IPACMDBG_H("hw-index-enable %d, counter %d\n", IPACM_Iface::ipacmcfg->hw_fnr_stats_support, IPACM_Iface::ipacmcfg->hw_counter_offset + DL_HW);
						result = m_routing.AddRoutingRule_hw_index(rt_rule, IPACM_Iface::ipacmcfg->hw_counter_offset + DL_HW);
					} else {
						result = m_routing.AddRoutingRule(rt_rule);
					}
#else
					result = m_routing.AddRoutingRule(rt_rule);
#endif

					if (result == false)
					{
						IPACMERR("Routing rule addition failed!\n");
						free(rt_rule);
						return IPACM_FAILURE;
					}

					get_client_memptr(wlan_client, wlan_index)->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v6_wan[v6_num] = rt_rule->rules[0].rt_rule_hdl;

					IPACMDBG_H("tx:%d, rt rule hdl=%x ip-type: %d\n", tx_index,
							get_client_memptr(wlan_client, wlan_index)->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v6_wan[v6_num], iptype);

					/* send client-v6 info to pcie modem only with global ipv6 with tx_index = 0 one time*/
					if(is_global_ipv6_addr(get_client_memptr(wlan_client, wlan_index)->v6_addr[v6_num]) && (IPACM_Wan::backhaul_mode == Q6_MHI_WAN))
					{
						if (add_connection(wlan_index, v6_num))
						{
							IPACMERR("PCIE filter rule addition failed! (%d-client) %d v6-entry\n",wlan_index, v6_num);
						}
					}
				}
			}

		} /* end of for loop */

		free(rt_rule);

		if (iptype == IPA_IP_v4)
		{
			get_client_memptr(wlan_client, wlan_index)->route_rule_set_v4 = true;
		}
		else
		{
			get_client_memptr(wlan_client, wlan_index)->route_rule_set_v6 = get_client_memptr(wlan_client, wlan_index)->ipv6_set;
		}
	}

	return IPACM_SUCCESS;
}

/*handle wifi client power-save mode*/
int IPACM_Wlan::handle_wlan_client_pwrsave(uint8_t *mac_addr)
{
	int clt_indx;
	IPACMDBG_H("wlan->handle_wlan_client_pwrsave();\n");

	clt_indx = get_wlan_client_index(mac_addr);
	if (clt_indx == IPACM_INVALID_INDEX)
	{
		IPACMDBG_H("wlan client not attached\n");
		return IPACM_SUCCESS;
	}

        if (get_client_memptr(wlan_client, clt_indx)->power_save_set == false)
	{
		/* First reset nat rules and then route rules */
	    if(get_client_memptr(wlan_client, clt_indx)->ipv4_set == true)
	    {
			IPACMDBG_H("Deleting Nat Rules\n");
			Nat_App->UpdatePwrSaveIf(get_client_memptr(wlan_client, clt_indx)->v4_addr);
 	     }

		IPACMDBG_H("Deleting default qos Route Rules\n");
		delete_default_qos_rtrules(clt_indx, IPA_IP_v4);
		delete_default_qos_rtrules(clt_indx, IPA_IP_v6);
                get_client_memptr(wlan_client, clt_indx)->power_save_set = true;
	}
	else
	{
		IPACMDBG_H("wlan client already in power-save mode\n");
	}
    return IPACM_SUCCESS;
}

/*handle wifi client del mode*/
int IPACM_Wlan::handle_wlan_client_down_evt(uint8_t *mac_addr)
{
	int clt_indx;
	uint32_t tx_index;
	int num_wifi_client_tmp = num_wifi_client;
	int num_v6;

	IPACMDBG_H("total client: %d\n", num_wifi_client_tmp);

	clt_indx = get_wlan_client_index(mac_addr);
	if (clt_indx == IPACM_INVALID_INDEX)
	{
		IPACMDBG_H("wlan client not attached\n");
		return IPACM_SUCCESS;
	}

	/* First reset nat rules and then route rules */
	if(get_client_memptr(wlan_client, clt_indx)->ipv4_set == true)
	{
		IPACMDBG_H("Clean Nat Rules for ipv4:0x%x\n", get_client_memptr(wlan_client, clt_indx)->v4_addr);
		CtList->HandleNeighIpAddrDelEvt(get_client_memptr(wlan_client, clt_indx)->v4_addr);
	}

	if(delete_default_qos_rtrules(clt_indx, IPA_IP_v4))
	{
		IPACMERR("unbale to delete v4 default qos route rules for index: %d\n", clt_indx);
		return IPACM_FAILURE;
	}

	if(delete_default_qos_rtrules(clt_indx, IPA_IP_v6))
	{
		IPACMERR("unbale to delete v6 default qos route rules for indexn: %d\n", clt_indx);
		return IPACM_FAILURE;
	}

	/* Delete wlan client header */
	if(get_client_memptr(wlan_client, clt_indx)->ipv4_header_set == true)
	{
	if (m_header.DeleteHeaderHdl(get_client_memptr(wlan_client, clt_indx)->hdr_hdl_v4)
			== false)
	{
		return IPACM_FAILURE;
	}
		get_client_memptr(wlan_client, clt_indx)->ipv4_header_set = false;
	}

	if(get_client_memptr(wlan_client, clt_indx)->ipv6_header_set == true)
	{
	if (m_header.DeleteHeaderHdl(get_client_memptr(wlan_client, clt_indx)->hdr_hdl_v6)
			== false)
	{
		return IPACM_FAILURE;
	}
		get_client_memptr(wlan_client, clt_indx)->ipv6_header_set = false;
	}

	/* Reset ip_set to 0*/
	get_client_memptr(wlan_client, clt_indx)->ipv4_set = false;
	get_client_memptr(wlan_client, clt_indx)->ipv6_set = 0;
	get_client_memptr(wlan_client, clt_indx)->ipv4_header_set = false;
	get_client_memptr(wlan_client, clt_indx)->ipv6_header_set = false;
	get_client_memptr(wlan_client, clt_indx)->route_rule_set_v4 = false;
	get_client_memptr(wlan_client, clt_indx)->route_rule_set_v6 = 0;
	free(get_client_memptr(wlan_client, clt_indx)->p_hdr_info);

	for (; clt_indx < num_wifi_client_tmp - 1; clt_indx++)
	{
		get_client_memptr(wlan_client, clt_indx)->p_hdr_info = get_client_memptr(wlan_client, (clt_indx + 1))->p_hdr_info;

		memcpy(get_client_memptr(wlan_client, clt_indx)->mac,
					 get_client_memptr(wlan_client, (clt_indx + 1))->mac,
					 sizeof(get_client_memptr(wlan_client, clt_indx)->mac));

		get_client_memptr(wlan_client, clt_indx)->hdr_hdl_v4 = get_client_memptr(wlan_client, (clt_indx + 1))->hdr_hdl_v4;
		get_client_memptr(wlan_client, clt_indx)->hdr_hdl_v6 = get_client_memptr(wlan_client, (clt_indx + 1))->hdr_hdl_v6;
		get_client_memptr(wlan_client, clt_indx)->v4_addr = get_client_memptr(wlan_client, (clt_indx + 1))->v4_addr;

		get_client_memptr(wlan_client, clt_indx)->ipv4_set = get_client_memptr(wlan_client, (clt_indx + 1))->ipv4_set;
		get_client_memptr(wlan_client, clt_indx)->ipv6_set = get_client_memptr(wlan_client, (clt_indx + 1))->ipv6_set;
		get_client_memptr(wlan_client, clt_indx)->ipv4_header_set = get_client_memptr(wlan_client, (clt_indx + 1))->ipv4_header_set;
		get_client_memptr(wlan_client, clt_indx)->ipv6_header_set = get_client_memptr(wlan_client, (clt_indx + 1))->ipv6_header_set;

		get_client_memptr(wlan_client, clt_indx)->route_rule_set_v4 = get_client_memptr(wlan_client, (clt_indx + 1))->route_rule_set_v4;
		get_client_memptr(wlan_client, clt_indx)->route_rule_set_v6 = get_client_memptr(wlan_client, (clt_indx + 1))->route_rule_set_v6;

                for(num_v6=0;num_v6< get_client_memptr(wlan_client, clt_indx)->ipv6_set;num_v6++)
	        {
		    get_client_memptr(wlan_client, clt_indx)->v6_addr[num_v6][0] = get_client_memptr(wlan_client, (clt_indx + 1))->v6_addr[num_v6][0];
		    get_client_memptr(wlan_client, clt_indx)->v6_addr[num_v6][1] = get_client_memptr(wlan_client, (clt_indx + 1))->v6_addr[num_v6][1];
		    get_client_memptr(wlan_client, clt_indx)->v6_addr[num_v6][2] = get_client_memptr(wlan_client, (clt_indx + 1))->v6_addr[num_v6][2];
		    get_client_memptr(wlan_client, clt_indx)->v6_addr[num_v6][3] = get_client_memptr(wlan_client, (clt_indx + 1))->v6_addr[num_v6][3];
                }

		for (tx_index = 0; tx_index < iface_query->num_tx_props; tx_index++)
		{
			get_client_memptr(wlan_client, clt_indx)->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v4 =
				 get_client_memptr(wlan_client, (clt_indx + 1))->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v4;

			for(num_v6=0;num_v6< get_client_memptr(wlan_client, clt_indx)->route_rule_set_v6;num_v6++)
			{
			  get_client_memptr(wlan_client, clt_indx)->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v6[num_v6] =
			   	 get_client_memptr(wlan_client, (clt_indx + 1))->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v6[num_v6];
			  get_client_memptr(wlan_client, clt_indx)->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v6_wan[num_v6] =
			   	 get_client_memptr(wlan_client, (clt_indx + 1))->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v6_wan[num_v6];
		    }
		}
	}

	IPACMDBG_H(" %d wifi client deleted successfully \n", num_wifi_client);
	num_wifi_client = num_wifi_client - 1;
	IPACM_Wlan::total_num_wifi_clients = IPACM_Wlan::total_num_wifi_clients - 1;
	IPACMDBG_H(" Number of wifi client: %d\n", num_wifi_client);

	return IPACM_SUCCESS;
}

/*handle wlan iface down event*/
int IPACM_Wlan::handle_down_evt()
{
	int res = IPACM_SUCCESS, num_private_subnet_fl_rule;
	uint32_t i;
	num_private_subnet_fl_rule = 0;

	IPACMDBG_H("WLAN ip-type: %d \n", ip_type);
	/* no iface address up, directly close iface*/
	if (ip_type == IPACM_IP_NULL)
	{
		IPACMERR("Invalid iptype: 0x%x\n", ip_type);
		goto fail;
	}

	/* delete wan filter rule */
	if (IPACM_Wan::isWanUP(ipa_if_num) && rx_prop != NULL)
	{
		IPACMDBG_H("LAN IF goes down, backhaul type %d\n", IPACM_Wan::backhaul_mode);
		IPACM_Lan::handle_wan_down(IPACM_Wan::backhaul_mode);
#ifdef FEATURE_IPA_ANDROID
#ifndef FEATURE_IPACM_HAL
		/* Clean-up tethered-iface list */
		IPACM_Wan::delete_tether_iface(IPA_IP_v4, ipa_if_num);
#endif
#endif
	}

	if (IPACM_Wan::isWanUP_V6(ipa_if_num) && rx_prop != NULL)
	{
		IPACMDBG_H("LAN IF goes down, backhaul type %d\n", IPACM_Wan::backhaul_mode);
		handle_wan_down_v6(IPACM_Wan::backhaul_mode);
#ifdef FEATURE_IPA_ANDROID
		/* Clean-up tethered-iface list */
		IPACM_Wan::delete_tether_iface(IPA_IP_v6, ipa_if_num);
#endif
	}
	IPACMDBG_H("finished deleting wan filtering rules\n ");

	/* Delete v4 filtering rules */
	if (ip_type != IPA_IP_v6 && rx_prop != NULL)
	{
		/* delete IPv4 icmp filter rules */
		if(m_filtering.DeleteFilteringHdls(ipv4_icmp_flt_rule_hdl, IPA_IP_v4, NUM_IPV4_ICMP_FLT_RULE) == false)
		{
			IPACMERR("Error Deleting ICMPv4 Filtering Rule, aborting...\n");
			res = IPACM_FAILURE;
			goto fail;
		}
		IPACM_Iface::ipacmcfg->decreaseFltRuleCount(rx_prop->rx[0].src_pipe, IPA_IP_v4, NUM_IPV4_ICMP_FLT_RULE);
		if (dft_v4fl_rule_hdl[0] != 0)
		{
			if (m_filtering.DeleteFilteringHdls(dft_v4fl_rule_hdl, IPA_IP_v4, IPV4_DEFAULT_FILTERTING_RULES) == false)
			{
				IPACMERR("Error Deleting Filtering Rule, aborting...\n");
				res = IPACM_FAILURE;
				goto fail;
			}
			IPACM_Iface::ipacmcfg->decreaseFltRuleCount(rx_prop->rx[0].src_pipe, IPA_IP_v4, IPV4_DEFAULT_FILTERTING_RULES);
			IPACMDBG_H("Deleted default v4 filter rules successfully.\n");
		}
		/* delete private-ipv4 filter rules */
#ifdef FEATURE_IPA_ANDROID
		if(m_filtering.DeleteFilteringHdls(private_fl_rule_hdl, IPA_IP_v4, IPA_MAX_PRIVATE_SUBNET_ENTRIES) == false)
		{
			IPACMERR("Error deleting private subnet IPv4 flt rules.\n");
			res = IPACM_FAILURE;
			goto fail;
		}
		IPACM_Iface::ipacmcfg->decreaseFltRuleCount(rx_prop->rx[0].src_pipe, IPA_IP_v4, IPA_MAX_PRIVATE_SUBNET_ENTRIES);
#else
		num_private_subnet_fl_rule = IPACM_Iface::ipacmcfg->ipa_num_private_subnet > IPA_MAX_PRIVATE_SUBNET_ENTRIES?
			IPA_MAX_PRIVATE_SUBNET_ENTRIES : IPACM_Iface::ipacmcfg->ipa_num_private_subnet;
		if(m_filtering.DeleteFilteringHdls(private_fl_rule_hdl, IPA_IP_v4, num_private_subnet_fl_rule) == false)
		{
			IPACMERR("Error deleting private subnet flt rules, aborting...\n");
			res = IPACM_FAILURE;
			goto fail;
		}
		IPACM_Iface::ipacmcfg->decreaseFltRuleCount(rx_prop->rx[0].src_pipe, IPA_IP_v4, num_private_subnet_fl_rule);
#endif
		IPACMDBG_H("Deleted private subnet v4 filter rules successfully.\n");

#ifdef FEATURE_L2TP
		if(m_filtering.DeleteFilteringHdls(&tcp_syn_flt_rule_hdl[IPA_IP_v4], IPA_IP_v4, 1) == false)
		{
			IPACMERR("Error deleting tcp syn flt rule, aborting...\n");
			res = IPACM_FAILURE;
			goto fail;
		}
#endif
	}

	/* Delete v6 filtering rules */
	if (ip_type != IPA_IP_v4 && rx_prop != NULL)
	{
		/* delete icmp filter rules */
		if(m_filtering.DeleteFilteringHdls(ipv6_icmp_flt_rule_hdl, IPA_IP_v6, NUM_IPV6_ICMP_FLT_RULE) == false)
		{
			IPACMERR("Error Deleting ICMPv6 Filtering Rule, aborting...\n");
			res = IPACM_FAILURE;
			goto fail;
		}
		IPACM_Iface::ipacmcfg->decreaseFltRuleCount(rx_prop->rx[0].src_pipe, IPA_IP_v6, NUM_IPV6_ICMP_FLT_RULE);

		if (dft_v6fl_rule_hdl[0] != 0)
		{
			if (m_filtering.DeleteFilteringHdls(dft_v6fl_rule_hdl, IPA_IP_v6, IPV6_DEFAULT_FILTERTING_RULES) == false)
			{
				IPACMERR("Error Adding RuleTable(1) to Filtering, aborting...\n");
				res = IPACM_FAILURE;
				goto fail;
			}
			IPACM_Iface::ipacmcfg->decreaseFltRuleCount(rx_prop->rx[0].src_pipe, IPA_IP_v6, IPV6_DEFAULT_FILTERTING_RULES);
			IPACMDBG_H("Deleted default v6 filter rules successfully.\n");
		}
#ifdef FEATURE_L2TP
		if(m_filtering.DeleteFilteringHdls(&tcp_syn_flt_rule_hdl[IPA_IP_v6], IPA_IP_v6, 1) == false)
		{
			IPACMERR("Error deleting tcp syn flt rule, aborting...\n");
			res = IPACM_FAILURE;
			goto fail;
		}
#endif
	}
	IPACMDBG_H("finished delete filtering rules\n ");

	/* Delete default v4 RT rule */
	if (ip_type != IPA_IP_v6)
	{
		IPACMDBG_H("Delete default v4 routing rules\n");
		if (m_routing.DeleteRoutingHdl(dft_rt_rule_hdl[0], IPA_IP_v4)
				== false)
		{
			IPACMERR("Routing rule deletion failed!\n");
			res = IPACM_FAILURE;
			goto fail;
		}
	}

	/* Delete default v6 RT rule */
	if (ip_type != IPA_IP_v4)
	{
		IPACMDBG_H("Delete default v6 routing rules\n");
		/* May have multiple ipv6 iface-RT rules */
		for (i = 0; i < 2*num_dft_rt_v6; i++)
		{
			if (m_routing.DeleteRoutingHdl(dft_rt_rule_hdl[MAX_DEFAULT_v4_ROUTE_RULES+i], IPA_IP_v6)
					== false)
			{
				IPACMERR("Routing rule deletion failed!\n");
				res = IPACM_FAILURE;
				goto fail;
			}
		}
	}
	IPACMDBG_H("finished deleting default RT rules\n ");

	eth_bridge_post_event(IPA_ETH_BRIDGE_IFACE_DOWN, IPA_IP_MAX, NULL, NULL, NULL, IPA_CLIENT_MAX);

	/* free the wlan clients cache */
	IPACMDBG_H("Free wlan clients cache\n");

	/* Delete private subnet*/
#ifdef FEATURE_IPA_ANDROID
	if (ip_type != IPA_IP_v6)
	{
		IPACMDBG_H("current IPACM private subnet_addr number(%d)\n", IPACM_Iface::ipacmcfg->ipa_num_private_subnet);
		IPACMDBG_H(" Delete IPACM private subnet_addr as: 0x%x \n", if_ipv4_subnet);
		if(IPACM_Iface::ipacmcfg->DelPrivateSubnet(if_ipv4_subnet, ipa_if_num) == false)
		{
			IPACMERR(" can't Delete IPACM private subnet_addr as: 0x%x \n", if_ipv4_subnet);
		}
	}
	/* reset the IPA-client pipe enum */
#ifdef FEATURE_IPACM_HAL
	handle_tethering_client(true, IPACM_CLIENT_MAX);
#else
	handle_tethering_client(true, IPACM_CLIENT_WLAN);
#endif
#endif /* defined(FEATURE_IPA_ANDROID)*/

fail:
	/* clean wifi-client header, routing rules */
	/* clean wifi client rule*/
	IPACMDBG_H("left %d wifi clients need to be deleted \n ", num_wifi_client);
	for (i = 0; i < num_wifi_client; i++)
	{
		/* First reset nat rules and then route rules */
		if(get_client_memptr(wlan_client, i)->ipv4_set == true)
		{
	        IPACMDBG_H("Clean Nat Rules for ipv4:0x%x\n", get_client_memptr(wlan_client, i)->v4_addr);
			CtList->HandleNeighIpAddrDelEvt(get_client_memptr(wlan_client, i)->v4_addr);
		}

		if (delete_default_qos_rtrules(i, IPA_IP_v4))
		{
			IPACMERR("unbale to delete v4 default qos route rules for index: %d\n", i);
			res = IPACM_FAILURE;
		}

		if (delete_default_qos_rtrules(i, IPA_IP_v6))
		{
			IPACMERR("unbale to delete v6 default qos route rules for index: %d\n", i);
			res = IPACM_FAILURE;
		}

		IPACMDBG_H("Delete %d client header\n", num_wifi_client);

		if(get_client_memptr(wlan_client, i)->ipv4_header_set == true)
		{
			if (m_header.DeleteHeaderHdl(get_client_memptr(wlan_client, i)->hdr_hdl_v4)
				== false)
			{
				res = IPACM_FAILURE;
			}
		}

		if(get_client_memptr(wlan_client, i)->ipv6_header_set == true)
		{
			if (m_header.DeleteHeaderHdl(get_client_memptr(wlan_client, i)->hdr_hdl_v6)
					== false)
			{
				res = IPACM_FAILURE;
			}
		}
	} /* end of for loop */

	/* check software routing fl rule hdl */
	if (softwarerouting_act == true && rx_prop != NULL )
	{
		IPACMDBG_H("Delete sw routing filtering rules\n");
		IPACM_Iface::handle_software_routing_disable(false);
	}
	IPACMDBG_H("finished delete software-routing filtering rules\n ");

	if (rx_prop != NULL)
	{
		if(IPACM_Iface::ipacmcfg->GetIPAVer() >= IPA_HW_None && IPACM_Iface::ipacmcfg->GetIPAVer() < IPA_HW_v4_0)
		{
			/* Delete corresponding ipa_rm_resource_name of RX-endpoint after delete all IPV4V6 FT-rule */
			IPACMDBG_H("dev %s add producer dependency\n", dev_name);
			IPACMDBG_H("depend Got pipe %d rm index : %d \n", rx_prop->rx[0].src_pipe, IPACM_Iface::ipacmcfg->ipa_client_rm_map_tbl[rx_prop->rx[0].src_pipe]);
			IPACM_Iface::ipacmcfg->DelRmDepend(IPACM_Iface::ipacmcfg->ipa_client_rm_map_tbl[rx_prop->rx[0].src_pipe]);
		}
#ifndef FEATURE_ETH_BRIDGE_LE
		free(rx_prop);
#endif
	}

	for (i = 0; i < num_wifi_client; i++)
	{
		if(get_client_memptr(wlan_client, i)->p_hdr_info != NULL)
		{
			free(get_client_memptr(wlan_client, i)->p_hdr_info);
		}
	}
	if(wlan_client != NULL)
	{
		free(wlan_client);
	}
#ifndef FEATURE_ETH_BRIDGE_LE
	if (tx_prop != NULL)
	{
		free(tx_prop);
	}

	if (iface_query != NULL)
	{
		free(iface_query);
	}
#endif

	is_active = false;
	post_del_self_evt();

	return res;
}

/*handle reset wifi-client rt-rules */
int IPACM_Wlan::handle_wlan_client_reset_rt(ipa_ip_type iptype)
{
	uint32_t i;
	int res = IPACM_SUCCESS;

	/* clean wifi-client routing rules */
	IPACMDBG_H("left %d wifi clients to reset ip-type(%d) rules \n ", num_wifi_client, iptype);

	for (i = 0; i < num_wifi_client; i++)
	{
		/* Reset RT rules */
		res = delete_default_qos_rtrules(i, iptype);
		if (res != IPACM_SUCCESS)
		{
			IPACMERR("Failed to delete old iptype(%d) rules.\n", iptype);
			return res;
		}

		/* Reset ip-address */
		if(iptype == IPA_IP_v4)
		{
			get_client_memptr(wlan_client, i)->ipv4_set = false;
		}
		else
		{
			get_client_memptr(wlan_client, i)->ipv6_set = 0;
		}
	} /* end of for loop */
	return res;
}

void IPACM_Wlan::handle_SCC_MCC_switch(ipa_ip_type iptype)
{
	struct ipa_ioc_mdfy_rt_rule *rt_rule = NULL;
	struct ipa_rt_rule_mdfy *rt_rule_entry;
	uint32_t tx_index;
	int wlan_index, v6_num;
	const int NUM = 1;
	int num_wifi_client_tmp = IPACM_Wlan::num_wifi_client;
	bool isAdded = false;

	if (tx_prop == NULL)
	{
		IPACMDBG_H("No rx properties registered for iface %s\n", dev_name);
		return;
	}

	if (rt_rule == NULL)
	{
		rt_rule = (struct ipa_ioc_mdfy_rt_rule *)
			calloc(1, sizeof(struct ipa_ioc_mdfy_rt_rule) +
					NUM * sizeof(struct ipa_rt_rule_mdfy));

		if (rt_rule == NULL)
		{
			PERROR("Error Locate ipa_ioc_mdfy_rt_rule memory...\n");
			return;
		}

		rt_rule->commit = 0;
		rt_rule->num_rules = NUM;
		rt_rule->ip = iptype;
	}
	rt_rule_entry = &rt_rule->rules[0];

	/* modify ipv4 routing rule */
	if (iptype == IPA_IP_v4)
	{
		for (wlan_index = 0; wlan_index < num_wifi_client_tmp; wlan_index++)
		{
			IPACMDBG_H("wlan client index: %d, ip-type: %d, ipv4_set:%d, ipv4_rule_set:%d \n",
					wlan_index, iptype,
					get_client_memptr(wlan_client, wlan_index)->ipv4_set,
					get_client_memptr(wlan_client, wlan_index)->route_rule_set_v4);

			if (get_client_memptr(wlan_client, wlan_index)->power_save_set == true ||
					get_client_memptr(wlan_client, wlan_index)->route_rule_set_v4 == false)
			{
				IPACMDBG_H("client %d route rules not set\n", wlan_index);
				continue;
			}

			IPACMDBG_H("Modify client %d route rule\n", wlan_index);
			for (tx_index = 0; tx_index < iface_query->num_tx_props; tx_index++)
			{
				if (iptype != tx_prop->tx[tx_index].ip)
				{
					IPACMDBG_H("Tx:%d, ip-type: %d ip-type not matching: %d ignore\n",
							tx_index, tx_prop->tx[tx_index].ip, iptype);
					continue;
				}

				IPACMDBG_H("client index(%d):ipv4 address: 0x%x\n", wlan_index,
						get_client_memptr(wlan_client, wlan_index)->v4_addr);

				IPACMDBG_H("client(%d): v4 header handle:(0x%x)\n",
						wlan_index,
						get_client_memptr(wlan_client, wlan_index)->hdr_hdl_v4);

				if (IPACM_Iface::ipacmcfg->isMCC_Mode)
				{
					IPACMDBG_H("In MCC mode, use alt dst pipe: %d\n",
							tx_prop->tx[tx_index].alt_dst_pipe);
					rt_rule_entry->rule.dst = tx_prop->tx[tx_index].alt_dst_pipe;
				}
				else
				{
					rt_rule_entry->rule.dst = tx_prop->tx[tx_index].dst_pipe;
				}

				memcpy(&rt_rule_entry->rule.attrib,
						&tx_prop->tx[tx_index].attrib,
						sizeof(rt_rule_entry->rule.attrib));

				rt_rule_entry->rule.attrib.attrib_mask |= IPA_FLT_DST_ADDR;
				rt_rule_entry->rule.hdr_hdl = get_client_memptr(wlan_client, wlan_index)->hdr_hdl_v4;

				rt_rule_entry->rule.attrib.u.v4.dst_addr = get_client_memptr(wlan_client, wlan_index)->v4_addr;
				rt_rule_entry->rule.attrib.u.v4.dst_addr_mask = 0xFFFFFFFF;

				IPACMDBG_H("tx:%d, rt rule hdl=%x ip-type: %d\n", tx_index,
						get_client_memptr(wlan_client, wlan_index)->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v4, iptype);

				rt_rule_entry->rt_rule_hdl =
					get_client_memptr(wlan_client, wlan_index)->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v4;

				if (false == m_routing.ModifyRoutingRule(rt_rule))
				{
					IPACMERR("Routing rule modify failed!\n");
					free(rt_rule);
					return;
				}
				isAdded = true;
			}

		}
	}

	/* modify ipv6 routing rule */
	if (iptype == IPA_IP_v6)
	{
		for (wlan_index = 0; wlan_index < num_wifi_client_tmp; wlan_index++)
		{

			IPACMDBG_H("wlan client index: %d, ip-type: %d, ipv6_set:%d, ipv6_rule_num:%d \n", wlan_index, iptype,
					get_client_memptr(wlan_client, wlan_index)->ipv6_set,
					get_client_memptr(wlan_client, wlan_index)->route_rule_set_v6);

			if (get_client_memptr(wlan_client, wlan_index)->power_save_set == true ||
					(get_client_memptr(wlan_client, wlan_index)->route_rule_set_v6 <
					 get_client_memptr(wlan_client, wlan_index)->ipv6_set) )
			{
				IPACMDBG_H("client %d route rules not set\n", wlan_index);
				continue;
			}

			IPACMDBG_H("Modify client %d route rule\n", wlan_index);
			for (tx_index = 0; tx_index < iface_query->num_tx_props; tx_index++)
			{
				if (iptype != tx_prop->tx[tx_index].ip)
				{
					IPACMDBG_H("Tx:%d, ip-type: %d ip-type not matching: %d Ignore\n",
							tx_index, tx_prop->tx[tx_index].ip, iptype);
					continue;
				}

				for (v6_num = get_client_memptr(wlan_client, wlan_index)->route_rule_set_v6;
						v6_num < get_client_memptr(wlan_client, wlan_index)->ipv6_set;
						v6_num++)
				{

					IPACMDBG_H("client(%d): v6 header handle:(0x%x)\n",
							wlan_index,
							get_client_memptr(wlan_client, wlan_index)->hdr_hdl_v6);

					if (IPACM_Iface::ipacmcfg->isMCC_Mode)
					{
						IPACMDBG_H("In MCC mode, use alt dst pipe: %d\n",
								tx_prop->tx[tx_index].alt_dst_pipe);
						rt_rule_entry->rule.dst = tx_prop->tx[tx_index].alt_dst_pipe;
					}
					else
					{
						rt_rule_entry->rule.dst = tx_prop->tx[tx_index].dst_pipe;
					}

					memcpy(&rt_rule_entry->rule.attrib,
							&tx_prop->tx[tx_index].attrib,
							sizeof(rt_rule_entry->rule.attrib));

					rt_rule_entry->rule.hdr_hdl = get_client_memptr(wlan_client, wlan_index)->hdr_hdl_v6;
					rt_rule_entry->rule.attrib.attrib_mask |= IPA_FLT_DST_ADDR;

					rt_rule_entry->rule.attrib.u.v6.dst_addr[0] = get_client_memptr(wlan_client, wlan_index)->v6_addr[v6_num][0];
					rt_rule_entry->rule.attrib.u.v6.dst_addr[1] = get_client_memptr(wlan_client, wlan_index)->v6_addr[v6_num][1];
					rt_rule_entry->rule.attrib.u.v6.dst_addr[2] = get_client_memptr(wlan_client, wlan_index)->v6_addr[v6_num][2];
					rt_rule_entry->rule.attrib.u.v6.dst_addr[3] = get_client_memptr(wlan_client, wlan_index)->v6_addr[v6_num][3];
					rt_rule_entry->rule.attrib.u.v6.dst_addr_mask[0] = 0xFFFFFFFF;
					rt_rule_entry->rule.attrib.u.v6.dst_addr_mask[1] = 0xFFFFFFFF;
					rt_rule_entry->rule.attrib.u.v6.dst_addr_mask[2] = 0xFFFFFFFF;
					rt_rule_entry->rule.attrib.u.v6.dst_addr_mask[3] = 0xFFFFFFFF;

					rt_rule_entry->rt_rule_hdl =
						get_client_memptr(wlan_client, wlan_index)->wifi_rt_hdl[tx_index].wifi_rt_rule_hdl_v6_wan[v6_num];

					if (false == m_routing.ModifyRoutingRule(rt_rule))
					{
						IPACMERR("Routing rule modify failed!\n");
						free(rt_rule);
						return;
					}
					isAdded = true;
				}
			}

		}
	}


	if (isAdded)
	{
		if (false == m_routing.Commit(iptype))
		{
			IPACMERR("Routing rule modify commit failed!\n");
			free(rt_rule);
			return;
		}

		IPACMDBG("Routing rule modified successfully \n");
	}

	if(rt_rule)
	{
		free(rt_rule);
	}
	return;
}

#ifdef FEATURE_IPACM_RESTART
int IPACM_Wlan::ipa_query_wlan_client()
{
	int fd = -1;

	if ((fd = open(IPA_DEVICE_NAME, O_RDWR)) < 0) {
		IPACMERR("Failed opening %s.\n", IPA_DEVICE_NAME);
		return IPACM_FAILURE;
	}

	if (ioctl(fd, IPA_IOC_QUERY_WLAN_CLIENT) < 0) {
		IPACMERR("IOCTL IPA_IOC_QUERY_WLAN_CLIENT call failed: %s \n", strerror(errno));
		close(fd);
		return IPACM_FAILURE;
	}

	IPACMDBG_H("send IPA_IOC_QUERY_WLAN_CLIENT \n");
	close(fd);
	return IPACM_SUCCESS;
}
#endif

void IPACM_Wlan::eth_bridge_handle_wlan_mode_switch()
{
	uint32_t i;

	/* ====== post events to mimic WLAN interface goes down/up when AP mode is changing ====== */

	/* first post IFACE_DOWN event */
	eth_bridge_post_event(IPA_ETH_BRIDGE_IFACE_DOWN, IPA_IP_MAX, NULL, NULL, NULL, IPA_CLIENT_MAX);

	/* then post IFACE_UP event */
	if(ip_type == IPA_IP_v4 || ip_type == IPA_IP_MAX)
	{
		eth_bridge_post_event(IPA_ETH_BRIDGE_IFACE_UP, IPA_IP_v4, NULL, NULL, NULL, IPA_CLIENT_MAX);
	}
	if(ip_type == IPA_IP_v6 || ip_type == IPA_IP_MAX)
	{
		eth_bridge_post_event(IPA_ETH_BRIDGE_IFACE_UP, IPA_IP_v6, NULL, NULL, NULL, IPA_CLIENT_MAX);
	}

	/* at last post CLIENT_ADD event */
	for(i = 0; i < num_wifi_client; i++)
	{
		eth_bridge_post_event(IPA_ETH_BRIDGE_CLIENT_ADD, IPA_IP_MAX,
			get_client_memptr(wlan_client, i)->mac, NULL, NULL, IPA_CLIENT_MAX);
	}

	return;
}

bool IPACM_Wlan::is_guest_ap()
{
	return m_is_guest_ap;
}

int IPACM_Wlan::add_connection(int client_index, int v6_num)
{
	int len, res = IPACM_SUCCESS;
	uint8_t mux_id;
	ipa_ioc_add_flt_rule *pFilteringTable = NULL;
	int fd;

	mux_id = IPACM_Iface::ipacmcfg->GetQmapId();
	/* contruct filter rules to pcie modem */
	struct ipa_flt_rule_add flt_rule_entry;
	ipa_ioc_generate_flt_eq flt_eq;

	IPACMDBG("\n");
	len = sizeof(struct ipa_ioc_add_flt_rule) + sizeof(struct ipa_flt_rule_add);
	pFilteringTable = (struct ipa_ioc_add_flt_rule*)malloc(len);
	if (pFilteringTable == NULL)
	{
		IPACMERR("Error Locate ipa_flt_rule_add memory...\n");
		return IPACM_FAILURE;
	}
	memset(pFilteringTable, 0, len);


	pFilteringTable->commit = 1;
	pFilteringTable->global = false;
	pFilteringTable->ip = IPA_IP_v6;
	pFilteringTable->num_rules = (uint8_t)1;

	/* Configuring Filtering Rule */
	memset(&flt_rule_entry, 0, sizeof(struct ipa_flt_rule_add));
	flt_rule_entry.at_rear = true;
	flt_rule_entry.flt_rule_hdl = -1;
	flt_rule_entry.status = -1;

	flt_rule_entry.rule.retain_hdr = 1;
	flt_rule_entry.rule.to_uc = 0;
	flt_rule_entry.rule.eq_attrib_type = 1;
	flt_rule_entry.rule.action = IPA_PASS_TO_ROUTING;
#ifdef FEATURE_IPA_V3
	flt_rule_entry.rule.hashable = true;
#endif
	flt_rule_entry.rule.attrib.attrib_mask |= IPA_FLT_DST_ADDR;
	flt_rule_entry.rule.attrib.u.v6.dst_addr[0] = get_client_memptr(wlan_client, client_index)->v6_addr[v6_num][0];
	flt_rule_entry.rule.attrib.u.v6.dst_addr[1] = get_client_memptr(wlan_client, client_index)->v6_addr[v6_num][1];
	flt_rule_entry.rule.attrib.u.v6.dst_addr[2] = get_client_memptr(wlan_client, client_index)->v6_addr[v6_num][2];
	flt_rule_entry.rule.attrib.u.v6.dst_addr[3] = get_client_memptr(wlan_client, client_index)->v6_addr[v6_num][3];
	flt_rule_entry.rule.attrib.u.v6.dst_addr_mask[0] = 0xFFFFFFFF;
	flt_rule_entry.rule.attrib.u.v6.dst_addr_mask[1] = 0xFFFFFFFF;
	flt_rule_entry.rule.attrib.u.v6.dst_addr_mask[2] = 0xFFFFFFFF;
	flt_rule_entry.rule.attrib.u.v6.dst_addr_mask[3] = 0xFFFFFFFF;

	IPACMDBG_H("ipv6 address got: 0x%x:%x:%x:%x\n", get_client_memptr(wlan_client, client_index)->v6_addr[v6_num][0],
		get_client_memptr(wlan_client, client_index)->v6_addr[v6_num][1],
		get_client_memptr(wlan_client, client_index)->v6_addr[v6_num][2],
		get_client_memptr(wlan_client, client_index)->v6_addr[v6_num][3]);

	/* change to network order for modem */
	change_to_network_order(IPA_IP_v6, &flt_rule_entry.rule.attrib);

	memset(&flt_eq, 0, sizeof(flt_eq));
	memcpy(&flt_eq.attrib, &flt_rule_entry.rule.attrib, sizeof(flt_eq.attrib));
	flt_eq.ip = IPA_IP_v6;

	fd = open(IPA_DEVICE_NAME, O_RDWR);
	if (fd < 0)
	{
		IPACMERR("Failed opening %s.\n", IPA_DEVICE_NAME);
		free(pFilteringTable);
		return IPACM_FAILURE;
	}

	if(0 != ioctl(fd, IPA_IOC_GENERATE_FLT_EQ, &flt_eq))
	{
		IPACMERR("Failed to get eq_attrib\n");
		res = IPACM_FAILURE;
		goto fail;
	}
	memcpy(&flt_rule_entry.rule.eq_attrib,
		&flt_eq.eq_attrib,
		sizeof(flt_rule_entry.rule.eq_attrib));
	memcpy(&(pFilteringTable->rules[0]), &flt_rule_entry, sizeof(struct ipa_flt_rule_add));

	if(false == m_filtering.AddOffloadFilteringRule(pFilteringTable, mux_id, 0))
	{
		IPACMERR("Failed to install WAN DL filtering table.\n");
		res = IPACM_FAILURE;
		goto fail;
	}

	get_client_memptr(wlan_client, client_index)->v6_rt_rule_id[v6_num] = pFilteringTable->rules[0].flt_rule_hdl;
	IPACMDBG_H("%d-st client v6_num %d: id handle 0x%x\n", client_index, v6_num, get_client_memptr(wlan_client, client_index)->v6_rt_rule_id[v6_num]);

fail:
	close(fd);
	if(pFilteringTable != NULL)
	{
		free(pFilteringTable);
	}
	return res;
}

int IPACM_Wlan::del_connection(int client_index, int v6_num)
{
	int len, res = IPACM_SUCCESS;
	ipa_ioc_del_flt_rule *pFilteringTable = NULL;

	struct ipa_flt_rule_del flt_rule_entry;

	IPACMDBG("\n");
	len = sizeof(struct ipa_ioc_del_flt_rule) + sizeof(struct ipa_flt_rule_del);
	pFilteringTable = (struct ipa_ioc_del_flt_rule*)malloc(len);
	if (pFilteringTable == NULL)
	{
		IPACMERR("Error Locate ipa_ioc_del_flt_rule memory...\n");
		return IPACM_FAILURE;
	}
	memset(pFilteringTable, 0, len);


	pFilteringTable->commit = 1;
	pFilteringTable->ip = IPA_IP_v6;
	pFilteringTable->num_hdls = (uint8_t)1;

	/* Configuring Software-Routing Filtering Rule */
	memset(&flt_rule_entry, 0, sizeof(struct ipa_flt_rule_del));
	flt_rule_entry.hdl = get_client_memptr(wlan_client, client_index)->v6_rt_rule_id[v6_num];

	memcpy(&(pFilteringTable->hdl[0]), &flt_rule_entry, sizeof(struct ipa_flt_rule_del));

	if(false == m_filtering.DelOffloadFilteringRule(pFilteringTable))
	{
		IPACMERR("Failed to install WAN DL filtering table.\n");
		res = IPACM_FAILURE;
		goto fail;
	}
	get_client_memptr(wlan_client, client_index)->v6_rt_rule_id[v6_num] = 0;

fail:
	if(pFilteringTable != NULL)
	{
		free(pFilteringTable);
	}
	return res;
}
