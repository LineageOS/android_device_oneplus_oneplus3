/*
 * Copyright (c) 2016 The CyanogenMod Project
 *               2018-2019 The LineageOS Project
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

package org.lineageos.pocketmode;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.text.TextUtils;
import android.util.Log;

import java.util.List;

import org.lineageos.internal.util.FileUtils;

public class PocketSensor implements SensorEventListener {

    private static final boolean DEBUG = false;
    private static final String TAG = "PocketSensor";

    private static final String FPC_FILE = "/sys/devices/soc/soc:fpc_fpc1020/proximity_state";

    private SensorManager mSensorManager;
    private Sensor mSensor;
    private Context mContext;

    public PocketSensor(Context context) {
        mContext = context;
        mSensorManager = mContext.getSystemService(SensorManager.class);
        mSensor = findSensorWithType("com.oneplus.sensor.pocket");
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        boolean isNear = event.values[0] == 1;
        if (FileUtils.isFileWritable(FPC_FILE)) {
            FileUtils.writeLine(FPC_FILE, isNear ? "1" : "0");
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        /* Empty */
    }

    protected void enable() {
        if (DEBUG) Log.d(TAG, "Enabling");
        mSensorManager.registerListener(this, mSensor,
                SensorManager.SENSOR_DELAY_NORMAL);
    }

    protected void disable() {
        if (DEBUG) Log.d(TAG, "Disabling");
        mSensorManager.unregisterListener(this, mSensor);
    }

    protected Sensor findSensorWithType(String type) {
        if (TextUtils.isEmpty(type)) {
            return null;
        }
        List<Sensor> sensorList = mSensorManager.getSensorList(Sensor.TYPE_ALL);
        for (Sensor s : sensorList) {
            if (type.equals(s.getStringType())) {
                return s;
            }
        }
        return null;
    }
}
