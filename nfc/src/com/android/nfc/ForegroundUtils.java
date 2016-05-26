/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.android.nfc;

import java.util.ArrayList;
import java.util.List;

import android.app.ActivityManagerNative;
import android.app.IActivityManager;
import android.app.IProcessObserver;
import android.os.RemoteException;
import android.util.Log;
import android.util.SparseArray;
import android.util.SparseBooleanArray;

public class ForegroundUtils extends IProcessObserver.Stub {
    static final boolean DBG = false;
    private final String TAG = "ForegroundUtils";
    private final IActivityManager mIActivityManager;

    private final Object mLock = new Object();
    // We need to keep track of the individual PIDs per UID,
    // since a single UID may have multiple processes running
    // that transition into foreground/background state.
    private final SparseArray<SparseBooleanArray> mForegroundUidPids =
            new SparseArray<SparseBooleanArray>();
    private final SparseArray<List<Callback>> mBackgroundCallbacks =
            new SparseArray<List<Callback>>();

    private static class Singleton {
        private static final ForegroundUtils INSTANCE = new ForegroundUtils();
    }

    private ForegroundUtils() {
        mIActivityManager = ActivityManagerNative.getDefault();
        try {
            mIActivityManager.registerProcessObserver(this);
        } catch (RemoteException e) {
            // Should not happen!
            Log.e(TAG, "ForegroundUtils: could not get IActivityManager");
        }
    }

    public interface Callback {
        void onUidToBackground(int uid);
    }

    public static ForegroundUtils getInstance() {
        return Singleton.INSTANCE;
    }

    /**
     * Checks whether the specified UID has any activities running in the foreground,
     * and if it does, registers a callback for when that UID no longer has any foreground
     * activities. This is done atomically, so callers can be ensured that they will
     * get a callback if this method returns true.
     *
     * @param callback Callback to be called
     * @param uid The UID to be checked
     * @return true when the UID has an Activity in the foreground and the callback
     * , false otherwise
     */
    public boolean registerUidToBackgroundCallback(Callback callback, int uid) {
        synchronized (mLock) {
            if (!isInForegroundLocked(uid)) {
                return false;
            }
            // This uid is in the foreground; register callback for when it moves
            // into the background.
            List<Callback> callbacks = mBackgroundCallbacks.get(uid, new ArrayList<Callback>());
            callbacks.add(callback);
            mBackgroundCallbacks.put(uid, callbacks);
            return true;
        }
    }

    /**
     * @param uid The UID to be checked
     * @return whether the UID has any activities running in the foreground
     */
    public boolean isInForeground(int uid) {
        synchronized (mLock) {
            return isInForegroundLocked(uid);
        }
    }

    /**
     * @return a list of UIDs currently in the foreground, or an empty list
     *         if none are found.
     */
    public List<Integer> getForegroundUids() {
        ArrayList<Integer> uids = new ArrayList<Integer>(mForegroundUidPids.size());
        synchronized (mLock) {
            for (int i = 0; i < mForegroundUidPids.size(); i++) {
                uids.add(mForegroundUidPids.keyAt(i));
            }
        }
        return uids;
    }

    private boolean isInForegroundLocked(int uid) {
        return mForegroundUidPids.get(uid) != null;
    }

    private void handleUidToBackground(int uid) {
        ArrayList<Callback> pendingCallbacks = null;
        synchronized (mLock) {
            List<Callback> callbacks = mBackgroundCallbacks.get(uid);
            if (callbacks != null) {
                pendingCallbacks = new ArrayList<Callback>(callbacks);
                // Only call them once
                mBackgroundCallbacks.remove(uid);
            }
        }
        // Release lock for callbacks
        if (pendingCallbacks != null) {
            for (Callback callback : pendingCallbacks) {
                callback.onUidToBackground(uid);
            }
        }
    }

    @Override
    public void onForegroundActivitiesChanged(int pid, int uid,
            boolean hasForegroundActivities) throws RemoteException {
        boolean uidToBackground = false;
        synchronized (mLock) {
            SparseBooleanArray foregroundPids = mForegroundUidPids.get(uid,
                    new SparseBooleanArray());
            if (hasForegroundActivities) {
               foregroundPids.put(pid, true);
            } else {
               foregroundPids.delete(pid);
            }
            if (foregroundPids.size() == 0) {
                mForegroundUidPids.remove(uid);
                uidToBackground = true;
            } else {
                mForegroundUidPids.put(uid, foregroundPids);
            }
        }
        if (uidToBackground) {
            handleUidToBackground(uid);
        }
        if (DBG) {
            if (DBG) Log.d(TAG, "Foreground changed, PID: " + Integer.toString(pid) + " UID: " +
                                    Integer.toString(uid) + " foreground: " +
                                    hasForegroundActivities);
            synchronized (mLock) {
                Log.d(TAG, "Foreground UID/PID combinations:");
                for (int i = 0; i < mForegroundUidPids.size(); i++) {
                    int foregroundUid = mForegroundUidPids.keyAt(i);
                    SparseBooleanArray foregroundPids = mForegroundUidPids.get(foregroundUid);
                    if (foregroundPids.size() == 0) {
                        Log.e(TAG, "No PIDS associated with foreground UID!");
                    }
                    for (int j = 0; j < foregroundPids.size(); j++)
                        Log.d(TAG, "UID: " + Integer.toString(foregroundUid) + " PID: " +
                                Integer.toString(foregroundPids.keyAt(j)));
                }
            }
        }
    }


    @Override
    public void onProcessDied(int pid, int uid) throws RemoteException {
        if (DBG) Log.d(TAG, "Process died; UID " + Integer.toString(uid) + " PID " +
                Integer.toString(pid));
        onForegroundActivitiesChanged(pid, uid, false);
    }

    @Override
    public void onProcessStateChanged(int pid, int uid, int procState)
            throws RemoteException {
        // Don't care
    }
}
