package org.ifaa.android.manager;

public abstract class IFAAManagerV3 extends IFAAManagerV2 {
    public static final String KEY_FINGERPRINT_FULLVIEW = "org.ifaa.ext.key.CUSTOM_VIEW";
    public static final String KEY_GET_SENSOR_LOCATION = "org.ifaa.ext.key.GET_SENSOR_LOCATION";
    public static final String VALUE_FINGERPRINT_DISABLE = "disable";
    public static final String VLAUE_FINGERPRINT_ENABLE = "enable";

    public abstract String getExtInfo(int authType, String keyExtInfo);

    public abstract void setExtInfo(int authType, String keyExtInfo, String valExtInfo);
}
