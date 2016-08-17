package org.ifaa.android.manager;

import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.util.OpFeatures;

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
            Intent intent;
            if (OpFeatures.isH2()) {
                intent = new Intent();
                intent.addFlags(268435456);
                intent.setComponent(new ComponentName("com.android.settings", "com.android.settings.Settings$OPSecuritySettingsActivity"));
                context.startActivity(intent);
            } else {
                intent = new Intent();
                intent.addFlags(268435456);
                intent.setComponent(new ComponentName("com.android.settings", "com.android.settings.Settings$SecuritySettingsActivity"));
                context.startActivity(intent);
            }
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
