/* Copyright (c) 2011-2015, The Linux Foundation. All rights reserved.
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
 *     * Neither the name of The Linux Foundatoin, nor the names of its
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
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/pps.h>

#ifndef _TIMEPPS_H
#define _TIMEPPS_H

#ifdef __cplusplus
extern "C" {
#endif
/* time of assert event */
typedef struct  timespec pps_info;
/* represents pps source */
typedef int pps_handle;

 /* Open the PPS source */
static __inline int pps_create(int source, pps_handle *handle)
{
   int ret;
   struct pps_kparams dummy;

   if (!handle)
   {
      errno = EINVAL;
      return -1;
   }
   /* check if current device is valid pps */
   ret = ioctl(source, PPS_GETPARAMS, &dummy);
   if (ret)
   {
      errno = EOPNOTSUPP;
      return -1;
   }
   *handle = source;

   return 0;
}
/* close the pps source */
static __inline int pps_destroy(pps_handle handle)
{
   return close(handle);
}
/*reads timestamp from pps device*/
static __inline int pps_fetch(pps_handle handle, const int tsformat,
                              pps_info *ppsinfobuf,
                              const struct timespec *timeout)
{
   struct pps_fdata fdata;
   int ret;

   if (tsformat != PPS_TSFMT_TSPEC)
   {
      errno = EINVAL;
      return -1;
   }
   if (timeout)
   {
      fdata.timeout.sec = timeout->tv_sec;
      fdata.timeout.nsec = timeout->tv_nsec;
      fdata.timeout.flags = ~PPS_TIME_INVALID;
   }
   else
   {
      fdata.timeout.flags = PPS_TIME_INVALID;
   }
   ret = ioctl(handle, PPS_FETCH, &fdata);

   ppsinfobuf->tv_sec = fdata.info.assert_tu.sec;
   ppsinfobuf->tv_nsec = fdata.info.assert_tu.nsec;

   return ret;
}

#ifdef __cplusplus
}
#endif
#endif
