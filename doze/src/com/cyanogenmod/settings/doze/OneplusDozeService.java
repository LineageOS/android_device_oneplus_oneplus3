/*
 * Copyright (c) 2015 The CyanogenMod Project
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

package com.cyanogenmod.settings.doze;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.IBinder;
import android.os.PowerManager;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.util.Log;

import java.lang.System;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;
import java.util.List;

public class OneplusDozeService extends Service {
    private static final String TAG = "OneplusDozeService";
    private static final boolean DEBUG = false;

    private static final String DOZE_INTENT = "com.android.systemui.doze.pulse";

    private static final String GESTURE_HAND_WAVE_KEY = "gesture_hand_wave";
    private static final String GESTURE_POCKET_KEY = "gesture_pocket";
    private static final String PROXIMITY_WAKE_KEY = "proximity_wake_enable";

    private static final int POCKET_DELTA_NS = 1000 * 1000 * 1000;

    private Context mContext;
    private OneplusProximitySensor mSensor;
    private PowerManager mPowerManager;

    private boolean mHandwaveGestureEnabled = false;
    private boolean mPocketGestureEnabled = false;
    private boolean mProximityWakeEnabled = false;

    class OneplusProximitySensor implements SensorEventListener {
        private SensorManager mSensorManager;
        private Sensor mSensor;

        private boolean mSawNear = false;
        private long mInPocketTime = 0;

        public OneplusProximitySensor(Context context) {
            mSensorManager = (SensorManager)context.getSystemService(Context.SENSOR_SERVICE);
            mSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_PROXIMITY);
        }

        @Override
        public void onSensorChanged(SensorEvent event) {
            boolean isNear = event.values[0] < mSensor.getMaximumRange();
            if (mSawNear && !isNear) {
                if (shouldPulse(event.timestamp)) {
                    launchDozePulse();
                }
            } else {
                mInPocketTime = event.timestamp;
            }
            mSawNear = isNear;
        }

        @Override
        public void onAccuracyChanged(Sensor sensor, int accuracy) {
            /* Empty */
        }

        private boolean shouldPulse(long timestamp) {
            long delta = timestamp - mInPocketTime;

            if (mHandwaveGestureEnabled && mPocketGestureEnabled) {
                return true;
            } else if (mProximityWakeEnabled && (delta < POCKET_DELTA_NS)) {
                mPowerManager.wakeUp(TimeUnit.NANOSECONDS.toMillis(System.nanoTime()));
                return false;
            } else if (mHandwaveGestureEnabled && !mPocketGestureEnabled) {
                return delta < POCKET_DELTA_NS;
            } else if (!mHandwaveGestureEnabled && mPocketGestureEnabled) {
                return delta >= POCKET_DELTA_NS;
            }
            return false;
        }

        public void testAndEnable() {
            if ((isDozeEnabled() && (mHandwaveGestureEnabled || mPocketGestureEnabled)) ||
                    mProximityWakeEnabled) {
                mSensorManager.registerListener(this, mSensor, SensorManager.SENSOR_DELAY_NORMAL);
            }
        }

        public void disable() {
            mSensorManager.unregisterListener(this, mSensor);
        }
    }

    @Override
    public void onCreate() {
        if (DEBUG) Log.d(TAG, "OneplusDozeService Started");
        mContext = this;
        mPowerManager = (PowerManager)getSystemService(Context.POWER_SERVICE);
        mSensor = new OneplusProximitySensor(mContext);
        SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(mContext);
        loadPreferences(sharedPrefs);
        sharedPrefs.registerOnSharedPreferenceChangeListener(mPrefListener);
        if (!isInteractive()) {
            mSensor.testAndEnable();
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (DEBUG) Log.d(TAG, "Starting service");
        IntentFilter screenStateFilter = new IntentFilter(Intent.ACTION_SCREEN_ON);
        screenStateFilter.addAction(Intent.ACTION_SCREEN_OFF);
        mContext.registerReceiver(mScreenStateReceiver, screenStateFilter);
        return START_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void launchDozePulse() {
        mContext.sendBroadcast(new Intent(DOZE_INTENT));
    }

    private boolean isInteractive() {
        return mPowerManager.isInteractive();
    }

    private boolean isDozeEnabled() {
        return Settings.Secure.getInt(mContext.getContentResolver(),
                Settings.Secure.DOZE_ENABLED, 1) != 0;
    }

    private void onDisplayOn() {
        if (DEBUG) Log.d(TAG, "Display on");
        mSensor.disable();
    }

    private void onDisplayOff() {
        if (DEBUG) Log.d(TAG, "Display off");
        mSensor.testAndEnable();
    }

    private void loadPreferences(SharedPreferences sharedPreferences) {
        mHandwaveGestureEnabled = sharedPreferences.getBoolean(GESTURE_HAND_WAVE_KEY, false);
        mPocketGestureEnabled = sharedPreferences.getBoolean(GESTURE_POCKET_KEY, false);
        mProximityWakeEnabled = sharedPreferences.getBoolean(PROXIMITY_WAKE_KEY, false);
    }

    private BroadcastReceiver mScreenStateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(Intent.ACTION_SCREEN_OFF)) {
                onDisplayOff();
            } else if (intent.getAction().equals(Intent.ACTION_SCREEN_ON)) {
                onDisplayOn();
            }
        }
    };

    private SharedPreferences.OnSharedPreferenceChangeListener mPrefListener =
            new SharedPreferences.OnSharedPreferenceChangeListener() {
        @Override
        public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
            if (GESTURE_HAND_WAVE_KEY.equals(key)) {
                mHandwaveGestureEnabled = sharedPreferences.getBoolean(GESTURE_HAND_WAVE_KEY, false);
            } else if (GESTURE_POCKET_KEY.equals(key)) {
                mPocketGestureEnabled = sharedPreferences.getBoolean(GESTURE_POCKET_KEY, false);
            } else if (PROXIMITY_WAKE_KEY.equals(key)) {
                mProximityWakeEnabled = sharedPreferences.getBoolean(PROXIMITY_WAKE_KEY, false);
            }
        }
    };
}
