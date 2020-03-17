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
	IPACM_Main.cpp

	@brief
	This file implements the IPAM functionality.

	@Author
	Skylar Chang

*/
/******************************************************************************

                      IPCM_MAIN.C

******************************************************************************/

#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <stdlib.h>
#include <signal.h>
#include "linux/ipa_qmi_service_v01.h"

#include "IPACM_CmdQueue.h"
#include "IPACM_EvtDispatcher.h"
#include "IPACM_Defs.h"
#include "IPACM_Neighbor.h"
#include "IPACM_IfaceManager.h"
#include "IPACM_Log.h"
#include "IPACM_Wan.h"

#include "IPACM_ConntrackListener.h"
#include "IPACM_ConntrackClient.h"
#include "IPACM_Netlink.h"

#ifdef FEATURE_IPACM_HAL
#include "IPACM_OffloadManager.h"
#include <HAL.h>
#endif

#include "IPACM_LanToLan.h"

#define IPA_DRIVER  "/dev/ipa"

#define IPACM_FIREWALL_FILE_NAME    "mobileap_firewall.xml"
#define IPACM_CFG_FILE_NAME    "IPACM_cfg.xml"
#ifdef FEATURE_IPA_ANDROID
#define IPACM_PID_FILE "/data/vendor/ipa/ipacm.pid"
#define IPACM_DIR_NAME     "/data"
#else/* defined(FEATURE_IPA_ANDROID) */
#define IPACM_PID_FILE "/etc/ipacm.pid"
#define IPACM_DIR_NAME     "/etc"
#endif /* defined(NOT FEATURE_IPA_ANDROID)*/
#define IPACM_NAME "ipacm"

#define INOTIFY_EVENT_SIZE  (sizeof(struct inotify_event))
#define INOTIFY_BUF_LEN     (INOTIFY_EVENT_SIZE + 2*sizeof(IPACM_FIREWALL_FILE_NAME))

#define IPA_DRIVER_WLAN_EVENT_MAX_OF_ATTRIBS  3
#define IPA_DRIVER_WLAN_EVENT_SIZE  (sizeof(struct ipa_wlan_msg_ex)+ IPA_DRIVER_WLAN_EVENT_MAX_OF_ATTRIBS*sizeof(ipa_wlan_hdr_attrib_val))
#define IPA_DRIVER_PIPE_STATS_EVENT_SIZE  (sizeof(struct ipa_get_data_stats_resp_msg_v01))
#define IPA_DRIVER_WLAN_META_MSG    (sizeof(struct ipa_msg_meta))
#define IPA_DRIVER_WLAN_BUF_LEN     (IPA_DRIVER_PIPE_STATS_EVENT_SIZE + IPA_DRIVER_WLAN_META_MSG)

uint32_t ipacm_event_stats[IPACM_EVENT_MAX];
bool ipacm_logging = true;

void ipa_is_ipacm_running(void);
int ipa_get_if_index(char *if_name, int *if_index);

IPACM_Neighbor *neigh;
IPACM_IfaceManager *ifacemgr;

#ifdef FEATURE_IPACM_RESTART
int ipa_reset();
/* support ipacm restart */
int ipa_query_wlan_client();
#endif


/* support ipa-hw-index-counters */
#ifdef IPA_IOCTL_SET_FNR_COUNTER_INFO
int ipa_reset_hw_index_counter();
#endif

#ifdef FEATURE_IPACM_HAL
	IPACM_OffloadManager* OffloadMng;
	HAL *hal;
#endif

/* start netlink socket monitor*/
void* netlink_start(void *param)
{
	param = NULL;
	ipa_nl_sk_fd_set_info_t sk_fdset;
	int ret_val = 0;
	memset(&sk_fdset, 0, sizeof(ipa_nl_sk_fd_set_info_t));
	IPACMDBG_H("netlink starter memset sk_fdset succeeds\n");
	ret_val = ipa_nl_listener_init(NETLINK_ROUTE, (RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_ROUTE | RTMGRP_LINK |
																										RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR | RTMGRP_NEIGH |
																										RTNLGRP_IPV6_PREFIX),
																 &sk_fdset, ipa_nl_recv_msg);

	if (ret_val != IPACM_SUCCESS)
	{
		IPACMERR("Failed to initialize IPA netlink event listener\n");
		return NULL;
	}

	return NULL;
}

/* start firewall-rule monitor*/
void* firewall_monitor(void *param)
{
	int length;
	int wd;
	char buffer[INOTIFY_BUF_LEN];
	int inotify_fd;
	ipacm_cmd_q_data evt_data;
	uint32_t mask = IN_MODIFY | IN_MOVE;

	param = NULL;
	inotify_fd = inotify_init();
	if (inotify_fd < 0)
	{
		PERROR("inotify_init");
	}

	IPACMDBG_H("Waiting for nofications in dir %s with mask: 0x%x\n", IPACM_DIR_NAME, mask);

	wd = inotify_add_watch(inotify_fd,
												 IPACM_DIR_NAME,
												 mask);

	while (1)
	{
		length = read(inotify_fd, buffer, INOTIFY_BUF_LEN);
		if (length < 0)
		{
			IPACMERR("inotify read() error return length: %d and mask: 0x%x\n", length, mask);
			continue;
		}

		struct inotify_event* event;
		event = (struct inotify_event*)malloc(length);
		if(event == NULL)
		{
			IPACMERR("Failed to allocate memory.\n");
			return NULL;
		}
		memset(event, 0, length);
		memcpy(event, buffer, length);

		if (event->len > 0)
		{
			if ( (event->mask & IN_MODIFY) || (event->mask & IN_MOVE))
			{
				if (event->mask & IN_ISDIR)
				{
					IPACMDBG_H("The directory %s was 0x%x\n", event->name, event->mask);
				}
				else if (!strncmp(event->name, IPACM_FIREWALL_FILE_NAME, event->len)) // firewall_rule change
				{
					IPACMDBG_H("File \"%s\" was 0x%x\n", event->name, event->mask);
					IPACMDBG_H("The interested file %s .\n", IPACM_FIREWALL_FILE_NAME);

					evt_data.event = IPA_FIREWALL_CHANGE_EVENT;
					evt_data.evt_data = NULL;

					/* Insert IPA_FIREWALL_CHANGE_EVENT to command queue */
					IPACM_EvtDispatcher::PostEvt(&evt_data);
				}
				else if (!strncmp(event->name, IPACM_CFG_FILE_NAME, event->len)) // IPACM_configuration change
				{
					IPACMDBG_H("File \"%s\" was 0x%x\n", event->name, event->mask);
					IPACMDBG_H("The interested file %s .\n", IPACM_CFG_FILE_NAME);

					evt_data.event = IPA_CFG_CHANGE_EVENT;
					evt_data.evt_data = NULL;

					/* Insert IPA_FIREWALL_CHANGE_EVENT to command queue */
					IPACM_EvtDispatcher::PostEvt(&evt_data);
				}
			}
			IPACMDBG_H("Received monitoring event %s.\n", event->name);
		}
		free(event);
	}

	(void)inotify_rm_watch(inotify_fd, wd);
	(void)close(inotify_fd);
	return NULL;
}


/* start IPACM wan-driver notifier */
void* ipa_driver_msg_notifier(void *param)
{
	int length, fd, cnt;
	char buffer[IPA_DRIVER_WLAN_BUF_LEN];
	struct ipa_msg_meta event_hdr;
	struct ipa_ecm_msg event_ecm;
	struct ipa_wan_msg event_wan;
	struct ipa_wlan_msg_ex event_ex_o;
	struct ipa_wlan_msg *event_wlan = NULL;
	struct ipa_wlan_msg_ex *event_ex = NULL;
#ifdef WIGIG_CLIENT_CONNECT
	struct ipa_wigig_msg *event_wigig = NULL;
#endif
	struct ipa_get_data_stats_resp_msg_v01 event_data_stats;
	struct ipa_get_apn_data_stats_resp_msg_v01 event_network_stats;
#ifdef IPA_RT_SUPPORT_COAL
	struct ipa_coalesce_info coalesce_info;
#endif

#ifdef FEATURE_IPACM_HAL
	IPACM_OffloadManager* OffloadMng;
#endif

	ipacm_cmd_q_data evt_data;
	ipacm_event_data_mac *data = NULL;
#ifdef WIGIG_CLIENT_CONNECT
	ipacm_event_data_mac_ep *data_wigig = NULL;
#endif
	ipacm_event_data_fid *data_fid = NULL;
	ipacm_event_data_iptype *data_iptype = NULL;
	ipacm_event_data_wlan_ex *data_ex;
	ipa_get_data_stats_resp_msg_v01 *data_tethering_stats = NULL;
	ipa_get_apn_data_stats_resp_msg_v01 *data_network_stats = NULL;
#ifdef FEATURE_L2TP
	ipa_ioc_vlan_iface_info *vlan_info = NULL;
	ipa_ioc_l2tp_vlan_mapping_info *mapping = NULL;
#endif
	ipacm_cmd_q_data new_neigh_evt;
	ipacm_event_data_all* new_neigh_data;

	param = NULL;
	fd = open(IPA_DRIVER, O_RDWR);
	if (fd < 0)
	{
		IPACMERR("Failed opening %s.\n", IPA_DRIVER);
		return NULL;
	}

	while (1)
	{
		IPACMDBG_H("Waiting for nofications from IPA driver \n");
		memset(buffer, 0, sizeof(buffer));
		memset(&evt_data, 0, sizeof(evt_data));
		memset(&new_neigh_evt, 0, sizeof(ipacm_cmd_q_data));
		new_neigh_data = NULL;
		data = NULL;
		data_fid = NULL;
		data_tethering_stats = NULL;
		data_network_stats = NULL;

		length = read(fd, buffer, IPA_DRIVER_WLAN_BUF_LEN);
		if (length < 0)
		{
			PERROR("didn't read IPA_driver correctly");
			continue;
		}

		memcpy(&event_hdr, buffer,sizeof(struct ipa_msg_meta));
		IPACMDBG_H("Message type: %d\n", event_hdr.msg_type);
		IPACMDBG_H("Event header length received: %d\n",event_hdr.msg_len);

		/* Insert WLAN_DRIVER_EVENT to command queue */
		switch (event_hdr.msg_type)
		{

		case SW_ROUTING_ENABLE:
			IPACMDBG_H("Received SW_ROUTING_ENABLE\n");
			evt_data.event = IPA_SW_ROUTING_ENABLE;
			IPACMDBG_H("Not supported anymore\n");
			continue;

		case SW_ROUTING_DISABLE:
			IPACMDBG_H("Received SW_ROUTING_DISABLE\n");
			evt_data.event = IPA_SW_ROUTING_DISABLE;
			IPACMDBG_H("Not supported anymore\n");
			continue;

		case WLAN_AP_CONNECT:
			event_wlan = (struct ipa_wlan_msg *) (buffer + sizeof(struct ipa_msg_meta));
			IPACMDBG_H("Received WLAN_AP_CONNECT name: %s\n",event_wlan->name);
			IPACMDBG_H("AP Mac Address %02x:%02x:%02x:%02x:%02x:%02x\n",
							 event_wlan->mac_addr[0], event_wlan->mac_addr[1], event_wlan->mac_addr[2],
							 event_wlan->mac_addr[3], event_wlan->mac_addr[4], event_wlan->mac_addr[5]);
                        data_fid = (ipacm_event_data_fid *)malloc(sizeof(ipacm_event_data_fid));
			if(data_fid == NULL)
			{
				IPACMERR("unable to allocate memory for event_wlan data_fid\n");
				return NULL;
			}
			ipa_get_if_index(event_wlan->name, &(data_fid->if_index));
			evt_data.event = IPA_WLAN_AP_LINK_UP_EVENT;
			evt_data.evt_data = data_fid;
			break;

		case WLAN_AP_DISCONNECT:
			event_wlan = (struct ipa_wlan_msg *)(buffer + sizeof(struct ipa_msg_meta));
			IPACMDBG_H("Received WLAN_AP_DISCONNECT name: %s\n",event_wlan->name);
			IPACMDBG_H("AP Mac Address %02x:%02x:%02x:%02x:%02x:%02x\n",
							 event_wlan->mac_addr[0], event_wlan->mac_addr[1], event_wlan->mac_addr[2],
							 event_wlan->mac_addr[3], event_wlan->mac_addr[4], event_wlan->mac_addr[5]);
                        data_fid = (ipacm_event_data_fid *)malloc(sizeof(ipacm_event_data_fid));
			if(data_fid == NULL)
			{
				IPACMERR("unable to allocate memory for event_wlan data_fid\n");
				return NULL;
			}
			ipa_get_if_index(event_wlan->name, &(data_fid->if_index));
			evt_data.event = IPA_WLAN_LINK_DOWN_EVENT;
			evt_data.evt_data = data_fid;
			break;
		case WLAN_STA_CONNECT:
			event_wlan = (struct ipa_wlan_msg *)(buffer + sizeof(struct ipa_msg_meta));
			IPACMDBG_H("Received WLAN_STA_CONNECT name: %s\n",event_wlan->name);
			IPACMDBG_H("STA Mac Address %02x:%02x:%02x:%02x:%02x:%02x\n",
							 event_wlan->mac_addr[0], event_wlan->mac_addr[1], event_wlan->mac_addr[2],
							 event_wlan->mac_addr[3], event_wlan->mac_addr[4], event_wlan->mac_addr[5]);
			data = (ipacm_event_data_mac *)malloc(sizeof(ipacm_event_data_mac));
			if(data == NULL)
			{
				IPACMERR("unable to allocate memory for event_wlan data_fid\n");
				return NULL;
			}
			memcpy(data->mac_addr,
				 event_wlan->mac_addr,
				 sizeof(event_wlan->mac_addr));
			ipa_get_if_index(event_wlan->name, &(data->if_index));
			evt_data.event = IPA_WLAN_STA_LINK_UP_EVENT;
			evt_data.evt_data = data;
			break;

		case WLAN_STA_DISCONNECT:
			event_wlan = (struct ipa_wlan_msg *)(buffer + sizeof(struct ipa_msg_meta));
			IPACMDBG_H("Received WLAN_STA_DISCONNECT name: %s\n",event_wlan->name);
			IPACMDBG_H("STA Mac Address %02x:%02x:%02x:%02x:%02x:%02x\n",
							 event_wlan->mac_addr[0], event_wlan->mac_addr[1], event_wlan->mac_addr[2],
							 event_wlan->mac_addr[3], event_wlan->mac_addr[4], event_wlan->mac_addr[5]);
                        data_fid = (ipacm_event_data_fid *)malloc(sizeof(ipacm_event_data_fid));
			if(data_fid == NULL)
			{
				IPACMERR("unable to allocate memory for event_wlan data_fid\n");
				return NULL;
			}
			ipa_get_if_index(event_wlan->name, &(data_fid->if_index));
			evt_data.event = IPA_WLAN_LINK_DOWN_EVENT;
			evt_data.evt_data = data_fid;
			break;

		case WLAN_CLIENT_CONNECT:
			event_wlan = (struct ipa_wlan_msg *)(buffer + sizeof(struct ipa_msg_meta));
			IPACMDBG_H("Received WLAN_CLIENT_CONNECT\n");
			IPACMDBG_H("Mac Address %02x:%02x:%02x:%02x:%02x:%02x\n",
							 event_wlan->mac_addr[0], event_wlan->mac_addr[1], event_wlan->mac_addr[2],
							 event_wlan->mac_addr[3], event_wlan->mac_addr[4], event_wlan->mac_addr[5]);
		        data = (ipacm_event_data_mac *)malloc(sizeof(ipacm_event_data_mac));
		        if (data == NULL)
		        {
		    	        IPACMERR("unable to allocate memory for event_wlan data\n");
		    	        return NULL;
		        }
			memcpy(data->mac_addr,
						 event_wlan->mac_addr,
						 sizeof(event_wlan->mac_addr));
			ipa_get_if_index(event_wlan->name, &(data->if_index));
		        evt_data.event = IPA_WLAN_CLIENT_ADD_EVENT;
			evt_data.evt_data = data;
			break;
#ifdef WIGIG_CLIENT_CONNECT
		case WIGIG_CLIENT_CONNECT:
			event_wigig = (struct ipa_wigig_msg *)(buffer + sizeof(struct ipa_msg_meta));
			IPACMDBG_H("Received WIGIG_CLIENT_CONNECT\n");
			IPACMDBG_H("Mac Address %02x:%02x:%02x:%02x:%02x:%02x, ep %d\n",
				event_wigig->client_mac_addr[0], event_wigig->client_mac_addr[1], event_wigig->client_mac_addr[2],
				event_wigig->client_mac_addr[3], event_wigig->client_mac_addr[4], event_wigig->client_mac_addr[5],
				event_wigig->u.ipa_client);

			data_wigig = (ipacm_event_data_mac_ep *)malloc(sizeof(ipacm_event_data_mac_ep));
			if(data_wigig == NULL)
			{
				IPACMERR("unable to allocate memory for event_wigig data\n");
				return NULL;
			}
			memcpy(data_wigig->mac_addr,
				event_wigig->client_mac_addr,
				sizeof(data_wigig->mac_addr));
			ipa_get_if_index(event_wigig->name, &(data_wigig->if_index));
			data_wigig->client = event_wigig->u.ipa_client;
			evt_data.event = IPA_WIGIG_CLIENT_ADD_EVENT;
			evt_data.evt_data = data_wigig;
			break;
#endif
		case WLAN_CLIENT_CONNECT_EX:
			IPACMDBG_H("Received WLAN_CLIENT_CONNECT_EX\n");

			memcpy(&event_ex_o, buffer + sizeof(struct ipa_msg_meta),sizeof(struct ipa_wlan_msg_ex));
			if(event_ex_o.num_of_attribs > IPA_DRIVER_WLAN_EVENT_MAX_OF_ATTRIBS)
			{
				IPACMERR("buffer size overflow\n");
				return NULL;
			}
			length = sizeof(ipa_wlan_msg_ex)+ event_ex_o.num_of_attribs * sizeof(ipa_wlan_hdr_attrib_val);
			IPACMDBG_H("num_of_attribs %d, length %d\n", event_ex_o.num_of_attribs, length);
			event_ex = (ipa_wlan_msg_ex *)malloc(length);
			if(event_ex == NULL )
			{
				IPACMERR("Unable to allocate memory\n");
				return NULL;
			}
			memcpy(event_ex, buffer + sizeof(struct ipa_msg_meta), length);
			data_ex = (ipacm_event_data_wlan_ex *)malloc(sizeof(ipacm_event_data_wlan_ex) + event_ex_o.num_of_attribs * sizeof(ipa_wlan_hdr_attrib_val));
		    if (data_ex == NULL)
		    {
				IPACMERR("unable to allocate memory for event data\n");
		    	return NULL;
		    }
			data_ex->num_of_attribs = event_ex->num_of_attribs;

			memcpy(data_ex->attribs,
						event_ex->attribs,
						event_ex->num_of_attribs * sizeof(ipa_wlan_hdr_attrib_val));

			ipa_get_if_index(event_ex->name, &(data_ex->if_index));
			evt_data.event = IPA_WLAN_CLIENT_ADD_EVENT_EX;
			evt_data.evt_data = data_ex;

			/* Construct new_neighbor msg with netdev device internally */
			new_neigh_data = (ipacm_event_data_all*)malloc(sizeof(ipacm_event_data_all));
			if(new_neigh_data == NULL)
			{
				IPACMERR("Failed to allocate memory.\n");
				return NULL;
			}
			memset(new_neigh_data, 0, sizeof(ipacm_event_data_all));
			new_neigh_data->iptype = IPA_IP_v6;
			for(cnt = 0; cnt < event_ex->num_of_attribs; cnt++)
			{
				if(event_ex->attribs[cnt].attrib_type == WLAN_HDR_ATTRIB_MAC_ADDR)
				{
					memcpy(new_neigh_data->mac_addr, event_ex->attribs[cnt].u.mac_addr, sizeof(new_neigh_data->mac_addr));
					IPACMDBG_H("Mac Address %02x:%02x:%02x:%02x:%02x:%02x\n",
								 event_ex->attribs[cnt].u.mac_addr[0], event_ex->attribs[cnt].u.mac_addr[1], event_ex->attribs[cnt].u.mac_addr[2],
								 event_ex->attribs[cnt].u.mac_addr[3], event_ex->attribs[cnt].u.mac_addr[4], event_ex->attribs[cnt].u.mac_addr[5]);
				}
				else if(event_ex->attribs[cnt].attrib_type == WLAN_HDR_ATTRIB_STA_ID)
				{
					IPACMDBG_H("Wlan client id %d\n",event_ex->attribs[cnt].u.sta_id);
				}
				else
				{
					IPACMDBG_H("Wlan message has unexpected type!\n");
				}
			}
			new_neigh_data->if_index = data_ex->if_index;
			new_neigh_evt.evt_data = (void*)new_neigh_data;
			new_neigh_evt.event = IPA_NEW_NEIGH_EVENT;
			free(event_ex);
			break;

		case WLAN_CLIENT_DISCONNECT:
			IPACMDBG_H("Received WLAN_CLIENT_DISCONNECT\n");
			event_wlan = (struct ipa_wlan_msg *)(buffer + sizeof(struct ipa_msg_meta));
			IPACMDBG_H("Mac Address %02x:%02x:%02x:%02x:%02x:%02x\n",
							 event_wlan->mac_addr[0], event_wlan->mac_addr[1], event_wlan->mac_addr[2],
							 event_wlan->mac_addr[3], event_wlan->mac_addr[4], event_wlan->mac_addr[5]);
		        data = (ipacm_event_data_mac *)malloc(sizeof(ipacm_event_data_mac));
		        if (data == NULL)
		        {
		    	        IPACMERR("unable to allocate memory for event_wlan data\n");
		    	        return NULL;
		        }
			memcpy(data->mac_addr,
						 event_wlan->mac_addr,
						 sizeof(event_wlan->mac_addr));
			ipa_get_if_index(event_wlan->name, &(data->if_index));
			evt_data.event = IPA_WLAN_CLIENT_DEL_EVENT;
			evt_data.evt_data = data;
			break;

		case WLAN_CLIENT_POWER_SAVE_MODE:
			IPACMDBG_H("Received WLAN_CLIENT_POWER_SAVE_MODE\n");
			event_wlan = (struct ipa_wlan_msg *)(buffer + sizeof(struct ipa_msg_meta));
			IPACMDBG_H("Mac Address %02x:%02x:%02x:%02x:%02x:%02x\n",
							 event_wlan->mac_addr[0], event_wlan->mac_addr[1], event_wlan->mac_addr[2],
							 event_wlan->mac_addr[3], event_wlan->mac_addr[4], event_wlan->mac_addr[5]);
		        data = (ipacm_event_data_mac *)malloc(sizeof(ipacm_event_data_mac));
		        if (data == NULL)
		        {
		    	        IPACMERR("unable to allocate memory for event_wlan data\n");
		    	        return NULL;
		        }
			memcpy(data->mac_addr,
						 event_wlan->mac_addr,
						 sizeof(event_wlan->mac_addr));
			ipa_get_if_index(event_wlan->name, &(data->if_index));
			evt_data.event = IPA_WLAN_CLIENT_POWER_SAVE_EVENT;
			evt_data.evt_data = data;
			break;

		case WLAN_CLIENT_NORMAL_MODE:
			IPACMDBG_H("Received WLAN_CLIENT_NORMAL_MODE\n");
			event_wlan = (struct ipa_wlan_msg *)(buffer + sizeof(struct ipa_msg_meta));
			IPACMDBG_H("Mac Address %02x:%02x:%02x:%02x:%02x:%02x\n",
							 event_wlan->mac_addr[0], event_wlan->mac_addr[1], event_wlan->mac_addr[2],
							 event_wlan->mac_addr[3], event_wlan->mac_addr[4], event_wlan->mac_addr[5]);
		        data = (ipacm_event_data_mac *)malloc(sizeof(ipacm_event_data_mac));
		        if (data == NULL)
		        {
		    	       IPACMERR("unable to allocate memory for event_wlan data\n");
		    	       return NULL;
		        }
			memcpy(data->mac_addr,
						 event_wlan->mac_addr,
						 sizeof(event_wlan->mac_addr));
			ipa_get_if_index(event_wlan->name, &(data->if_index));
			evt_data.evt_data = data;
			evt_data.event = IPA_WLAN_CLIENT_RECOVER_EVENT;
			break;

		case ECM_CONNECT:
			memcpy(&event_ecm, buffer + sizeof(struct ipa_msg_meta), sizeof(struct ipa_ecm_msg));
			IPACMDBG_H("Received ECM_CONNECT name: %s\n",event_ecm.name);
			data_fid = (ipacm_event_data_fid *)malloc(sizeof(ipacm_event_data_fid));
			if(data_fid == NULL)
			{
				IPACMERR("unable to allocate memory for event_ecm data_fid\n");
				return NULL;
			}
			data_fid->if_index = event_ecm.ifindex;
			evt_data.event = IPA_USB_LINK_UP_EVENT;
			evt_data.evt_data = data_fid;
			break;

		case ECM_DISCONNECT:
			memcpy(&event_ecm, buffer + sizeof(struct ipa_msg_meta), sizeof(struct ipa_ecm_msg));
			IPACMDBG_H("Received ECM_DISCONNECT name: %s\n",event_ecm.name);
			data_fid = (ipacm_event_data_fid *)malloc(sizeof(ipacm_event_data_fid));
			if(data_fid == NULL)
			{
				IPACMERR("unable to allocate memory for event_ecm data_fid\n");
				return NULL;
			}
			data_fid->if_index = event_ecm.ifindex;
			evt_data.event = IPA_LINK_DOWN_EVENT;
			evt_data.evt_data = data_fid;
			break;
		/* Add for 8994 Android case */
		case WAN_UPSTREAM_ROUTE_ADD:
			memcpy(&event_wan, buffer + sizeof(struct ipa_msg_meta), sizeof(struct ipa_wan_msg));
			IPACMDBG_H("Received WAN_UPSTREAM_ROUTE_ADD name: %s, tethered name: %s\n", event_wan.upstream_ifname, event_wan.tethered_ifname);
			data_iptype = (ipacm_event_data_iptype *)malloc(sizeof(ipacm_event_data_iptype));
			if(data_iptype == NULL)
			{
				IPACMERR("unable to allocate memory for event_ecm data_iptype\n");
				return NULL;
			}
			ipa_get_if_index(event_wan.upstream_ifname, &(data_iptype->if_index));
			ipa_get_if_index(event_wan.tethered_ifname, &(data_iptype->if_index_tether));
			data_iptype->iptype = event_wan.ip;
#ifdef IPA_WAN_MSG_IPv6_ADDR_GW_LEN
			data_iptype->ipv4_addr_gw = event_wan.ipv4_addr_gw;
			data_iptype->ipv6_addr_gw[0] = event_wan.ipv6_addr_gw[0];
			data_iptype->ipv6_addr_gw[1] = event_wan.ipv6_addr_gw[1];
			data_iptype->ipv6_addr_gw[2] = event_wan.ipv6_addr_gw[2];
			data_iptype->ipv6_addr_gw[3] = event_wan.ipv6_addr_gw[3];
			IPACMDBG_H("default gw ipv4 (%x)\n", data_iptype->ipv4_addr_gw);
			IPACMDBG_H("IPV6 gateway: %08x:%08x:%08x:%08x \n",
							data_iptype->ipv6_addr_gw[0], data_iptype->ipv6_addr_gw[1], data_iptype->ipv6_addr_gw[2], data_iptype->ipv6_addr_gw[3]);
#endif
			IPACMDBG_H("Received WAN_UPSTREAM_ROUTE_ADD: fid(%d) tether_fid(%d) ip-type(%d)\n", data_iptype->if_index,
					data_iptype->if_index_tether, data_iptype->iptype);
			evt_data.event = IPA_WAN_UPSTREAM_ROUTE_ADD_EVENT;
			evt_data.evt_data = data_iptype;
			break;
		case WAN_UPSTREAM_ROUTE_DEL:
			memcpy(&event_wan, buffer + sizeof(struct ipa_msg_meta), sizeof(struct ipa_wan_msg));
			IPACMDBG_H("Received WAN_UPSTREAM_ROUTE_DEL name: %s, tethered name: %s\n", event_wan.upstream_ifname, event_wan.tethered_ifname);
			data_iptype = (ipacm_event_data_iptype *)malloc(sizeof(ipacm_event_data_iptype));
			if(data_iptype == NULL)
			{
				IPACMERR("unable to allocate memory for event_ecm data_iptype\n");
				return NULL;
			}
			ipa_get_if_index(event_wan.upstream_ifname, &(data_iptype->if_index));
			ipa_get_if_index(event_wan.tethered_ifname, &(data_iptype->if_index_tether));
			data_iptype->iptype = event_wan.ip;
			IPACMDBG_H("Received WAN_UPSTREAM_ROUTE_DEL: fid(%d) ip-type(%d)\n", data_iptype->if_index, data_iptype->iptype);
			evt_data.event = IPA_WAN_UPSTREAM_ROUTE_DEL_EVENT;
			evt_data.evt_data = data_iptype;
			break;
		/* End of adding for 8994 Android case */

		/* Add for embms case */
		case WAN_EMBMS_CONNECT:
			memcpy(&event_wan, buffer + sizeof(struct ipa_msg_meta), sizeof(struct ipa_wan_msg));
			IPACMDBG("Received WAN_EMBMS_CONNECT name: %s\n",event_wan.upstream_ifname);
			data_fid = (ipacm_event_data_fid *)malloc(sizeof(ipacm_event_data_fid));
			if(data_fid == NULL)
			{
				IPACMERR("unable to allocate memory for event data_fid\n");
				return NULL;
			}
			ipa_get_if_index(event_wan.upstream_ifname, &(data_fid->if_index));
			evt_data.event = IPA_WAN_EMBMS_LINK_UP_EVENT;
			evt_data.evt_data = data_fid;
			break;

		case WLAN_SWITCH_TO_SCC:
			IPACMDBG_H("Received WLAN_SWITCH_TO_SCC\n");
			[[fallthrough]];
		case WLAN_WDI_ENABLE:
			IPACMDBG_H("Received WLAN_WDI_ENABLE\n");
			if (IPACM_Iface::ipacmcfg->isMCC_Mode == true)
			{
				IPACM_Iface::ipacmcfg->isMCC_Mode = false;
				evt_data.event = IPA_WLAN_SWITCH_TO_SCC;
				break;
			}
			continue;
		case WLAN_SWITCH_TO_MCC:
			IPACMDBG_H("Received WLAN_SWITCH_TO_MCC\n");
			[[fallthrough]];
		case WLAN_WDI_DISABLE:
			IPACMDBG_H("Received WLAN_WDI_DISABLE\n");
			if (IPACM_Iface::ipacmcfg->isMCC_Mode == false)
			{
				IPACM_Iface::ipacmcfg->isMCC_Mode = true;
				evt_data.event = IPA_WLAN_SWITCH_TO_MCC;
				break;
			}
			continue;

		case WAN_XLAT_CONNECT:
			memcpy(&event_wan, buffer + sizeof(struct ipa_msg_meta),
				sizeof(struct ipa_wan_msg));
			IPACMDBG_H("Received WAN_XLAT_CONNECT name: %s\n",
					event_wan.upstream_ifname);

			/* post IPA_LINK_UP_EVENT event
			 * may be WAN interface is not up
			*/
			data_fid = (ipacm_event_data_fid *)calloc(1, sizeof(ipacm_event_data_fid));
			if(data_fid == NULL)
			{
				IPACMERR("unable to allocate memory for xlat event\n");
				return NULL;
			}
			ipa_get_if_index(event_wan.upstream_ifname, &(data_fid->if_index));
			evt_data.event = IPA_LINK_UP_EVENT;
			evt_data.evt_data = data_fid;
			IPACMDBG_H("Posting IPA_LINK_UP_EVENT event:%d\n", evt_data.event);
			IPACM_EvtDispatcher::PostEvt(&evt_data);

			/* post IPA_WAN_XLAT_CONNECT_EVENT event */
			memset(&evt_data, 0, sizeof(evt_data));
			data_fid = (ipacm_event_data_fid *)calloc(1, sizeof(ipacm_event_data_fid));
			if(data_fid == NULL)
			{
				IPACMERR("unable to allocate memory for xlat event\n");
				return NULL;
			}
			ipa_get_if_index(event_wan.upstream_ifname, &(data_fid->if_index));
			evt_data.event = IPA_WAN_XLAT_CONNECT_EVENT;
			evt_data.evt_data = data_fid;
			IPACMDBG_H("Posting IPA_WAN_XLAT_CONNECT_EVENT event:%d\n", evt_data.event);
			break;

		case IPA_TETHERING_STATS_UPDATE_STATS:
			memcpy(&event_data_stats, buffer + sizeof(struct ipa_msg_meta), sizeof(struct ipa_get_data_stats_resp_msg_v01));
			data_tethering_stats = (ipa_get_data_stats_resp_msg_v01 *)malloc(sizeof(struct ipa_get_data_stats_resp_msg_v01));
			if(data_tethering_stats == NULL)
			{
				IPACMERR("unable to allocate memory for event data_tethering_stats\n");
				return NULL;
			}
			memcpy(data_tethering_stats,
					 &event_data_stats,
						 sizeof(struct ipa_get_data_stats_resp_msg_v01));
			IPACMDBG("Received IPA_TETHERING_STATS_UPDATE_STATS ipa_stats_type: %d\n",data_tethering_stats->ipa_stats_type);
			IPACMDBG("Received %d UL, %d DL pipe stats\n",data_tethering_stats->ul_src_pipe_stats_list_len, data_tethering_stats->dl_dst_pipe_stats_list_len);
			evt_data.event = IPA_TETHERING_STATS_UPDATE_EVENT;
			evt_data.evt_data = data_tethering_stats;
			break;

		case IPA_TETHERING_STATS_UPDATE_NETWORK_STATS:
			memcpy(&event_network_stats, buffer + sizeof(struct ipa_msg_meta), sizeof(struct ipa_get_apn_data_stats_resp_msg_v01));
			data_network_stats = (ipa_get_apn_data_stats_resp_msg_v01 *)malloc(sizeof(ipa_get_apn_data_stats_resp_msg_v01));
			if(data_network_stats == NULL)
			{
				IPACMERR("unable to allocate memory for event data_network_stats\n");
				return NULL;
			}
			memcpy(data_network_stats,
					 &event_network_stats,
						 sizeof(struct ipa_get_apn_data_stats_resp_msg_v01));
			IPACMDBG("Received %d apn network stats \n", data_network_stats->apn_data_stats_list_len);
			evt_data.event = IPA_NETWORK_STATS_UPDATE_EVENT;
			evt_data.evt_data = data_network_stats;
			break;

#ifdef FEATURE_IPACM_HAL
		case IPA_QUOTA_REACH:
			IPACMDBG_H("Received IPA_QUOTA_REACH\n");
			OffloadMng = IPACM_OffloadManager::GetInstance();
			if (OffloadMng->elrInstance == NULL) {
				IPACMERR("OffloadMng->elrInstance is NULL, can't forward to framework!\n");
			} else {
				IPACMERR("calling OffloadMng->elrInstance->onLimitReached \n");
				OffloadMng->elrInstance->onLimitReached();
			}
			continue;
		case IPA_SSR_BEFORE_SHUTDOWN:
			IPACMDBG_H("Received IPA_SSR_BEFORE_SHUTDOWN\n");
			IPACM_Wan::clearExtProp();
			OffloadMng = IPACM_OffloadManager::GetInstance();
			if (OffloadMng->elrInstance == NULL) {
				IPACMERR("OffloadMng->elrInstance is NULL, can't forward to framework!\n");
			} else {
				IPACMERR("calling OffloadMng->elrInstance->onOffloadStopped \n");
				OffloadMng->elrInstance->onOffloadStopped(IpaEventRelay::ERROR);
			}
			/* WA to clean up wlan instances during SSR */
			evt_data.event = IPA_SSR_NOTICE;
			evt_data.evt_data = NULL;
			break;
		case IPA_SSR_AFTER_POWERUP:
			IPACMDBG_H("Received IPA_SSR_AFTER_POWERUP\n");
			OffloadMng = IPACM_OffloadManager::GetInstance();
			if (OffloadMng->elrInstance == NULL) {
				IPACMERR("OffloadMng->elrInstance is NULL, can't forward to framework!\n");
			} else {
				IPACMERR("calling OffloadMng->elrInstance->onOffloadSupportAvailable \n");
				OffloadMng->elrInstance->onOffloadSupportAvailable();
			}
			continue;
#ifdef IPA_WLAN_FW_SSR_EVENT_MAX
		case WLAN_FWR_SSR_BEFORE_SHUTDOWN:
                        IPACMDBG_H("Received WLAN_FWR_SSR_BEFORE_SHUTDOWN\n");
                        evt_data.event = IPA_WLAN_FWR_SSR_BEFORE_SHUTDOWN_NOTICE;
                        evt_data.evt_data = NULL;
                        break;
#endif
#endif
#ifdef FEATURE_L2TP
		case ADD_VLAN_IFACE:
			vlan_info = (ipa_ioc_vlan_iface_info *)malloc(sizeof(*vlan_info));
			if(vlan_info == NULL)
			{
				IPACMERR("Failed to allocate memory.\n");
				return NULL;
			}
			memcpy(vlan_info, buffer + sizeof(struct ipa_msg_meta), sizeof(*vlan_info));
			evt_data.event = IPA_ADD_VLAN_IFACE;
			evt_data.evt_data = vlan_info;
			break;

		case DEL_VLAN_IFACE:
			vlan_info = (ipa_ioc_vlan_iface_info *)malloc(sizeof(*vlan_info));
			if(vlan_info == NULL)
			{
				IPACMERR("Failed to allocate memory.\n");
				return NULL;
			}
			memcpy(vlan_info, buffer + sizeof(struct ipa_msg_meta), sizeof(*vlan_info));
			evt_data.event = IPA_DEL_VLAN_IFACE;
			evt_data.evt_data = vlan_info;
			break;

		case ADD_L2TP_VLAN_MAPPING:
			mapping = (ipa_ioc_l2tp_vlan_mapping_info *)malloc(sizeof(*mapping));
			if(mapping == NULL)
			{
				IPACMERR("Failed to allocate memory.\n");
				return NULL;
			}
			memcpy(mapping, buffer + sizeof(struct ipa_msg_meta), sizeof(*mapping));
			evt_data.event = IPA_ADD_L2TP_VLAN_MAPPING;
			evt_data.evt_data = mapping;
			break;

		case DEL_L2TP_VLAN_MAPPING:
			mapping = (ipa_ioc_l2tp_vlan_mapping_info *)malloc(sizeof(*mapping));
			if(mapping == NULL)
			{
				IPACMERR("Failed to allocate memory.\n");
				return NULL;
			}
			memcpy(mapping, buffer + sizeof(struct ipa_msg_meta), sizeof(*mapping));
			evt_data.event = IPA_DEL_L2TP_VLAN_MAPPING;
			evt_data.evt_data = mapping;
			break;
#endif
#ifdef IPA_RT_SUPPORT_COAL
		case IPA_COALESCE_ENABLE:
			memcpy(&coalesce_info, buffer + sizeof(struct ipa_msg_meta), sizeof(struct ipa_coalesce_info));
			IPACMDBG_H("Received IPA_COALESCE_ENABLE qmap-id:%d tcp:%d, udp%d\n",
				coalesce_info.qmap_id, coalesce_info.tcp_enable, coalesce_info.udp_enable);
			if (coalesce_info.qmap_id >=IPA_MAX_NUM_SW_PDNS)
			{
				IPACMERR("qmap_id (%d) beyond the Max range (%d), abort\n",
				coalesce_info.qmap_id, IPA_MAX_NUM_SW_PDNS);
				return NULL;
			}
			IPACM_Wan::coalesce_config(coalesce_info.qmap_id, coalesce_info.tcp_enable, coalesce_info.udp_enable);
			/* Notify all LTE instance to do RSC configuration */
			evt_data.event = IPA_COALESCE_NOTICE;
			evt_data.evt_data = NULL;
			break;

		case IPA_COALESCE_DISABLE:
			memcpy(&coalesce_info, buffer + sizeof(struct ipa_msg_meta), sizeof(struct ipa_coalesce_info));
			IPACMDBG_H("Received IPA_COALESCE_DISABLE qmap-id:%d tcp:%d, udp%d\n",
				coalesce_info.qmap_id, coalesce_info.tcp_enable, coalesce_info.udp_enable);
			if (coalesce_info.qmap_id >=IPA_MAX_NUM_SW_PDNS)
			{
				IPACMERR("qmap_id (%d) beyond the Max range (%d), abort\n",
				coalesce_info.qmap_id, IPA_MAX_NUM_SW_PDNS);
				return NULL;
			}
			IPACM_Wan::coalesce_config(coalesce_info.qmap_id, false, false);
			/* Notify all LTE instance to do RSC configuration */
			evt_data.event = IPA_COALESCE_NOTICE;
			evt_data.evt_data = NULL;
			break;
#endif

		default:
			IPACMDBG_H("Unhandled message type: %d\n", event_hdr.msg_type);
			continue;

		}
		/* finish command queue */
		IPACMDBG_H("Posting event:%d\n", evt_data.event);
		IPACM_EvtDispatcher::PostEvt(&evt_data);
		/* push new_neighbor with netdev device internally */
		if(new_neigh_data != NULL)
		{
			IPACMDBG_H("Internally post event IPA_NEW_NEIGH_EVENT\n");
			IPACM_EvtDispatcher::PostEvt(&new_neigh_evt);
		}
	}

	(void)close(fd);
	return NULL;
}

void IPACM_Sig_Handler(int sig)
{
	ipacm_cmd_q_data evt_data;

	printf("Received Signal: %d\n", sig);
	memset(&evt_data, 0, sizeof(evt_data));

	switch(sig)
	{
		case SIGUSR1:
			IPACMDBG_H("Received SW_ROUTING_ENABLE request \n");
			evt_data.event = IPA_SW_ROUTING_ENABLE;
			IPACM_Iface::ipacmcfg->ipa_sw_rt_enable = true;
			break;

		case SIGUSR2:
			IPACMDBG_H("Received SW_ROUTING_DISABLE request \n");
			evt_data.event = IPA_SW_ROUTING_DISABLE;
			IPACM_Iface::ipacmcfg->ipa_sw_rt_enable = false;
			break;
	}
	/* finish command queue */
	IPACMDBG_H("Posting event:%d\n", evt_data.event);
	IPACM_EvtDispatcher::PostEvt(&evt_data);
	return;
}

void RegisterForSignals(void)
{

	signal(SIGUSR1, IPACM_Sig_Handler);
	signal(SIGUSR2, IPACM_Sig_Handler);
}


int main(int argc, char **argv)
{
	int ret;
	pthread_t netlink_thread = 0, monitor_thread = 0, ipa_driver_thread = 0;
	pthread_t cmd_queue_thread = 0;

	/* check if ipacm is already running or not */
	ipa_is_ipacm_running();

	IPACMDBG_H("In main()\n");
	(void)argc;
	(void)argv;

#ifdef FEATURE_IPACM_RESTART
	IPACMDBG_H("RESET IPA-HW rules\n");
	ipa_reset();
#endif

#ifdef IPA_IOCTL_SET_FNR_COUNTER_INFO
	IPACMDBG_H("Configure IPA-HW index-counter\n");
	ipa_reset_hw_index_counter();
#endif

	neigh = new IPACM_Neighbor();
	ifacemgr = new IPACM_IfaceManager();
#ifdef FEATURE_IPACM_HAL
	OffloadMng = IPACM_OffloadManager::GetInstance();
	hal = HAL::makeIPAHAL(1, OffloadMng);
	IPACMDBG_H(" START IPACM_OffloadManager and link to android framework\n");
#endif

#ifdef FEATURE_ETH_BRIDGE_LE
	IPACM_LanToLan* lan2lan = IPACM_LanToLan::get_instance();
	IPACMDBG_H("Staring IPACM_LanToLan instance %p\n", lan2lan);
#endif

	CtList = new IPACM_ConntrackListener();

	IPACMDBG_H("Staring IPA main\n");
	IPACMDBG_H("ipa_cmdq_successful\n");

	/* reset coalesce settings */
	IPACM_Wan::coalesce_config_reset();

	RegisterForSignals();

	if (IPACM_SUCCESS == cmd_queue_thread)
	{
		ret = pthread_create(&cmd_queue_thread, NULL, MessageQueue::Process, NULL);
		if (IPACM_SUCCESS != ret)
		{
			IPACMERR("unable to command queue thread\n");
			return ret;
		}
		IPACMDBG_H("created command queue thread\n");
		if(pthread_setname_np(cmd_queue_thread, "cmd queue process") != 0)
		{
			IPACMERR("unable to set thread name\n");
		}
	}

	if (IPACM_SUCCESS == netlink_thread)
	{
		ret = pthread_create(&netlink_thread, NULL, netlink_start, NULL);
		if (IPACM_SUCCESS != ret)
		{
			IPACMERR("unable to create netlink thread\n");
			return ret;
		}
		IPACMDBG_H("created netlink thread\n");
		if(pthread_setname_np(netlink_thread, "netlink socket") != 0)
		{
			IPACMERR("unable to set thread name\n");
		}
	}

	/* Enable Firewall support only on MDM targets */
#ifndef FEATURE_IPA_ANDROID
	if (IPACM_SUCCESS == monitor_thread)
	{
		ret = pthread_create(&monitor_thread, NULL, firewall_monitor, NULL);
		if (IPACM_SUCCESS != ret)
		{
			IPACMERR("unable to create monitor thread\n");
			return ret;
		}
		IPACMDBG_H("created firewall monitor thread\n");
		if(pthread_setname_np(monitor_thread, "firewall cfg process") != 0)
		{
			IPACMERR("unable to set thread name\n");
		}
	}
#endif

	if (IPACM_SUCCESS == ipa_driver_thread)
	{
		ret = pthread_create(&ipa_driver_thread, NULL, ipa_driver_msg_notifier, NULL);
		if (IPACM_SUCCESS != ret)
		{
			IPACMERR("unable to create ipa_driver_wlan thread\n");
			return ret;
		}
		IPACMDBG_H("created ipa_driver_wlan thread\n");
		if(pthread_setname_np(ipa_driver_thread, "ipa driver ntfy") != 0)
		{
			IPACMERR("unable to set thread name\n");
		}
	}

	pthread_join(cmd_queue_thread, NULL);
	pthread_join(netlink_thread, NULL);
	pthread_join(monitor_thread, NULL);
	pthread_join(ipa_driver_thread, NULL);
	return IPACM_SUCCESS;
}

/*===========================================================================
		FUNCTION  ipa_is_ipacm_running
===========================================================================*/
/*!
@brief
  Determine whether there's already an IPACM process running, if so, terminate
  the current one

@return
	None

@note

- Dependencies
		- None

- Side Effects
		- None
*/
/*=========================================================================*/

void ipa_is_ipacm_running(void) {

	int fd;
	struct flock lock;
	int retval;

	fd = open(IPACM_PID_FILE, O_RDWR | O_CREAT, 0600);
	if ( fd <= 0 )
	{
		IPACMERR("Failed to open %s, error is %d - %s\n",
				 IPACM_PID_FILE, errno, strerror(errno));
		exit(0);
	}

	/*
	 * Getting an exclusive Write lock on the file, if it fails,
	 * it means that another instance of IPACM is running and it
	 * got the lock before us.
	 */
	memset(&lock, 0, sizeof(lock));
	lock.l_type = F_WRLCK;
	retval = fcntl(fd, F_SETLK, &lock);

	if (retval != 0)
	{
		retval = fcntl(fd, F_GETLK, &lock);
		if (retval == 0)
		{
			IPACMERR("Unable to get lock on file %s (my PID %d), PID %d already has it\n",
					 IPACM_PID_FILE, getpid(), lock.l_pid);
			close(fd);
			exit(0);
		}
	}
	else
	{
		IPACMERR("PID %d is IPACM main process\n", getpid());
	}

	return;
}

/*===========================================================================
		FUNCTION  ipa_get_if_index
===========================================================================*/
/*!
@brief
  get ipa interface index by given the interface name

@return
	IPACM_SUCCESS or IPA_FALUIRE

@note

- Dependencies
		- None

- Side Effects
		- None
*/
/*=========================================================================*/
int ipa_get_if_index
(
	 char *if_name,
	 int *if_index
	 )
{
	int fd;
	struct ifreq ifr;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		PERROR("get interface index socket create failed");
		return IPACM_FAILURE;
	}

	memset(&ifr, 0, sizeof(struct ifreq));

	(void)strlcpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name));

	if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0)
	{
		IPACMERR("call_ioctl_on_dev: ioctl failed: can't find device %s",if_name);
		*if_index = -1;
		close(fd);
		return IPACM_FAILURE;
	}

	*if_index = ifr.ifr_ifindex;
	close(fd);
	return IPACM_SUCCESS;
}

#ifdef FEATURE_IPACM_RESTART
int ipa_reset()
{
	int fd = -1;

	if ((fd = open(IPA_DEVICE_NAME, O_RDWR)) < 0) {
		IPACMERR("Failed opening %s.\n", IPA_DEVICE_NAME);
		return IPACM_FAILURE;
	}

	if (ioctl(fd, IPA_IOC_CLEANUP) < 0) {
		IPACMERR("IOCTL IPA_IOC_CLEANUP call failed: %s \n", strerror(errno));
		close(fd);
		return IPACM_FAILURE;
	}

	IPACMDBG_H("send IPA_IOC_CLEANUP \n");
	close(fd);
	return IPACM_SUCCESS;
}
#endif

#ifdef IPA_IOCTL_SET_FNR_COUNTER_INFO
int ipa_reset_hw_index_counter()
{
	int fd = -1;
	struct ipa_ioc_flt_rt_counter_alloc fnr_counters;
	struct ipa_ioc_fnr_index_info fnr_info;

	if ((fd = open(IPA_DEVICE_NAME, O_RDWR)) < 0) {
		IPACMERR("Failed opening %s.\n", IPA_DEVICE_NAME);
		return IPACM_FAILURE;
	}

	memset(&fnr_counters, 0, sizeof(fnr_counters));
	fnr_counters.hw_counter.num_counters = 4;
	fnr_counters.hw_counter.allow_less = false;
	fnr_counters.sw_counter.num_counters = 4;
	fnr_counters.sw_counter.allow_less = false;
	IPACMDBG_H("Allocating %d hw counters and %d sw counters\n",
		fnr_counters.hw_counter.num_counters, fnr_counters.sw_counter.num_counters);

	if (ioctl(fd, IPA_IOC_FNR_COUNTER_ALLOC, &fnr_counters) < 0) {
		IPACMERR("IPA_IOC_FNR_COUNTER_ALLOC call failed: %s \n", strerror(errno));
		close(fd);
		return IPACM_FAILURE;
	}

	IPACMDBG_H("hw-counter start offset %d, sw-counter start offset %d\n",
		fnr_counters.hw_counter.start_id, fnr_counters.sw_counter.start_id);
	IPACM_Iface::ipacmcfg->hw_fnr_stats_support = true;
	IPACM_Iface::ipacmcfg->hw_counter_offset = fnr_counters.hw_counter.start_id;
	IPACM_Iface::ipacmcfg->sw_counter_offset = fnr_counters.sw_counter.start_id;

	/* set FNR counter info */
	memset(&fnr_info, 0, sizeof(fnr_info));
	fnr_info.hw_counter_offset = fnr_counters.hw_counter.start_id;
	fnr_info.sw_counter_offset = fnr_counters.sw_counter.start_id;

	if (ioctl(fd, IPA_IOC_SET_FNR_COUNTER_INFO, &fnr_info) < 0) {
		IPACMERR("IPA_IOC_SET_FNR_COUNTER_INFO call failed: %s \n", strerror(errno));
		close(fd);
		return IPACM_FAILURE;
	}

	close(fd);
	return IPACM_SUCCESS;
}
#endif
