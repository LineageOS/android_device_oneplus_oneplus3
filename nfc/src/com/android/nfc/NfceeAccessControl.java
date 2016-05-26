/*
 * Copyright (C) 2011 The Android Open Source Project
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
/******************************************************************************
 *
 *  The original Work has been changed by NXP Semiconductors.
 *
 *  Copyright (C) 2015 NXP Semiconductors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

package com.android.nfc;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashMap;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Environment;
import android.util.Log;

public class NfceeAccessControl {
    static final String TAG = "NfceeAccess";
    static final boolean DBG = true;

    public static final String NFCEE_ACCESS_PATH = "/etc/nfcee_access.xml";

    /**
     * Map of signatures to valid packages names, as read from nfcee_access.xml.
     * An empty list of package names indicates that any package
     * with this signature is allowed.
     */
    final HashMap<Signature, String[]> mNfceeAccess;  // contents final after onCreate()

    /**
     * Map from UID to NFCEE access, used as a cache.
     * Note: if a UID contains multiple packages they must all be
     * signed with the same certificate so in effect UID == certificate
     * used to sign the package.
     */
    final HashMap<Integer, Boolean> mUidCache;  // contents guarded by this

    final Context mContext;
    final boolean mDebugPrintSignature;

    NfceeAccessControl(Context context) {
        mContext = context;
        mNfceeAccess = new HashMap<Signature, String[]>();
        mUidCache = new HashMap<Integer, Boolean>();
        mDebugPrintSignature = parseNfceeAccess();
    }

    /**
     * Check if the {uid, pkg} combination may use NFCEE.
     * Also verify with package manager that this {uid, pkg} combination
     * is valid if it is not cached.
     */
    public boolean check(int uid, String pkg) {
        synchronized (this) {
            Boolean cached = mUidCache.get(uid);
            if (cached != null) {
                return cached;
            }

            boolean access = false;

            // Ensure the claimed package is present in the calling UID
            PackageManager pm = mContext.getPackageManager();
            String[] pkgs = pm.getPackagesForUid(uid);
            for (String uidPkg : pkgs) {
                if (uidPkg.equals(pkg)) {
                    // Ensure the package has access permissions
                    if (checkPackageNfceeAccess(pkg)) {
                        access = true;
                    }
                    break;
                }
            }

            mUidCache.put(uid, access);
            return access;
        }
    }

    /**
     * Check if the given ApplicationInfo may use the NFCEE.
     * Assumes ApplicationInfo came from package manager,
     * so no need to confirm {uid, pkg} is valid.
     */
    public boolean check(ApplicationInfo info) {
        synchronized (this) {
            Boolean access = mUidCache.get(info.uid);
            if (access == null) {
                access = checkPackageNfceeAccess(info.packageName);
                mUidCache.put(info.uid, access);
            }
            return access;
        }
    }

    public void invalidateCache() {
        synchronized (this) {
            mUidCache.clear();
        }
    }

    /**
     * Check with package manager if the pkg may use NFCEE.
     * Does not use cache.
     */
    boolean checkPackageNfceeAccess(String pkg) {
        PackageManager pm = mContext.getPackageManager();
        try {
            PackageInfo info = pm.getPackageInfo(pkg, PackageManager.GET_SIGNATURES);
            if (info.signatures == null) {
                return false;
            }

            for (Signature s : info.signatures){
                if (s == null) {
                    continue;
                }
                String[] packages = mNfceeAccess.get(s);
                if (packages == null) {
                    continue;
                }
                if (packages.length == 0) {
                    // wildcard access
                    if (DBG) Log.d(TAG, "Granted NFCEE access to " + pkg + " (wildcard)");
                    return true;
                }
                for (String p : packages) {
                    if (pkg.equals(p)) {
                        // explicit package access
                        if (DBG) Log.d(TAG, "Granted access to " + pkg + " (explicit)");
                        return true;
                    }
                }
            }

            if (mDebugPrintSignature) {
                Log.w(TAG, "denied NFCEE access for " + pkg + " with signature:");
                for (Signature s : info.signatures) {
                    if (s != null) {
                        Log.w(TAG, s.toCharsString());
                    }
                }
            }
        } catch (NameNotFoundException e) {
            // ignore
        }
        return false;
    }

    /**
     * Parse nfcee_access.xml, populate mNfceeAccess
     * Policy is to ignore unexpected XML elements and continue processing,
     * except for obvious errors within a <signer> group since they might cause
     * package names to by ignored and therefore wildcard access granted
     * by mistake. Those errors invalidate the entire <signer> group.
     */
    boolean parseNfceeAccess() {
        File file = new File(Environment.getRootDirectory(), NFCEE_ACCESS_PATH);
        FileReader reader = null;
        boolean debug = false;
        try {
            reader = new FileReader(file);
            XmlPullParserFactory factory = XmlPullParserFactory.newInstance();
            XmlPullParser parser = factory.newPullParser();
            parser.setInput(reader);

            int event;
            ArrayList<String> packages = new ArrayList<String>();
            Signature signature = null;
            parser.setFeature(XmlPullParser.FEATURE_PROCESS_NAMESPACES, false);
            while (true) {
                event = parser.next();
                String tag = parser.getName();
                if (event == XmlPullParser.START_TAG && "signer".equals(tag)) {
                    signature = null;
                    packages.clear();
                    for (int i = 0; i < parser.getAttributeCount(); i++) {
                        if ("android:signature".equals(parser.getAttributeName(i))) {
                            signature = new Signature(parser.getAttributeValue(i));
                            break;
                        }
                    }
                    if (signature == null) {
                        Log.w(TAG, "signer tag is missing android:signature attribute, igorning");
                        continue;
                    }
                    if (mNfceeAccess.containsKey(signature)) {
                        Log.w(TAG, "duplicate signature, ignoring");
                        signature = null;
                        continue;
                    }
                } else if (event == XmlPullParser.END_TAG && "signer".equals(tag)) {
                    if (signature == null) {
                        Log.w(TAG, "mis-matched signer tag");
                        continue;
                    }
                    mNfceeAccess.put(signature, packages.toArray(new String[0]));
                    packages.clear();
                } else if (event == XmlPullParser.START_TAG && "package".equals(tag)) {
                    if (signature == null) {
                        Log.w(TAG, "ignoring unnested packge tag");
                        continue;
                    }
                    String name = null;
                    for (int i = 0; i < parser.getAttributeCount(); i++) {
                        if ("android:name".equals(parser.getAttributeName(i))) {
                            name = parser.getAttributeValue(i);
                            break;
                        }
                    }
                    if (name == null) {
                        Log.w(TAG, "package missing android:name, ignoring signer group");
                        signature = null;  // invalidate signer
                        continue;
                    }
                    // check for duplicate package names
                    if (packages.contains(name)) {
                        Log.w(TAG, "duplicate package name in signer group, ignoring");
                        continue;
                    }
                    packages.add(name);
                } else if (event == XmlPullParser.START_TAG && "debug".equals(tag)) {
                    debug = true;
                } else if (event == XmlPullParser.END_DOCUMENT) {
                    break;
                }
            }
        } catch (XmlPullParserException e) {
            Log.w(TAG, "failed to load NFCEE access list", e);
            mNfceeAccess.clear();  // invalidate entire access list
        } catch (FileNotFoundException e) {
            Log.w(TAG, "could not find " + NFCEE_ACCESS_PATH + ", no NFCEE access allowed");
        } catch (IOException e) {
            Log.e(TAG, "Failed to load NFCEE access list", e);
            mNfceeAccess.clear();  // invalidate entire access list
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException e2)  { }
            }
        }
        Log.i(TAG, "read " + mNfceeAccess.size() + " signature(s) for NFCEE access");
        return debug;
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("mNfceeAccess=");
        for (Signature s : mNfceeAccess.keySet()) {
            pw.printf("\t%s [", s.toCharsString());
            String[] ps = mNfceeAccess.get(s);
            for (String p : ps) {
                pw.printf("%s, ", p);
            }
            pw.println("]");
        }
        synchronized (this) {
            pw.println("mNfceeUidCache=");
            for (Integer uid : mUidCache.keySet()) {
                Boolean b = mUidCache.get(uid);
                pw.printf("\t%d %s\n", uid, b);
            }
        }
    }
}
