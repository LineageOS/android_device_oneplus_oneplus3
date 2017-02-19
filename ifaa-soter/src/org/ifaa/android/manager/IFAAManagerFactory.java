package org.ifaa.android.manager;
import android.os.Build;
import android.content.Intent;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.os.UserHandle;
import android.util.Slog;
import android.content.ComponentName;
import android.os.UserHandle;
import android.app.Activity;

public class IFAAManagerFactory  extends IFAAManager{
    public static IFAAManagerFactory mIFAAManagerFactory = null;
    private static final int BIOTypeFingerprint = 0x01;
    private static final int BIOTypeIris = 0x02;

    private static final int ACTIVITY_START_SUCCESS = 0;
	private static final int ACTIVITY_START_FAILED = -1;

	private static final String TAG = "IFAAManagerFactory";

    public IFAAManagerFactory() {
    }
    public int getSupportBIOTypes(Context context) {
        Slog.e(TAG, "BIOTypeFingerprint" + BIOTypeFingerprint);
        return BIOTypeFingerprint;
    }

    public int startBIOManager(Context context, int authType) {
        try {
            Slog.e(TAG, "startBIOManager" + context);
            Intent intent = new Intent();
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.setComponent(new ComponentName("com.android.settings", "com.android.settings.Settings$SecuritySettingsActivity"));
            Slog.e(TAG, "OOS context" + context);
            context.startActivity(intent);
         } catch (ActivityNotFoundException e) {
            e.printStackTrace();
            return ACTIVITY_START_FAILED;
         } finally {
            return ACTIVITY_START_SUCCESS;
         }
    }

    public String getDeviceModel() {
        //return Build.MODEL;
        Slog.e(TAG, "device model");
        return "ONEPLUS-A3000";
    }

    public int getVersion() {
        return 1;
    }

    public static IFAAManager getIFAAManager(Context context, int authType) {
         Slog.e(TAG, "getIFAAManager");
        if(mIFAAManagerFactory == null) {
            mIFAAManagerFactory = new IFAAManagerFactory();
            return mIFAAManagerFactory;
        } else {
            return mIFAAManagerFactory;
        }
    }

}
