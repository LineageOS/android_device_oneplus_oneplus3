/*
Copyright (c) 2014-2019, The Linux Foundation. All rights reserved.

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

/*
 * IPACM_LanToLan.h
 *
 *  Created on: Mar 4th, 2014
 *      Author: Shihuan Liu
 */

#ifndef IPACM_LANTOLAN_H
#define IPACM_LANTOLAN_H

#include <stdint.h>
#include "linux/msm_ipa.h"
#include "IPACM_Iface.h"
#include "IPACM_Defs.h"
#include "IPACM_Lan.h"

#ifdef FEATURE_IPA_ANDROID
#include <libxml/list.h>
#endif /* defined(FEATURE_IPA_ANDROID) */
#include <list>

#define MAX_NUM_CACHED_CLIENT_ADD_EVENT 10
#define MAX_NUM_IFACE 10
#define MAX_NUM_CLIENT 16

struct vlan_iface_info
{
	char vlan_iface_name[IPA_RESOURCE_NAME_MAX];
	uint8_t vlan_id;
	uint32_t vlan_iface_ipv6_addr[4];
	uint8_t vlan_client_mac[6];
	uint32_t vlan_client_ipv6_addr[4];
};

struct l2tp_vlan_mapping_info
{
	/* the following are l2tp iface info (name, session id) */
	char l2tp_iface_name[IPA_RESOURCE_NAME_MAX];
	uint8_t l2tp_session_id;
	/* the following are mdm vlan iface info (name, vlan id, ipv6 addr) */
	char vlan_iface_name[IPA_RESOURCE_NAME_MAX];
	uint8_t vlan_id;
	uint32_t vlan_iface_ipv6_addr[4];
	/* the following are MIB3 vlan client info (mac, ipv6 addr) */
	uint8_t vlan_client_mac[6];
	uint32_t vlan_client_ipv6_addr[4];
	/* the following is MIB3 l2tp client info (mac) */
	uint8_t l2tp_client_mac[6];
};

struct rt_rule_info
{
	int num_hdl[IPA_IP_MAX];	/* one client may need more than one routing rules on the same routing table depending on tx_prop */
	uint32_t  rule_hdl[IPA_IP_MAX][MAX_NUM_PROP];
};

struct l2tp_rt_rule_info
{
	uint32_t first_pass_hdr_hdl;	/* first pass hdr template (IPv4 and IPv6 use the same hdr template) */
	uint32_t first_pass_hdr_proc_ctx_hdl[IPA_IP_MAX]; /* first pass hdr proc ctx */
	uint32_t second_pass_hdr_hdl;	/* second pass hdr template (IPv4 and IPv6 use the same hdr template) */
	int num_rt_hdl[IPA_IP_MAX];		/* number of TX properties for IPv4 and IPv6 respectively */
	uint32_t  first_pass_rt_rule_hdl[IPA_IP_MAX][MAX_NUM_PROP];	/* first pass routing rule */
	uint32_t  second_pass_rt_rule_hdl[MAX_NUM_PROP];	/*second pass routing rule (only ipv6 rt rule is needed) */
};

struct client_info
{
	uint8_t mac_addr[6];
	rt_rule_info inter_iface_rt_rule_hdl[IPA_HDR_L2_MAX];	/* routing rule handles of inter interface communication based on source l2 header type */
	rt_rule_info intra_iface_rt_rule_hdl;	/* routing rule handles of inter interface communication */
	bool is_l2tp_client;
	l2tp_vlan_mapping_info *mapping_info;
	l2tp_rt_rule_info l2tp_rt_rule_hdl[IPA_HDR_L2_MAX];
	int ep;
};

struct flt_rule_info
{
	client_info *p_client;
	uint32_t flt_rule_hdl[IPA_IP_MAX];
	uint32_t l2tp_first_pass_flt_rule_hdl[IPA_IP_MAX];	/* L2TP filtering rules are destination MAC based */
	uint32_t l2tp_second_pass_flt_rule_hdl;
};

struct peer_iface_info
{
	class IPACM_LanToLan_Iface *peer;
	char rt_tbl_name_for_rt[IPA_IP_MAX][IPA_RESOURCE_NAME_MAX];
	char rt_tbl_name_for_flt[IPA_IP_MAX][IPA_RESOURCE_NAME_MAX];
	list<flt_rule_info> flt_rule;
};

class IPACM_LanToLan_Iface
{
public:
	IPACM_LanToLan_Iface(IPACM_Lan *p_iface);
	~IPACM_LanToLan_Iface();

	void add_client_rt_rule_for_new_iface();

	void add_all_inter_interface_client_flt_rule(ipa_ip_type iptype);

	void add_all_intra_interface_client_flt_rule(ipa_ip_type iptype);

	void handle_down_event();

	void handle_wlan_scc_mcc_switch();

	void handle_intra_interface_info();

	void handle_new_iface_up(char rt_tbl_name_for_flt[][IPA_RESOURCE_NAME_MAX], char rt_tbl_name_for_rt[][IPA_RESOURCE_NAME_MAX],
		IPACM_LanToLan_Iface *peer_iface);

	void handle_client_add(uint8_t *mac, bool is_l2tp_client, l2tp_vlan_mapping_info *mapping_info, int ep);

	void handle_client_del(uint8_t *mac);

	void print_data_structure_info();

	IPACM_Lan* get_iface_pointer();

	bool get_m_is_ip_addr_assigned(ipa_ip_type iptype);

	void set_m_is_ip_addr_assigned(ipa_ip_type iptype, bool value);

	bool get_m_support_inter_iface_offload();

	bool get_m_support_intra_iface_offload();

	void increment_ref_cnt_peer_l2_hdr_type(ipa_hdr_l2_type peer_l2_type);

	void decrement_ref_cnt_peer_l2_hdr_type(ipa_hdr_l2_type peer_l2_type);
#ifdef FEATURE_L2TP
	void switch_to_l2tp_iface();

	bool set_l2tp_iface(char *vlan_iface_name);

	bool is_l2tp_iface();

	void handle_l2tp_enable();

	void handle_l2tp_disable();
#endif
private:

	IPACM_Lan *m_p_iface;
	bool m_is_ip_addr_assigned[IPA_IP_MAX];
	bool m_support_inter_iface_offload;
	bool m_support_intra_iface_offload;
	bool m_is_l2tp_iface;

	int ref_cnt_peer_l2_hdr_type[IPA_HDR_L2_MAX];	/* reference count of l2 header type of peer interfaces */
	uint32_t hdr_proc_ctx_for_inter_interface[IPA_HDR_L2_MAX];
	uint32_t hdr_proc_ctx_for_intra_interface;
	uint32_t hdr_proc_ctx_for_l2tp;		/* uc needs to remove 62 bytes IPv6 + L2TP + inner Ethernet header */

	list<client_info> m_client_info;	/* client list */
	list<peer_iface_info> m_peer_iface_info;	/* peer information list */

	/* The following members are for intra-interface communication*/
	peer_iface_info m_intra_interface_info;

	void add_one_client_flt_rule(IPACM_LanToLan_Iface *peer_iface, client_info *client);

	void add_client_flt_rule(peer_iface_info *peer, client_info *client, ipa_ip_type iptype);

	void del_one_client_flt_rule(IPACM_LanToLan_Iface *peer_iface, client_info *client);

	void del_client_flt_rule(peer_iface_info *peer, client_info *client);

	void add_client_rt_rule(peer_iface_info *peer, client_info *client);

	void del_client_rt_rule(peer_iface_info *peer, client_info *client);

	void add_l2tp_client_rt_rule(peer_iface_info *peer, client_info *client);

	void clear_all_flt_rule_for_one_peer_iface(peer_iface_info *peer);

	void clear_all_rt_rule_for_one_peer_iface(peer_iface_info *peer);

	void add_hdr_proc_ctx(ipa_hdr_l2_type peer_l2_type);

	void del_hdr_proc_ctx(ipa_hdr_l2_type peer_l2_type);

	void print_peer_info(peer_iface_info *peer_info);

};

class IPACM_LanToLan : public IPACM_Listener
{

public:

	static IPACM_LanToLan* p_instance;
	static IPACM_LanToLan* get_instance();
#ifdef FEATURE_L2TP
	bool has_l2tp_iface();
#endif

private:

	IPACM_LanToLan();

	~IPACM_LanToLan();

	bool m_has_l2tp_iface;

	list<class IPACM_LanToLan_Iface> m_iface;

	list<ipacm_event_eth_bridge> m_cached_client_add_event;

	list<vlan_iface_info> m_vlan_iface;

	list<l2tp_vlan_mapping_info> m_l2tp_vlan_mapping;

	void handle_iface_up(ipacm_event_eth_bridge *data);

	void handle_iface_down(ipacm_event_eth_bridge *data);

	void handle_client_add(ipacm_event_eth_bridge *data);

	void handle_client_del(ipacm_event_eth_bridge *data);

	void handle_wlan_scc_mcc_switch(ipacm_event_eth_bridge *data);

#ifdef FEATURE_L2TP
	void handle_add_vlan_iface(ipa_ioc_vlan_iface_info *data);

	void handle_del_vlan_iface(ipa_ioc_vlan_iface_info *data);

	void handle_add_l2tp_vlan_mapping(ipa_ioc_l2tp_vlan_mapping_info *data);

	void handle_del_l2tp_vlan_mapping(ipa_ioc_l2tp_vlan_mapping_info *data);

	void handle_vlan_client_info(ipacm_event_data_all *data);

	void handle_vlan_iface_info(ipacm_event_data_all *data);
#endif

	void handle_new_iface_up(IPACM_LanToLan_Iface *new_iface, IPACM_LanToLan_Iface *exist_iface);

	void event_callback(ipa_cm_event_id event, void* param);

	void handle_cached_client_add_event(IPACM_Lan *p_iface);

	void clear_cached_client_add_event(IPACM_Lan *p_iface);

	void print_data_structure_info();

};

#endif
