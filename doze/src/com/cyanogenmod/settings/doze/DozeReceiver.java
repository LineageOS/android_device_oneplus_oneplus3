package com.cyanogenmod.settings.doze;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

import cyanogenmod.preference.RemotePreferenceUpdater;

public class DozeReceiver extends RemotePreferenceUpdater {

    private static final boolean DEBUG = false;
    private static final String TAG = "OneplusDoze";

    private static final String DOZE_CATEGORY_KEY = "doze_device_settings";

    @Override
    public void onReceive(Context context, Intent intent) {
        super.onReceive(context, intent);

        if (intent.getAction().equals(Intent.ACTION_BOOT_COMPLETED)) {
            if (Utils.isDozeEnabled(context) && Utils.sensorsEnabled(context)) {
                if (DEBUG) Log.d(TAG, "Starting service");
                Utils.startService(context);
            }
        }
    }

    @Override
    public String getSummary(Context context, String key) {
        if (DOZE_CATEGORY_KEY.equals(key)) {
            return DozeSettingsFragment.getDozeSummary(context);
        }
        return null;
    }

    static void notifyChanged(Context context) {
        notifyChanged(context, DOZE_CATEGORY_KEY);
    }
}
