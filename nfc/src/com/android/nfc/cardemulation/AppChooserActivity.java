/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.nfc.cardemulation;

import com.android.internal.R;
import com.android.internal.app.AlertActivity;
import com.android.internal.app.AlertController;

import java.util.ArrayList;
import java.util.List;

import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.drawable.Drawable;
import android.nfc.NfcAdapter;
import android.nfc.cardemulation.ApduServiceInfo;
import android.nfc.cardemulation.CardEmulation;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

public class AppChooserActivity extends AlertActivity
        implements AdapterView.OnItemClickListener {

    static final String TAG = "AppChooserActivity";

    public static final String EXTRA_APDU_SERVICES = "services";
    public static final String EXTRA_CATEGORY = "category";
    public static final String EXTRA_FAILED_COMPONENT = "failed_component";

    private int mIconSize;
    private ListView mListView;
    private ListAdapter mListAdapter;
    private CardEmulation mCardEmuManager;
    private String mCategory;

    final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            finish();
        }
    };

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mReceiver);
    }

    protected void onCreate(Bundle savedInstanceState, String category,
            ArrayList<ApduServiceInfo> options, ComponentName failedComponent) {
        super.onCreate(savedInstanceState);
        setTheme(R.style.Theme_DeviceDefault_Light_Dialog_Alert);

        IntentFilter filter = new IntentFilter(Intent.ACTION_SCREEN_OFF);
        registerReceiver(mReceiver, filter);

        if ((options == null || options.size() == 0) && failedComponent == null) {
            Log.e(TAG, "No components passed in.");
            finish();
            return;
        }

        mCategory = category;
        boolean isPayment = CardEmulation.CATEGORY_PAYMENT.equals(mCategory);

        final NfcAdapter adapter = NfcAdapter.getDefaultAdapter(this);
        mCardEmuManager = CardEmulation.getInstance(adapter);
        AlertController.AlertParams ap = mAlertParams;

        final ActivityManager am = (ActivityManager) getSystemService(ACTIVITY_SERVICE);
        mIconSize = am.getLauncherLargeIconSize();

        // Three cases:
        // 1. Failed component and no alternatives: just an OK box
        // 2. Failed component and alternatives: pick alternative
        // 3. No failed component and alternatives: pick alternative
        PackageManager pm = getPackageManager();

        CharSequence applicationLabel = "unknown";
        if (failedComponent != null) {
            try {
                ApplicationInfo info = pm.getApplicationInfo(failedComponent.getPackageName(), 0);
                applicationLabel = info.loadLabel(pm);
            } catch (NameNotFoundException e) {
            }

        }
        if (options.size() == 0 && failedComponent != null) {
            String formatString = getString(com.android.nfc.R.string.transaction_failure);
            ap.mTitle = "";
            ap.mMessage = String.format(formatString, applicationLabel);
            ap.mPositiveButtonText = getString(R.string.ok);
            setupAlert();
        } else {
            mListAdapter = new ListAdapter(this, options);
            if (failedComponent != null) {
                String formatString = getString(com.android.nfc.R.string.could_not_use_app);
                ap.mTitle = String.format(formatString, applicationLabel);
                ap.mNegativeButtonText = getString(R.string.cancel);
            } else {
                if (CardEmulation.CATEGORY_PAYMENT.equals(category)) {
                    ap.mTitle = getString(com.android.nfc.R.string.pay_with);
                } else {
                    ap.mTitle = getString(com.android.nfc.R.string.complete_with);
                }
            }
            ap.mView = getLayoutInflater().inflate(com.android.nfc.R.layout.cardemu_resolver, null);

            mListView = (ListView) ap.mView.findViewById(com.android.nfc.R.id.resolver_list);
            if (isPayment) {
                mListView.setDivider(getResources().getDrawable(android.R.color.transparent));
                int height = (int) (getResources().getDisplayMetrics().density * 16);
                mListView.setDividerHeight(height);
            } else {
                mListView.setPadding(0, 0, 0, 0);
            }
            mListView.setAdapter(mListAdapter);
            mListView.setOnItemClickListener(this);

            setupAlert();
        }
        Window window = getWindow();
        window.addFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Intent intent = getIntent();
        ArrayList<ApduServiceInfo> services = intent.getParcelableArrayListExtra(EXTRA_APDU_SERVICES);
        String category = intent.getStringExtra(EXTRA_CATEGORY);
        ComponentName failedComponent = intent.getParcelableExtra(EXTRA_FAILED_COMPONENT);
        onCreate(savedInstanceState, category, services, failedComponent);
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        DisplayAppInfo info = (DisplayAppInfo) mListAdapter.getItem(position);
        mCardEmuManager.setDefaultForNextTap(info.serviceInfo.getComponent());
        Intent dialogIntent = new Intent(this, TapAgainDialog.class);
        dialogIntent.putExtra(TapAgainDialog.EXTRA_CATEGORY, mCategory);
        dialogIntent.putExtra(TapAgainDialog.EXTRA_APDU_SERVICE, info.serviceInfo);
        startActivity(dialogIntent);
        finish();
    }

    final class DisplayAppInfo {
        ApduServiceInfo serviceInfo;
        CharSequence displayLabel;
        Drawable displayIcon;
        Drawable displayBanner;

        public DisplayAppInfo(ApduServiceInfo serviceInfo, CharSequence label, Drawable icon,
                Drawable banner) {
            this.serviceInfo = serviceInfo;
            displayIcon = icon;
            displayLabel = label;
            displayBanner = banner;
        }
    }

    final class ListAdapter extends BaseAdapter {
        private final LayoutInflater mInflater;
        private final boolean mIsPayment;
        private List<DisplayAppInfo> mList;

        public ListAdapter(Context context, ArrayList<ApduServiceInfo> services) {
            mInflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            // For each component, get the corresponding app name and icon
            PackageManager pm = getPackageManager();
            mList = new ArrayList<DisplayAppInfo>();
            mIsPayment = CardEmulation.CATEGORY_PAYMENT.equals(mCategory);
            for (ApduServiceInfo service : services) {
                CharSequence label = service.getDescription();
                if (label == null) label = service.loadLabel(pm);
                Drawable icon = service.loadIcon(pm);
                Drawable banner = null;
                if (mIsPayment) {
                    banner = service.loadBanner(pm);
                    if (banner == null) {
                        Log.e(TAG, "Not showing " + label + " because no banner specified.");
                        continue;
                    }
                }
                DisplayAppInfo info = new DisplayAppInfo(service, label, icon, banner);
                mList.add(info);
            }
        }

        @Override
        public int getCount() {
            return mList.size();
        }

        @Override
        public Object getItem(int position) {
            return mList.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            View view;
            if (convertView == null) {
                if (mIsPayment) {
                    view = mInflater.inflate(
                            com.android.nfc.R.layout.cardemu_payment_item, parent, false);
                } else {
                    view = mInflater.inflate(
                            com.android.nfc.R.layout.cardemu_item, parent, false);
                }
                final ViewHolder holder = new ViewHolder(view);
                view.setTag(holder);

            } else {
                view = convertView;
            }

            final ViewHolder holder = (ViewHolder) view.getTag();
            DisplayAppInfo appInfo = mList.get(position);
            if (mIsPayment) {
                holder.banner.setImageDrawable(appInfo.displayBanner);
            } else {
                ViewGroup.LayoutParams lp = holder.icon.getLayoutParams();
                lp.width = lp.height = mIconSize;
                holder.icon.setImageDrawable(appInfo.displayIcon);
                holder.text.setText(appInfo.displayLabel);
            }
            return view;
        }
    }

    static class ViewHolder {
        public TextView text;
        public ImageView icon;
        public ImageView banner;
        public ViewHolder(View view) {
            text = (TextView) view.findViewById(com.android.nfc.R.id.applabel);
            icon = (ImageView) view.findViewById(com.android.nfc.R.id.appicon);
            banner = (ImageView) view.findViewById(com.android.nfc.R.id.banner);
        }
    }
}
