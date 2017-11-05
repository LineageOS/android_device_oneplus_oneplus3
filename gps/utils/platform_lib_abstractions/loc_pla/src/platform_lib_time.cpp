/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
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
 */
#include "platform_lib_time.h"

#ifdef USE_GLIB
#include <loc_stub_time.h>
#else
#include <utils/SystemClock.h>
#include <utils/Timers.h>

#endif /* USE_GLIB */

int64_t platform_lib_abstraction_elapsed_millis_since_boot()
{
#ifdef USE_GLIB

    return elapsedMillisSinceBoot();

#else

    //return android::nanoseconds_to_microseconds(systemTime(SYSTEM_TIME_BOOTTIME))/1000;
    return nanoseconds_to_microseconds(systemTime(SYSTEM_TIME_BOOTTIME))/1000;
#endif
}
int64_t platform_lib_abstraction_elapsed_micros_since_boot()
{
#ifdef USE_GLIB
    return elapsedMicrosSinceBoot();

#else
    //return android::nanoseconds_to_microseconds(systemTime(SYSTEM_TIME_BOOTTIME));
    return nanoseconds_to_microseconds(systemTime(SYSTEM_TIME_BOOTTIME));
#endif
}
