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
package com.gsma.nfc.internal;

import android.content.Context;
import android.content.Intent;

import java.util.List;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import android.content.BroadcastReceiver;
import android.content.pm.ResolveInfo;
import android.content.pm.PackageInfo;

import android.util.Log;
import com.nxp.nfc.gsma.internal.INxpNfcController;
import com.android.nfc.cardemulation.CardEmulationManager;
import com.android.nfc.cardemulation.RegisteredAidCache;
import android.nfc.cardemulation.ApduServiceInfo;
import android.os.Binder;
import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import com.android.nfc.NfcPermissions;
import com.android.nfc.NfcService;
import com.nxp.nfc.NxpConstants;


public class NxpNfcController {

    private static int ROUTING_TABLE_EE_MAX_AID_CFG_LEN = 580;
    public static final int PN65T_ID = 2;
    public static final int PN66T_ID = 4;


    private Context mContext;
    final NxpNfcControllerInterface mNxpNfcControllerInterface;
    final RegisteredNxpServicesCache mServiceCache;
    private RegisteredAidCache mRegisteredAidCache;
    static final String TAG = "NxpNfcController";
    boolean DBG = true;

    public ArrayList<String> mEnabledMultiEvts = new ArrayList<String>();
    public final HashMap<String, Boolean> mMultiReceptionMap = new HashMap<String, Boolean>();
    //private NfcService mService;
    private Object mWaitCheckCert = null;
    private boolean mHasCert = false;
    private Object mWaitOMACheckCert = null;
    private boolean mHasOMACert = false;
    private ComponentName unicastPkg = null;

    public NxpNfcController(Context context, CardEmulationManager cardEmulationManager) {
        mContext = context;
        mServiceCache = cardEmulationManager.getRegisteredNxpServicesCache();
        mRegisteredAidCache = cardEmulationManager.getRegisteredAidCache();
        mNxpNfcControllerInterface = new NxpNfcControllerInterface();
    }

    public INxpNfcController getNxpNfcControllerInterface() {
        if(mNxpNfcControllerInterface != null) {
            Log.d("NxpNfcController", "GSMA: mNxpNfcControllerInterface is not Null");
            return mNxpNfcControllerInterface;
        }
        return null;
    }

    public ArrayList<String> getEnabledMultiEvtsPackageList() {
        if(mEnabledMultiEvts.size() == 0x00) {
            Log.d(TAG, " check for unicast mode service resolution");
            getPackageListUnicastMode();
        }
        return mEnabledMultiEvts;
    }

    public void setResultForCertificates(boolean result) {
        Log.d(TAG, "setResultForCertificates() Start");
        synchronized (mWaitCheckCert) {
            if (mWaitCheckCert != null) {
                if (result) {
                    mHasCert = true;
                } else {
                    mHasCert = false;
                }
                mWaitCheckCert.notify();
            }
        }
        Log.d(TAG, "setResultForCertificates() End");
    }

    private boolean checkCertificatesFromUICC(String pkg, String seName) {
        Log.d(TAG, "checkCertificatesFromUICC() " + pkg + ", " + seName);
        Intent CertificateIntent = new Intent();
        CertificateIntent.setAction(NxpConstants.ACTION_CHECK_X509);
        CertificateIntent.setPackage(NxpConstants.SET_PACKAGE_NAME);
        CertificateIntent.putExtra(NxpConstants.EXTRA_SE_NAME, seName);
        CertificateIntent.putExtra(NxpConstants.EXTRA_PKG, pkg);
        mContext.sendBroadcast(CertificateIntent);

        mWaitCheckCert = new Object();
        mHasCert = false;
        try {
            synchronized (mWaitCheckCert) {
                mWaitCheckCert.wait(1000); // timeout ms
            }
        } catch (InterruptedException e) {
            Log.w(TAG, "interruped exception .");
        }
        mWaitCheckCert = null;

        if (mHasCert) {
            return true;
        } else {
            return false;
        }
   }

    private boolean checkX509CertificatesFromSim (String pkg, String seName) {
        if (DBG) Log.d(TAG, "checkX509CertificatesFromSim() " + pkg + ", " + seName);

        Intent checkCertificateIntent = new Intent();
        checkCertificateIntent.setAction("org.simalliance.openmobileapi.service.ACTION_CHECK_X509");
        checkCertificateIntent.setPackage("org.simalliance.openmobileapi.service");
        checkCertificateIntent.putExtra("org.simalliance.openmobileapi.service.EXTRA_SE_NAME", seName);
        checkCertificateIntent.putExtra("org.simalliance.openmobileapi.service.EXTRA_PKG", pkg);
        mContext.sendBroadcast(checkCertificateIntent);

        mWaitOMACheckCert = new Object();
        mHasOMACert = false;
        try {
            synchronized (mWaitOMACheckCert) {
                mWaitOMACheckCert.wait(1000); //add timeout ms
            }
        } catch (InterruptedException e) {
            // Should not happen; fall-through to abort.
            Log.w(TAG, "interruped.");
        }
        mWaitOMACheckCert = null;

        if (mHasOMACert) {
            return true;
        } else {
            return false;
        }
   }

    public void setResultForX509Certificates(boolean result) {
        Log.d(TAG, "setResultForX509Certificates() Start");
        synchronized (mWaitOMACheckCert) {
            if (mWaitOMACheckCert != null) {
                if (result) {
                    mHasOMACert = true;
                } else {
                    mHasOMACert = false;
                }
                mWaitOMACheckCert.notify();
            }
        }
        Log.d(TAG, "setResultForX509Certificates() End");
    }

    static byte[] hexStringToBytes(String s) {
        if (s == null || s.length() == 0) return null;
        int len = s.length();
        if (len % 2 != 0) {
            s = '0' + s;
            len++;
        }
        byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            data[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4)
                    + Character.digit(s.charAt(i + 1), 16));
        }
        return data;
    }

    private long getApplicationInstallTime(String packageName) {
        PackageManager pm = mContext.getPackageManager();
        try {
            PackageInfo pInfo = pm.getPackageInfo(packageName ,0);
            return pInfo.firstInstallTime;
        }catch(NameNotFoundException exception) {
            Log.e(TAG, "Application install time not retrieved");
            return 0;
        }
    }

    private void getPackageListUnicastMode () {
        unicastPkg = null;
        ArrayList<ApduServiceInfo> regApduServices = mServiceCache.getApduservicesList();
        PackageManager pm = mContext.getPackageManager();
        List<ResolveInfo> intentServices = pm.queryIntentActivities(
                new Intent(NxpConstants.ACTION_MULTI_EVT_TRANSACTION),
                PackageManager.GET_INTENT_FILTERS| PackageManager.GET_RESOLVED_FILTER);
        ArrayList<String> apduResolvedServices = new ArrayList<String>();
        String packageName = null;
        String resolvedApduService = null;
        int highestPriority = -1000;
        long minInstallTime;
        ResolveInfo resolveInfoService = null;

        for(ApduServiceInfo service : regApduServices) {
            packageName = service.getComponent().getPackageName();
            for(ResolveInfo resInfo : intentServices){
                resolveInfoService = null;
                Log.e(TAG, " Registered Service in resolved cache"+resInfo.activityInfo.packageName);
                if(resInfo.activityInfo.packageName.equals(packageName)) {
                    resolveInfoService = resInfo;
                    break;
                }
            }
            if(resolveInfoService == null) {
                Log.e(TAG, " Registered Service is not found in cache");
                continue;
            }
            int priority = resolveInfoService.priority;
            if(pm.checkPermission(NxpConstants.PERMISSIONS_TRANSACTION_EVENT , packageName) ==
                    PackageManager.PERMISSION_GRANTED)
            {
                if(checkCertificatesFromUICC(packageName, NxpConstants.UICC_ID) == true)
                {
                    if(priority == highestPriority) {
                        apduResolvedServices.add(packageName);
                    } else if(highestPriority < priority) {
                        highestPriority = priority;
                        apduResolvedServices.clear();
                        apduResolvedServices.add(packageName);
                    }
                }
            }
        }
        if(apduResolvedServices.size() == 0x00) {
            Log.e(TAG, "No services to resolve, not starting the activity");
            return;
        }else if(apduResolvedServices.size() > 0x01) {
            Log.e(TAG, " resolved"+apduResolvedServices.size());
            minInstallTime = getApplicationInstallTime(apduResolvedServices.get(0));
            for(String resolvedService : apduResolvedServices) {
                if(getApplicationInstallTime(resolvedService) <= minInstallTime ) {
                    minInstallTime = getApplicationInstallTime(resolvedService);
                    resolvedApduService = resolvedService;
                }
                Log.e(TAG, " Install time  of application"+ minInstallTime);
            }

        } else  resolvedApduService = apduResolvedServices.get(0);

        Log.e(TAG, " Final Resolved Service"+resolvedApduService);
        if(resolvedApduService != null) {
            for(ResolveInfo resolve : intentServices) {
                if(resolve.activityInfo.packageName.equals(resolvedApduService)) {
                    unicastPkg = new ComponentName(resolvedApduService ,resolve.activityInfo.name);
                    break;
                }
            }
        }
    }

    public ComponentName getUnicastPackage() {
        return unicastPkg;
    }

    final class NxpNfcControllerInterface extends INxpNfcController.Stub {

        @Override
        public boolean deleteOffHostService(int userId, String packageName, ApduServiceInfo service) {
            return mServiceCache.deleteApduService(userId, Binder.getCallingUid(), packageName, service);
        }

        @Override
        public ArrayList<ApduServiceInfo> getOffHostServices(int userId, String packageName) {
            return mServiceCache.getApduServices(userId, Binder.getCallingUid(), packageName);
        }

        @Override
        public ApduServiceInfo getDefaultOffHostService(int userId, String packageName) {
            HashMap<ComponentName, ApduServiceInfo> mapServices = mServiceCache.getApduservicesMaps();
            ComponentName preferredPaymentService = mRegisteredAidCache.getPreferredPaymentService();
            if(preferredPaymentService != null) {
                if(preferredPaymentService.getPackageName() != null &&
                    !preferredPaymentService.getPackageName().equals(packageName)) {
                    Log.d(TAG, "getDefaultOffHostService unregistered package Name");
                    return null;
                }
                String defaultservice = preferredPaymentService.getClassName();

                //If Default is Dynamic Service
                for (Map.Entry<ComponentName, ApduServiceInfo> entry : mapServices.entrySet())
                {
                    if(defaultservice.equals(entry.getKey().getClassName())) {
                        Log.d(TAG, "getDefaultOffHostService: Dynamic: "+ entry.getValue().getAids().size());
                        return entry.getValue();
                    }
                }

                //If Default is Static Service
                HashMap<ComponentName, ApduServiceInfo>  staticServices = mServiceCache.getInstalledStaticServices();
                for (Map.Entry<ComponentName, ApduServiceInfo> entry : staticServices.entrySet()) {
                    if(defaultservice.equals(entry.getKey().getClassName())) {
                        Log.d(TAG, "getDefaultOffHostService: Static: "+ entry.getValue().getAids().size());
                        return entry.getValue();
                    }
                }
            }
            return null;
        }

        @Override
        public boolean commitOffHostService(int userId, String packageName, String serviceName, ApduServiceInfo service) {
            int aidLength = 0;
            boolean is_table_size_required = true;
            List<String>  newAidList = new ArrayList<String>();
            List<String>  oldAidList = new ArrayList<String>();

            for (int i=0; i<service.getAids().size(); i++){   // Convering String AIDs to Aids Length
                aidLength = aidLength + hexStringToBytes(service.getAids().get(i)).length;
            }
            Log.d(TAG, "Total commiting aids Length:  "+ aidLength);

            ArrayList<ApduServiceInfo> serviceList = mServiceCache.getApduServices(userId, Binder.getCallingUid(), packageName);
           for(int i=0; i< serviceList.size(); i++) {
                Log.d(TAG, "All Service Names["+i +"] "+ serviceList.get(i).getComponent().getClassName());
                if(serviceName.equalsIgnoreCase(serviceList.get(i).getComponent().getClassName())) {
                    oldAidList = serviceList.get(i).getAids();
                    newAidList = service.getAids();
                    Log.d(TAG, "Commiting Existing Service:  "+ serviceName);
                    break;
                }
           }

           int newAidListSize;
           for(newAidListSize = 0; newAidListSize < newAidList.size(); newAidListSize++) {
               if(!oldAidList.contains(newAidList.get(newAidListSize))) {
                   is_table_size_required = true;             // Need to calculate Roting table Size, if New Aids Added
                   Log.d(TAG, "New Aids Added  ");
                   break;
               }
           }

           if((newAidList.size() != 0) && (newAidListSize == newAidList.size())) {
               is_table_size_required = false;        // No Need to calculate Routing size
           }

           Log.d(TAG, "is routing Table size calcution required :  "+ is_table_size_required);
           if((is_table_size_required == true) && NfcService.getInstance().getRemainingAidTableSize() < aidLength) {
               return false;
           }

           Log.d(TAG, "Commiting :  ");
           return mServiceCache.registerApduService(userId, Binder.getCallingUid(), packageName, serviceName, service);
        }

        @Override
        public boolean enableMultiEvt_NxptransactionReception(String packageName, String seName) {
            boolean result = false;
            if(checkCertificatesFromUICC(packageName, seName) == true) {
                mEnabledMultiEvts.add(packageName);
                result = true;
            } else {
                result = false;
            }

            return result;
        }

        @Override
        public void enableMultiReception(String pkg, String seName) {
            if (DBG) Log.d(TAG, "enableMultiReception() " + pkg + " " + seName);

            if (seName.startsWith("SIM")) {
                if (checkX509CertificatesFromSim (pkg, seName) == false) {
                    throw new SecurityException("No cerficates from " + seName);
                }
            } else {
                NfcService.getInstance().enforceNfceeAdminPerm(pkg);
                //NfcPermissions.enforceAdminPermissions(mContext);
            }

            mMultiReceptionMap.remove(seName);
            mMultiReceptionMap.put(seName, Boolean.TRUE);
        }
    }
}
