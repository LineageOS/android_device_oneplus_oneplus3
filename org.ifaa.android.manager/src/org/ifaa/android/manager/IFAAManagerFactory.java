/*
 * Copyright (C) 2016 The CyanogenMod Project
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

package org.ifaa.android.manager;

import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;

public class IFAAManagerFactory extends IFAAManager {
    private static final int ACTIVITY_START_FAILED = -1;
    private static final int ACTIVITY_START_SUCCESS = 0;
    private static final int BIOTypeFingerprint = 1;
    private static final int BIOTypeIris = 2;
    private static final String TAG = "IFAAManagerFactory";
    public static IFAAManagerFactory mIFAAManagerFactory = null;

    public int getSupportBIOTypes(Context context) {
        return BIOTypeFingerprint;
    }

    public int startBIOManager(Context context, int authType) {
        try {
            Intent intent = new Intent();
            intent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);
            intent.setComponent(new ComponentName("com.android.settings", "com.android.settings.Settings$SecuritySettingsActivity"));
            context.startActivity(intent);
        } catch (ActivityNotFoundException e) {
            e.printStackTrace();
        } catch (Throwable th) {
        }
        return ACTIVITY_START_SUCCESS;
    }

    public String getDeviceModel() {
        return "ONEPLUS-A3000";
    }

    public int getVersion() {
        return BIOTypeFingerprint;
    }

    public static IFAAManager getIFAAManager(Context context, int authType) {
        if (mIFAAManagerFactory != null) {
            return mIFAAManagerFactory;
        }
        mIFAAManagerFactory = new IFAAManagerFactory();
        return mIFAAManagerFactory;
    }
}
