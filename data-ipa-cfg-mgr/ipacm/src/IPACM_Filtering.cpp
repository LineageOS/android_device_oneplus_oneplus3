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
	IPACM_Filtering.cpp

	@brief
	This file implements the IPACM filtering functionality.

	@Author
	Skylar Chang

*/
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "IPACM_Filtering.h"
#include <IPACM_Log.h>
#include "IPACM_Defs.h"


const char *IPACM_Filtering::DEVICE_NAME = "/dev/ipa";

IPACM_Filtering::IPACM_Filtering()
{
	fd = open(DEVICE_NAME, O_RDWR);
	if (fd < 0)
	{
		IPACMERR("Failed opening %s.\n", DEVICE_NAME);
	}
	total_num_offload_rules = 0;
	pcie_modem_rule_id = 0;
}

IPACM_Filtering::~IPACM_Filtering()
{
	close(fd);
}

bool IPACM_Filtering::DeviceNodeIsOpened()
{
	return fd;
}

bool IPACM_Filtering::AddFilteringRule(struct ipa_ioc_add_flt_rule const *ruleTable)
{
	int retval = 0;

	IPACMDBG("Printing filter add attributes\n");
	IPACMDBG("ip type: %d\n", ruleTable->ip);
	IPACMDBG("Number of rules: %d\n", ruleTable->num_rules);
	IPACMDBG("End point: %d and global value: %d\n", ruleTable->ep, ruleTable->global);
	IPACMDBG("commit value: %d\n", ruleTable->commit);
	for (int cnt=0; cnt<ruleTable->num_rules; cnt++)
	{
		IPACMDBG("Filter rule:%d attrib mask: 0x%x\n", cnt,
				ruleTable->rules[cnt].rule.attrib.attrib_mask);
	}

	retval = ioctl(fd, IPA_IOC_ADD_FLT_RULE, ruleTable);
	if (retval != 0)
	{
		IPACMERR("Failed adding Filtering rule %pK\n", ruleTable);
		PERROR("unable to add filter rule:");

		for (int cnt = 0; cnt < ruleTable->num_rules; cnt++)
		{
			if (ruleTable->rules[cnt].status != 0)
			{
				IPACMERR("Adding Filter rule:%d failed with status:%d\n",
								 cnt, ruleTable->rules[cnt].status);
			}
		}
		return false;
	}

	for (int cnt = 0; cnt<ruleTable->num_rules; cnt++)
	{
		if(ruleTable->rules[cnt].status != 0)
		{
			IPACMERR("Adding Filter rule:%d failed with status:%d\n",
							 cnt, ruleTable->rules[cnt].status);
		}
	}

	IPACMDBG("Added Filtering rule %pK\n", ruleTable);
	return true;
}

#ifdef IPA_IOCTL_SET_FNR_COUNTER_INFO
bool IPACM_Filtering::AddFilteringRule_hw_index(struct ipa_ioc_add_flt_rule *ruleTable, int hw_counter_index)
{
	int retval=0, cnt = 0, len = 0;
	struct ipa_ioc_add_flt_rule_v2 *ruleTable_v2;
	struct ipa_flt_rule_add_v2 flt_rule_entry;
	bool ret = true;

	IPACMDBG("Printing filter add attributes\n");
	IPACMDBG("ip type: %d\n", ruleTable->ip);
	IPACMDBG("Number of rules: %d\n", ruleTable->num_rules);
	IPACMDBG("End point: %d and global value: %d\n", ruleTable->ep, ruleTable->global);
	IPACMDBG("commit value: %d\n", ruleTable->commit);

	/* change to v2 format*/
	len = sizeof(struct ipa_ioc_add_flt_rule_v2);
	ruleTable_v2 = (struct ipa_ioc_add_flt_rule_v2*)malloc(len);
	if (ruleTable_v2 == NULL)
	{
		IPACMERR("Error Locate ipa_ioc_add_flt_rule_v2 memory...\n");
		return false;
	}
	memset(ruleTable_v2, 0, len);
	ruleTable_v2->rules = (uint64_t)calloc(ruleTable->num_rules, sizeof(struct ipa_flt_rule_add_v2));
	if (!ruleTable_v2->rules) {
		IPACMERR("Failed to allocate memory for filtering rules\n");
		ret = false;
		goto fail_tbl;
	}

	ruleTable_v2->commit = ruleTable->commit;
	ruleTable_v2->ep = ruleTable->ep;
	ruleTable_v2->global = ruleTable->global;
	ruleTable_v2->ip = ruleTable->ip;
	ruleTable_v2->num_rules = ruleTable->num_rules;
	ruleTable_v2->flt_rule_size = sizeof(struct ipa_flt_rule_add_v2);

	for (cnt=0; cnt < ruleTable->num_rules; cnt++)
	{
		memset(&flt_rule_entry, 0, sizeof(struct ipa_flt_rule_add_v2));
		flt_rule_entry.at_rear = ruleTable->rules[cnt].at_rear;
		flt_rule_entry.rule.retain_hdr = ruleTable->rules[cnt].rule.retain_hdr;
		flt_rule_entry.rule.to_uc = ruleTable->rules[cnt].rule.to_uc;
		flt_rule_entry.rule.action = ruleTable->rules[cnt].rule.action;
		flt_rule_entry.rule.rt_tbl_hdl = ruleTable->rules[cnt].rule.rt_tbl_hdl;
		flt_rule_entry.rule.rt_tbl_idx = ruleTable->rules[cnt].rule.rt_tbl_idx;
		flt_rule_entry.rule.eq_attrib_type = ruleTable->rules[cnt].rule.eq_attrib_type;
		flt_rule_entry.rule.max_prio = ruleTable->rules[cnt].rule.max_prio;
		flt_rule_entry.rule.hashable = ruleTable->rules[cnt].rule.hashable;
		flt_rule_entry.rule.rule_id = ruleTable->rules[cnt].rule.rule_id;
		flt_rule_entry.rule.set_metadata = ruleTable->rules[cnt].rule.set_metadata;
		flt_rule_entry.rule.pdn_idx = ruleTable->rules[cnt].rule.pdn_idx;
		memcpy(&flt_rule_entry.rule.eq_attrib,
					 &ruleTable->rules[cnt].rule.eq_attrib,
					 sizeof(flt_rule_entry.rule.eq_attrib));
		memcpy(&flt_rule_entry.rule.attrib,
					 &ruleTable->rules[cnt].rule.attrib,
					 sizeof(flt_rule_entry.rule.attrib));
		IPACMDBG("Filter rule:%d attrib mask: 0x%x\n", cnt,
				ruleTable->rules[cnt].rule.attrib.attrib_mask);
		/* 0 means disable hw-counter-sats */
		if (hw_counter_index != 0)
		{
			flt_rule_entry.rule.enable_stats = 1;
			flt_rule_entry.rule.cnt_idx = hw_counter_index;
		}

		/* copy to v2 table*/
		memcpy((void *)(ruleTable_v2->rules + (cnt * sizeof(struct ipa_flt_rule_add_v2))),
			&flt_rule_entry, sizeof(flt_rule_entry));
	}

	retval = ioctl(fd, IPA_IOC_ADD_FLT_RULE_V2, ruleTable_v2);
	if (retval != 0)
	{
		IPACMERR("Failed adding Filtering rule %pK\n", ruleTable_v2);
		PERROR("unable to add filter rule:");

		for (int cnt = 0; cnt < ruleTable_v2->num_rules; cnt++)
		{
			if (((struct ipa_flt_rule_add_v2 *)ruleTable_v2->rules)[cnt].status != 0)
			{
				IPACMERR("Adding Filter rule:%d failed with status:%d\n",
								 cnt, ((struct ipa_flt_rule_add_v2 *)ruleTable_v2->rules)[cnt].status);
			}
		}
		ret = false;
		goto fail_rule;
	}

	/* copy results from v2 to v1 format */
	for (int cnt = 0; cnt < ruleTable->num_rules; cnt++)
	{
		/* copy status to v1 format */
		ruleTable->rules[cnt].status = ((struct ipa_flt_rule_add_v2 *)ruleTable_v2->rules)[cnt].status;
		ruleTable->rules[cnt].flt_rule_hdl = ((struct ipa_flt_rule_add_v2 *)ruleTable_v2->rules)[cnt].flt_rule_hdl;

		if(((struct ipa_flt_rule_add_v2 *)ruleTable_v2->rules)[cnt].status != 0)
		{
			IPACMERR("Adding Filter rule:%d failed with status:%d\n",
							 cnt, ((struct ipa_flt_rule_add_v2 *) ruleTable_v2->rules)[cnt].status);
		}
	}

	IPACMDBG("Added Filtering rule %pK\n", ruleTable_v2);

fail_rule:
	if((void *)ruleTable_v2->rules != NULL)
		free((void *)ruleTable_v2->rules);
fail_tbl:
	if (ruleTable_v2 != NULL)
		free(ruleTable_v2);
	return ret;
}

bool IPACM_Filtering::AddFilteringRuleAfter_hw_index(struct ipa_ioc_add_flt_rule_after *ruleTable, int hw_counter_index)
{
	bool ret = true;
#ifdef FEATURE_IPA_V3
	int retval=0, cnt = 0, len = 0;
	struct ipa_ioc_add_flt_rule_after_v2 *ruleTable_v2;
	struct ipa_flt_rule_add_v2 flt_rule_entry;

	IPACMDBG("Printing filter add attributes\n");
	IPACMDBG("ep: %d\n", ruleTable->ep);
	IPACMDBG("ip type: %d\n", ruleTable->ip);
	IPACMDBG("Number of rules: %d\n", ruleTable->num_rules);
	IPACMDBG("add_after_hdl: %d\n", ruleTable->add_after_hdl);
	IPACMDBG("commit value: %d\n", ruleTable->commit);

	/* change to v2 format*/
	len = sizeof(struct ipa_ioc_add_flt_rule_after_v2);
	ruleTable_v2 = (struct ipa_ioc_add_flt_rule_after_v2*)malloc(len);
	if (ruleTable_v2 == NULL)
	{
		IPACMERR("Error Locate ipa_ioc_add_flt_rule_after_v2 memory...\n");
		return false;
	}
	memset(ruleTable_v2, 0, len);
	ruleTable_v2->rules = (uint64_t)calloc(ruleTable->num_rules, sizeof(struct ipa_flt_rule_add_v2));
	if (!ruleTable_v2->rules) {
		IPACMERR("Failed to allocate memory for filtering rules\n");
		ret = false;
		goto fail_tbl;
	}

	ruleTable_v2->commit = ruleTable->commit;
	ruleTable_v2->ep = ruleTable->ep;
	ruleTable_v2->ip = ruleTable->ip;
	ruleTable_v2->num_rules = ruleTable->num_rules;
	ruleTable_v2->add_after_hdl = ruleTable->add_after_hdl;
	ruleTable_v2->flt_rule_size = sizeof(struct ipa_flt_rule_add_v2);

	for (cnt=0; cnt < ruleTable->num_rules; cnt++)
	{
		memset(&flt_rule_entry, 0, sizeof(struct ipa_flt_rule_add_v2));
		flt_rule_entry.at_rear = ruleTable->rules[cnt].at_rear;
		flt_rule_entry.rule.retain_hdr = ruleTable->rules[cnt].rule.retain_hdr;
		flt_rule_entry.rule.to_uc = ruleTable->rules[cnt].rule.to_uc;
		flt_rule_entry.rule.action = ruleTable->rules[cnt].rule.action;
		flt_rule_entry.rule.rt_tbl_hdl = ruleTable->rules[cnt].rule.rt_tbl_hdl;
		flt_rule_entry.rule.rt_tbl_idx = ruleTable->rules[cnt].rule.rt_tbl_idx;
		flt_rule_entry.rule.eq_attrib_type = ruleTable->rules[cnt].rule.eq_attrib_type;
		flt_rule_entry.rule.max_prio = ruleTable->rules[cnt].rule.max_prio;
		flt_rule_entry.rule.hashable = ruleTable->rules[cnt].rule.hashable;
		flt_rule_entry.rule.rule_id = ruleTable->rules[cnt].rule.rule_id;
		flt_rule_entry.rule.set_metadata = ruleTable->rules[cnt].rule.set_metadata;
		flt_rule_entry.rule.pdn_idx = ruleTable->rules[cnt].rule.pdn_idx;
		memcpy(&flt_rule_entry.rule.eq_attrib,
					 &ruleTable->rules[cnt].rule.eq_attrib,
					 sizeof(flt_rule_entry.rule.eq_attrib));
		memcpy(&flt_rule_entry.rule.attrib,
					 &ruleTable->rules[cnt].rule.attrib,
					 sizeof(flt_rule_entry.rule.attrib));
		IPACMDBG("Filter rule:%d attrib mask: 0x%x\n", cnt,
				ruleTable->rules[cnt].rule.attrib.attrib_mask);
		/* 0 means disable hw-counter-sats */
		if (hw_counter_index != 0)
		{
			flt_rule_entry.rule.enable_stats = 1;
			flt_rule_entry.rule.cnt_idx = hw_counter_index;
		}

		/* copy to v2 table*/
		memcpy((void *)(ruleTable_v2->rules + (cnt * sizeof(struct ipa_flt_rule_add_v2))),
			&flt_rule_entry, sizeof(flt_rule_entry));
	}

	retval = ioctl(fd, IPA_IOC_ADD_FLT_RULE_AFTER_V2, ruleTable_v2);
	if (retval != 0)
	{
		IPACMERR("Failed adding Filtering rule %pK\n", ruleTable_v2);
		PERROR("unable to add filter rule:");

		for (int cnt = 0; cnt < ruleTable_v2->num_rules; cnt++)
		{
			if (((struct ipa_flt_rule_add_v2 *)ruleTable_v2->rules)[cnt].status != 0)
			{
				IPACMERR("Adding Filter rule:%d failed with status:%d\n",
								 cnt, ((struct ipa_flt_rule_add_v2 *)ruleTable_v2->rules)[cnt].status);
			}
		}
		ret = false;
		goto fail_rule;
	}

	/* copy results from v2 to v1 format */
	for (int cnt = 0; cnt < ruleTable->num_rules; cnt++)
	{
		/* copy status to v1 format */
		ruleTable->rules[cnt].status = ((struct ipa_flt_rule_add_v2 *)ruleTable_v2->rules)[cnt].status;
		ruleTable->rules[cnt].flt_rule_hdl = ((struct ipa_flt_rule_add_v2 *)ruleTable_v2->rules)[cnt].flt_rule_hdl;

		if(((struct ipa_flt_rule_add_v2 *)ruleTable_v2->rules)[cnt].status != 0)
		{
			IPACMERR("Adding Filter rule:%d failed with status:%d\n",
							 cnt, ((struct ipa_flt_rule_add_v2 *) ruleTable_v2->rules)[cnt].status);
		}
	}

	IPACMDBG("Added Filtering rule %pK\n", ruleTable_v2);

fail_rule:
	if((void *)ruleTable_v2->rules != NULL)
		free((void *)ruleTable_v2->rules);
fail_tbl:
	if (ruleTable_v2 != NULL)
		free(ruleTable_v2);
#else
	if (ruleTable)
	IPACMERR("Not support adding Filtering rule %pK\n", ruleTable);
#endif
	return ret;
}
#endif //IPA_IOCTL_SET_FNR_COUNTER_INFO

bool IPACM_Filtering::AddFilteringRuleAfter(struct ipa_ioc_add_flt_rule_after const *ruleTable)
{
#ifdef FEATURE_IPA_V3
	int retval = 0;

	IPACMDBG("Printing filter add attributes\n");
	IPACMDBG("ip type: %d\n", ruleTable->ip);
	IPACMDBG("Number of rules: %d\n", ruleTable->num_rules);
	IPACMDBG("End point: %d\n", ruleTable->ep);
	IPACMDBG("commit value: %d\n", ruleTable->commit);

	retval = ioctl(fd, IPA_IOC_ADD_FLT_RULE_AFTER, ruleTable);

	for (int cnt = 0; cnt<ruleTable->num_rules; cnt++)
	{
		if(ruleTable->rules[cnt].status != 0)
		{
			IPACMERR("Adding Filter rule:%d failed with status:%d\n",
							 cnt, ruleTable->rules[cnt].status);
		}
	}

	if (retval != 0)
	{
		IPACMERR("Failed adding Filtering rule %pK\n", ruleTable);
		return false;
	}
	IPACMDBG("Added Filtering rule %pK\n", ruleTable);
#else
	if (ruleTable)
	IPACMERR("Not support adding Filtering rule %pK\n", ruleTable);
#endif
	return true;
}

bool IPACM_Filtering::DeleteFilteringRule(struct ipa_ioc_del_flt_rule *ruleTable)
{
	int retval = 0;

	retval = ioctl(fd, IPA_IOC_DEL_FLT_RULE, ruleTable);
	if (retval != 0)
	{
		IPACMERR("Failed deleting Filtering rule %pK\n", ruleTable);
		return false;
	}

	IPACMDBG("Deleted Filtering rule %pK\n", ruleTable);
	return true;
}

bool IPACM_Filtering::Commit(enum ipa_ip_type ip)
{
	int retval = 0;

	retval = ioctl(fd, IPA_IOC_COMMIT_FLT, ip);
	if (retval != 0)
	{
		IPACMERR("failed committing Filtering rules.\n");
		return false;
	}

	IPACMDBG("Committed Filtering rules to IPA HW.\n");
	return true;
}

bool IPACM_Filtering::Reset(enum ipa_ip_type ip)
{
	int retval = 0;

	retval = ioctl(fd, IPA_IOC_RESET_FLT, ip);
	retval |= ioctl(fd, IPA_IOC_COMMIT_FLT, ip);
	if (retval)
	{
		IPACMERR("failed resetting Filtering block.\n");
		return false;
	}

	IPACMDBG("Reset command issued to IPA Filtering block.\n");
	return true;
}

bool IPACM_Filtering::DeleteFilteringHdls
(
	 uint32_t *flt_rule_hdls,
	 ipa_ip_type ip,
	 uint8_t num_rules
)
{
	struct ipa_ioc_del_flt_rule *flt_rule;
	bool res = true;
	int len = 0, cnt = 0;
        const uint8_t UNIT_RULES = 1;

	len = (sizeof(struct ipa_ioc_del_flt_rule)) + (UNIT_RULES * sizeof(struct ipa_flt_rule_del));
	flt_rule = (struct ipa_ioc_del_flt_rule *)malloc(len);
	if (flt_rule == NULL)
	{
		IPACMERR("unable to allocate memory for del filter rule\n");
		return false;
	}

	for (cnt = 0; cnt < num_rules; cnt++)
	{
	    memset(flt_rule, 0, len);
	    flt_rule->commit = 1;
	    flt_rule->num_hdls = UNIT_RULES;
	    flt_rule->ip = ip;

	    if (flt_rule_hdls[cnt] == 0)
	    {
		   IPACMERR("invalid filter handle passed, ignoring it: %d\n", cnt)
	    }
            else
	    {

		   flt_rule->hdl[0].status = -1;
		   flt_rule->hdl[0].hdl = flt_rule_hdls[cnt];
		   IPACMDBG("Deleting filter hdl:(0x%x) with ip type: %d\n", flt_rule_hdls[cnt], ip);

	           if (DeleteFilteringRule(flt_rule) == false)
	           {
		        PERROR("Filter rule deletion failed!\n");
		        res = false;
		        goto fail;
	           }
		   else
	           {

		        if (flt_rule->hdl[0].status != 0)
		        {
			     IPACMERR("Filter rule hdl 0x%x deletion failed with error:%d\n",
		        					 flt_rule->hdl[0].hdl, flt_rule->hdl[0].status);
			     res = false;
			     goto fail;
		        }
		   }
	    }
	}

fail:
	free(flt_rule);

	return res;
}

bool IPACM_Filtering::AddWanDLFilteringRule(struct ipa_ioc_add_flt_rule const *rule_table_v4, struct ipa_ioc_add_flt_rule const * rule_table_v6, uint8_t mux_id)
{
	int ret = 0, cnt, num_rules = 0, pos = 0;
	ipa_install_fltr_rule_req_msg_v01 qmi_rule_msg;
#ifdef FEATURE_IPA_V3
	ipa_install_fltr_rule_req_ex_msg_v01 qmi_rule_ex_msg;
#endif

	memset(&qmi_rule_msg, 0, sizeof(qmi_rule_msg));
	int fd_wwan_ioctl = open(WWAN_QMI_IOCTL_DEVICE_NAME, O_RDWR);
	if(fd_wwan_ioctl < 0)
	{
		IPACMERR("Failed to open %s.\n",WWAN_QMI_IOCTL_DEVICE_NAME);
		return false;
	}

	if(rule_table_v4 != NULL)
	{
		num_rules += rule_table_v4->num_rules;
		IPACMDBG_H("Get %d WAN DL IPv4 filtering rules.\n", rule_table_v4->num_rules);
	}
	if(rule_table_v6 != NULL)
	{
		num_rules += rule_table_v6->num_rules;
		IPACMDBG_H("Get %d WAN DL IPv6 filtering rules.\n", rule_table_v6->num_rules);
	}

	/* if it is not IPA v3, use old QMI format */
#ifndef FEATURE_IPA_V3
	if(num_rules > QMI_IPA_MAX_FILTERS_V01)
	{
		IPACMERR("The number of filtering rules exceed limit.\n");
		close(fd_wwan_ioctl);
		return false;
	}
	else
	{
		if (num_rules > 0)
		{
			qmi_rule_msg.filter_spec_list_valid = true;
		}
		else
		{
			qmi_rule_msg.filter_spec_list_valid = false;
		}

		qmi_rule_msg.filter_spec_list_len = num_rules;
		qmi_rule_msg.source_pipe_index_valid = 0;

		IPACMDBG_H("Get %d WAN DL filtering rules in total.\n", num_rules);

		if(rule_table_v4 != NULL)
		{
			for(cnt = rule_table_v4->num_rules - 1; cnt >= 0; cnt--)
			{
				if (pos < QMI_IPA_MAX_FILTERS_V01)
				{
					qmi_rule_msg.filter_spec_list[pos].filter_spec_identifier = pos;
					qmi_rule_msg.filter_spec_list[pos].ip_type = QMI_IPA_IP_TYPE_V4_V01;
					qmi_rule_msg.filter_spec_list[pos].filter_action = GetQmiFilterAction(rule_table_v4->rules[cnt].rule.action);
					qmi_rule_msg.filter_spec_list[pos].is_routing_table_index_valid = 1;
					qmi_rule_msg.filter_spec_list[pos].route_table_index = rule_table_v4->rules[cnt].rule.rt_tbl_idx;
					qmi_rule_msg.filter_spec_list[pos].is_mux_id_valid = 1;
					qmi_rule_msg.filter_spec_list[pos].mux_id = mux_id;
					memcpy(&qmi_rule_msg.filter_spec_list[pos].filter_rule,
						&rule_table_v4->rules[cnt].rule.eq_attrib,
						sizeof(struct ipa_filter_rule_type_v01));
					pos++;
				}
				else
				{
					IPACMERR(" QMI only support max %d rules, current (%d)\n ",QMI_IPA_MAX_FILTERS_V01, pos);
				}
			}
		}

		if(rule_table_v6 != NULL)
		{
			for(cnt = rule_table_v6->num_rules - 1; cnt >= 0; cnt--)
			{
				if (pos < QMI_IPA_MAX_FILTERS_V01)
				{
					qmi_rule_msg.filter_spec_list[pos].filter_spec_identifier = pos;
					qmi_rule_msg.filter_spec_list[pos].ip_type = QMI_IPA_IP_TYPE_V6_V01;
					qmi_rule_msg.filter_spec_list[pos].filter_action = GetQmiFilterAction(rule_table_v6->rules[cnt].rule.action);
					qmi_rule_msg.filter_spec_list[pos].is_routing_table_index_valid = 1;
					qmi_rule_msg.filter_spec_list[pos].route_table_index = rule_table_v6->rules[cnt].rule.rt_tbl_idx;
					qmi_rule_msg.filter_spec_list[pos].is_mux_id_valid = 1;
					qmi_rule_msg.filter_spec_list[pos].mux_id = mux_id;
					memcpy(&qmi_rule_msg.filter_spec_list[pos].filter_rule,
						&rule_table_v6->rules[cnt].rule.eq_attrib,
						sizeof(struct ipa_filter_rule_type_v01));
					pos++;
				}
				else
				{
					IPACMERR(" QMI only support max %d rules, current (%d)\n ",QMI_IPA_MAX_FILTERS_V01, pos);
				}
			}
		}

		ret = ioctl(fd_wwan_ioctl, WAN_IOC_ADD_FLT_RULE, &qmi_rule_msg);
		if (ret != 0)
		{
			IPACMERR("Failed adding Filtering rule %p with ret %d\n ", &qmi_rule_msg, ret);
			close(fd_wwan_ioctl);
			return false;
		}
	}
	/* if it is IPA v3, use new QMI format */
#else
	if(num_rules > QMI_IPA_MAX_FILTERS_EX_V01)
	{
		IPACMERR("The number of filtering rules exceed limit.\n");
		close(fd_wwan_ioctl);
		return false;
	}
	else
	{
		memset(&qmi_rule_ex_msg, 0, sizeof(qmi_rule_ex_msg));

		if (num_rules > 0)
		{
			qmi_rule_ex_msg.filter_spec_ex_list_valid = true;
		}
		else
		{
			qmi_rule_ex_msg.filter_spec_ex_list_valid = false;
		}
		qmi_rule_ex_msg.filter_spec_ex_list_len = num_rules;
		qmi_rule_ex_msg.source_pipe_index_valid = 0;

		IPACMDBG_H("Get %d WAN DL filtering rules in total.\n", num_rules);

		if(rule_table_v4 != NULL)
		{
			for(cnt = rule_table_v4->num_rules - 1; cnt >= 0; cnt--)
			{
				if (pos < QMI_IPA_MAX_FILTERS_EX_V01)
				{
					qmi_rule_ex_msg.filter_spec_ex_list[pos].ip_type = QMI_IPA_IP_TYPE_V4_V01;
					qmi_rule_ex_msg.filter_spec_ex_list[pos].filter_action = GetQmiFilterAction(rule_table_v4->rules[cnt].rule.action);
					qmi_rule_ex_msg.filter_spec_ex_list[pos].is_routing_table_index_valid = 1;
					qmi_rule_ex_msg.filter_spec_ex_list[pos].route_table_index = rule_table_v4->rules[cnt].rule.rt_tbl_idx;
					qmi_rule_ex_msg.filter_spec_ex_list[pos].is_mux_id_valid = 1;
					qmi_rule_ex_msg.filter_spec_ex_list[pos].mux_id = mux_id;
					qmi_rule_ex_msg.filter_spec_ex_list[pos].rule_id = rule_table_v4->rules[cnt].rule.rule_id;
					qmi_rule_ex_msg.filter_spec_ex_list[pos].is_rule_hashable = rule_table_v4->rules[cnt].rule.hashable;
					memcpy(&qmi_rule_ex_msg.filter_spec_ex_list[pos].filter_rule,
						&rule_table_v4->rules[cnt].rule.eq_attrib,
						sizeof(struct ipa_filter_rule_type_v01));

					pos++;
				}
				else
				{
					IPACMERR(" QMI only support max %d rules, current (%d)\n ",QMI_IPA_MAX_FILTERS_EX_V01, pos);
				}
			}
		}

		if(rule_table_v6 != NULL)
		{
			for(cnt = rule_table_v6->num_rules - 1; cnt >= 0; cnt--)
			{
				if (pos < QMI_IPA_MAX_FILTERS_EX_V01)
				{
					qmi_rule_ex_msg.filter_spec_ex_list[pos].ip_type = QMI_IPA_IP_TYPE_V6_V01;
					qmi_rule_ex_msg.filter_spec_ex_list[pos].filter_action = GetQmiFilterAction(rule_table_v6->rules[cnt].rule.action);
					qmi_rule_ex_msg.filter_spec_ex_list[pos].is_routing_table_index_valid = 1;
					qmi_rule_ex_msg.filter_spec_ex_list[pos].route_table_index = rule_table_v6->rules[cnt].rule.rt_tbl_idx;
					qmi_rule_ex_msg.filter_spec_ex_list[pos].is_mux_id_valid = 1;
					qmi_rule_ex_msg.filter_spec_ex_list[pos].mux_id = mux_id;
					qmi_rule_ex_msg.filter_spec_ex_list[pos].rule_id = rule_table_v6->rules[cnt].rule.rule_id;
					qmi_rule_ex_msg.filter_spec_ex_list[pos].is_rule_hashable = rule_table_v6->rules[cnt].rule.hashable;
					memcpy(&qmi_rule_ex_msg.filter_spec_ex_list[pos].filter_rule,
						&rule_table_v6->rules[cnt].rule.eq_attrib,
						sizeof(struct ipa_filter_rule_type_v01));

					pos++;
				}
				else
				{
					IPACMERR(" QMI only support max %d rules, current (%d)\n ",QMI_IPA_MAX_FILTERS_EX_V01, pos);
				}
			}
		}

		ret = ioctl(fd_wwan_ioctl, WAN_IOC_ADD_FLT_RULE_EX, &qmi_rule_ex_msg);
		if (ret != 0)
		{
			IPACMERR("Failed adding Filtering rule %pK with ret %d\n ", &qmi_rule_ex_msg, ret);
			close(fd_wwan_ioctl);
			return false;
		}
	}
#endif

	close(fd_wwan_ioctl);
	return true;
}

bool IPACM_Filtering::AddOffloadFilteringRule(struct ipa_ioc_add_flt_rule *flt_rule_tbl, uint8_t mux_id, uint8_t default_path)
{
#ifdef WAN_IOCTL_ADD_OFFLOAD_CONNECTION
	int ret = 0, cnt, pos = 0;
	ipa_add_offload_connection_req_msg_v01 qmi_add_msg;
	int fd_wwan_ioctl = open(WWAN_QMI_IOCTL_DEVICE_NAME, O_RDWR);
	if(fd_wwan_ioctl < 0)
	{
		IPACMERR("Failed to open %s.\n",WWAN_QMI_IOCTL_DEVICE_NAME);
		return false;
	}

	if(flt_rule_tbl == NULL)
	{
		if(mux_id ==0)
		{
			IPACMERR("Invalid add_offload_req muxd: (%d)\n", mux_id);
			close(fd_wwan_ioctl);
			return false;
		}
#ifdef QMI_IPA_MAX_FILTERS_EX2_V01
		/* used for sending mux_id info to modem for UL sky*/
		IPACMDBG_H("sending mux_id info (%d) to modem for UL\n", mux_id);
		memset(&qmi_add_msg, 0, sizeof(qmi_add_msg));
		qmi_add_msg.embedded_call_mux_id_valid = true;
		qmi_add_msg.embedded_call_mux_id = mux_id;
		ret = ioctl(fd_wwan_ioctl, WAN_IOC_ADD_OFFLOAD_CONNECTION, &qmi_add_msg);
		if (ret != 0)
		{
			IPACMERR("Failed sending WAN_IOC_ADD_OFFLOAD_CONNECTION with ret %d\n ", ret);
			close(fd_wwan_ioctl);
			return false;
		}
#endif
		close(fd_wwan_ioctl);
		return true;
	}
	/* check Max offload connections */
	if (total_num_offload_rules + flt_rule_tbl->num_rules > QMI_IPA_MAX_FILTERS_V01)
	{
		IPACMERR("(%d) add_offload req with curent(%d), exceed max (%d).\n",
		flt_rule_tbl->num_rules, total_num_offload_rules,
		QMI_IPA_MAX_FILTERS_V01);
		close(fd_wwan_ioctl);
		return false;
	}
	else
	{
		memset(&qmi_add_msg, 0, sizeof(qmi_add_msg));

		if (flt_rule_tbl->num_rules > 0)
		{
			qmi_add_msg.filter_spec_ex2_list_valid = true;
		}
		else
		{
			IPACMDBG_H("Get %d offload-req\n", flt_rule_tbl->num_rules);
			close(fd_wwan_ioctl);
			return true;
		}
		qmi_add_msg.filter_spec_ex2_list_len = flt_rule_tbl->num_rules;

		/* check if we want to take default MHI path */
		if (default_path)
		{
			qmi_add_msg.default_mhi_path_valid = true;
			qmi_add_msg.default_mhi_path = true;
		}

		IPACMDBG_H("passing %d offload req to modem. default %d\n", flt_rule_tbl->num_rules, qmi_add_msg.default_mhi_path);

		if(flt_rule_tbl != NULL)
		{
			for(cnt = flt_rule_tbl->num_rules - 1; cnt >= 0; cnt--)
			{
				if (pos < QMI_IPA_MAX_FILTERS_V01)
				{
					if (flt_rule_tbl->ip == IPA_IP_v4)
					{
						qmi_add_msg.filter_spec_ex2_list[pos].ip_type = QMI_IPA_IP_TYPE_V4_V01;
					} else if (flt_rule_tbl->ip == IPA_IP_v6) {
						qmi_add_msg.filter_spec_ex2_list[pos].ip_type = QMI_IPA_IP_TYPE_V6_V01;
					} else {
						IPACMDBG_H("invalid ip-type %d\n", flt_rule_tbl->ip);
						close(fd_wwan_ioctl);
						return true;
					}

					qmi_add_msg.filter_spec_ex2_list[pos].filter_action = GetQmiFilterAction(flt_rule_tbl->rules[cnt].rule.action);
					qmi_add_msg.filter_spec_ex2_list[pos].is_mux_id_valid = 1;
					qmi_add_msg.filter_spec_ex2_list[pos].mux_id = mux_id;
					/* assign the rule-id */
					flt_rule_tbl->rules[cnt].flt_rule_hdl = IPA_PCIE_MODEM_RULE_ID_START + pcie_modem_rule_id;
					qmi_add_msg.filter_spec_ex2_list[pos].rule_id = flt_rule_tbl->rules[cnt].flt_rule_hdl;
					qmi_add_msg.filter_spec_ex2_list[pos].is_rule_hashable = flt_rule_tbl->rules[cnt].rule.hashable;
					memcpy(&qmi_add_msg.filter_spec_ex2_list[pos].filter_rule,
						&flt_rule_tbl->rules[cnt].rule.eq_attrib,
						sizeof(struct ipa_filter_rule_type_v01));
					IPACMDBG_H("mux-id %d, hashable %d\n", qmi_add_msg.filter_spec_ex2_list[pos].mux_id, qmi_add_msg.filter_spec_ex2_list[pos].is_rule_hashable);
					pos++;
					pcie_modem_rule_id = (pcie_modem_rule_id + 1)%100;
				}
				else
				{
					IPACMERR(" QMI only support max %d rules, current (%d)\n ",QMI_IPA_MAX_FILTERS_V01, pos);
				}
			}
		}

		ret = ioctl(fd_wwan_ioctl, WAN_IOC_ADD_OFFLOAD_CONNECTION, &qmi_add_msg);
		if (ret != 0)
		{
			IPACMERR("Failed sending WAN_IOC_ADD_OFFLOAD_CONNECTION with ret %d\n ", ret);
			close(fd_wwan_ioctl);
			return false;
		}
	}
	/* update total_num_offload_rules */
	total_num_offload_rules += flt_rule_tbl->num_rules;
	IPACMDBG_H("total_num_offload_rules %d \n", total_num_offload_rules);
	close(fd_wwan_ioctl);
	return true;
#else
	if(flt_rule_tbl != NULL)
	{
		IPACMERR("Not support (%d) AddOffloadFilteringRule with mux-id (%d) and default path = %d\n", flt_rule_tbl->num_rules, mux_id, default_path);
	}
	return false;
#endif
}

bool IPACM_Filtering::DelOffloadFilteringRule(struct ipa_ioc_del_flt_rule const *flt_rule_tbl)
{
#ifdef WAN_IOCTL_ADD_OFFLOAD_CONNECTION
	bool result = true;
	int ret = 0, cnt, pos = 0;
	ipa_remove_offload_connection_req_msg_v01 qmi_del_msg;
	int fd_wwan_ioctl = open(WWAN_QMI_IOCTL_DEVICE_NAME, O_RDWR);

	if(fd_wwan_ioctl < 0)
	{
		IPACMERR("Failed to open %s.\n",WWAN_QMI_IOCTL_DEVICE_NAME);
		return false;
	}

	if(flt_rule_tbl == NULL)
	{
		IPACMERR("Invalid add_offload_req\n");
		result =  false;
		goto fail;
	}

	/* check # of offload connections */
	if (flt_rule_tbl->num_hdls > total_num_offload_rules) {
		IPACMERR("(%d) del_offload req , exceed curent(%d)\n",
		flt_rule_tbl->num_hdls, total_num_offload_rules);
		result =  false;
		goto fail;
	}
	else
	{
		memset(&qmi_del_msg, 0, sizeof(qmi_del_msg));

		if (flt_rule_tbl->num_hdls > 0)
		{
			qmi_del_msg.filter_handle_list_valid = true;
		}
		else
		{
			IPACMERR("Get %d offload-req\n", flt_rule_tbl->num_hdls);
			goto fail;
		}
		qmi_del_msg.filter_handle_list_len = flt_rule_tbl->num_hdls;

		IPACMDBG_H("passing %d offload req to modem.\n", flt_rule_tbl->num_hdls);

		if(flt_rule_tbl != NULL)
		{
			for(cnt = flt_rule_tbl->num_hdls - 1; cnt >= 0; cnt--)
			{
				if (pos < QMI_IPA_MAX_FILTERS_V01)
				{
					/* passing rule-id to wan-driver */
					qmi_del_msg.filter_handle_list[pos].filter_spec_identifier = flt_rule_tbl->hdl[cnt].hdl;
					pos++;
				}
				else
				{
					IPACMERR(" QMI only support max %d rules, current (%d)\n ",QMI_IPA_MAX_FILTERS_V01, pos);
					result =  false;
					goto fail;
				}
			}
		}

		ret = ioctl(fd_wwan_ioctl, WAN_IOC_RMV_OFFLOAD_CONNECTION, &qmi_del_msg);
		if (ret != 0)
		{
			IPACMERR("Failed deleting Filtering rule %pK with ret %d\n ", &qmi_del_msg, ret);
			result =  false;
			goto fail;
		}
	}
	/* update total_num_offload_rules */
	total_num_offload_rules -= flt_rule_tbl->num_hdls;
	IPACMDBG_H("total_num_offload_rules %d \n", total_num_offload_rules);

fail:
	close(fd_wwan_ioctl);
	return result;
#else
	if(flt_rule_tbl != NULL)
	{
		IPACMERR("Not support (%d) DelOffloadFilteringRule\n", flt_rule_tbl->num_hdls);
	}
	return false;
#endif
}

bool IPACM_Filtering::SendFilteringRuleIndex(struct ipa_fltr_installed_notif_req_msg_v01* table)
{
	int ret = 0;
	int fd_wwan_ioctl = open(WWAN_QMI_IOCTL_DEVICE_NAME, O_RDWR);
	if(fd_wwan_ioctl < 0)
	{
		IPACMERR("Failed to open %s.\n",WWAN_QMI_IOCTL_DEVICE_NAME);
		return false;
	}

	ret = ioctl(fd_wwan_ioctl, WAN_IOC_ADD_FLT_RULE_INDEX, table);
	if (ret != 0)
	{
		IPACMERR("Failed adding filtering rule index %pK with ret %d\n", table, ret);
		close(fd_wwan_ioctl);
		return false;
	}

	IPACMDBG("Added Filtering rule index %pK\n", table);
	close(fd_wwan_ioctl);
	return true;
}

ipa_filter_action_enum_v01 IPACM_Filtering::GetQmiFilterAction(ipa_flt_action action)
{
	switch(action)
	{
	case IPA_PASS_TO_ROUTING:
		return QMI_IPA_FILTER_ACTION_ROUTING_V01;

	case IPA_PASS_TO_SRC_NAT:
		return QMI_IPA_FILTER_ACTION_SRC_NAT_V01;

	case IPA_PASS_TO_DST_NAT:
		return QMI_IPA_FILTER_ACTION_DST_NAT_V01;

	case IPA_PASS_TO_EXCEPTION:
		return QMI_IPA_FILTER_ACTION_EXCEPTION_V01;

	default:
		return IPA_FILTER_ACTION_ENUM_MAX_ENUM_VAL_V01;
	}
}

bool IPACM_Filtering::ModifyFilteringRule(struct ipa_ioc_mdfy_flt_rule* ruleTable)
{
	int i, ret = 0;

	IPACMDBG("Printing filtering add attributes\n");
	IPACMDBG("IP type: %d Number of rules: %d commit value: %d\n", ruleTable->ip, ruleTable->num_rules, ruleTable->commit);

	for (i=0; i<ruleTable->num_rules; i++)
	{
		IPACMDBG("Filter rule:%d attrib mask: 0x%x\n", i, ruleTable->rules[i].rule.attrib.attrib_mask);
	}

	ret = ioctl(fd, IPA_IOC_MDFY_FLT_RULE, ruleTable);
	if (ret != 0)
	{
		IPACMERR("Failed modifying filtering rule %pK\n", ruleTable);

		for (i = 0; i < ruleTable->num_rules; i++)
		{
			if (ruleTable->rules[i].status != 0)
			{
				IPACMERR("Modifying filter rule %d failed\n", i);
			}
		}
		return false;
	}

	IPACMDBG("Modified filtering rule %p\n", ruleTable);
	return true;
}

