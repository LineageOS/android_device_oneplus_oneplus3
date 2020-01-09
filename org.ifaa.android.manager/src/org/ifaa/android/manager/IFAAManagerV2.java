package org.ifaa.android.manager;

import android.annotation.UnsupportedAppUsage;
import android.content.Context;

public abstract class IFAAManagerV2 extends IFAAManager{
    @UnsupportedAppUsage
    public abstract byte[] processCmdV2(Context paramContext, byte[] paramArrayOfByte);
}
