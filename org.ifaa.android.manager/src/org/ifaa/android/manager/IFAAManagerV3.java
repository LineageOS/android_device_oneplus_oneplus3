package org.ifaa.android.manager;

import android.annotation.UnsupportedAppUsage;

public abstract class IFAAManagerV3 extends IFAAManagerV2 {
    @UnsupportedAppUsage
    public static final String KEY_FINGERPRINT_FULLVIEW = "org.ifaa.ext.key.CUSTOM_VIEW";
    @UnsupportedAppUsage
    public static final String KEY_GET_SENSOR_LOCATION = "org.ifaa.ext.key.GET_SENSOR_LOCATION";
    @UnsupportedAppUsage
    public static final String VALUE_FINGERPRINT_DISABLE = "disable";
    @UnsupportedAppUsage
    public static final String VLAUE_FINGERPRINT_ENABLE = "enable";

    @UnsupportedAppUsage
    public abstract String getExtInfo(int authType, String keyExtInfo);

    @UnsupportedAppUsage
    public abstract void setExtInfo(int authType, String keyExtInfo, String valExtInfo);
}
