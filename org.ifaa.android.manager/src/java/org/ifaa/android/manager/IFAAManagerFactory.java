package org.ifaa.android.manager;

import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Build;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Slog;

import com.android.internal.app.IIFAAService;

public class IFAAManagerFactory  extends IFAAManagerV2{
    public static IFAAManagerFactory mIFAAManagerFactory = null;
    private static final int BIOTypeFingerprint = 0x01;
    private static final int BIOTypeIris = 0x02;

    private static final int ACTIVITY_START_SUCCESS = 0;
	private static final int ACTIVITY_START_FAILED = -1;

	private static final String TAG = "IFAAManagerFactory";

    static final String IFAA_SERVICE_PACKAGE = "com.oneplus.ifaaservice";
    static final String IFAA_SERVICE_CLASS = "com.oneplus.ifaaservice.IFAAService";
    static final ComponentName IFAA_SERVICE_COMPONENT = new ComponentName(
            IFAA_SERVICE_PACKAGE,
            IFAA_SERVICE_CLASS);
    private IIFAAService mIFAAService = null;
    private static final int BIND_IFAASER_SERVICE_TIMEOUT = 10000;

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
        return 2;
    }

    public static IFAAManagerV2 getIFAAManager(Context context, int authType) {
         Slog.e(TAG, "getIFAAManager");
        if(mIFAAManagerFactory == null) {
            mIFAAManagerFactory = new IFAAManagerFactory();
            return mIFAAManagerFactory;
        } else {
            return mIFAAManagerFactory;
        }
    }

    public byte[] processCmdV2(Context context, byte[] data){
        //if(Build.DEBUG_ONEPLUS) Slog.i(TAG, "processCmdV2", new RuntimeException("IFAAManagerFactory").fillInStackTrace());

        // set default return as null
        byte[] result = null;

        // to ensure that IFAAService was bound successfully
        ensureIfaaService(context);

        // get processCmdV2 from pass through remote service: IFAAService
        try {
            result = mIFAAService.processCmdV2(data);
        } catch (RemoteException e) {
            Slog.e(TAG, "exception while invoking processCmdV2 of remote IFAAService: " + e);
        }

        return result;
    }

    private void ensureIfaaService(Context context) {
        if(mIFAAService == null) {
            Intent service = new Intent().setComponent(IFAA_SERVICE_COMPONENT);
            context.bindService(service, mConnection, Context.BIND_AUTO_CREATE);
            synchronized (mConnection) {
                try {
                    mConnection.wait(BIND_IFAASER_SERVICE_TIMEOUT);
                } catch (InterruptedException e) {
                    Slog.e(TAG, "exception while binding IFAAService: " + e);
                }
            }
        }
    }

    private ServiceConnection mConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            mIFAAService = IIFAAService.Stub.asInterface(service);
           // if(Build.DEBUG_ONEPLUS) Slog.i(TAG, "IFAAService was bound successfully: " + mIFAAService);
            synchronized (mConnection) {
                notifyAll();
            }
        }

        public void onServiceDisconnected(ComponentName name) {
            mIFAAService = null;
           // if(Build.DEBUG_ONEPLUS) Slog.i(TAG, "IFAAService was unbound");
        }
    };
}
