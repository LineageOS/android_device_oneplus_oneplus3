/*
 * Copyright (c) 2012-2013,2015-2016, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * *    * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
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
 */

#include <cutils/properties.h>

int sysfs_read(char *path, char *s, int num_bytes);
int sysfs_write(char *path, char *s);
int get_scaling_governor(char governor[], int size);
int get_scaling_governor_check_cores(char governor[], int size,int core_num);
int is_interactive_governor(char*);

void vote_ondemand_io_busy_off();
void unvote_ondemand_io_busy_off();
void vote_ondemand_sdf_low();
void unvote_ondemand_sdf_low();
void perform_hint_action(int hint_id, int resource_values[],
    int num_resources);
void undo_hint_action(int hint_id);
void release_request(int lock_handle);
int interaction_with_handle(int lock_handle, int duration, int num_args, int opt_list[]);
void undo_initial_hint_action();
void set_profile(int profile);
void start_prefetch(int pid, const char *packageName);

long long calc_timespan_us(struct timespec start, struct timespec end);
int get_soc_id(void);
