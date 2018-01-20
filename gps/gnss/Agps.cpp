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

#define LOG_TAG "LocSvc_Agps"

#include <Agps.h>
#include <platform_lib_includes.h>
#include <ContextBase.h>
#include <loc_timer.h>

/* --------------------------------------------------------------------
 *   AGPS State Machine Methods
 * -------------------------------------------------------------------*/
void AgpsStateMachine::processAgpsEvent(AgpsEvent event){

    LOC_LOGD("processAgpsEvent(): SM %p, Event %d, State %d",
               this, event, mState);

    switch (event) {

        case AGPS_EVENT_SUBSCRIBE:
            processAgpsEventSubscribe();
            break;

        case AGPS_EVENT_UNSUBSCRIBE:
            processAgpsEventUnsubscribe();
            break;

        case AGPS_EVENT_GRANTED:
            processAgpsEventGranted();
            break;

        case AGPS_EVENT_RELEASED:
            processAgpsEventReleased();
            break;

        case AGPS_EVENT_DENIED:
            processAgpsEventDenied();
            break;

        default:
            LOC_LOGE("Invalid Loc Agps Event");
    }
}

void AgpsStateMachine::processAgpsEventSubscribe(){

    switch (mState) {

        case AGPS_STATE_RELEASED:
            /* Add subscriber to list
             * No notifications until we get RSRC_GRANTED */
            addSubscriber(mCurrentSubscriber);

            /* Send data request
             * The if condition below is added so that if the data call setup
             * fails for DS State Machine, we want to retry in released state.
             * for Agps State Machine, sendRsrcRequest() will always return
             * success. */
            if (requestOrReleaseDataConn(true) == 0) {
                // If data request successful, move to pending state
                transitionState(AGPS_STATE_PENDING);
            }
            break;

        case AGPS_STATE_PENDING:
            /* Already requested for data connection,
             * do nothing until we get RSRC_GRANTED event;
             * Just add this subscriber to the list, for notifications */
            addSubscriber(mCurrentSubscriber);
            break;

        case AGPS_STATE_ACQUIRED:
            /* We already have the data connection setup,
             * Notify current subscriber with GRANTED event,
             * And add it to the subscriber list for further notifications. */
            notifyEventToSubscriber(AGPS_EVENT_GRANTED, mCurrentSubscriber, false);
            addSubscriber(mCurrentSubscriber);
            break;

        case AGPS_STATE_RELEASING:
            addSubscriber(mCurrentSubscriber);
            break;

        default:
            LOC_LOGE("Invalid state: %d", mState);
    }
}

void AgpsStateMachine::processAgpsEventUnsubscribe(){

    switch (mState) {

        case AGPS_STATE_RELEASED:
            notifyEventToSubscriber(
                    AGPS_EVENT_UNSUBSCRIBE, mCurrentSubscriber, false);
            break;

        case AGPS_STATE_PENDING:
        case AGPS_STATE_ACQUIRED:
            /* If the subscriber wishes to wait for connection close,
             * before being removed from list, move to inactive state
             * and notify */
            if (mCurrentSubscriber->mWaitForCloseComplete) {
                mCurrentSubscriber->mIsInactive = true;
            }
            else {
                /* Notify only current subscriber and then delete it from
                 * subscriberList */
                notifyEventToSubscriber(
                        AGPS_EVENT_UNSUBSCRIBE, mCurrentSubscriber, true);
            }

            /* If no subscribers in list, release data connection */
            if (mSubscriberList.empty()) {
                transitionState(AGPS_STATE_RELEASED);
                requestOrReleaseDataConn(false);
            }
            /* Some subscribers in list, but all inactive;
             * Release data connection */
            else if(!anyActiveSubscribers()) {
                transitionState(AGPS_STATE_RELEASING);
                requestOrReleaseDataConn(false);
            }
            break;

        case AGPS_STATE_RELEASING:
            /* If the subscriber wishes to wait for connection close,
             * before being removed from list, move to inactive state
             * and notify */
            if (mCurrentSubscriber->mWaitForCloseComplete) {
                mCurrentSubscriber->mIsInactive = true;
            }
            else {
                /* Notify only current subscriber and then delete it from
                 * subscriberList */
                notifyEventToSubscriber(
                        AGPS_EVENT_UNSUBSCRIBE, mCurrentSubscriber, true);
            }

            /* If no subscribers in list, just move the state.
             * Request for releasing data connection should already have been
             * sent */
            if (mSubscriberList.empty()) {
                transitionState(AGPS_STATE_RELEASED);
            }
            break;

        default:
            LOC_LOGE("Invalid state: %d", mState);
    }
}

void AgpsStateMachine::processAgpsEventGranted(){

    switch (mState) {

        case AGPS_STATE_RELEASED:
        case AGPS_STATE_ACQUIRED:
        case AGPS_STATE_RELEASING:
            LOC_LOGE("Unexpected event GRANTED in state %d", mState);
            break;

        case AGPS_STATE_PENDING:
            // Move to acquired state
            transitionState(AGPS_STATE_ACQUIRED);
            notifyAllSubscribers(
                    AGPS_EVENT_GRANTED, false,
                    AGPS_NOTIFICATION_TYPE_FOR_ACTIVE_SUBSCRIBERS);
            break;

        default:
            LOC_LOGE("Invalid state: %d", mState);
    }
}

void AgpsStateMachine::processAgpsEventReleased(){

    switch (mState) {

        case AGPS_STATE_RELEASED:
            /* Subscriber list should be empty if we are in released state */
            if (!mSubscriberList.empty()) {
                LOC_LOGE("Unexpected event RELEASED in RELEASED state");
            }
            break;

        case AGPS_STATE_ACQUIRED:
            /* Force release received */
            LOC_LOGW("Force RELEASED event in ACQUIRED state");
            transitionState(AGPS_STATE_RELEASED);
            notifyAllSubscribers(
                    AGPS_EVENT_RELEASED, true,
                    AGPS_NOTIFICATION_TYPE_FOR_ALL_SUBSCRIBERS);
            break;

        case AGPS_STATE_RELEASING:
            /* Notify all inactive subscribers about the event */
            notifyAllSubscribers(
                    AGPS_EVENT_RELEASED, true,
                    AGPS_NOTIFICATION_TYPE_FOR_INACTIVE_SUBSCRIBERS);

            /* If we have active subscribers now, they must be waiting for
             * data conn setup */
            if (anyActiveSubscribers()) {
                transitionState(AGPS_STATE_PENDING);
                requestOrReleaseDataConn(true);
            }
            /* No active subscribers, move to released state */
            else {
                transitionState(AGPS_STATE_RELEASED);
            }
            break;

        case AGPS_STATE_PENDING:
            /* NOOP */
            break;

        default:
            LOC_LOGE("Invalid state: %d", mState);
    }
}

void AgpsStateMachine::processAgpsEventDenied(){

    switch (mState) {

        case AGPS_STATE_RELEASED:
            LOC_LOGE("Unexpected event DENIED in state %d", mState);
            break;

        case AGPS_STATE_ACQUIRED:
            /* NOOP */
            break;

        case AGPS_STATE_RELEASING:
            /* Notify all inactive subscribers about the event */
            notifyAllSubscribers(
                    AGPS_EVENT_RELEASED, true,
                    AGPS_NOTIFICATION_TYPE_FOR_INACTIVE_SUBSCRIBERS);

            /* If we have active subscribers now, they must be waiting for
             * data conn setup */
            if (anyActiveSubscribers()) {
                transitionState(AGPS_STATE_PENDING);
                requestOrReleaseDataConn(true);
            }
            /* No active subscribers, move to released state */
            else {
                transitionState(AGPS_STATE_RELEASED);
            }
            break;

        case AGPS_STATE_PENDING:
            transitionState(AGPS_STATE_RELEASED);
            notifyAllSubscribers(
                    AGPS_EVENT_DENIED, true,
                    AGPS_NOTIFICATION_TYPE_FOR_ALL_SUBSCRIBERS);
            break;

        default:
            LOC_LOGE("Invalid state: %d", mState);
    }
}

/* Request or Release data connection
 * bool request :
 *      true  = Request data connection
 *      false = Release data connection */
int AgpsStateMachine::requestOrReleaseDataConn(bool request){

    AGnssExtStatusIpV4 nifRequest;
    memset(&nifRequest, 0, sizeof(nifRequest));

    nifRequest.type = mAgpsType;

    if (request) {
        LOC_LOGD("AGPS Data Conn Request");
        nifRequest.status = LOC_GPS_REQUEST_AGPS_DATA_CONN;
    }
    else{
        LOC_LOGD("AGPS Data Conn Release");
        nifRequest.status = LOC_GPS_RELEASE_AGPS_DATA_CONN;
    }

    mAgpsManager->mFrameworkStatusV4Cb(nifRequest);
    return 0;
}

void AgpsStateMachine::notifyAllSubscribers(
        AgpsEvent event, bool deleteSubscriberPostNotify,
        AgpsNotificationType notificationType){

    LOC_LOGD("notifyAllSubscribers(): "
            "SM %p, Event %d Delete %d Notification Type %d",
            this, event, deleteSubscriberPostNotify, notificationType);

    std::list<AgpsSubscriber*>::const_iterator it = mSubscriberList.begin();
    while ( it != mSubscriberList.end() ) {

        AgpsSubscriber* subscriber = *it;

        if (notificationType == AGPS_NOTIFICATION_TYPE_FOR_ALL_SUBSCRIBERS ||
                (notificationType == AGPS_NOTIFICATION_TYPE_FOR_INACTIVE_SUBSCRIBERS &&
                        subscriber->mIsInactive) ||
                (notificationType == AGPS_NOTIFICATION_TYPE_FOR_ACTIVE_SUBSCRIBERS &&
                        !subscriber->mIsInactive)) {

            /* Deleting via this call would require another traversal
             * through subscriber list, inefficient; hence pass in false*/
            notifyEventToSubscriber(event, subscriber, false);

            if (deleteSubscriberPostNotify) {
                it = mSubscriberList.erase(it);
                delete subscriber;
            } else {
                it++;
            }
        } else {
            it++;
        }
    }
}

void AgpsStateMachine::notifyEventToSubscriber(
        AgpsEvent event, AgpsSubscriber* subscriberToNotify,
        bool deleteSubscriberPostNotify) {

    LOC_LOGD("notifyEventToSubscriber(): "
            "SM %p, Event %d Subscriber %p Delete %d",
            this, event, subscriberToNotify, deleteSubscriberPostNotify);

    switch (event) {

        case AGPS_EVENT_GRANTED:
            mAgpsManager->mAtlOpenStatusCb(
                    subscriberToNotify->mConnHandle, 1, getAPN(),
                    getBearer(), mAgpsType);
            break;

        case AGPS_EVENT_DENIED:
            mAgpsManager->mAtlOpenStatusCb(
                    subscriberToNotify->mConnHandle, 0, getAPN(),
                    getBearer(), mAgpsType);
            break;

        case AGPS_EVENT_UNSUBSCRIBE:
        case AGPS_EVENT_RELEASED:
            mAgpsManager->mAtlCloseStatusCb(subscriberToNotify->mConnHandle, 1);
            break;

        default:
            LOC_LOGE("Invalid event %d", event);
    }

    /* Search this subscriber in list and delete */
    if (deleteSubscriberPostNotify) {
        deleteSubscriber(subscriberToNotify);
    }
}

void AgpsStateMachine::transitionState(AgpsState newState){

    LOC_LOGD("transitionState(): SM %p, old %d, new %d",
               this, mState, newState);

    mState = newState;

    // notify state transitions to all subscribers ?
}

void AgpsStateMachine::addSubscriber(AgpsSubscriber* subscriberToAdd){

    LOC_LOGD("addSubscriber(): SM %p, Subscriber %p",
               this, subscriberToAdd);

    // Check if subscriber is already present in the current list
    // If not, then add
    std::list<AgpsSubscriber*>::const_iterator it = mSubscriberList.begin();
    for (; it != mSubscriberList.end(); it++) {
        AgpsSubscriber* subscriber = *it;
        if (subscriber->equals(subscriberToAdd)) {
            LOC_LOGE("Subscriber already in list");
            return;
        }
    }

    AgpsSubscriber* cloned = subscriberToAdd->clone();
    LOC_LOGD("addSubscriber(): cloned subscriber: %p", cloned);
    mSubscriberList.push_back(cloned);
}

void AgpsStateMachine::deleteSubscriber(AgpsSubscriber* subscriberToDelete){

    LOC_LOGD("deleteSubscriber(): SM %p, Subscriber %p",
               this, subscriberToDelete);

    std::list<AgpsSubscriber*>::const_iterator it = mSubscriberList.begin();
    while ( it != mSubscriberList.end() ) {

        AgpsSubscriber* subscriber = *it;
        if (subscriber && subscriber->equals(subscriberToDelete)) {

            it = mSubscriberList.erase(it);
            delete subscriber;
        } else {
            it++;
        }
    }
}

bool AgpsStateMachine::anyActiveSubscribers(){

    std::list<AgpsSubscriber*>::const_iterator it = mSubscriberList.begin();
    for (; it != mSubscriberList.end(); it++) {
        AgpsSubscriber* subscriber = *it;
        if (!subscriber->mIsInactive) {
            return true;
        }
    }
    return false;
}

void AgpsStateMachine::setAPN(char* apn, unsigned int len){

    if (NULL != mAPN) {
        delete mAPN;
    }

    if (apn == NULL || len <= 0) {
        LOC_LOGD("Invalid apn len (%d) or null apn", len);
        mAPN = NULL;
        mAPNLen = 0;
    }

    if (NULL != apn) {
        mAPN = new char[len+1];
        if (NULL != mAPN) {
            memcpy(mAPN, apn, len);
            mAPN[len] = '\0';
            mAPNLen = len;
        }
    }
}

AgpsSubscriber* AgpsStateMachine::getSubscriber(int connHandle){

    /* Go over the subscriber list */
    std::list<AgpsSubscriber*>::const_iterator it = mSubscriberList.begin();
    for (; it != mSubscriberList.end(); it++) {
        AgpsSubscriber* subscriber = *it;
        if (subscriber->mConnHandle == connHandle) {
            return subscriber;
        }
    }

    /* Not found, return NULL */
    return NULL;
}

AgpsSubscriber* AgpsStateMachine::getFirstSubscriber(bool isInactive){

    /* Go over the subscriber list */
    std::list<AgpsSubscriber*>::const_iterator it = mSubscriberList.begin();
    for (; it != mSubscriberList.end(); it++) {
        AgpsSubscriber* subscriber = *it;
        if(subscriber->mIsInactive == isInactive) {
            return subscriber;
        }
    }

    /* Not found, return NULL */
    return NULL;
}

void AgpsStateMachine::dropAllSubscribers(){

    LOC_LOGD("dropAllSubscribers(): SM %p", this);

    /* Go over the subscriber list */
    std::list<AgpsSubscriber*>::const_iterator it = mSubscriberList.begin();
    while ( it != mSubscriberList.end() ) {
        AgpsSubscriber* subscriber = *it;
        it = mSubscriberList.erase(it);
        delete subscriber;
    }
}

/* --------------------------------------------------------------------
 *   DS State Machine Methods
 * -------------------------------------------------------------------*/
const int DSStateMachine::MAX_START_DATA_CALL_RETRIES = 4;
const int DSStateMachine::DATA_CALL_RETRY_DELAY_MSEC = 500;

/* Overridden method
 * DS SM needs to handle one scenario differently */
void DSStateMachine::processAgpsEvent(AgpsEvent event) {

    LOC_LOGD("DSStateMachine::processAgpsEvent() %d", event);

    /* DS Client call setup APIs don't return failure/closure separately.
     * Hence we receive RELEASED event in both cases.
     * If we are in pending, we should consider RELEASED as DENIED */
    if (event == AGPS_EVENT_RELEASED && mState == AGPS_STATE_PENDING) {

        LOC_LOGD("Translating RELEASED to DENIED event");
        event = AGPS_EVENT_DENIED;
    }

    /* Redirect process to base class */
    AgpsStateMachine::processAgpsEvent(event);
}

/* Timer Callback
 * For the retry timer started in case of DS Client call setup failure */
void delay_callback(void *callbackData, int result)
{
    LOC_LOGD("delay_callback(): cbData %p", callbackData);

    (void)result;

    if (callbackData == NULL) {
        LOC_LOGE("delay_callback(): NULL argument received !");
        return;
    }
    DSStateMachine* dsStateMachine = (DSStateMachine *)callbackData;
    dsStateMachine->retryCallback();
}

/* Invoked from Timer Callback
 * For the retry timer started in case of DS Client call setup failure */
void DSStateMachine :: retryCallback()
{
    LOC_LOGD("DSStateMachine::retryCallback()");

    /* Request SUPL ES
     * There must be at least one active subscriber in list */
    AgpsSubscriber* subscriber = getFirstSubscriber(false);
    if (subscriber == NULL) {

        LOC_LOGE("No active subscriber for DS Client call setup");
        return;
    }

    /* Send message to retry */
    mAgpsManager->mSendMsgToAdapterQueueFn(
            new AgpsMsgRequestATL(
                    mAgpsManager, subscriber->mConnHandle,
                    LOC_AGPS_TYPE_SUPL_ES));
}

/* Overridden method
 * Request or Release data connection
 * bool request :
 *      true  = Request data connection
 *      false = Release data connection */
int DSStateMachine::requestOrReleaseDataConn(bool request){

    LOC_LOGD("DSStateMachine::requestOrReleaseDataConn(): "
             "request %d", request);

    /* Release data connection required ? */
    if (!request && mAgpsManager->mDSClientStopDataCallFn) {

        mAgpsManager->mDSClientStopDataCallFn();
        LOC_LOGD("DS Client release data call request sent !");
        return 0;
    }

    /* Setup data connection request
     * There must be at least one active subscriber in list */
    AgpsSubscriber* subscriber = getFirstSubscriber(false);
    if (subscriber == NULL) {

        LOC_LOGE("No active subscriber for DS Client call setup");
        return -1;
    }

    /* DS Client Fn registered ? */
    if (!mAgpsManager->mDSClientOpenAndStartDataCallFn) {

        LOC_LOGE("DS Client start fn not registered, fallback to SUPL ATL");
        notifyEventToSubscriber(AGPS_EVENT_DENIED, subscriber, false);
        return -1;
    }

    /* Setup the call */
    int ret = mAgpsManager->mDSClientOpenAndStartDataCallFn();

    /* Check if data call start failed */
    switch (ret) {

        case LOC_API_ADAPTER_ERR_ENGINE_BUSY:
            LOC_LOGE("DS Client open call failed, err: %d", ret);
            mRetries++;
            if (mRetries > MAX_START_DATA_CALL_RETRIES) {

                LOC_LOGE("DS Client call retries exhausted, "
                         "falling back to normal SUPL ATL");
                notifyEventToSubscriber(AGPS_EVENT_DENIED, subscriber, false);
            }
            /* Retry DS Client call setup after some delay */
            else if(loc_timer_start(
                        DATA_CALL_RETRY_DELAY_MSEC, delay_callback, this)) {
                    LOC_LOGE("Error: Could not start delay thread\n");
                    return -1;
                }
            break;

        case LOC_API_ADAPTER_ERR_UNSUPPORTED:
            LOC_LOGE("No emergency profile found. Fall back to normal SUPL ATL");
            notifyEventToSubscriber(AGPS_EVENT_DENIED, subscriber, false);
            break;

        case LOC_API_ADAPTER_ERR_SUCCESS:
            LOC_LOGD("Request to start data call sent");
            break;

        default:
            LOC_LOGE("Unrecognized return value: %d", ret);
    }

    return ret;
}

void DSStateMachine::notifyEventToSubscriber(
        AgpsEvent event, AgpsSubscriber* subscriberToNotify,
        bool deleteSubscriberPostNotify) {

    LOC_LOGD("DSStateMachine::notifyEventToSubscriber(): "
             "SM %p, Event %d Subscriber %p Delete %d",
             this, event, subscriberToNotify, deleteSubscriberPostNotify);

    switch (event) {

        case AGPS_EVENT_GRANTED:
            mAgpsManager->mAtlOpenStatusCb(
                    subscriberToNotify->mConnHandle, 1, NULL,
                    AGPS_APN_BEARER_INVALID, LOC_AGPS_TYPE_SUPL_ES);
            break;

        case AGPS_EVENT_DENIED:
            /* Now try with regular SUPL
             * We need to send request via message queue */
            mRetries = 0;
            mAgpsManager->mSendMsgToAdapterQueueFn(
                    new AgpsMsgRequestATL(
                            mAgpsManager, subscriberToNotify->mConnHandle,
                            LOC_AGPS_TYPE_SUPL));
            break;

        case AGPS_EVENT_UNSUBSCRIBE:
            mAgpsManager->mAtlCloseStatusCb(subscriberToNotify->mConnHandle, 1);
            break;

        case AGPS_EVENT_RELEASED:
            mAgpsManager->mDSClientCloseDataCallFn();
            mAgpsManager->mAtlCloseStatusCb(subscriberToNotify->mConnHandle, 1);
            break;

        default:
            LOC_LOGE("Invalid event %d", event);
    }

    /* Search this subscriber in list and delete */
    if (deleteSubscriberPostNotify) {
        deleteSubscriber(subscriberToNotify);
    }
}

/* --------------------------------------------------------------------
 *   Loc AGPS Manager Methods
 * -------------------------------------------------------------------*/

/* CREATE AGPS STATE MACHINES
 * Must be invoked in Msg Handler context */
void AgpsManager::createAgpsStateMachines() {

    LOC_LOGD("AgpsManager::createAgpsStateMachines");

    bool agpsCapable =
            ((loc_core::ContextBase::mGps_conf.CAPABILITIES & LOC_GPS_CAPABILITY_MSA) ||
                    (loc_core::ContextBase::mGps_conf.CAPABILITIES & LOC_GPS_CAPABILITY_MSB));

    if (NULL == mInternetNif) {
        mInternetNif = new AgpsStateMachine(this, LOC_AGPS_TYPE_WWAN_ANY);
        LOC_LOGD("Internet NIF: %p", mInternetNif);
    }
    if (agpsCapable) {
        if (NULL == mAgnssNif) {
            mAgnssNif = new AgpsStateMachine(this, LOC_AGPS_TYPE_SUPL);
            LOC_LOGD("AGNSS NIF: %p", mAgnssNif);
        }
        if (NULL == mDsNif &&
                loc_core::ContextBase::mGps_conf.USE_EMERGENCY_PDN_FOR_EMERGENCY_SUPL) {

            if(!mDSClientInitFn){

                LOC_LOGE("DS Client Init Fn not registered !");
                return;
            }
            if (mDSClientInitFn(false) != 0) {

                LOC_LOGE("Failed to init data service client");
                return;
            }
            mDsNif = new DSStateMachine(this);
            LOC_LOGD("DS NIF: %p", mDsNif);
        }
    }
}

AgpsStateMachine* AgpsManager::getAgpsStateMachine(AGpsExtType agpsType) {

    LOC_LOGD("AgpsManager::getAgpsStateMachine(): agpsType %d", agpsType);

    switch (agpsType) {

        case LOC_AGPS_TYPE_INVALID:
        case LOC_AGPS_TYPE_SUPL:
            if (mAgnssNif == NULL) {
                LOC_LOGE("NULL AGNSS NIF !");
            }
            return mAgnssNif;

        case LOC_AGPS_TYPE_SUPL_ES:
            if (loc_core::ContextBase::mGps_conf.USE_EMERGENCY_PDN_FOR_EMERGENCY_SUPL) {
                if (mDsNif == NULL) {
                    createAgpsStateMachines();
                }
                return mDsNif;
            } else {
                return mAgnssNif;
            }

        default:
            return mInternetNif;
    }

    LOC_LOGE("No SM found !");
    return NULL;
}

void AgpsManager::requestATL(int connHandle, AGpsExtType agpsType){

    LOC_LOGD("AgpsManager::requestATL(): connHandle %d, agpsType %d",
               connHandle, agpsType);

    AgpsStateMachine* sm = getAgpsStateMachine(agpsType);

    if (sm == NULL) {

        LOC_LOGE("No AGPS State Machine for agpsType: %d", agpsType);
        mAtlOpenStatusCb(
                connHandle, 0, NULL, AGPS_APN_BEARER_INVALID, agpsType);
        return;
    }

    /* Invoke AGPS SM processing */
    AgpsSubscriber subscriber(connHandle, false, false);
    sm->setCurrentSubscriber(&subscriber);

    /* If DS State Machine, wait for close complete */
    if (agpsType == LOC_AGPS_TYPE_SUPL_ES) {
        subscriber.mWaitForCloseComplete = true;
    }

    /* Send subscriber event */
    sm->processAgpsEvent(AGPS_EVENT_SUBSCRIBE);
}

void AgpsManager::releaseATL(int connHandle){

    LOC_LOGD("AgpsManager::releaseATL(): connHandle %d", connHandle);

    /* First find the subscriber with specified handle.
     * We need to search in all state machines. */
    AgpsStateMachine* sm = NULL;
    AgpsSubscriber* subscriber = NULL;

    if (mAgnssNif &&
            (subscriber = mAgnssNif->getSubscriber(connHandle)) != NULL) {
        sm = mAgnssNif;
    }
    else if (mInternetNif &&
            (subscriber = mInternetNif->getSubscriber(connHandle)) != NULL) {
        sm = mInternetNif;
    }
    else if (mDsNif &&
            (subscriber = mDsNif->getSubscriber(connHandle)) != NULL) {
        sm = mDsNif;
    }

    if (sm == NULL) {
        LOC_LOGE("Subscriber with connHandle %d not found in any SM",
                    connHandle);
        mAtlCloseStatusCb(connHandle, 0);
        return;
    }

    /* Now send unsubscribe event */
    sm->setCurrentSubscriber(subscriber);
    sm->processAgpsEvent(AGPS_EVENT_UNSUBSCRIBE);
}

void AgpsManager::reportDataCallOpened(){

    LOC_LOGD("AgpsManager::reportDataCallOpened");

    if (mDsNif) {
        mDsNif->processAgpsEvent(AGPS_EVENT_GRANTED);
    }
}

void AgpsManager::reportDataCallClosed(){

    LOC_LOGD("AgpsManager::reportDataCallClosed");

    if (mDsNif) {
        mDsNif->processAgpsEvent(AGPS_EVENT_RELEASED);
    }
}

void AgpsManager::reportAtlOpenSuccess(
        AGpsExtType agpsType, char* apnName, int apnLen,
        AGpsBearerType bearerType){

    LOC_LOGD("AgpsManager::reportAtlOpenSuccess(): "
             "AgpsType %d, APN [%s], Len %d, BearerType %d",
             agpsType, apnName, apnLen, bearerType);

    /* Find the state machine instance */
    AgpsStateMachine* sm = getAgpsStateMachine(agpsType);

    /* Set bearer and apn info in state machine instance */
    sm->setBearer(bearerType);
    sm->setAPN(apnName, apnLen);

    /* Send GRANTED event to state machine */
    sm->processAgpsEvent(AGPS_EVENT_GRANTED);
}

void AgpsManager::reportAtlOpenFailed(AGpsExtType agpsType){

    LOC_LOGD("AgpsManager::reportAtlOpenFailed(): AgpsType %d", agpsType);

    /* Fetch SM and send DENIED event */
    AgpsStateMachine* sm = getAgpsStateMachine(agpsType);
    sm->processAgpsEvent(AGPS_EVENT_DENIED);
}

void AgpsManager::reportAtlClosed(AGpsExtType agpsType){

    LOC_LOGD("AgpsManager::reportAtlClosed(): AgpsType %d", agpsType);

    /* Fetch SM and send RELEASED event */
    AgpsStateMachine* sm = getAgpsStateMachine(agpsType);
    sm->processAgpsEvent(AGPS_EVENT_RELEASED);
}

void AgpsManager::handleModemSSR(){

    LOC_LOGD("AgpsManager::handleModemSSR");

    /* Drop subscribers from all state machines */
    if (mAgnssNif) {
        mAgnssNif->dropAllSubscribers();
    }
    if (mInternetNif) {
        mInternetNif->dropAllSubscribers();
    }
    if (mDsNif) {
        mDsNif->dropAllSubscribers();
    }

    // reinitialize DS client in SSR mode
    if (loc_core::ContextBase::mGps_conf.
            USE_EMERGENCY_PDN_FOR_EMERGENCY_SUPL) {

        mDSClientStopDataCallFn();
        mDSClientCloseDataCallFn();
        mDSClientReleaseFn();

        mDSClientInitFn(true);
    }
}
