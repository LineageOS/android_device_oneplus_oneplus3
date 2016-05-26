package com.android.nfc;

import android.app.KeyguardManager;
import android.content.Context;
import android.os.PowerManager;

/**
 * Helper class for determining the current screen state for NFC activities.
 */
class ScreenStateHelper {

    static final int SCREEN_STATE_UNKNOWN = 0;
    static final int SCREEN_STATE_OFF = 1;
    static final int SCREEN_STATE_ON_LOCKED = 2;
    static final int SCREEN_STATE_ON_UNLOCKED = 3;

    static final int POWER_STATE_ON = 6;
    static final int POWER_STATE_OFF = 7;

    private final PowerManager mPowerManager;
    private final KeyguardManager mKeyguardManager;

    ScreenStateHelper(Context context) {
        mKeyguardManager = (KeyguardManager)
                context.getSystemService(Context.KEYGUARD_SERVICE);
        mPowerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
    }

    int checkScreenState() {
        //TODO: fix deprecated api
        if (!mPowerManager.isScreenOn()) {
            return SCREEN_STATE_OFF;
        } else if (mKeyguardManager.isKeyguardLocked()) {
            return SCREEN_STATE_ON_LOCKED;
        } else {
            return SCREEN_STATE_ON_UNLOCKED;
        }
    }

    /**
     * For debugging only - no i18n
     */
    static String screenStateToString(int screenState) {
        switch (screenState) {
            case SCREEN_STATE_OFF:
                return "OFF";
            case SCREEN_STATE_ON_LOCKED:
                return "ON_LOCKED";
            case SCREEN_STATE_ON_UNLOCKED:
                return "ON_UNLOCKED";
            default:
                return "UNKNOWN";
        }
    }
}
