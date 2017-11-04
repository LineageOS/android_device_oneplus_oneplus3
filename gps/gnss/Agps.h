/* Copyright (c) 2012-2017, The Linux Foundation. All rights reserved.
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
 *     * Neither the name of The Linux Foundation, nor the names of its
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

#ifndef AGPS_H
#define AGPS_H

#include <functional>
#include <list>
#include <MsgTask.h>
#include <gps_extended_c.h>
#include <platform_lib_log_util.h>

/* ATL callback function pointers
 * Passed in by Adapter to AgpsManager */
typedef std::function<void(
        int handle, int isSuccess, char* apn,
        AGpsBearerType bearerType, AGpsExtType agpsType)>  AgpsAtlOpenStatusCb;

typedef std::function<void(int handle, int isSuccess)>     AgpsAtlCloseStatusCb;

/* DS Client control APIs
 * Passed in by Adapter to AgpsManager */
typedef std::function<int(bool isDueToSSR)>  AgpsDSClientInitFn;
typedef std::function<int()>                 AgpsDSClientOpenAndStartDataCallFn;
typedef std::function<void()>                AgpsDSClientStopDataCallFn;
typedef std::function<void()>                AgpsDSClientCloseDataCallFn;
typedef std::function<void()>                AgpsDSClientReleaseFn;

/* Post message to adapter's message queue */
typedef std::function<void(LocMsg* msg)>     SendMsgToAdapterMsgQueueFn;

/* AGPS States */
typedef enum {
    AGPS_STATE_INVALID = 0,
    AGPS_STATE_RELEASED,
    AGPS_STATE_PENDING,
    AGPS_STATE_ACQUIRED,
    AGPS_STATE_RELEASING
} AgpsState;

typedef enum {
    AGPS_EVENT_INVALID = 0,
    AGPS_EVENT_SUBSCRIBE,
    AGPS_EVENT_UNSUBSCRIBE,
    AGPS_EVENT_GRANTED,
    AGPS_EVENT_RELEASED,
    AGPS_EVENT_DENIED
} AgpsEvent;

/* Notification Types sent to subscribers */
typedef enum {
    AGPS_NOTIFICATION_TYPE_INVALID = 0,

    /* Meant for all subscribers, either active or inactive */
    AGPS_NOTIFICATION_TYPE_FOR_ALL_SUBSCRIBERS,

    /* Meant for only inactive subscribers */
    AGPS_NOTIFICATION_TYPE_FOR_INACTIVE_SUBSCRIBERS,

    /* Meant for only active subscribers */
    AGPS_NOTIFICATION_TYPE_FOR_ACTIVE_SUBSCRIBERS
} AgpsNotificationType;

/* Framework AGNSS interface
 * This interface is defined in IAGnssCallback provided by
 * Android Framework.
 * Must be kept in sync with that interface. */
namespace AgpsFrameworkInterface {

    /** AGNSS type **/
    enum AGnssType : uint8_t {
        TYPE_SUPL         = 1,
        TYPE_C2K          = 2
    };

    enum AGnssStatusValue : uint8_t {
        /** GNSS requests data connection for AGNSS. */
        REQUEST_AGNSS_DATA_CONN  = 1,
        /** GNSS releases the AGNSS data connection. */
        RELEASE_AGNSS_DATA_CONN  = 2,
        /** AGNSS data connection initiated */
        AGNSS_DATA_CONNECTED     = 3,
        /** AGNSS data connection completed */
        AGNSS_DATA_CONN_DONE     = 4,
        /** AGNSS data connection failed */
        AGNSS_DATA_CONN_FAILED   = 5
    };

    /*
     * Represents the status of AGNSS augmented to support IPv4.
     */
    struct AGnssStatusIpV4 {
        AGnssType type;
        AGnssStatusValue status;
        /*
         * 32-bit IPv4 address.
         */
        unsigned int ipV4Addr;
    };

    /*
     * Represents the status of AGNSS augmented to support IPv6.
     */
    struct AGnssStatusIpV6 {
        AGnssType type;
        AGnssStatusValue status;
        /*
         * 128-bit IPv6 address.
         */
        unsigned char ipV6Addr[16];
    };

    /*
     * Callback with AGNSS(IpV4) status information.
     *
     * @param status Will be of type AGnssStatusIpV4.
     */
    typedef void (*AgnssStatusIpV4Cb)(AGnssStatusIpV4 status);

    /*
     * Callback with AGNSS(IpV6) status information.
     *
     * @param status Will be of type AGnssStatusIpV6.
     */
    typedef void (*AgnssStatusIpV6Cb)(AGnssStatusIpV6 status);
}

/* Classes in this header */
class AgpsSubscriber;
class AgpsManager;
class AgpsStateMachine;
class DSStateMachine;


/* SUBSCRIBER
 * Each Subscriber instance corresponds to one AGPS request,
 * received by the AGPS state machine */
class AgpsSubscriber {

public:
    int mConnHandle;

    /* Does this subscriber wait for data call close complete,
     * before being notified ATL close ?
     * While waiting for data call close, subscriber will be in
     * inactive state. */
    bool mWaitForCloseComplete;
    bool mIsInactive;

    inline AgpsSubscriber(
            int connHandle, bool waitForCloseComplete, bool isInactive) :
            mConnHandle(connHandle),
            mWaitForCloseComplete(waitForCloseComplete),
            mIsInactive(isInactive) {}
    inline virtual ~AgpsSubscriber() {}

    inline virtual bool equals(const AgpsSubscriber *s) const
    { return (mConnHandle == s->mConnHandle); }

    inline virtual AgpsSubscriber* clone()
    { return new AgpsSubscriber(
            mConnHandle, mWaitForCloseComplete, mIsInactive); }
};

/* AGPS STATE MACHINE */
class AgpsStateMachine {
protected:
    /* AGPS Manager instance, from where this state machine is created */
    AgpsManager* mAgpsManager;

    /* List of all subscribers for this State Machine.
     * Once a subscriber is notified for ATL open/close status,
     * it is deleted */
    std::list<AgpsSubscriber*> mSubscriberList;

    /* Current subscriber, whose request this State Machine is
     * currently processing */
    AgpsSubscriber* mCurrentSubscriber;

    /* Current state for this state machine */
    AgpsState mState;

private:
    /* AGPS Type for this state machine
       LOC_AGPS_TYPE_ANY           0
       LOC_AGPS_TYPE_SUPL          1
       LOC_AGPS_TYPE_WWAN_ANY      3
       LOC_AGPS_TYPE_SUPL_ES       5 */
    AGpsExtType mAgpsType;

    /* APN and IP Type info for AGPS Call */
    char* mAPN;
    unsigned int mAPNLen;
    AGpsBearerType mBearer;

public:
    /* CONSTRUCTOR */
    AgpsStateMachine(AgpsManager* agpsManager, AGpsExtType agpsType):
        mAgpsManager(agpsManager), mSubscriberList(),
        mCurrentSubscriber(NULL), mState(AGPS_STATE_RELEASED),
        mAgpsType(agpsType), mAPN(NULL), mAPNLen(0),
        mBearer(AGPS_APN_BEARER_INVALID) {};

    virtual ~AgpsStateMachine() { if(NULL != mAPN) delete[] mAPN; };

    /* Getter/Setter methods */
    void setAPN(char* apn, unsigned int len);
    inline char* getAPN() const { return (char*)mAPN; }
    inline void setBearer(AGpsBearerType bearer) { mBearer = bearer; }
    inline AGpsBearerType getBearer() const { return mBearer; }
    inline AGpsExtType getType() const { return mAgpsType; }
    inline void setCurrentSubscriber(AgpsSubscriber* subscriber)
    { mCurrentSubscriber = subscriber; }

    /* Fetch subscriber with specified handle */
    AgpsSubscriber* getSubscriber(int connHandle);

    /* Fetch first active or inactive subscriber in list
     * isInactive = true : fetch first inactive subscriber
     * isInactive = false : fetch first active subscriber */
    AgpsSubscriber* getFirstSubscriber(bool isInactive);

    /* Process LOC AGPS Event being passed in
     * onRsrcEvent */
    virtual void processAgpsEvent(AgpsEvent event);

    /* Drop all subscribers, in case of Modem SSR */
    void dropAllSubscribers();

protected:
    /* Remove the specified subscriber from list if present.
     * Also delete the subscriber instance. */
    void deleteSubscriber(AgpsSubscriber* subscriber);

private:
    /* Send call setup request to framework
     * sendRsrcRequest(LOC_GPS_REQUEST_AGPS_DATA_CONN)
     * sendRsrcRequest(LOC_GPS_RELEASE_AGPS_DATA_CONN) */
    virtual int requestOrReleaseDataConn(bool request);

    /* Individual event processing methods */
    void processAgpsEventSubscribe();
    void processAgpsEventUnsubscribe();
    void processAgpsEventGranted();
    void processAgpsEventReleased();
    void processAgpsEventDenied();

    /* Clone the passed in subscriber and add to the subscriber list
     * if not already present */
    void addSubscriber(AgpsSubscriber* subscriber);

    /* Notify subscribers about AGPS events */
    void notifyAllSubscribers(
            AgpsEvent event, bool deleteSubscriberPostNotify,
            AgpsNotificationType notificationType);
    virtual void notifyEventToSubscriber(
            AgpsEvent event, AgpsSubscriber* subscriber,
            bool deleteSubscriberPostNotify);

    /* Do we have any subscribers in active state */
    bool anyActiveSubscribers();

    /* Transition state */
    void transitionState(AgpsState newState);
};

/* DS STATE MACHINE */
class DSStateMachine : public AgpsStateMachine {

private:
    static const int MAX_START_DATA_CALL_RETRIES;
    static const int DATA_CALL_RETRY_DELAY_MSEC;

    int mRetries;

public:
    /* CONSTRUCTOR */
    DSStateMachine(AgpsManager* agpsManager):
        AgpsStateMachine(agpsManager, LOC_AGPS_TYPE_SUPL_ES), mRetries(0) {}

    /* Overridden method
     * DS SM needs to handle one event differently */
    void processAgpsEvent(AgpsEvent event);

    /* Retry callback, used in case call failure */
    void retryCallback();

private:
    /* Overridden method, different functionality for DS SM
     * Send call setup request to framework
     * sendRsrcRequest(LOC_GPS_REQUEST_AGPS_DATA_CONN)
     * sendRsrcRequest(LOC_GPS_RELEASE_AGPS_DATA_CONN) */
    int requestOrReleaseDataConn(bool request);

    /* Overridden method, different functionality for DS SM */
    void notifyEventToSubscriber(
            AgpsEvent event, AgpsSubscriber* subscriber,
            bool deleteSubscriberPostNotify);
};

/* LOC AGPS MANAGER */
class AgpsManager {

    friend class AgpsStateMachine;
    friend class DSStateMachine;

public:
    /* CONSTRUCTOR */
    AgpsManager():
        mFrameworkStatusV4Cb(NULL),
        mAtlOpenStatusCb(), mAtlCloseStatusCb(),
        mDSClientInitFn(), mDSClientOpenAndStartDataCallFn(),
        mDSClientStopDataCallFn(), mDSClientCloseDataCallFn(), mDSClientReleaseFn(),
        mSendMsgToAdapterQueueFn(),
        mAgnssNif(NULL), mInternetNif(NULL), mDsNif(NULL) {}

    /* Register callbacks */
    void registerCallbacks(
            AgpsFrameworkInterface::AgnssStatusIpV4Cb
                                                frameworkStatusV4Cb,

            AgpsAtlOpenStatusCb                 atlOpenStatusCb,
            AgpsAtlCloseStatusCb                atlCloseStatusCb,

            AgpsDSClientInitFn                  dsClientInitFn,
            AgpsDSClientOpenAndStartDataCallFn  dsClientOpenAndStartDataCallFn,
            AgpsDSClientStopDataCallFn          dsClientStopDataCallFn,
            AgpsDSClientCloseDataCallFn         dsClientCloseDataCallFn,
            AgpsDSClientReleaseFn               dsClientReleaseFn,

            SendMsgToAdapterMsgQueueFn          sendMsgToAdapterQueueFn ){

        mFrameworkStatusV4Cb = frameworkStatusV4Cb;

        mAtlOpenStatusCb = atlOpenStatusCb;
        mAtlCloseStatusCb = atlCloseStatusCb;

        mDSClientInitFn = dsClientInitFn;
        mDSClientOpenAndStartDataCallFn = dsClientOpenAndStartDataCallFn;
        mDSClientStopDataCallFn = dsClientStopDataCallFn;
        mDSClientCloseDataCallFn = dsClientCloseDataCallFn;
        mDSClientReleaseFn = dsClientReleaseFn;

        mSendMsgToAdapterQueueFn = sendMsgToAdapterQueueFn;
    }

    /* Create all AGPS state machines */
    void createAgpsStateMachines();

    /* Process incoming ATL requests */
    void requestATL(int connHandle, AGpsExtType agpsType);
    void releaseATL(int connHandle);

    /* Process incoming DS Client data call events */
    void reportDataCallOpened();
    void reportDataCallClosed();

    /* Process incoming framework data call events */
    void reportAtlOpenSuccess(
            AGpsExtType agpsType, char* apnName, int apnLen,
            LocApnIpType ipType);
    void reportAtlOpenFailed(AGpsExtType agpsType);
    void reportAtlClosed(AGpsExtType agpsType);

    /* Handle Modem SSR */
    void handleModemSSR();

protected:
    AgpsFrameworkInterface::AgnssStatusIpV4Cb mFrameworkStatusV4Cb;

    AgpsAtlOpenStatusCb   mAtlOpenStatusCb;
    AgpsAtlCloseStatusCb  mAtlCloseStatusCb;

    AgpsDSClientInitFn                  mDSClientInitFn;
    AgpsDSClientOpenAndStartDataCallFn  mDSClientOpenAndStartDataCallFn;
    AgpsDSClientStopDataCallFn          mDSClientStopDataCallFn;
    AgpsDSClientCloseDataCallFn         mDSClientCloseDataCallFn;
    AgpsDSClientReleaseFn               mDSClientReleaseFn;

    SendMsgToAdapterMsgQueueFn          mSendMsgToAdapterQueueFn;

    AgpsStateMachine*   mAgnssNif;
    AgpsStateMachine*   mInternetNif;
    AgpsStateMachine*   mDsNif;

private:
    /* Fetch state machine for handling request ATL call */
    AgpsStateMachine* getAgpsStateMachine(AGpsExtType agpsType);
};

/* Request SUPL/INTERNET/SUPL_ES ATL
 * This LocMsg is defined in this header since it has to be used from more
 * than one place, other Agps LocMsg are restricted to GnssAdapter and
 * declared inline */
struct AgpsMsgRequestATL: public LocMsg {

    AgpsManager* mAgpsManager;
    int mConnHandle;
    AGpsExtType mAgpsType;

    inline AgpsMsgRequestATL(AgpsManager* agpsManager, int connHandle,
            AGpsExtType agpsType) :
            LocMsg(), mAgpsManager(agpsManager), mConnHandle(connHandle), mAgpsType(
                    agpsType) {

        LOC_LOGV("AgpsMsgRequestATL");
    }

    inline virtual void proc() const {

        LOC_LOGV("AgpsMsgRequestATL::proc()");
        mAgpsManager->requestATL(mConnHandle, mAgpsType);
    }
};

namespace AgpsUtils {

AGpsBearerType ipTypeToBearerType(LocApnIpType ipType);
LocApnIpType bearerTypeToIpType(AGpsBearerType bearerType);

}

#endif /* AGPS_H */
