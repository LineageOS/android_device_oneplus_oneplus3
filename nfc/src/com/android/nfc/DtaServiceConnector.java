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

package com.android.nfc;

import java.util.List;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;

public class DtaServiceConnector {

    Context mContext;
    Messenger dtaMessenger = null;
    boolean isBound;

    public DtaServiceConnector(Context mContext) {
        this.mContext = mContext;
    }

    public void bindService() {
        if(!isBound){
        Intent intent = new Intent("com.phdtaui.messageservice.ACTION_BIND");
        mContext.bindService(createExplicitFromImplicitIntent(mContext,intent), myConnection,Context.BIND_AUTO_CREATE);
        }
    }

    private ServiceConnection myConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
            dtaMessenger = new Messenger(service);
            isBound = true;
        }

        public void onServiceDisconnected(ComponentName className) {
            dtaMessenger = null;
            isBound = false;
        }
    };

    public void sendMessage(String ndefMessage) {
        if(!isBound)return;
        Message msg = Message.obtain();
        Bundle bundle = new Bundle();
        bundle.putString("NDEF_MESSAGE", ndefMessage);
        msg.setData(bundle);
        try {
            dtaMessenger.send(msg);
        } catch (RemoteException e) {
            e.printStackTrace();
        }catch (NullPointerException e) {
            e.printStackTrace();
    }
    }

    public static Intent createExplicitFromImplicitIntent(Context context, Intent implicitIntent){
        PackageManager pm = context.getPackageManager();
        List<ResolveInfo> resolveInfo = pm.queryIntentServices(implicitIntent, 0);
        if (resolveInfo == null || resolveInfo.size() != 1) {
        return null;
        }
        ResolveInfo serviceInfo = resolveInfo.get(0);
        String packageName = serviceInfo.serviceInfo.packageName;
        String className = serviceInfo.serviceInfo.name;
        ComponentName component = new ComponentName(packageName, className);
        Intent explicitIntent = new Intent(implicitIntent);
        explicitIntent.setComponent(component);
        return explicitIntent;
    }

}
