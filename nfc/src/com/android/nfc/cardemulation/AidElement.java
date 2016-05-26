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
package com.android.nfc.cardemulation;

import android.util.Log;

import java.util.Collections;
import java.util.Hashtable;
import java.util.List;
import java.util.Iterator;

public class AidElement implements Comparable {

    static final int ROUTE_WIEGHT_FOREGROUND = 4;
    static final int ROUTE_WIEGHT_PAYMENT = 2;
    static final int ROUTE_WIEGHT_OTHER = 1;

    private String mAid;
    private int mWeight;
    private int mRouteLocation;
    private int mPowerState;

    public AidElement(String aid, int weight, int route, int power) {
        mAid = aid;
        mWeight = weight;
        mRouteLocation = route;
        mPowerState = power;
    }

    public int getWeight() {
        return mWeight;
    }

    public void setRouteLocation(int routeLocation) {
        mRouteLocation = routeLocation;
    }

    public void setAid(String aid) {
        mAid = aid;
    }

    public void setPowerState(int powerState) {
        mPowerState = powerState;
    }

    public String getAid() {
        return mAid;
    }

    public int getRouteLocation() {
        return mRouteLocation;
    }

    public int getPowerState() {
        return mPowerState;
    }

    @Override
    public int compareTo(Object o) {

        AidElement elem = (AidElement)o;

        if (mWeight > elem.getWeight()) {
            return -1;
        }
        else if (mWeight < elem.getWeight()) {
            return 1;
        }
        return elem.getAid().length() - mAid.length();
    }

    @Override
    public String toString() {
        return "aid: " + mAid + ", location: " + mRouteLocation
                    + ", power: " + mPowerState + ",weight: " + mWeight;
    }
}
