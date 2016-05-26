package com.android.nfc;

import android.nfc.INfcUnlockHandler;
import android.nfc.Tag;
import android.os.IBinder;
import android.util.Log;

import java.util.HashMap;
import java.util.Iterator;

/**
 * Singleton for handling NFC Unlock related logic and state.
 */
class NfcUnlockManager {
    private static final String TAG = "NfcUnlockManager";

    private final HashMap<IBinder, UnlockHandlerWrapper> mUnlockHandlers;
    private int mLockscreenPollMask;

    private static class UnlockHandlerWrapper {
        final INfcUnlockHandler mUnlockHandler;
        final int mPollMask;


        private UnlockHandlerWrapper(INfcUnlockHandler unlockHandler, int pollMask) {
            mUnlockHandler = unlockHandler;
            mPollMask = pollMask;
        }
    }

    public static NfcUnlockManager getInstance() {
        return Singleton.INSTANCE;
    }


    synchronized int addUnlockHandler(INfcUnlockHandler unlockHandler, int pollMask) {
        if (mUnlockHandlers.containsKey(unlockHandler.asBinder())) {
            return mLockscreenPollMask;
        }

        mUnlockHandlers.put(unlockHandler.asBinder(),
                new UnlockHandlerWrapper(unlockHandler, pollMask));
        return (mLockscreenPollMask |= pollMask);
    }

    synchronized int removeUnlockHandler(IBinder unlockHandler) {
        if (mUnlockHandlers.containsKey(unlockHandler)) {
            mUnlockHandlers.remove(unlockHandler);
            mLockscreenPollMask = recomputePollMask();
        }

        return mLockscreenPollMask;
    }

    synchronized boolean tryUnlock(Tag tag) {
        Iterator<IBinder> iterator = mUnlockHandlers.keySet().iterator();
        while (iterator.hasNext()) {
            try {
                IBinder binder = iterator.next();
                UnlockHandlerWrapper handlerWrapper = mUnlockHandlers.get(binder);
                if (handlerWrapper.mUnlockHandler.onUnlockAttempted(tag)) {
                    return true;
                }
            } catch (Exception e) {
                Log.e(TAG, "failed to communicate with unlock handler, removing", e);
                iterator.remove();
                mLockscreenPollMask = recomputePollMask();
            }
        }

        return false;
    }

    private int recomputePollMask() {
        int pollMask = 0;
        for (UnlockHandlerWrapper wrapper : mUnlockHandlers.values()) {
            pollMask |= wrapper.mPollMask;
        }
        return pollMask;
    }

    synchronized int getLockscreenPollMask() {
        return mLockscreenPollMask;
    }

    synchronized boolean isLockscreenPollingEnabled() {
        return mLockscreenPollMask != 0;
    }

    private static class Singleton {
        private static final NfcUnlockManager INSTANCE = new NfcUnlockManager();
    }

    private NfcUnlockManager() {
        mUnlockHandlers = new HashMap<IBinder, UnlockHandlerWrapper>();
        mLockscreenPollMask = 0;
    }
}
