package org.ifaa.android.manager;

import android.annotation.UnsupportedAppUsage;
import android.content.Context;

public abstract class IFAAManager {

    /**
     * 返回手机系统上支持的校验方式，目前IFAF协议1.0版本指纹为0x01、虹膜为0x02
     */
    @UnsupportedAppUsage
    public abstract int getSupportBIOTypes(Context context);

    /**
     * 启动系统的指纹/虹膜管理应用界面，让用户进行指纹录入。指纹录入是在系统的指纹管理应用中实现的，
     * 本函数的作用只是将指纹管理应用运行起来，直接进行页面跳转，方便用户录入。
     * @param context
     * @param authType 生物特征识别类型，指纹为1，虹膜为2
     * @return 0，成功启动指纹管理应用；-1，启动指纹管理应用失败。
     */
    @UnsupportedAppUsage
    public abstract int startBIOManager(Context context, int authType);

    /**
     * 获取设备型号，同一款机型型号需要保持一致
     */
    @UnsupportedAppUsage
    public abstract String getDeviceModel();

    /**
     * 获取IFAAManager接口定义版本，目前为1
     */
    @UnsupportedAppUsage
    public abstract int getVersion();
}
