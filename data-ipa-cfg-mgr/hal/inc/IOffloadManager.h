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
#ifndef _I_OFFLOAD_MANAGER_H_
#define _I_OFFLOAD_MANAGER_H_

/* External Includes */
#include <sys/types.h>

/* Internal Includes */
#include "OffloadStatistics.h"


class IOffloadManager {
public:
    enum RET {
        FAIL_TOO_MANY_PREFIXES = -6,
        FAIL_UNSUPPORTED = -5,
        FAIL_INPUT_CHECK = -4,
        FAIL_HARDWARE = -3,
        FAIL_UNNEEDED = -2,
        FAIL_TRY_AGAIN = -1,
        SUCCESS = 0,
        SUCCESS_DUPLICATE_CONFIG = 1,
        SUCCESS_NO_OP = 2,
        SUCCESS_OPTIMIZED = 3
    }; /* RET */

    enum IP_FAM {
        V4 = 0,
        V6 = 1,
        INVALID = 2
    }; /* IP_FAM */

    /* Overloading to use for addresses as well */
    typedef struct Prefix {
        IP_FAM fam;
        uint32_t v4Addr;
        uint32_t v4Mask;
        uint32_t v6Addr[4];
        uint32_t v6Mask[4];
    } prefix_t;

    /* ---------------------------- LIFECYCLE ------------------------------- */
    virtual ~IOffloadManager(){}

    /* ---------------------- ASYNC EVENT CALLBACKS ------------------------- */
    class IpaEventListener {
    public:
        enum StoppedReason {
            /**
             * Offload was stopped due to the configuration being removed via
             * setUpstreamParameters/removeDownstream.
             */
            REQUESTED,
            /**
             * Offload was stopped due to an internal (to IPA or modem) error.
             *
             * Statistics may be temporarily unavailable.
             */
            ERROR,
            /**
             * Offload was stopped because the upstream connection has
             * migrated to unsupported radio access technology.
             *
             * Statistics will still be available.
             */
            UNSUPPORTED
        }; /* StoppedReason */
        virtual ~IpaEventListener(){}
        /**
         * Called when Offload first begins to occur on any upstream and
         * tether interface pair.  It should be paired with an onOffloadStopped
         * call.
         */
        virtual void onOffloadStarted(){}
        /**
         * Called when Offload stops occurring on all upstream and tether
         * interface pairs.  It comes after a call to onOffloadStarted.
         *
         * @param reason Reason that Offload was stopped
         */
        virtual void onOffloadStopped(StoppedReason /* reason */){}
        /**
         * Called when the hardware can support Offload again.
         *
         * Any statistics that were previously unavailable, may be queried
         * again at this time.
         */
        virtual void onOffloadSupportAvailable(){}
        /**
         * Called when the limit set via setQuota has expired.
         *
         * It is implied that Offload has been stopped on all upstream and
         * tether interface pairs when this callback is called.
         */
        virtual void onLimitReached(){}
    }; /* IpaEventListener */

    /**
     * Request notifications about asynchronous events that occur in hardware.
     *
     * The calling client must be able to handle the callback on a separate
     * thread (i.e. their implementation of IpaEventListener must be thread
     * safe).
     *
     * @return SUCCESS iff callback successfully registered
     *
     * Remarks: This can't really be allowed to fail.
     */
    virtual RET registerEventListener(IpaEventListener* /* listener */) = 0;
    /**
     * Unregister a previously registered listener.
     *
     * @return SUCCESS iff callback successfully unregistered
     *         FAIL_INPUT_CHECK if callback was never registered
     */
    virtual RET unregisterEventListener(IpaEventListener* /* listener */) = 0;

    class ConntrackTimeoutUpdater {
    public:
        enum L4Protocol {
            TCP = 0,
            UDP = 1
        }; /* L4Protocol */
        typedef struct IpAddrPortPair {
            uint32_t ipAddr;
            uint16_t port;
        } ipAddrPortPair_t;
        typedef struct NatTimeoutUpdate {
            IpAddrPortPair src;
            IpAddrPortPair dst;
            L4Protocol proto;
        } natTimeoutUpdate_t;
        virtual ~ConntrackTimeoutUpdater(){}
        virtual void updateTimeout(NatTimeoutUpdate /* update */) {}
    }; /* ConntrackTimeoutUpdater */

    /**
     * Register a callback that may be called if the OffloadManager wants to
     * update the timeout value in conntrack of kernel.
     *
     * The calling client must be able to handle the callback on a separate
     * thread (i.e. their implementation of ConntrackTimeoutUpdater must be
     * thread safe)
     *
     * @return SUCCESS iff callback successfully registered
     *
     * Remarks: This can't really be allowed to fail
     */
    virtual RET registerCtTimeoutUpdater(ConntrackTimeoutUpdater* /* cb */) = 0;
    /**
     * Unregister a previously registered callback.
     *
     * @return SUCCESS iff callback successfully unregistered
     *         FAIL_INPUT_CHECK if callback was never registered
     */
    virtual RET unregisterCtTimeoutUpdater(ConntrackTimeoutUpdater* /* cb */) = 0;

    /* ----------------------------- CONFIG --------------------------------- */
    /**
     * Provide a file descriptor for use with conntrack library
     *
     * @param fd File Descriptor that has been opened and bound to groups
     * @param groups Groups (bit mask) that fd has been bound to
     *
     * @return SUCCESS iff IOffloadManager needed this file descriptor and
     *                 it was properly bound.
     *         FAIL_INPUT_CHECK if IOffloadManager needed this file descriptor
     *                          but it was found to not be properly bound
     *         FAIL_UNNEEDED if IOffloadManager determined that it does not need
     *                       a file descriptor bound to these groups.
     */
    virtual RET provideFd(int /* fd */, unsigned int /* group */) = 0;
    /**
     * Indicate that IOffloadManager <b>must</b> cease using all file
     * descriptors passed via provideFd API.
     *
     * After this call returns, the file descriptors will likely be closed by
     * the calling client.
     *
     * @return SUCCESS iff IOffloadManager has stopped using all file
     *                 descriptors
     *         FAIL_TRY_AGAIN if IOffloadManager needs more time with these
     *                        file descriptors before it can release them
     *
     * Remarks: Currently, it would be very difficult to handle a FAIL_TRY_AGAIN
     *          because HAL serivce does not own a thread outside of RPC
     *          Threadpool to reschedule this call.
     */
    virtual RET clearAllFds() = 0;
    /**
     * Query whether STA+AP offload is supported on this device.
     *
     * @return true if supported, false otherwise
     */
    virtual bool isStaApSupported() = 0;

    /* ------------------------------ ROUTE --------------------------------- */
    /**
     * Add a downstream prefix that <i>may</i> be forwarded.
     *
     * The Prefix may be an IPv4 or IPv6 address to signify which family can be
     * offloaded from the specified tether interface.  If the given IP family,
     * as determined by the Prefix, has a corresponding upstream configured,
     * then traffic should be forwarded between the two interfaces.
     *
     * Only traffic that has a downstream address within the specified Prefix
     * can be forwarded.  Traffic from the same downstream interface that falls
     * outside of the Prefix will be unaffected and can be forwarded iff it was
     * previously configured via a separate addDownstream call.
     *
     * If no upstream has been configured, then this information must be cached
     * so that offload may begin once an upstream is configured.
     *
     * This API does <b>not</b> replace any previously configured downstreams
     * and must be explicitly removed by calling removeDownstream or by clearing
     * the entire configuration by calling stopAllOffload.
     *
     * @return SUCCESS The new information was accepted
     *         FAIL_TOO_MANY_PREFIXES The hardware has already accepted the max
     *                                number of Prefixes that can be supported.
     *                                If offload is desired on this Prefix then
     *                                another must be removed first.
     *         FAIL_UNSUPPORTED The hardware cannot forward traffic from this
     *                          downstream interface and will never be able to.
     */
    virtual RET addDownstream(const char* /* downstream */,
            const Prefix& /* prefix */) = 0;
    /**
     * Remove a downstream Prefix that forwarding was previously requested for.
     *
     * The Prefix may be an IPv4 or IPv6 address.  Traffic outside of this
     * Prefix is not affected.
     *
     * @return SUCCESS iff forwarding was previously occurring and has been
     *                 stopped
     *         SUCCESS_NO_OP iff forwarding was not previously occurring and
     *                       therefore no action needed to be taken
     */
    virtual RET removeDownstream(const char* /* downstream */,
            const Prefix& /* prefix */) = 0;
    /**
     * Indicate that hardware should forward traffic from any configured
     * downstreams to the specified upstream.
     *
     * When iface is non-null and non-empty and v4Gw is valid, then any
     * currently configured or future configured IPv4 downstreams should be
     * forwarded to this upstream interface.
     *
     * When iface is non-null and non-empty and v6Gw is valid, then any
     * currently configured or future configured IPv6 downstreams should be
     * forwarded to this upstream interface.
     *
     * @param iface Upstream interface name.  Only one is needed because IPv4
     *              and IPv6 interface names are required to match.
     * @param v4Gw The address of the IPv4 Gateway on the iface
     * @param v6Gw The address of one of the IPv6 Gateways on the iface
     *
     * @return SUCCESS iff the specified configuration was applied
     *         SUCCESS_DUPLICATE_CONFIG if this configuration <i>exactly</i>
     *                                  matches a previously provided
     *                                  configuration.  This means that no
     *                                  action has to be taken, but, the
     *                                  configuration was previously accepted
     *                                  and applied.
     *         FAIL_UNSUPPORTED if hardware cannot support forwarding to this
     *                          upstream interface
     *
     * Remarks: This overrides any previously configured parameters
     */
    virtual RET setUpstream(const char* /* iface */, const Prefix& /* v4Gw */,
            const Prefix& /* v6Gw */) = 0;
    /**
     * All traffic must be returned to the software path and all configuration
     * (including provided file descriptors) must be forgotten.
     *
     * @return SUCCESS If all offload was successfully stopped and provided
     *                 file descriptors were released.
     *
     * Remarks: This can't really fail?
     */
    virtual RET stopAllOffload() = 0;

    /* --------------------------- STATS/POLICY ----------------------------- */
    /**
     * Instruct hardware to stop forwarding traffic and send a callback after
     * limit bytes have been transferred in either direction on this upstream
     * interface.
     *
     * @param upstream Upstream interface name that the limit should apply to
     * @param limit Bytes limit that can occur before action should be taken
     *
     * @return SUCCESS If the limit was successfully applied
     *         SUCCESS_OPTIMIZED If the limit was sufficiently high to be
     *                           interpreted as "no quota".
     *         FAIL_HARDWARE If the limit was rejected by the hardware
     *         FAIL_UNSUPPORTED If metering is not supported on this interface
     *         FAIL_TRY_AGAIN If this upstream has not been previously
     *                        configured to allow offload
     *                        (via setUpstreamParameters)
     */
    virtual RET setQuota(const char* /* upstream */, uint64_t /* limit */) = 0;
    /**
     * Query for statistics counters in hardware.
     *
     * This returns an aggregate of all hardware accelerated traffic which
     * has occurred on this upstream interface.
     *
     * @param upstream Interface on which traffic entered/exited
     * @param reset Whether hardware counters should reset after returning
     *              current statistics
     * @param ret Output variable where statistics are returned
     *
     * @return SUCCESS If the statistics were successfully populated in ret and
     *                 were successfully reset if requested.
     *         FAIL_TRY_AGAIN If the statistics are not currently available but
     *                        may be available later.  This may occur during
     *                        a subsystem restart.
     *         FAIL_UNSUPPORTED If statistics are not supported on this upstream
     */
    virtual RET getStats(const char* /* upstream */, bool /* reset */,
            OffloadStatistics& /* ret */) = 0;
}; /* IOffloadManager */
#endif /* _I_OFFLOAD_MANAGER_H_ */
