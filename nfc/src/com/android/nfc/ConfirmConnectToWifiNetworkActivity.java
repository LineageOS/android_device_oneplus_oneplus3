package com.android.nfc;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.widget.Toast;

public class ConfirmConnectToWifiNetworkActivity extends Activity
        implements View.OnClickListener, DialogInterface.OnDismissListener {

    public static final int ENABLE_WIFI_TIMEOUT_MILLIS = 5000;
    private WifiConfiguration mCurrentWifiConfiguration;
    private AlertDialog mAlertDialog;
    private boolean mEnableWifiInProgress;
    private Handler mHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        Intent intent = getIntent();
        mCurrentWifiConfiguration =
                intent.getParcelableExtra(NfcWifiProtectedSetup.EXTRA_WIFI_CONFIG);

        String printableSsid = mCurrentWifiConfiguration.getPrintableSsid();
        mAlertDialog = new AlertDialog.Builder(this,  AlertDialog.THEME_DEVICE_DEFAULT_LIGHT)
                .setTitle(R.string.title_connect_to_network)
                .setMessage(
                        String.format(getResources().getString(R.string.prompt_connect_to_network),
                        printableSsid))
                .setOnDismissListener(this)
                .setNegativeButton(com.android.internal.R.string.cancel, null)
                .setPositiveButton(R.string.wifi_connect, null)
                .create();

        mEnableWifiInProgress = false;
        mHandler = new Handler();

        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION);
        registerReceiver(mBroadcastReceiver, intentFilter);

        mAlertDialog.show();

        super.onCreate(savedInstanceState);

        mAlertDialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(this);
    }


    @Override
    public void onClick(View v) {
        WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);

        if (!wifiManager.isWifiEnabled()) {
            wifiManager.setWifiEnabled(true);
            mEnableWifiInProgress = true;

            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    if (getAndClearEnableWifiInProgress()) {
                        showFailToast();
                        ConfirmConnectToWifiNetworkActivity.this.finish();
                    }
                }
            }, ENABLE_WIFI_TIMEOUT_MILLIS);

        } else {
            doConnect(wifiManager);
        }

        mAlertDialog.dismiss();
    }

    private void doConnect(WifiManager wifiManager) {
        int networkId = wifiManager.addNetwork(mCurrentWifiConfiguration);

        if (networkId < 0) {
            showFailToast();
        } else {

            wifiManager.connect(networkId,
                    new WifiManager.ActionListener() {
                        @Override
                        public void onSuccess() {
                            Toast.makeText(ConfirmConnectToWifiNetworkActivity.this,
                                    R.string.status_wifi_connected, Toast.LENGTH_SHORT).show();
                        }

                        @Override
                        public void onFailure(int reason) {
                            showFailToast();
                        }
                    });
        }
    }


    private void showFailToast() {
        Toast.makeText(ConfirmConnectToWifiNetworkActivity.this,
                R.string.status_unable_to_connect, Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        if (!mEnableWifiInProgress) {
            finish();
        }
    }


    @Override
    protected void onDestroy() {
        ConfirmConnectToWifiNetworkActivity.this.unregisterReceiver(mBroadcastReceiver);
        super.onDestroy();
    }

    private final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(WifiManager.WIFI_STATE_CHANGED_ACTION)) {
                int wifiState = intent.getIntExtra(WifiManager.EXTRA_WIFI_STATE, 0);
                if (mCurrentWifiConfiguration != null
                        && wifiState == WifiManager.WIFI_STATE_ENABLED) {
                    if (getAndClearEnableWifiInProgress()) {
                        doConnect(
                                (WifiManager) ConfirmConnectToWifiNetworkActivity.this
                                        .getSystemService(Context.WIFI_SERVICE));
                    }
                }
            }
        }
    };

    private boolean getAndClearEnableWifiInProgress() {
        boolean enableWifiInProgress;

        synchronized (this)  {
            enableWifiInProgress = mEnableWifiInProgress;
            mEnableWifiInProgress = false;
        }

        return enableWifiInProgress;
    }
}
