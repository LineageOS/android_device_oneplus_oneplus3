/* Copyright (c) 2012, 2014, The Linux Foundation. All rights reserved.
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
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#define FAILED                  -1
#define SUCCESS                 0
#define INDEFINITE_DURATION     0

enum SCREEN_DISPLAY_TYPE {
    DISPLAY_OFF = 0x00FF,
};

enum PWR_CLSP_TYPE {
#ifdef MPCTLV3
    ALL_CPUS_PWR_CLPS_DIS_V3 = 0x40400000, /* v3 resource */
#endif
    ALL_CPUS_PWR_CLPS_DIS = 0x101,
};

/* For CPUx min freq, the leftmost byte
 * represents the CPU and the
 * rightmost byte represents the frequency
 * All intermediate frequencies on the
 * device are supported. The hex value
 * passed into PerfLock will be multiplied
 * by 10^5. This frequency or the next
 * highest frequency available will be set
 *
 * For example, if 1.4 Ghz is required on
 * CPU0, use 0x20E
 *
 * If the highest available frequency
 * on the device is required, use
 * CPUx_MIN_FREQ_TURBO_MAX
 * where x represents the CPU
 */
enum CPU0_MIN_FREQ_LVL {
    CPU0_MIN_FREQ_NONTURBO_MAX = 0x20A,
    CPU0_MIN_FREQ_TURBO_MAX = 0x2FE,
};

enum CPU1_MIN_FREQ_LVL {
    CPU1_MIN_FREQ_NONTURBO_MAX = 0x30A,
    CPU1_MIN_FREQ_TURBO_MAX = 0x3FE,
};

enum CPU2_MIN_FREQ_LVL {
    CPU2_MIN_FREQ_NONTURBO_MAX = 0x40A,
    CPU2_MIN_FREQ_TURBO_MAX = 0x4FE,
};

enum CPU3_MIN_FREQ_LVL {
    CPU3_MIN_FREQ_NONTURBO_MAX = 0x50A,
    CPU3_MIN_FREQ_TURBO_MAX = 0x5FE,
};

enum CPU0_MAX_FREQ_LVL {
    CPU0_MAX_FREQ_NONTURBO_MAX = 0x150A,
};

enum CPU1_MAX_FREQ_LVL {
    CPU1_MAX_FREQ_NONTURBO_MAX = 0x160A,
};

enum CPU2_MAX_FREQ_LVL {
    CPU2_MAX_FREQ_NONTURBO_MAX = 0x170A,
};

enum CPU3_MAX_FREQ_LVL {
    CPU3_MAX_FREQ_NONTURBO_MAX = 0x180A,
};

enum MIN_CPUS_ONLINE_LVL {
#ifdef MPCTLV3
    CPUS_ONLINE_MIN_BIG = 0x41000000, /* v3 resource */
    CPUS_ONLINE_MIN_LITTLE = 0x41000100, /* v3 resource */
#endif
    CPUS_ONLINE_MIN_2 = 0x702,
    CPUS_ONLINE_MIN_3 = 0x703,
    CPUS_ONLINE_MIN_4 = 0x704,
    CPUS_ONLINE_MPD_OVERRIDE = 0x777,
    CPUS_ONLINE_MAX = 0x7FF,
};

enum MAX_CPUS_ONLINE_LVL {
#ifdef MPCTLV3
    CPUS_ONLINE_MAX_LIMIT_BIG = 0x41004000, /* v3 resource */
    CPUS_ONLINE_MAX_LIMIT_LITTLE = 0x41004100, /* v3 resource */
#endif
    CPUS_ONLINE_MAX_LIMIT_1 = 0x8FE,
    CPUS_ONLINE_MAX_LIMIT_2 = 0x8FD,
    CPUS_ONLINE_MAX_LIMIT_3 = 0x8FC,
    CPUS_ONLINE_MAX_LIMIT_4 = 0x8FB,
    CPUS_ONLINE_MAX_LIMIT_MAX = 0x8FB,
};

enum SAMPLING_RATE_LVL {
    MS_500 = 0xBCD,
    MS_50 = 0xBFA,
    MS_20 = 0xBFD,
};

enum ONDEMAND_IO_BUSY_LVL {
    IO_BUSY_OFF = 0xC00,
    IO_BUSY_ON = 0xC01,
};

enum ONDEMAND_SAMPLING_DOWN_FACTOR_LVL {
    SAMPLING_DOWN_FACTOR_1 = 0xD01,
    SAMPLING_DOWN_FACTOR_4 = 0xD04,
};


enum INTERACTIVE_TIMER_RATE_LVL {
    TR_MS_500 = 0xECD,
    TR_MS_100 = 0xEF5,
    TR_MS_50 = 0xEFA,
    TR_MS_30 = 0xEFC,
    TR_MS_20 = 0xEFD,
};

/* This timer rate applicable to cpu0
    across 8939 series chipset */
enum INTERACTIVE_TIMER_RATE_LVL_CPU0_8939 {
    TR_MS_CPU0_500 = 0x30CD,
    TR_MS_CPU0_100 = 0x30F5,
    TR_MS_CPU0_50 = 0x30FA,
    TR_MS_CPU0_30 = 0x30FC,
    TR_MS_CPU0_20 = 0x30FD,
};

/* This timer rate applicable to cpu4
    across 8939 series chipset */
enum INTERACTIVE_TIMER_RATE_LVL_CPU4_8939 {
    TR_MS_CPU4_500 = 0x3BCD,
    TR_MS_CPU4_100 = 0x3BF5,
    TR_MS_CPU4_50 = 0x3BFA,
    TR_MS_CPU4_30 = 0x3BFC,
    TR_MS_CPU4_20 = 0x3BFD,
};

enum INTERACTIVE_HISPEED_FREQ_LVL {
    HS_FREQ_1026 = 0xF0A,
    HS_FREQ_800  = 0xF08,
};

enum INTERACTIVE_HISPEED_LOAD_LVL {
    HISPEED_LOAD_90 = 0x105A,
};

enum SYNC_FREQ_LVL {
    SYNC_FREQ_300 = 0x1103,
    SYNC_FREQ_600 = 0X1106,
    SYNC_FREQ_384 = 0x1103,
    SYNC_FREQ_NONTURBO_MAX = 0x110A,
    SYNC_FREQ_TURBO = 0x110F,
};

enum OPTIMAL_FREQ_LVL {
    OPTIMAL_FREQ_300 = 0x1203,
    OPTIMAL_FREQ_600 = 0x1206,
    OPTIMAL_FREQ_384 = 0x1203,
    OPTIMAL_FREQ_NONTURBO_MAX = 0x120A,
    OPTIMAL_FREQ_TURBO = 0x120F,
};

enum SCREEN_PWR_CLPS_LVL {
    PWR_CLPS_DIS = 0x1300,
    PWR_CLPS_ENA = 0x1301,
};

enum THREAD_MIGRATION_LVL {
    THREAD_MIGRATION_SYNC_OFF = 0x1400,
#ifdef MPCTLV3
    THREAD_MIGRATION_SYNC_ON_V3 = 0x4241C000
#endif
};

enum INTERACTIVE_IO_BUSY_LVL {
    INTERACTIVE_IO_BUSY_OFF = 0x1B00,
    INTERACTIVE_IO_BUSY_ON = 0x1B01,
};

enum SCHED_BOOST_LVL {
#ifdef MPCTLV3
    SCHED_BOOST_ON_V3 = 0x40C00000, /* v3 resource */
#endif
    SCHED_BOOST_ON = 0x1E01,
};

enum CPU4_MIN_FREQ_LVL {
    CPU4_MIN_FREQ_NONTURBO_MAX = 0x1F0A,
    CPU4_MIN_FREQ_TURBO_MAX = 0x1FFE,
};

enum CPU5_MIN_FREQ_LVL {
    CPU5_MIN_FREQ_NONTURBO_MAX = 0x200A,
    CPU5_MIN_FREQ_TURBO_MAX = 0x20FE,
};

enum CPU6_MIN_FREQ_LVL {
    CPU6_MIN_FREQ_NONTURBO_MAX = 0x210A,
    CPU6_MIN_FREQ_TURBO_MAX = 0x21FE,
};

enum CPU7_MIN_FREQ_LVL {
    CPU7_MIN_FREQ_NONTURBO_MAX = 0x220A,
    CPU7_MIN_FREQ_TURBO_MAX = 0x22FE,
};

enum CPU4_MAX_FREQ_LVL {
    CPU4_MAX_FREQ_NONTURBO_MAX = 0x230A,
};

enum CPU5_MAX_FREQ_LVL {
    CPU5_MAX_FREQ_NONTURBO_MAX = 0x240A,
};

enum CPU6_MAX_FREQ_LVL {
    CPU6_MAX_FREQ_NONTURBO_MAX = 0x250A,
};

enum CPU7_MAX_FREQ_LVL {
    CPU7_MAX_FREQ_NONTURBO_MAX = 0x260A,
};

enum SCHED_PREFER_IDLE {
#ifdef MPCTLV3
    SCHED_PREFER_IDLE_DIS_V3 = 0x40C04000,
#endif
    SCHED_PREFER_IDLE_DIS = 0x3E01,
};

enum SCHED_MIGRATE_COST_CHNG {
    SCHED_MIGRATE_COST_SET = 0x3F01,
};

#ifdef MPCTLV3
/**
 * MPCTL v3 opcodes
 */
enum MAX_FREQ_CLUSTER_BIG {
    MAX_FREQ_BIG_CORE_0         = 0x40804000,
};

enum MAX_FREQ_CLUSTER_LITTLE {
    MAX_FREQ_LITTLE_CORE_0      = 0x40804100,
};

enum MIN_FREQ_CLUSTER_BIG {
    MIN_FREQ_BIG_CORE_0         = 0x40800000,
};

enum MIN_FREQ_CLUSTER_LITTLE {
    MIN_FREQ_LITTLE_CORE_0      = 0x40800100,
};

enum INTERACTIVE_CLUSTER_BIG {
    ABOVE_HISPEED_DELAY_BIG     = 0x41400000,
    GO_HISPEED_LOAD_BIG         = 0x41410000,
    HISPEED_FREQ_BIG            = 0x41414000,
    TARGET_LOADS_BIG            = 0x41420000,
    TIMER_RATE_BIG              = 0x41424000,
    USE_SCHED_LOAD_BIG          = 0x41430000,
    USE_MIGRATION_NOTIF_BIG     = 0x41434000,
    IGNORE_HISPEED_NOTIF_BIG    = 0x41438000,
};

enum INTERACTIVE_CLUSTER_LITTLE {
    ABOVE_HISPEED_DELAY_LITTLE  = 0x41400100,
    GO_HISPEED_LOAD_LITTLE      = 0x41410100,
    HISPEED_FREQ_LITTLE         = 0x41414100,
    TARGET_LOADS_LITTLE         = 0x41420100,
    TIMER_RATE_LITTLE           = 0x41424100,
    USE_SCHED_LOAD_LITTLE       = 0x41430100,
    USE_MIGRATION_NOTIF_LITTLE  = 0x41434100,
    IGNORE_HISPEED_NOTIF_LITTLE = 0x41438100,
};

enum CPUBW_HWMON {
    CPUBW_HWMON_MIN_FREQ        = 0x41800000,
    CPUBW_HWMON_V1              = 0x4180C000,
    LOW_POWER_CEIL_MBPS         = 0x41810000,
    LOW_POWER_IO_PERCENT        = 0x41814000,
    CPUBW_HWMON_SAMPLE_MS       = 0x41820000,
};

enum SCHEDULER {
    SCHED_SMALL_TASK_DIS        = 0x40C0C000,
    SCHED_IDLE_LOAD_DIS         = 0x40C10000,
    SCHED_IDLE_NR_RUN_DIS       = 0x40C14000,
    SCHED_GROUP_ON              = 0x40C28000,
};

enum STORAGE {
    STOR_CLK_SCALE_DIS          = 0x42C10000,
};

enum GPU {
    GPU_MIN_PWRLVL_BOOST        = 0x42804000,
};
#endif

#ifdef __cplusplus
}
#endif
