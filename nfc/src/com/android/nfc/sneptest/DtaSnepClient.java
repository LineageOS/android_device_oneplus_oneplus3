/*
 * Copyright (C) 2015 NXP Semiconductors
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
package com.android.nfc.sneptest;

import java.io.IOException;
import java.io.UnsupportedEncodingException;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.nfc.NdefMessage;
import android.os.IBinder;
import android.os.Messenger;
import android.util.Log;

import com.android.nfc.DeviceHost.LlcpSocket;
import com.android.nfc.LlcpException;
import com.android.nfc.NfcService;
import com.android.nfc.DtaServiceConnector;
import com.android.nfc.snep.SnepException;
import com.android.nfc.snep.SnepMessage;
import com.android.nfc.snep.SnepMessenger;

public final class DtaSnepClient
{
    private static final String TAG = "DtaSnepClient";
    private static final boolean DBG = true;
    private static final int DEFAULT_ACCEPTABLE_LENGTH = 1024;
    private static final int DEFAULT_MIU = 128;
    private static final int DEFAULT_RWSIZE = 1;
    private static final int DEFAULT_PORT = 63;
    private static final String SNEP_SERVICE_NAME = "urn:nfc:sn:snep";
    private static final String DEFAULT_SERVICE_NAME = SNEP_SERVICE_NAME;
    private final Object mTransmissionLock = new Object();

    private int mState = DISCONNECTED;
    private final int mAcceptableLength;
    private final int mFragmentLength;
    private final int mMiu;
    private final int mPort;
    private final int mRwSize;
    private final String mServiceName;
    public static int mTestCaseId;

    private static final int DISCONNECTED = 0;
    private static final int CONNECTING = 1;
    private static final int CONNECTED = 2;

    SnepMessenger mMessenger = null;

    public DtaSnepClient()
    {
        mServiceName = DEFAULT_SERVICE_NAME;
        mPort = DEFAULT_PORT;
        mAcceptableLength = DEFAULT_ACCEPTABLE_LENGTH;
        mFragmentLength = -1;
        mMiu = DEFAULT_MIU;
        mRwSize = DEFAULT_RWSIZE;
    }

    public DtaSnepClient(String serviceName, int miu, int rwSize, int testCaseId) {
        mServiceName = serviceName;
        mPort = -1;
        mAcceptableLength = DEFAULT_ACCEPTABLE_LENGTH;
        mFragmentLength = -1;
        mMiu = miu;
        mRwSize = rwSize;
        mTestCaseId = testCaseId;
    }

    public void DtaClientOperations(Context mContext)
    {
        DtaServiceConnector dtaServiceConnector=new DtaServiceConnector(mContext);
        dtaServiceConnector.bindService();
        if (DBG) Log.d(TAG, "Connecting remote server");
        try{
            connect();
        }catch(IOException e){
            Log.e(TAG, "Error connecting remote server");
        }
        switch(mTestCaseId)
        {
           case 1: //TC_C_BIT_BV_01
           {
               try {
                   if (DBG) Log.d(TAG, "PUT Small Ndef Data");
                   put(SnepMessage.getSmallNdef());
                   dtaServiceConnector.sendMessage(SnepMessage.getSmallNdef().toString());
               } catch (UnsupportedEncodingException e) {
                     // TODO Auto-generated catch block
                     e.printStackTrace();
               } catch (IOException e) {
                     // TODO Auto-generated catch block
                     e.printStackTrace();
               }
               close();
           }
           break;
           case 2: //TC_C_BIT_BI_01_0
           {
               try {
                       if (DBG) Log.d(TAG, "PUT Small Ndef Data");
                       put(SnepMessage.getSmallNdef());
                       dtaServiceConnector.sendMessage(SnepMessage.getSmallNdef().toString());
               } catch (UnsupportedEncodingException e) {
                     // TODO Auto-generated catch block
                     e.printStackTrace();
               } catch (IOException e) {
                   // TODO Auto-generated catch block
                   e.printStackTrace();
               }
               close();
           }
           break;
           case 3: //TC_C_BIT_BI_01_1
           {
               try {
                   if (DBG) Log.d(TAG, "PUT Small Ndef Data");
                   put(SnepMessage.getSmallNdef());
                   dtaServiceConnector.sendMessage(SnepMessage.getSmallNdef().toString());
               } catch (UnsupportedEncodingException e) {
                   // TODO Auto-generated catch block
                   e.printStackTrace();
               } catch (IOException e) {
                   // TODO Auto-generated catch block
                   e.printStackTrace();
               }
               close();
           }
           break;
           case 4: //TC_C_PUT_BV_01
           {
               try {
                   if (DBG) Log.d(TAG, "PUT Small Ndef Data");
                   put(SnepMessage.getSmallNdef());
                   dtaServiceConnector.sendMessage(SnepMessage.getSmallNdef().toString());
               } catch (UnsupportedEncodingException e) {
                   // TODO Auto-generated catch block
                   e.printStackTrace();
               } catch (IOException e) {
                   // TODO Auto-generated catch block
                   e.printStackTrace();
               }
               close();
           }
           break;
           case 5: //TC_C_PUT_BV_02
           {
               try {
                if (DBG) Log.d(TAG, "PUT Large Ndef Data");
                put(SnepMessage.getLargeNdef());
                dtaServiceConnector.sendMessage(SnepMessage.getLargeNdef().toString());
            } catch (UnsupportedEncodingException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
               close();
           }
           break;
           case 6: //TC_C_PUT_BI_01
           {
               try {
                if (DBG) Log.d(TAG, "PUT Large Ndef Data");
                put(SnepMessage.getLargeNdef());
                dtaServiceConnector.sendMessage(SnepMessage.getLargeNdef().toString());
            } catch (UnsupportedEncodingException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
               close();
           }
           break;
           case 7: //TC_C_GET_BV_01
           {
               try {
                   if (DBG) Log.d(TAG, "GET Ndef Message");
                   get(SnepMessage.getSmallNdef());
                   dtaServiceConnector.sendMessage(SnepMessage.getSmallNdef().toString());
               } catch (UnsupportedEncodingException e) {
                   // TODO Auto-generated catch block
                   e.printStackTrace();
               } catch (IOException e) {
                   // TODO Auto-generated catch block
                   e.printStackTrace();
               }
               close();
           }
           break;
           case 8: //TC_C_GET_BV_02
           {
               try {
                   if (DBG) Log.d(TAG, "GET Ndef Message");
                   get(SnepMessage.getSmallNdef());
                   dtaServiceConnector.sendMessage(SnepMessage.getSmallNdef().toString());
               } catch (UnsupportedEncodingException e) {
                   // TODO Auto-generated catch block
                   e.printStackTrace();
               } catch (IOException e) {
                   // TODO Auto-generated catch block
                   e.printStackTrace();
               }
               close();
           }
           break;
           case 9: //TC_C_GET_BV_03
           {
               try {
                   if (DBG) Log.d(TAG, "GET Ndef Message");
                   get(SnepMessage.getSmallNdef());
                   dtaServiceConnector.sendMessage(SnepMessage.getSmallNdef().toString());
               } catch (UnsupportedEncodingException e) {
                   // TODO Auto-generated catch block
                   e.printStackTrace();
               } catch (IOException e) {
                   // TODO Auto-generated catch block
                   e.printStackTrace();
               }
               close();
           }
           break;
           default:
               if (DBG) Log.d(TAG, "Unknown test case");
        }
    }

    public void put(NdefMessage msg) throws IOException {
        SnepMessenger messenger;
        synchronized (this) {
            if (mState != CONNECTED) {
                throw new IOException("Socket not connected.");
            }
            messenger = mMessenger;
        }

        synchronized (mTransmissionLock) {
            try {
                messenger.sendMessage(SnepMessage.getPutRequest(msg));
                messenger.getMessage();
            } catch (SnepException e) {
                throw new IOException(e);
            }
        }
    }

    public SnepMessage get(NdefMessage msg) throws IOException {
        SnepMessenger messenger;
        synchronized (this) {
            if (mState != CONNECTED) {
                throw new IOException("Socket not connected.");
            }
            messenger = mMessenger;
        }

        synchronized (mTransmissionLock) {
            try {
                messenger.sendMessage(SnepMessage.getGetRequest(mAcceptableLength, msg));
                return messenger.getMessage();
            } catch (SnepException e) {
                throw new IOException(e);
            }
        }
    }

    public void connect() throws IOException {
        synchronized (this) {
            if (mState != DISCONNECTED) {
                throw new IOException("Socket already in use.");
            }
            mState = CONNECTING;
        }

        LlcpSocket socket = null;
        SnepMessenger messenger;
        try {
            if (DBG) Log.d(TAG, "about to create socket");
            // Connect to the snep server on the remote side
            socket = NfcService.getInstance().createLlcpSocket(0, mMiu, mRwSize, 1024);
            if (socket == null) {
                throw new IOException("Could not connect to socket.");
            }
            if (mPort == -1) {
                if (DBG) Log.d(TAG, "about to connect to service " + mServiceName);
                socket.connectToService(mServiceName);
            } else {
                if (DBG) Log.d(TAG, "about to connect to port " + mPort);
                socket.connectToSap(mPort);
            }
            int miu = socket.getRemoteMiu();
            int fragmentLength = (mFragmentLength == -1) ?  miu : Math.min(miu, mFragmentLength);
            messenger = new SnepMessenger(true, socket, fragmentLength);
        } catch (LlcpException e) {
            synchronized (this) {
                mState = DISCONNECTED;
            }
            throw new IOException("Could not connect to socket");
        } catch (IOException e) {
            if (socket != null) {
                try {
                    socket.close();
                } catch (IOException e2) {
                }
            }
            synchronized (this) {
                mState = DISCONNECTED;
            }
            throw new IOException("Failed to connect to socket");
        }

        synchronized (this) {
            mMessenger = messenger;
            mState = CONNECTED;
        }
    }

    public void close() {
        synchronized (this) {
            if (mMessenger != null) {
               try {
                   mMessenger.close();
               } catch (IOException e) {
                   // ignore
               } finally {
                   mMessenger = null;
                   mState = DISCONNECTED;
               }
            }
        }
    }
}
