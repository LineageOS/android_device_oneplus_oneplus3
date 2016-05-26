/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/******************************************************************************
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/
/*
 *  Communicate with a peer using NFC-DEP, LLCP, SNEP.
 */
#include "OverrideLog.h"
#include "PeerToPeer.h"
#include "NfcJniUtil.h"
#include "llcp_defs.h"
#include "config.h"
#include "JavaClassConstants.h"
#include <ScopedLocalRef.h>

/* Some older PN544-based solutions would only send the first SYMM back
 * (as an initiator) after the full LTO (750ms). But our connect timer
 * starts immediately, and hence we may timeout if the timer is set to
 * 1000 ms. Worse, this causes us to immediately connect to the NPP
 * socket, causing concurrency issues in that stack. Increase the default
 * timeout to 2000 ms, giving us enough time to complete the first connect.
 */
#define LLCP_DATA_LINK_TIMEOUT    2000

using namespace android;

namespace android
{
    extern void nativeNfcTag_registerNdefTypeHandler ();
    extern void nativeNfcTag_deregisterNdefTypeHandler ();
    extern int getScreenState();
    extern void startRfDiscovery(bool isStart);
    extern bool isDiscoveryStarted();
    extern int gGeneralPowershutDown;
}


PeerToPeer PeerToPeer::sP2p;
const std::string P2pServer::sSnepServiceName ("urn:nfc:sn:snep");


/*******************************************************************************
**
** Function:        PeerToPeer
**
** Description:     Initialize member variables.
**
** Returns:         None
**
*******************************************************************************/
PeerToPeer::PeerToPeer ()
:   mRemoteWKS (0),
    mIsP2pListening (false),
    mP2pListenTechMask (NFA_TECHNOLOGY_MASK_A
                        | NFA_TECHNOLOGY_MASK_F
                        | NFA_TECHNOLOGY_MASK_A_ACTIVE
                        | NFA_TECHNOLOGY_MASK_F_ACTIVE),
    mNextJniHandle (1)
{
    memset (mServers, 0, sizeof(mServers));
    memset (mClients, 0, sizeof(mClients));
}


/*******************************************************************************
**
** Function:        ~PeerToPeer
**
** Description:     Free all resources.
**
** Returns:         None
**
*******************************************************************************/
PeerToPeer::~PeerToPeer ()
{
}


/*******************************************************************************
**
** Function:        getInstance
**
** Description:     Get the singleton PeerToPeer object.
**
** Returns:         Singleton PeerToPeer object.
**
*******************************************************************************/
PeerToPeer& PeerToPeer::getInstance ()
{
    return sP2p;
}


/*******************************************************************************
**
** Function:        initialize
**
** Description:     Initialize member variables.
**
** Returns:         None
**
*******************************************************************************/
void PeerToPeer::initialize ()
{
    ALOGD ("PeerToPeer::initialize");
    unsigned long num = 0;

    if (GetNumValue ("P2P_LISTEN_TECH_MASK", &num, sizeof (num)))
        mP2pListenTechMask = num;
}


/*******************************************************************************
**
** Function:        findServerLocked
**
** Description:     Find a PeerToPeer object by connection handle.
**                  Assumes mMutex is already held
**                  nfaP2pServerHandle: Connectin handle.
**
** Returns:         PeerToPeer object.
**
*******************************************************************************/
sp<P2pServer> PeerToPeer::findServerLocked (tNFA_HANDLE nfaP2pServerHandle)
{
    for (int i = 0; i < sMax; i++)
    {
        if ( (mServers[i] != NULL)
          && (mServers[i]->mNfaP2pServerHandle == nfaP2pServerHandle) )
        {
            return (mServers [i]);
        }
    }

    // If here, not found
    return NULL;
}


/*******************************************************************************
**
** Function:        findServerLocked
**
** Description:     Find a PeerToPeer object by connection handle.
**                  Assumes mMutex is already held
**                  serviceName: service name.
**
** Returns:         PeerToPeer object.
**
*******************************************************************************/
sp<P2pServer> PeerToPeer::findServerLocked (tJNI_HANDLE jniHandle)
{
    for (int i = 0; i < sMax; i++)
    {
        if ( (mServers[i] != NULL)
          && (mServers[i]->mJniHandle == jniHandle) )
        {
            return (mServers [i]);
        }
    }

    // If here, not found
    return NULL;
}


/*******************************************************************************
**
** Function:        findServerLocked
**
** Description:     Find a PeerToPeer object by service name
**                  Assumes mMutex is already heldf
**                  serviceName: service name.
**
** Returns:         PeerToPeer object.
**
*******************************************************************************/
sp<P2pServer> PeerToPeer::findServerLocked (const char *serviceName)
{
    for (int i = 0; i < sMax; i++)
    {
        if ( (mServers[i] != NULL) && (mServers[i]->mServiceName.compare(serviceName) == 0) )
            return (mServers [i]);
    }

    // If here, not found
    return NULL;
}


/*******************************************************************************
**
** Function:        registerServer
**
** Description:     Let a server start listening for peer's connection request.
**                  jniHandle: Connection handle.
**                  serviceName: Server's service name.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool PeerToPeer::registerServer (tJNI_HANDLE jniHandle, const char *serviceName)
{
    static const char fn [] = "PeerToPeer::registerServer";
    ALOGD ("%s: enter; service name: %s  JNI handle: %u", fn, serviceName, jniHandle);
    sp<P2pServer>   pSrv = NULL;

    mMutex.lock();
    // Check if already registered
    if ((pSrv = findServerLocked(serviceName)) != NULL)
    {
        ALOGD ("%s: service name=%s  already registered, handle: 0x%04x", fn, serviceName, pSrv->mNfaP2pServerHandle);

        // Update JNI handle
        pSrv->mJniHandle = jniHandle;
        mMutex.unlock();
        return (true);
    }

    for (int ii = 0; ii < sMax; ii++)
    {
        if (mServers[ii] == NULL)
        {
            pSrv = mServers[ii] = new P2pServer(jniHandle, serviceName);

            ALOGD ("%s: added new p2p server  index: %d  handle: %u  name: %s", fn, ii, jniHandle, serviceName);
            break;
        }
    }
    mMutex.unlock();

    if (pSrv == NULL)
    {
        ALOGE ("%s: service name=%s  no free entry", fn, serviceName);
        return (false);
    }

    if (pSrv->registerWithStack()) {
        ALOGD ("%s: got new p2p server h=0x%X", fn, pSrv->mNfaP2pServerHandle);
        return (true);
    } else {
        ALOGE ("%s: invalid server handle", fn);
        removeServer (jniHandle);
        return (false);
    }
}


/*******************************************************************************
**
** Function:        removeServer
**
** Description:     Free resources related to a server.
**                  jniHandle: Connection handle.
**
** Returns:         None
**
*******************************************************************************/
void PeerToPeer::removeServer (tJNI_HANDLE jniHandle)
{
    static const char fn [] = "PeerToPeer::removeServer";

    AutoMutex mutex(mMutex);

    for (int i = 0; i < sMax; i++)
    {
        if ( (mServers[i] != NULL) && (mServers[i]->mJniHandle == jniHandle) )
        {
            ALOGD ("%s: server jni_handle: %u;  nfa_handle: 0x%04x; name: %s; index=%d",
                    fn, jniHandle, mServers[i]->mNfaP2pServerHandle, mServers[i]->mServiceName.c_str(), i);

            mServers [i] = NULL;
            return;
        }
    }
    ALOGE ("%s: unknown server jni handle: %u", fn, jniHandle);
}


/*******************************************************************************
**
** Function:        llcpActivatedHandler
**
** Description:     Receive LLLCP-activated event from stack.
**                  nat: JVM-related data.
**                  activated: Event data.
**
** Returns:         None
**
*******************************************************************************/
void PeerToPeer::llcpActivatedHandler (nfc_jni_native_data* nat, tNFA_LLCP_ACTIVATED& activated)
{
    static const char fn [] = "PeerToPeer::llcpActivatedHandler";
    ALOGD ("%s: enter", fn);

    //no longer need to receive NDEF message from a tag
    android::nativeNfcTag_deregisterNdefTypeHandler ();

    mRemoteWKS = activated.remote_wks;

    JNIEnv* e = NULL;
    ScopedAttach attach(nat->vm, &e);
    if (e == NULL)
    {
        ALOGE ("%s: jni env is null", fn);
        return;
    }

    ALOGD ("%s: get object class", fn);
    ScopedLocalRef<jclass> tag_cls(e, e->GetObjectClass(nat->cached_P2pDevice));
    if (e->ExceptionCheck()) {
        e->ExceptionClear();
        ALOGE ("%s: fail get p2p device", fn);
        return;
    }

    ALOGD ("%s: instantiate", fn);
    /* New target instance */
    jmethodID ctor = e->GetMethodID(tag_cls.get(), "<init>", "()V");
    ScopedLocalRef<jobject> tag(e, e->NewObject(tag_cls.get(), ctor));

    /* Set P2P Target mode */
    jfieldID f = e->GetFieldID(tag_cls.get(), "mMode", "I");

    if (activated.is_initiator == TRUE) {
        ALOGD ("%s: p2p initiator", fn);
        e->SetIntField(tag.get(), f, (jint) MODE_P2P_INITIATOR);
    } else {
        ALOGD ("%s: p2p target", fn);
        e->SetIntField(tag.get(), f, (jint) MODE_P2P_TARGET);
    }
    /* Set LLCP version */
    f = e->GetFieldID(tag_cls.get(), "mLlcpVersion", "B");
    e->SetByteField(tag.get(), f, (jbyte) activated.remote_version);

    /* Set tag handle */
    f = e->GetFieldID(tag_cls.get(), "mHandle", "I");
    e->SetIntField(tag.get(), f, (jint) 0x1234); // ?? This handle is not used for anything

    if (nat->tag != NULL) {
        e->DeleteGlobalRef(nat->tag);
    }
    nat->tag = e->NewGlobalRef(tag.get());

    ALOGD ("%s: notify nfc service", fn);

    /* Notify manager that new a P2P device was found */
    e->CallVoidMethod(nat->manager, android::gCachedNfcManagerNotifyLlcpLinkActivation, tag.get());
    if (e->ExceptionCheck()) {
        e->ExceptionClear();
        ALOGE ("%s: fail notify", fn);
    }

    ALOGD ("%s: exit", fn);
}


/*******************************************************************************
**
** Function:        llcpDeactivatedHandler
**
** Description:     Receive LLLCP-deactivated event from stack.
**                  nat: JVM-related data.
**                  deactivated: Event data.
**
** Returns:         None
**
*******************************************************************************/
void PeerToPeer::llcpDeactivatedHandler (nfc_jni_native_data* nat, tNFA_LLCP_DEACTIVATED& /*deactivated*/)
{
    static const char fn [] = "PeerToPeer::llcpDeactivatedHandler";
    ALOGD ("%s: enter", fn);

    JNIEnv* e = NULL;
    ScopedAttach attach(nat->vm, &e);
    if (e == NULL)
    {
        ALOGE ("%s: jni env is null", fn);
        return;
    }

    ALOGD ("%s: notify nfc service", fn);
    /* Notify manager that the LLCP is lost or deactivated */
    e->CallVoidMethod (nat->manager, android::gCachedNfcManagerNotifyLlcpLinkDeactivated, nat->tag);
    if (e->ExceptionCheck())
    {
        e->ExceptionClear();
        ALOGE ("%s: fail notify", fn);
    }

    //let the tag-reading code handle NDEF data event
    android::nativeNfcTag_registerNdefTypeHandler ();
    ALOGD ("%s: exit", fn);
}

void PeerToPeer::llcpFirstPacketHandler (nfc_jni_native_data* nat)
{
    static const char fn [] = "PeerToPeer::llcpFirstPacketHandler";
    ALOGD ("%s: enter", fn);

    JNIEnv* e = NULL;
    ScopedAttach attach(nat->vm, &e);
    if (e == NULL)
    {
        ALOGE ("%s: jni env is null", fn);
        return;
    }

    ALOGD ("%s: notify nfc service", fn);
    /* Notify manager that the LLCP is lost or deactivated */
    e->CallVoidMethod (nat->manager, android::gCachedNfcManagerNotifyLlcpFirstPacketReceived, nat->tag);
    if (e->ExceptionCheck())
    {
        e->ExceptionClear();
        ALOGE ("%s: fail notify", fn);
    }

    ALOGD ("%s: exit", fn);

}
/*******************************************************************************
**
** Function:        accept
**
** Description:     Accept a peer's request to connect.
**                  serverJniHandle: Server's handle.
**                  connJniHandle: Connection handle.
**                  maxInfoUnit: Maximum information unit.
**                  recvWindow: Receive window size.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool PeerToPeer::accept (tJNI_HANDLE serverJniHandle, tJNI_HANDLE connJniHandle, int maxInfoUnit, int recvWindow)
{
    static const char fn [] = "PeerToPeer::accept";
    sp<P2pServer> pSrv = NULL;

    ALOGD ("%s: enter; server jni handle: %u; conn jni handle: %u; maxInfoUnit: %d; recvWindow: %d", fn,
            serverJniHandle, connJniHandle, maxInfoUnit, recvWindow);

    mMutex.lock();
    if ((pSrv = findServerLocked (serverJniHandle)) == NULL)
    {
        ALOGE ("%s: unknown server jni handle: %u", fn, serverJniHandle);
        mMutex.unlock();
        return (false);
    }
    mMutex.unlock();

    return pSrv->accept(serverJniHandle, connJniHandle, maxInfoUnit, recvWindow);
}


/*******************************************************************************
**
** Function:        deregisterServer
**
** Description:     Stop a P2pServer from listening for peer.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool PeerToPeer::deregisterServer (tJNI_HANDLE jniHandle)
{
    static const char fn [] = "PeerToPeer::deregisterServer";
    ALOGD ("%s: enter; JNI handle: %u", fn, jniHandle);
    tNFA_STATUS     nfaStat = NFA_STATUS_FAILED;
    sp<P2pServer>   pSrv = NULL;

    bool isDiscStopped = false;
    mMutex.lock();
    if ((pSrv = findServerLocked (jniHandle)) == NULL)
    {
        ALOGE ("%s: unknown service handle: %u", fn, jniHandle);
        mMutex.unlock();
        return (false);
    }
    mMutex.unlock();
    if(isDiscoveryStarted())
    {
        isDiscStopped = true;
        startRfDiscovery(false);
    }

    //Check if discovery  is started
    if(isDiscoveryStarted())
    {

        startRfDiscovery(false);
    }

    {
        // Server does not call NFA_P2pDisconnect(), so unblock the accept()
        SyncEventGuard guard (pSrv->mConnRequestEvent);
        pSrv->mConnRequestEvent.notifyOne();
    }

    nfaStat = NFA_P2pDeregister (pSrv->mNfaP2pServerHandle);
    if (nfaStat != NFA_STATUS_OK)
    {
        ALOGE ("%s: deregister error=0x%X", fn, nfaStat);
    }

    removeServer (jniHandle);

    /*
     * conditional check is added to avoid multiple dicovery cmds
     * at the time of NFC OFF in progress
     */
    if((gGeneralPowershutDown != NFC_MODE_OFF) && isDiscStopped == true)
    {
        startRfDiscovery(true);
    }

    ALOGD ("%s: exit", fn);
    return true;
}


/*******************************************************************************
**
** Function:        createClient
**
** Description:     Create a P2pClient object for a new out-bound connection.
**                  jniHandle: Connection handle.
**                  miu: Maximum information unit.
**                  rw: Receive window size.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool PeerToPeer::createClient (tJNI_HANDLE jniHandle, UINT16 miu, UINT8 rw)
{
    static const char fn [] = "PeerToPeer::createClient";
    int i = 0;
    ALOGD ("%s: enter: jni h: %u  miu: %u  rw: %u", fn, jniHandle, miu, rw);

    mMutex.lock();
    sp<P2pClient> client = NULL;
    for (i = 0; i < sMax; i++)
    {
        if (mClients[i] == NULL)
        {
            mClients [i] = client = new P2pClient();

            mClients [i]->mClientConn->mJniHandle   = jniHandle;
            mClients [i]->mClientConn->mMaxInfoUnit = miu;
            mClients [i]->mClientConn->mRecvWindow  = rw;
            break;
        }
    }
    mMutex.unlock();

    if (client == NULL || i >=sMax)
    {
        ALOGE ("%s: fail", fn);
        return (false);
    }

    ALOGD ("%s: pClient: 0x%p  assigned for client jniHandle: %u", fn, client.get(), jniHandle);

    {
        SyncEventGuard guard (mClients[i]->mRegisteringEvent);
        NFA_P2pRegisterClient (NFA_P2P_DLINK_TYPE, nfaClientCallback);
        mClients[i]->mRegisteringEvent.wait(); //wait for NFA_P2P_REG_CLIENT_EVT
    }

    if (mClients[i]->mNfaP2pClientHandle != NFA_HANDLE_INVALID)
    {
        ALOGD ("%s: exit; new client jniHandle: %u   NFA Handle: 0x%04x", fn, jniHandle, client->mClientConn->mNfaConnHandle);
        return (true);
    }
    else
    {
        ALOGE ("%s: FAILED; new client jniHandle: %u   NFA Handle: 0x%04x", fn, jniHandle, client->mClientConn->mNfaConnHandle);
        removeConn (jniHandle);
        return (false);
    }
}


/*******************************************************************************
**
** Function:        removeConn
**
** Description:     Free resources related to a connection.
**                  jniHandle: Connection handle.
**
** Returns:         None
**
*******************************************************************************/
void PeerToPeer::removeConn(tJNI_HANDLE jniHandle)
{
    static const char fn[] = "PeerToPeer::removeConn";

    AutoMutex mutex(mMutex);
    // If the connection is a for a client, delete the client itself
    for (int ii = 0; ii < sMax; ii++)
    {
        if ((mClients[ii] != NULL) && (mClients[ii]->mClientConn->mJniHandle == jniHandle))
        {
            if (mClients[ii]->mNfaP2pClientHandle != NFA_HANDLE_INVALID)
                NFA_P2pDeregister (mClients[ii]->mNfaP2pClientHandle);

            mClients[ii] = NULL;
            ALOGD ("%s: deleted client handle: %u  index: %u", fn, jniHandle, ii);
            return;
        }
    }

    // If the connection is for a server, just delete the connection
    for (int ii = 0; ii < sMax; ii++)
    {
        if (mServers[ii] != NULL)
        {
            if (mServers[ii]->removeServerConnection(jniHandle)) {
                return;
            }
        }
    }

    ALOGE ("%s: could not find handle: %u", fn, jniHandle);
}


/*******************************************************************************
**
** Function:        connectConnOriented
**
** Description:     Establish a connection-oriented connection to a peer.
**                  jniHandle: Connection handle.
**                  serviceName: Peer's service name.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool PeerToPeer::connectConnOriented (tJNI_HANDLE jniHandle, const char* serviceName)
{
    static const char fn [] = "PeerToPeer::connectConnOriented";
    ALOGD ("%s: enter; h: %u  service name=%s", fn, jniHandle, serviceName);
    bool stat = createDataLinkConn (jniHandle, serviceName, 0);
    ALOGD ("%s: exit; h: %u  stat: %u", fn, jniHandle, stat);
    return stat;
}


/*******************************************************************************
**
** Function:        connectConnOriented
**
** Description:     Establish a connection-oriented connection to a peer.
**                  jniHandle: Connection handle.
**                  destinationSap: Peer's service access point.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool PeerToPeer::connectConnOriented (tJNI_HANDLE jniHandle, UINT8 destinationSap)
{
    static const char fn [] = "PeerToPeer::connectConnOriented";
    ALOGD ("%s: enter; h: %u  dest sap: 0x%X", fn, jniHandle, destinationSap);
    bool stat = createDataLinkConn (jniHandle, NULL, destinationSap);
    ALOGD ("%s: exit; h: %u  stat: %u", fn, jniHandle, stat);
    return stat;
}


/*******************************************************************************
**
** Function:        createDataLinkConn
**
** Description:     Establish a connection-oriented connection to a peer.
**                  jniHandle: Connection handle.
**                  serviceName: Peer's service name.
**                  destinationSap: Peer's service access point.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool PeerToPeer::createDataLinkConn (tJNI_HANDLE jniHandle, const char* serviceName, UINT8 destinationSap)
{
    static const char fn [] = "PeerToPeer::createDataLinkConn";
    ALOGD ("%s: enter", fn);
    tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
    sp<P2pClient>   pClient = NULL;

    if ((pClient = findClient (jniHandle)) == NULL)
    {
        ALOGE ("%s: can't find client, JNI handle: %u", fn, jniHandle);
        return (false);
    }

    {
        SyncEventGuard guard (pClient->mConnectingEvent);
        pClient->mIsConnecting = true;

        if (serviceName)
            nfaStat = NFA_P2pConnectByName (pClient->mNfaP2pClientHandle,
                    const_cast<char*>(serviceName), pClient->mClientConn->mMaxInfoUnit,
                    pClient->mClientConn->mRecvWindow);
        else if (destinationSap)
            nfaStat = NFA_P2pConnectBySap (pClient->mNfaP2pClientHandle, destinationSap,
                    pClient->mClientConn->mMaxInfoUnit, pClient->mClientConn->mRecvWindow);
        if (nfaStat == NFA_STATUS_OK)
        {
            ALOGD ("%s: wait for connected event  mConnectingEvent: 0x%p", fn, pClient.get());
            pClient->mConnectingEvent.wait();
        }
    }

    if (nfaStat == NFA_STATUS_OK)
    {
        if (pClient->mClientConn->mNfaConnHandle == NFA_HANDLE_INVALID)
        {
            removeConn (jniHandle);
            nfaStat = NFA_STATUS_FAILED;
        }
        else
            pClient->mIsConnecting = false;
    }
    else
    {
        removeConn (jniHandle);
        ALOGE ("%s: fail; error=0x%X", fn, nfaStat);
    }

    ALOGD ("%s: exit", fn);
    return nfaStat == NFA_STATUS_OK;
}


/*******************************************************************************
**
** Function:        findClient
**
** Description:     Find a PeerToPeer object with a client connection handle.
**                  nfaConnHandle: Connection handle.
**
** Returns:         PeerToPeer object.
**
*******************************************************************************/
sp<P2pClient> PeerToPeer::findClient (tNFA_HANDLE nfaConnHandle)
{
    AutoMutex mutex(mMutex);
    for (int i = 0; i < sMax; i++)
    {
        if ((mClients[i] != NULL) && (mClients[i]->mNfaP2pClientHandle == nfaConnHandle))
            return (mClients[i]);
    }
    return (NULL);
}


/*******************************************************************************
**
** Function:        findClient
**
** Description:     Find a PeerToPeer object with a client connection handle.
**                  jniHandle: Connection handle.
**
** Returns:         PeerToPeer object.
**
*******************************************************************************/
sp<P2pClient> PeerToPeer::findClient (tJNI_HANDLE jniHandle)
{
    AutoMutex mutex(mMutex);
    for (int i = 0; i < sMax; i++)
    {
        if ((mClients[i] != NULL) && (mClients[i]->mClientConn->mJniHandle == jniHandle))
            return (mClients[i]);
    }
    return (NULL);
}


/*******************************************************************************
**
** Function:        findClientCon
**
** Description:     Find a PeerToPeer object with a client connection handle.
**                  nfaConnHandle: Connection handle.
**
** Returns:         PeerToPeer object.
**
*******************************************************************************/
sp<P2pClient> PeerToPeer::findClientCon (tNFA_HANDLE nfaConnHandle)
{
    AutoMutex mutex(mMutex);
    for (int i = 0; i < sMax; i++)
    {
        if ((mClients[i] != NULL) && (mClients[i]->mClientConn->mNfaConnHandle == nfaConnHandle))
            return (mClients[i]);
    }
    return (NULL);
}


/*******************************************************************************
**
** Function:        findConnection
**
** Description:     Find a PeerToPeer object with a connection handle.
**                  nfaConnHandle: Connection handle.
**
** Returns:         PeerToPeer object.
**
*******************************************************************************/
sp<NfaConn> PeerToPeer::findConnection (tNFA_HANDLE nfaConnHandle)
{
    AutoMutex mutex(mMutex);
    // First, look through all the client control blocks
    for (int ii = 0; ii < sMax; ii++)
    {
        if ( (mClients[ii] != NULL)
           && (mClients[ii]->mClientConn->mNfaConnHandle == nfaConnHandle) ) {
            return mClients[ii]->mClientConn;
        }
    }

    // Not found yet. Look through all the server control blocks
    for (int ii = 0; ii < sMax; ii++)
    {
        if (mServers[ii] != NULL)
        {
            sp<NfaConn> conn = mServers[ii]->findServerConnection(nfaConnHandle);
            if (conn != NULL) {
                return conn;
            }
        }
    }

    // Not found...
    return NULL;
}


/*******************************************************************************
**
** Function:        findConnection
**
** Description:     Find a PeerToPeer object with a connection handle.
**                  jniHandle: Connection handle.
**
** Returns:         PeerToPeer object.
**
*******************************************************************************/
sp<NfaConn> PeerToPeer::findConnection (tJNI_HANDLE jniHandle)
{
    AutoMutex mutex(mMutex);
    // First, look through all the client control blocks
    for (int ii = 0; ii < sMax; ii++)
    {
        if ( (mClients[ii] != NULL)
          && (mClients[ii]->mClientConn->mJniHandle == jniHandle) ) {
            return mClients[ii]->mClientConn;
        }
    }

    // Not found yet. Look through all the server control blocks
    for (int ii = 0; ii < sMax; ii++)
    {
        if (mServers[ii] != NULL)
        {
            sp<NfaConn> conn = mServers[ii]->findServerConnection(jniHandle);
            if (conn != NULL) {
                return conn;
            }
        }
    }

    // Not found...
    return NULL;
}


/*******************************************************************************
**
** Function:        send
**
** Description:     Send data to peer.
**                  jniHandle: Handle of connection.
**                  buffer: Buffer of data.
**                  bufferLen: Length of data.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool PeerToPeer::send (tJNI_HANDLE jniHandle, UINT8 *buffer, UINT16 bufferLen)
{
    static const char fn [] = "PeerToPeer::send";
    tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
    sp<NfaConn>     pConn =  NULL;

    if ((pConn = findConnection (jniHandle)) == NULL)
    {
        ALOGE ("%s: can't find connection handle: %u", fn, jniHandle);
        return (false);
    }

    ALOGD_IF ((appl_trace_level>=BT_TRACE_LEVEL_DEBUG), "%s: send data; jniHandle: %u  nfaHandle: 0x%04X",
            fn, pConn->mJniHandle, pConn->mNfaConnHandle);

    while (true)
    {
        SyncEventGuard guard (pConn->mCongEvent);
        nfaStat = NFA_P2pSendData (pConn->mNfaConnHandle, bufferLen, buffer);
        if (nfaStat == NFA_STATUS_CONGESTED)
            pConn->mCongEvent.wait (); //wait for NFA_P2P_CONGEST_EVT
        else
            break;

        if (pConn->mNfaConnHandle == NFA_HANDLE_INVALID) //peer already disconnected
        {
            ALOGD_IF ((appl_trace_level>=BT_TRACE_LEVEL_DEBUG), "%s: peer disconnected", fn);
            return (false);
        }
    }

    if (nfaStat == NFA_STATUS_OK)
        ALOGD_IF ((appl_trace_level>=BT_TRACE_LEVEL_DEBUG), "%s: exit OK; JNI handle: %u  NFA Handle: 0x%04x", fn, jniHandle, pConn->mNfaConnHandle);
    else
        ALOGE ("%s: Data not sent; JNI handle: %u  NFA Handle: 0x%04x  error: 0x%04x",
              fn, jniHandle, pConn->mNfaConnHandle, nfaStat);

    return nfaStat == NFA_STATUS_OK;
}


/*******************************************************************************
**
** Function:        receive
**
** Description:     Receive data from peer.
**                  jniHandle: Handle of connection.
**                  buffer: Buffer to store data.
**                  bufferLen: Max length of buffer.
**                  actualLen: Actual length received.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool PeerToPeer::receive (tJNI_HANDLE jniHandle, UINT8* buffer, UINT16 bufferLen, UINT16& actualLen)
{
    static const char fn [] = "PeerToPeer::receive";
    ALOGD_IF ((appl_trace_level>=BT_TRACE_LEVEL_DEBUG), "%s: enter; jniHandle: %u  bufferLen: %u", fn, jniHandle, bufferLen);
    sp<NfaConn> pConn = NULL;
    tNFA_STATUS stat = NFA_STATUS_FAILED;
    UINT32 actualDataLen2 = 0;
    BOOLEAN isMoreData = TRUE;
    bool retVal = false;

    if ((pConn = findConnection (jniHandle)) == NULL)
    {
        ALOGE ("%s: can't find connection handle: %u", fn, jniHandle);
        return (false);
    }

    ALOGD_IF ((appl_trace_level>=BT_TRACE_LEVEL_DEBUG), "%s: jniHandle: %u  nfaHandle: 0x%04X  buf len=%u", fn, pConn->mJniHandle, pConn->mNfaConnHandle, bufferLen);

    while (pConn->mNfaConnHandle != NFA_HANDLE_INVALID)
    {
        //NFA_P2pReadData() is synchronous
        stat = NFA_P2pReadData (pConn->mNfaConnHandle, bufferLen, &actualDataLen2, buffer, &isMoreData);
        if ((stat == NFA_STATUS_OK) && (actualDataLen2 > 0)) //received some data
        {
            actualLen = (UINT16) actualDataLen2;
            retVal = true;
            break;
        }
        ALOGD_IF ((appl_trace_level>=BT_TRACE_LEVEL_DEBUG), "%s: waiting for data...", fn);
        {
            SyncEventGuard guard (pConn->mReadEvent);
            pConn->mReadEvent.wait();
        }
    } //while

    ALOGD_IF ((appl_trace_level>=BT_TRACE_LEVEL_DEBUG), "%s: exit; nfa h: 0x%X  ok: %u  actual len: %u", fn, pConn->mNfaConnHandle, retVal, actualLen);
    return retVal;
}


/*******************************************************************************
**
** Function:        disconnectConnOriented
**
** Description:     Disconnect a connection-oriented connection with peer.
**                  jniHandle: Handle of connection.
**
** Returns:         True if ok.
**
*******************************************************************************/
bool PeerToPeer::disconnectConnOriented (tJNI_HANDLE jniHandle)
{
    static const char fn [] = "PeerToPeer::disconnectConnOriented";
    tNFA_STATUS nfaStat = NFA_STATUS_FAILED;
    sp<P2pClient>   pClient = NULL;
    sp<NfaConn>     pConn = NULL;

    ALOGD ("%s: enter; jni handle: %u", fn, jniHandle);

    if ((pConn = findConnection(jniHandle)) == NULL)
    {
        ALOGE ("%s: can't find connection handle: %u", fn, jniHandle);
        return (false);
    }

    // If this is a client, he may not be connected yet, so unblock him just in case
    if ( ((pClient = findClient(jniHandle)) != NULL) && (pClient->mIsConnecting) )
    {
        SyncEventGuard guard (pClient->mConnectingEvent);
        pClient->mConnectingEvent.notifyOne();
        return (true);
    }

    {
        SyncEventGuard guard1 (pConn->mCongEvent);
        pConn->mCongEvent.notifyOne (); //unblock send() if congested
    }
    {
        SyncEventGuard guard2 (pConn->mReadEvent);
        pConn->mReadEvent.notifyOne (); //unblock receive()
    }

    if (pConn->mNfaConnHandle != NFA_HANDLE_INVALID)
    {
        ALOGD ("%s: try disconn nfa h=0x%04X", fn, pConn->mNfaConnHandle);
        SyncEventGuard guard (pConn->mDisconnectingEvent);
        nfaStat = NFA_P2pDisconnect (pConn->mNfaConnHandle, FALSE);

        if (nfaStat != NFA_STATUS_OK)
            ALOGE ("%s: fail p2p disconnect", fn);
        else
            pConn->mDisconnectingEvent.wait();
    }

    mDisconnectMutex.lock ();
    removeConn (jniHandle);
    mDisconnectMutex.unlock ();

    ALOGD ("%s: exit; jni handle: %u", fn, jniHandle);
    return nfaStat == NFA_STATUS_OK;
}


/*******************************************************************************
**
** Function:        getRemoteMaxInfoUnit
**
** Description:     Get peer's max information unit.
**                  jniHandle: Handle of the connection.
**
** Returns:         Peer's max information unit.
**
*******************************************************************************/
UINT16 PeerToPeer::getRemoteMaxInfoUnit (tJNI_HANDLE jniHandle)
{
    static const char fn [] = "PeerToPeer::getRemoteMaxInfoUnit";
    sp<NfaConn> pConn = NULL;

    if ((pConn = findConnection(jniHandle)) == NULL)
    {
        ALOGE ("%s: can't find client  jniHandle: %u", fn, jniHandle);
        return 0;
    }
    ALOGD ("%s: jniHandle: %u   MIU: %u", fn, jniHandle, pConn->mRemoteMaxInfoUnit);
    return (pConn->mRemoteMaxInfoUnit);
}


/*******************************************************************************
**
** Function:        getRemoteRecvWindow
**
** Description:     Get peer's receive window size.
**                  jniHandle: Handle of the connection.
**
** Returns:         Peer's receive window size.
**
*******************************************************************************/
UINT8 PeerToPeer::getRemoteRecvWindow (tJNI_HANDLE jniHandle)
{
    static const char fn [] = "PeerToPeer::getRemoteRecvWindow";
    ALOGD ("%s: client jni handle: %u", fn, jniHandle);
    sp<NfaConn> pConn = NULL;

    if ((pConn = findConnection(jniHandle)) == NULL)
    {
        ALOGE ("%s: can't find client", fn);
        return 0;
    }
    return pConn->mRemoteRecvWindow;
}

/*******************************************************************************
**
** Function:        setP2pListenMask
**
** Description:     Sets the p2p listen technology mask.
**                  p2pListenMask: the p2p listen mask to be set?
**
** Returns:         None
**
*******************************************************************************/
void PeerToPeer::setP2pListenMask (tNFA_TECHNOLOGY_MASK p2pListenMask) {
    mP2pListenTechMask = p2pListenMask;
}


/*******************************************************************************
**
** Function:        getP2pListenMask
**
** Description:     Get the set of technologies that P2P is listening.
**
** Returns:         Set of technologies.
**
*******************************************************************************/
tNFA_TECHNOLOGY_MASK PeerToPeer::getP2pListenMask ()
{
    return mP2pListenTechMask;
}


/*******************************************************************************
**
** Function:        resetP2pListenMask
**
** Description:     Reset the p2p listen technology mask to initial value.
**
** Returns:         None.
**
*******************************************************************************/
void PeerToPeer::resetP2pListenMask ()
{
    unsigned long num = 0;
    mP2pListenTechMask = NFA_TECHNOLOGY_MASK_A
                        | NFA_TECHNOLOGY_MASK_F
                        | NFA_TECHNOLOGY_MASK_A_ACTIVE
                        | NFA_TECHNOLOGY_MASK_F_ACTIVE;
    if (GetNumValue ("P2P_LISTEN_TECH_MASK", &num, sizeof (num)))
        mP2pListenTechMask = num;
}


/*******************************************************************************
**
** Function:        enableP2pListening
**
** Description:     Start/stop polling/listening to peer that supports P2P.
**                  isEnable: Is enable polling/listening?
**
** Returns:         None
**
*******************************************************************************/
void PeerToPeer::enableP2pListening (bool isEnable)
{
    static const char    fn []   = "PeerToPeer::enableP2pListening";
    tNFA_STATUS          nfaStat = NFA_STATUS_FAILED;

    ALOGD ("%s: enter isEnable: %u  mIsP2pListening: %u", fn, isEnable, mIsP2pListening);

    // If request to enable P2P listening, and we were not already listening
    if ( (isEnable == true) && (mIsP2pListening == false) && (mP2pListenTechMask != 0) )
    {
        SyncEventGuard guard (mSetTechEvent);
        if ((nfaStat = NFA_SetP2pListenTech (mP2pListenTechMask)) == NFA_STATUS_OK)
        {
            mSetTechEvent.wait ();
            mIsP2pListening = true;
        }
        else
            ALOGE ("%s: fail enable listen; error=0x%X", fn, nfaStat);
    }
    else if ( (isEnable == false) && (mIsP2pListening == true) )
    {
        SyncEventGuard guard (mSetTechEvent);
        // Request to disable P2P listening, check if it was enabled
        if ((nfaStat = NFA_SetP2pListenTech(0)) == NFA_STATUS_OK)
        {
            mSetTechEvent.wait ();
            mIsP2pListening = false;
        }
        else
            ALOGE ("%s: fail disable listen; error=0x%X", fn, nfaStat);
    }
    ALOGD ("%s: exit; mIsP2pListening: %u", fn, mIsP2pListening);
}

/*******************************************************************************
**
** Function:        handleNfcOnOff
**
** Description:     Handle events related to turning NFC on/off by the user.
**                  isOn: Is NFC turning on?
**
** Returns:         None
**
*******************************************************************************/
void PeerToPeer::handleNfcOnOff (bool isOn)
{
    static const char fn [] = "PeerToPeer::handleNfcOnOff";
    ALOGD ("%s: enter; is on=%u", fn, isOn);

    mIsP2pListening = false;            // In both cases, P2P will not be listening

    AutoMutex mutex(mMutex);
    if (isOn)
    {
        // Start with no clients or servers
        memset (mServers, 0, sizeof(mServers));
        memset (mClients, 0, sizeof(mClients));
    }
    else
    {
        // Disconnect through all the clients
        for (int ii = 0; ii < sMax; ii++)
        {
            if (mClients[ii] != NULL)
            {
                if (mClients[ii]->mClientConn->mNfaConnHandle == NFA_HANDLE_INVALID)
                {
                    SyncEventGuard guard (mClients[ii]->mConnectingEvent);
                    mClients[ii]->mConnectingEvent.notifyOne();
                }
                else
                {
                    mClients[ii]->mClientConn->mNfaConnHandle = NFA_HANDLE_INVALID;
                    {
                        SyncEventGuard guard1 (mClients[ii]->mClientConn->mCongEvent);
                        mClients[ii]->mClientConn->mCongEvent.notifyOne (); //unblock send()
                    }
                    {
                        SyncEventGuard guard2 (mClients[ii]->mClientConn->mReadEvent);
                        mClients[ii]->mClientConn->mReadEvent.notifyOne (); //unblock receive()
                    }
                }
            }
        } //loop

        // Now look through all the server control blocks
        for (int ii = 0; ii < sMax; ii++)
        {
            if (mServers[ii] != NULL)
            {
                mServers[ii]->unblockAll();
            }
        } //loop

    }
    ALOGD ("%s: exit", fn);
}


/*******************************************************************************
**
** Function:        nfaServerCallback
**
** Description:     Receive LLCP-related events from the stack.
**                  p2pEvent: Event code.
**                  eventData: Event data.
**
** Returns:         None
**
*******************************************************************************/
void PeerToPeer::nfaServerCallback (tNFA_P2P_EVT p2pEvent, tNFA_P2P_EVT_DATA* eventData)
{
    static const char fn [] = "PeerToPeer::nfaServerCallback";
    sp<P2pServer>   pSrv = NULL;
    sp<NfaConn>     pConn = NULL;

    ALOGD_IF ((appl_trace_level>=BT_TRACE_LEVEL_DEBUG), "%s: enter; event=0x%X", fn, p2pEvent);

    switch (p2pEvent)
    {
    case NFA_P2P_REG_SERVER_EVT:  // NFA_P2pRegisterServer() has started to listen
        ALOGD ("%s: NFA_P2P_REG_SERVER_EVT; handle: 0x%04x; service sap=0x%02x  name: %s", fn,
              eventData->reg_server.server_handle, eventData->reg_server.server_sap, eventData->reg_server.service_name);

        sP2p.mMutex.lock();
        pSrv = sP2p.findServerLocked(eventData->reg_server.service_name);
        sP2p.mMutex.unlock();
        if (pSrv == NULL)
        {
            ALOGE ("%s: NFA_P2P_REG_SERVER_EVT for unknown service: %s", fn, eventData->reg_server.service_name);
        }
        else
        {
            SyncEventGuard guard (pSrv->mRegServerEvent);
            pSrv->mNfaP2pServerHandle = eventData->reg_server.server_handle;
            pSrv->mRegServerEvent.notifyOne(); //unblock registerServer()
        }
        break;

    case NFA_P2P_ACTIVATED_EVT: //remote device has activated
        ALOGD ("%s: NFA_P2P_ACTIVATED_EVT; handle: 0x%04x", fn, eventData->activated.handle);
        break;

    case NFA_P2P_DEACTIVATED_EVT:
        ALOGD ("%s: NFA_P2P_DEACTIVATED_EVT; handle: 0x%04x", fn, eventData->activated.handle);
        break;

    case NFA_P2P_CONN_REQ_EVT:
        ALOGD ("%s: NFA_P2P_CONN_REQ_EVT; nfa server h=0x%04x; nfa conn h=0x%04x; remote sap=0x%02x", fn,
                eventData->conn_req.server_handle, eventData->conn_req.conn_handle, eventData->conn_req.remote_sap);

        sP2p.mMutex.lock();
        pSrv = sP2p.findServerLocked(eventData->conn_req.server_handle);
        sP2p.mMutex.unlock();
        if (pSrv == NULL)
        {
            ALOGE ("%s: NFA_P2P_CONN_REQ_EVT; unknown server h", fn);
            return;
        }
        ALOGD ("%s: NFA_P2P_CONN_REQ_EVT; server jni h=%u", fn, pSrv->mJniHandle);

        // Look for a connection block that is waiting (handle invalid)
        if ((pConn = pSrv->findServerConnection((tNFA_HANDLE) NFA_HANDLE_INVALID)) == NULL)
        {
            ALOGE ("%s: NFA_P2P_CONN_REQ_EVT; server not listening", fn);
        }
        else
        {
            SyncEventGuard guard (pSrv->mConnRequestEvent);
            pConn->mNfaConnHandle = eventData->conn_req.conn_handle;
            pConn->mRemoteMaxInfoUnit = eventData->conn_req.remote_miu;
            pConn->mRemoteRecvWindow = eventData->conn_req.remote_rw;
            ALOGD ("%s: NFA_P2P_CONN_REQ_EVT; server jni h=%u; conn jni h=%u; notify conn req", fn, pSrv->mJniHandle, pConn->mJniHandle);
            pSrv->mConnRequestEvent.notifyOne(); //unblock accept()
        }
        break;

    case NFA_P2P_CONNECTED_EVT:
        ALOGD ("%s: NFA_P2P_CONNECTED_EVT; h=0x%x  remote sap=0x%X", fn,
                eventData->connected.client_handle, eventData->connected.remote_sap);
        break;

    case NFA_P2P_DISC_EVT:
        ALOGD ("%s: NFA_P2P_DISC_EVT; h=0x%04x; reason=0x%X", fn, eventData->disc.handle, eventData->disc.reason);
        // Look for the connection block
        if ((pConn = sP2p.findConnection(eventData->disc.handle)) == NULL)
        {
            ALOGE ("%s: NFA_P2P_DISC_EVT: can't find conn for NFA handle: 0x%04x", fn, eventData->disc.handle);
        }
        else
        {
            sP2p.mDisconnectMutex.lock ();
            pConn->mNfaConnHandle = NFA_HANDLE_INVALID;
            {
                ALOGD ("%s: NFA_P2P_DISC_EVT; try guard disconn event", fn);
                SyncEventGuard guard3 (pConn->mDisconnectingEvent);
                pConn->mDisconnectingEvent.notifyOne ();
                ALOGD ("%s: NFA_P2P_DISC_EVT; notified disconn event", fn);
            }
            {
                ALOGD ("%s: NFA_P2P_DISC_EVT; try guard congest event", fn);
                SyncEventGuard guard1 (pConn->mCongEvent);
                pConn->mCongEvent.notifyOne (); //unblock write (if congested)
                ALOGD ("%s: NFA_P2P_DISC_EVT; notified congest event", fn);
            }
            {
                ALOGD ("%s: NFA_P2P_DISC_EVT; try guard read event", fn);
                SyncEventGuard guard2 (pConn->mReadEvent);
                pConn->mReadEvent.notifyOne (); //unblock receive()
                ALOGD ("%s: NFA_P2P_DISC_EVT; notified read event", fn);
            }
            sP2p.mDisconnectMutex.unlock ();
        }
        break;

    case NFA_P2P_DATA_EVT:
        // Look for the connection block
        if ((pConn = sP2p.findConnection(eventData->data.handle)) == NULL)
        {
            ALOGE ("%s: NFA_P2P_DATA_EVT: can't find conn for NFA handle: 0x%04x", fn, eventData->data.handle);
        }
        else
        {
            ALOGD_IF ((appl_trace_level>=BT_TRACE_LEVEL_DEBUG), "%s: NFA_P2P_DATA_EVT; h=0x%X; remote sap=0x%X", fn,
                    eventData->data.handle, eventData->data.remote_sap);
            SyncEventGuard guard (pConn->mReadEvent);
            pConn->mReadEvent.notifyOne();
        }
        break;

    case NFA_P2P_CONGEST_EVT:
        // Look for the connection block
        if ((pConn = sP2p.findConnection(eventData->congest.handle)) == NULL)
        {
            ALOGE ("%s: NFA_P2P_CONGEST_EVT: can't find conn for NFA handle: 0x%04x", fn, eventData->congest.handle);
        }
        else
        {
            ALOGD ("%s: NFA_P2P_CONGEST_EVT; nfa handle: 0x%04x  congested: %u", fn,
                    eventData->congest.handle, eventData->congest.is_congested);
            if (eventData->congest.is_congested == FALSE)
            {
                SyncEventGuard guard (pConn->mCongEvent);
                pConn->mCongEvent.notifyOne();
            }
        }
        break;

    default:
        ALOGE ("%s: unknown event 0x%X ????", fn, p2pEvent);
        break;
    }
    ALOGD_IF ((appl_trace_level>=BT_TRACE_LEVEL_DEBUG), "%s: exit", fn);
}


/*******************************************************************************
**
** Function:        nfaClientCallback
**
** Description:     Receive LLCP-related events from the stack.
**                  p2pEvent: Event code.
**                  eventData: Event data.
**
** Returns:         None
**
*******************************************************************************/
void PeerToPeer::nfaClientCallback (tNFA_P2P_EVT p2pEvent, tNFA_P2P_EVT_DATA* eventData)
{
    static const char fn [] = "PeerToPeer::nfaClientCallback";
    sp<NfaConn>     pConn = NULL;
    sp<P2pClient>   pClient = NULL;

    ALOGD_IF ((appl_trace_level>=BT_TRACE_LEVEL_DEBUG), "%s: enter; event=%u", fn, p2pEvent);

    switch (p2pEvent)
    {
    case NFA_P2P_REG_CLIENT_EVT:
        // Look for a client that is trying to register
        if ((pClient = sP2p.findClient ((tNFA_HANDLE)NFA_HANDLE_INVALID)) == NULL)
        {
            ALOGE ("%s: NFA_P2P_REG_CLIENT_EVT: can't find waiting client", fn);
        }
        else
        {
            ALOGD ("%s: NFA_P2P_REG_CLIENT_EVT; Conn Handle: 0x%04x, pClient: 0x%p", fn, eventData->reg_client.client_handle, pClient.get());

            SyncEventGuard guard (pClient->mRegisteringEvent);
            pClient->mNfaP2pClientHandle = eventData->reg_client.client_handle;
            pClient->mRegisteringEvent.notifyOne();
        }
        break;

    case NFA_P2P_ACTIVATED_EVT:
        // Look for a client that is trying to register
        if ((pClient = sP2p.findClient (eventData->activated.handle)) == NULL)
        {
            ALOGE ("%s: NFA_P2P_ACTIVATED_EVT: can't find client", fn);
        }
        else
        {
            ALOGD ("%s: NFA_P2P_ACTIVATED_EVT; Conn Handle: 0x%04x, pClient: 0x%p", fn, eventData->activated.handle, pClient.get());
        }
        break;

    case NFA_P2P_DEACTIVATED_EVT:
        ALOGD ("%s: NFA_P2P_DEACTIVATED_EVT: conn handle: 0x%X", fn, eventData->deactivated.handle);
        break;

    case NFA_P2P_CONNECTED_EVT:
        // Look for the client that is trying to connect
        if ((pClient = sP2p.findClient (eventData->connected.client_handle)) == NULL)
        {
            ALOGE ("%s: NFA_P2P_CONNECTED_EVT: can't find client: 0x%04x", fn, eventData->connected.client_handle);
        }
        else
        {
            ALOGD ("%s: NFA_P2P_CONNECTED_EVT; client_handle=0x%04x  conn_handle: 0x%04x  remote sap=0x%X  pClient: 0x%p", fn,
                    eventData->connected.client_handle, eventData->connected.conn_handle, eventData->connected.remote_sap, pClient.get());

            SyncEventGuard guard (pClient->mConnectingEvent);
            pClient->mClientConn->mNfaConnHandle     = eventData->connected.conn_handle;
            pClient->mClientConn->mRemoteMaxInfoUnit = eventData->connected.remote_miu;
            pClient->mClientConn->mRemoteRecvWindow  = eventData->connected.remote_rw;
            pClient->mConnectingEvent.notifyOne(); //unblock createDataLinkConn()
        }
        break;

    case NFA_P2P_DISC_EVT:
        ALOGD ("%s: NFA_P2P_DISC_EVT; h=0x%04x; reason=0x%X", fn, eventData->disc.handle, eventData->disc.reason);
        // Look for the connection block
        if ((pConn = sP2p.findConnection(eventData->disc.handle)) == NULL)
        {
            // If no connection, may be a client that is trying to connect
            if ((pClient = sP2p.findClient (eventData->disc.handle)) == NULL)
            {
                ALOGE ("%s: NFA_P2P_DISC_EVT: can't find client for NFA handle: 0x%04x", fn, eventData->disc.handle);
                return;
            }
            // Unblock createDataLinkConn()
            SyncEventGuard guard (pClient->mConnectingEvent);
            pClient->mConnectingEvent.notifyOne();
        }
        else
        {
            sP2p.mDisconnectMutex.lock ();
            pConn->mNfaConnHandle = NFA_HANDLE_INVALID;
            {
                ALOGD ("%s: NFA_P2P_DISC_EVT; try guard disconn event", fn);
                SyncEventGuard guard3 (pConn->mDisconnectingEvent);
                pConn->mDisconnectingEvent.notifyOne ();
                ALOGD ("%s: NFA_P2P_DISC_EVT; notified disconn event", fn);
            }
            {
                ALOGD ("%s: NFA_P2P_DISC_EVT; try guard congest event", fn);
                SyncEventGuard guard1 (pConn->mCongEvent);
                pConn->mCongEvent.notifyOne(); //unblock write (if congested)
                ALOGD ("%s: NFA_P2P_DISC_EVT; notified congest event", fn);
            }
            {
                ALOGD ("%s: NFA_P2P_DISC_EVT; try guard read event", fn);
                SyncEventGuard guard2 (pConn->mReadEvent);
                pConn->mReadEvent.notifyOne(); //unblock receive()
                ALOGD ("%s: NFA_P2P_DISC_EVT; notified read event", fn);
            }
            sP2p.mDisconnectMutex.unlock ();
        }
        break;

    case NFA_P2P_DATA_EVT:
        // Look for the connection block
        if ((pConn = sP2p.findConnection(eventData->data.handle)) == NULL)
        {
            ALOGE ("%s: NFA_P2P_DATA_EVT: can't find conn for NFA handle: 0x%04x", fn, eventData->data.handle);
        }
        else
        {
            ALOGD_IF ((appl_trace_level>=BT_TRACE_LEVEL_DEBUG), "%s: NFA_P2P_DATA_EVT; h=0x%X; remote sap=0x%X", fn,
                    eventData->data.handle, eventData->data.remote_sap);
            SyncEventGuard guard (pConn->mReadEvent);
            pConn->mReadEvent.notifyOne();
        }
        break;

    case NFA_P2P_CONGEST_EVT:
        // Look for the connection block
        if ((pConn = sP2p.findConnection(eventData->congest.handle)) == NULL)
        {
            ALOGE ("%s: NFA_P2P_CONGEST_EVT: can't find conn for NFA handle: 0x%04x", fn, eventData->congest.handle);
        }
        else
        {
            ALOGD_IF ((appl_trace_level>=BT_TRACE_LEVEL_DEBUG), "%s: NFA_P2P_CONGEST_EVT; nfa handle: 0x%04x  congested: %u", fn,
                    eventData->congest.handle, eventData->congest.is_congested);

            SyncEventGuard guard (pConn->mCongEvent);
            pConn->mCongEvent.notifyOne();
        }
        break;

    default:
        ALOGE ("%s: unknown event 0x%X ????", fn, p2pEvent);
        break;
    }
}


/*******************************************************************************
**
** Function:        connectionEventHandler
**
** Description:     Receive events from the stack.
**                  event: Event code.
**                  eventData: Event data.
**
** Returns:         None
**
*******************************************************************************/
void PeerToPeer::connectionEventHandler (UINT8 event, tNFA_CONN_EVT_DATA* /*eventData*/)
{
    switch (event)
    {
    case NFA_SET_P2P_LISTEN_TECH_EVT:
        {
            SyncEventGuard guard (mSetTechEvent);
            mSetTechEvent.notifyOne(); //unblock NFA_SetP2pListenTech()
            break;
        }
    }
}


/*******************************************************************************
**
** Function:        getNextJniHandle
**
** Description:     Get a new JNI handle.
**
** Returns:         A new JNI handle.
**
*******************************************************************************/
PeerToPeer::tJNI_HANDLE PeerToPeer::getNewJniHandle ()
{
    tJNI_HANDLE newHandle = 0;

    mNewJniHandleMutex.lock ();
    newHandle = mNextJniHandle++;
    mNewJniHandleMutex.unlock ();
    return newHandle;
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


/*******************************************************************************
**
** Function:        P2pServer
**
** Description:     Initialize member variables.
**
** Returns:         None
**
*******************************************************************************/
P2pServer::P2pServer(PeerToPeer::tJNI_HANDLE jniHandle, const char* serviceName)
:   mNfaP2pServerHandle (NFA_HANDLE_INVALID),
    mJniHandle (jniHandle)
{
    mServiceName.assign (serviceName);

    memset (mServerConn, 0, sizeof(mServerConn));
}

bool P2pServer::registerWithStack()
{
    static const char fn [] = "P2pServer::registerWithStack";
    ALOGD ("%s: enter; service name: %s  JNI handle: %u", fn, mServiceName.c_str(), mJniHandle);
    tNFA_STATUS     stat  = NFA_STATUS_OK;
    UINT8           serverSap = NFA_P2P_ANY_SAP;

    /**********************
   default values for all LLCP parameters:
   - Local Link MIU (LLCP_MIU)
   - Option parameter (LLCP_OPT_VALUE)
   - Response Waiting Time Index (LLCP_WAITING_TIME)
   - Local Link Timeout (LLCP_LTO_VALUE)
   - Inactivity Timeout as initiator role (LLCP_INIT_INACTIVITY_TIMEOUT)
   - Inactivity Timeout as target role (LLCP_TARGET_INACTIVITY_TIMEOUT)
   - Delay SYMM response (LLCP_DELAY_RESP_TIME)
   - Data link connection timeout (LLCP_DATA_LINK_CONNECTION_TOUT)
   - Delay timeout to send first PDU as initiator (LLCP_DELAY_TIME_TO_SEND_FIRST_PDU)
   ************************/
   stat = NFA_P2pSetLLCPConfig (LLCP_MAX_MIU,
           LLCP_OPT_VALUE,
           LLCP_WAITING_TIME,
           LLCP_LTO_VALUE,
           0, //use 0 for infinite timeout for symmetry procedure when acting as initiator
           0, //use 0 for infinite timeout for symmetry procedure when acting as target
           LLCP_DELAY_RESP_TIME,
           LLCP_DATA_LINK_TIMEOUT,
           LLCP_DELAY_TIME_TO_SEND_FIRST_PDU);
   if (stat != NFA_STATUS_OK)
       ALOGE ("%s: fail set LLCP config; error=0x%X", fn, stat);

   if (sSnepServiceName.compare(mServiceName) == 0)
       serverSap = LLCP_SAP_SNEP; //LLCP_SAP_SNEP == 4

   {
       SyncEventGuard guard (mRegServerEvent);
       stat = NFA_P2pRegisterServer (serverSap, NFA_P2P_DLINK_TYPE, const_cast<char*>(mServiceName.c_str()),
               PeerToPeer::nfaServerCallback);
       if (stat != NFA_STATUS_OK)
       {
           ALOGE ("%s: fail register p2p server; error=0x%X", fn, stat);
           return (false);
       }
       ALOGD ("%s: wait for listen-completion event", fn);
       // Wait for NFA_P2P_REG_SERVER_EVT
       mRegServerEvent.wait ();
   }

   return (mNfaP2pServerHandle != NFA_HANDLE_INVALID);
}

bool P2pServer::accept(PeerToPeer::tJNI_HANDLE serverJniHandle, PeerToPeer::tJNI_HANDLE connJniHandle,
        int maxInfoUnit, int recvWindow)
{
    static const char fn [] = "P2pServer::accept";
    tNFA_STATUS     nfaStat  = NFA_STATUS_OK;

    sp<NfaConn> connection = allocateConnection(connJniHandle);
    if (connection == NULL) {
        ALOGE ("%s: failed to allocate new server connection", fn);
        return false;
    }

    {
        // Wait for NFA_P2P_CONN_REQ_EVT or NFA_NDEF_DATA_EVT when remote device requests connection
        SyncEventGuard guard (mConnRequestEvent);
        ALOGD ("%s: serverJniHandle: %u; connJniHandle: %u; wait for incoming connection", fn,
                serverJniHandle, connJniHandle);
        mConnRequestEvent.wait();
        ALOGD ("%s: serverJniHandle: %u; connJniHandle: %u; nfa conn h: 0x%X; got incoming connection", fn,
                serverJniHandle, connJniHandle, connection->mNfaConnHandle);
    }

    if (connection->mNfaConnHandle == NFA_HANDLE_INVALID)
    {
        removeServerConnection(connJniHandle);
        ALOGD ("%s: no handle assigned", fn);
        return (false);
    }

    if (maxInfoUnit > (int)LLCP_MIU)
    {
        ALOGD ("%s: overriding the miu passed by the app(%d) with stack miu(%d)", fn, maxInfoUnit, LLCP_MIU);
        maxInfoUnit = LLCP_MIU;
    }

    ALOGD ("%s: serverJniHandle: %u; connJniHandle: %u; nfa conn h: 0x%X; try accept", fn,
            serverJniHandle, connJniHandle, connection->mNfaConnHandle);
    nfaStat = NFA_P2pAcceptConn (connection->mNfaConnHandle, maxInfoUnit, recvWindow);

    if (nfaStat != NFA_STATUS_OK)
    {
        ALOGE ("%s: fail to accept remote; error=0x%X", fn, nfaStat);
        return (false);
    }

    ALOGD ("%s: exit; serverJniHandle: %u; connJniHandle: %u; nfa conn h: 0x%X", fn,
            serverJniHandle, connJniHandle, connection->mNfaConnHandle);
    return (true);
}

void P2pServer::unblockAll()
{
    AutoMutex mutex(mMutex);
    for (int jj = 0; jj < MAX_NFA_CONNS_PER_SERVER; jj++)
    {
        if (mServerConn[jj] != NULL)
        {
            mServerConn[jj]->mNfaConnHandle = NFA_HANDLE_INVALID;
            {
                SyncEventGuard guard1 (mServerConn[jj]->mCongEvent);
                mServerConn[jj]->mCongEvent.notifyOne (); //unblock write (if congested)
            }
            {
                SyncEventGuard guard2 (mServerConn[jj]->mReadEvent);
                mServerConn[jj]->mReadEvent.notifyOne (); //unblock receive()
            }
        }
    }
}

sp<NfaConn> P2pServer::allocateConnection (PeerToPeer::tJNI_HANDLE jniHandle)
{
    AutoMutex mutex(mMutex);
    // First, find a free connection block to handle the connection
    for (int ii = 0; ii < MAX_NFA_CONNS_PER_SERVER; ii++)
    {
        if (mServerConn[ii] == NULL)
        {
            mServerConn[ii] = new NfaConn;
            mServerConn[ii]->mJniHandle = jniHandle;
            return mServerConn[ii];
        }
    }

    return NULL;
}


/*******************************************************************************
**
** Function:        findServerConnection
**
** Description:     Find a P2pServer that has the handle.
**                  nfaConnHandle: NFA connection handle.
**
** Returns:         P2pServer object.
**
*******************************************************************************/
sp<NfaConn> P2pServer::findServerConnection (tNFA_HANDLE nfaConnHandle)
{
    int jj = 0;

    AutoMutex mutex(mMutex);
    for (jj = 0; jj < MAX_NFA_CONNS_PER_SERVER; jj++)
    {
        if ( (mServerConn[jj] != NULL) && (mServerConn[jj]->mNfaConnHandle == nfaConnHandle) )
            return (mServerConn[jj]);
    }

    // If here, not found
    return (NULL);
}

/*******************************************************************************
**
** Function:        findServerConnection
**
** Description:     Find a P2pServer that has the handle.
**                  nfaConnHandle: NFA connection handle.
**
** Returns:         P2pServer object.
**
*******************************************************************************/
sp<NfaConn> P2pServer::findServerConnection (PeerToPeer::tJNI_HANDLE jniHandle)
{
    int jj = 0;

    AutoMutex mutex(mMutex);
    for (jj = 0; jj < MAX_NFA_CONNS_PER_SERVER; jj++)
    {
        if ( (mServerConn[jj] != NULL) && (mServerConn[jj]->mJniHandle == jniHandle) )
            return (mServerConn[jj]);
    }

    // If here, not found
    return (NULL);
}

/*******************************************************************************
**
** Function:        removeServerConnection
**
** Description:     Find a P2pServer that has the handle.
**                  nfaConnHandle: NFA connection handle.
**
** Returns:         P2pServer object.
**
*******************************************************************************/
bool P2pServer::removeServerConnection (PeerToPeer::tJNI_HANDLE jniHandle)
{
    int jj = 0;

    AutoMutex mutex(mMutex);
    for (jj = 0; jj < MAX_NFA_CONNS_PER_SERVER; jj++)
    {
        if ( (mServerConn[jj] != NULL) && (mServerConn[jj]->mJniHandle == jniHandle) ) {
            mServerConn[jj] = NULL;
            return true;
        }
    }

    // If here, not found
    return false;
}
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


/*******************************************************************************
**
** Function:        P2pClient
**
** Description:     Initialize member variables.
**
** Returns:         None
**
*******************************************************************************/
P2pClient::P2pClient ()
:   mNfaP2pClientHandle (NFA_HANDLE_INVALID),
    mIsConnecting (false)
{
    mClientConn = new NfaConn();
}


/*******************************************************************************
**
** Function:        ~P2pClient
**
** Description:     Free all resources.
**
** Returns:         None
**
*******************************************************************************/
P2pClient::~P2pClient ()
{
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


/*******************************************************************************
**
** Function:        NfaConn
**
** Description:     Initialize member variables.
**
** Returns:         None
**
*******************************************************************************/
NfaConn::NfaConn()
:   mNfaConnHandle (NFA_HANDLE_INVALID),
    mJniHandle (0),
    mMaxInfoUnit (0),
    mRecvWindow (0),
    mRemoteMaxInfoUnit (0),
    mRemoteRecvWindow (0)
{
}
