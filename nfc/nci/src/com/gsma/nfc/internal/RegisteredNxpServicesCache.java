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



import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.pm.PackageItemInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.nfc.cardemulation.AidGroup;
import android.nfc.cardemulation.ApduServiceInfo;
import android.nfc.cardemulation.CardEmulation;
import android.nfc.cardemulation.HostApduService;
import android.nfc.cardemulation.OffHostApduService;
import android.os.UserHandle;
import android.util.AtomicFile;
import android.util.Log;
import android.util.SparseArray;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.util.Xml;
import android.graphics.drawable.Drawable;
import android.graphics.BitmapFactory;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import com.android.internal.util.FastXmlSerializer;
import com.google.android.collect.Maps;

import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicReference;
import com.android.nfc.cardemulation.RegisteredServicesCache;

public class RegisteredNxpServicesCache {
    static final String TAG = "RegisteredNxpServicesCache";
    static final String XML_INDENT_OUTPUT_FEATURE = "http://xmlpull.org/v1/doc/features.html#indent-output";
    final Context mContext;
    final Object mLock = new Object();

    private RegisteredServicesCache mRegisteredServicesCache;
    final HashMap<ComponentName, ApduServiceInfo> mApduServices = Maps.newHashMap();
    final HashMap<ComponentName , String> mApduBanner = Maps.newHashMap();
    AtomicFile mDynamicApduServiceFile = null;
    File dataDir = null;

    public RegisteredNxpServicesCache(Context context) {
        mContext = context;
        //mDynamicApduServiceFile = new AtomicFile(new File(dataDir, "dynamic_apduservice.xml"));
    }

    public RegisteredNxpServicesCache(Context context, RegisteredServicesCache registeredServicesCache) {
        mContext = context;
        mRegisteredServicesCache = registeredServicesCache;

        dataDir = mContext.getFilesDir();
        mDynamicApduServiceFile = new AtomicFile(new File(dataDir, "dynamic_apduservice.xml"));
    }


    private String writeDrawableAsBitMap(Drawable drawable , String path) {
        File file = new File(dataDir, path);
        Log.d(TAG, " input Path "+ path);
        Bitmap bitmap = ((BitmapDrawable)drawable).getBitmap();
        Log.d(TAG, "drawablePath: "+ file.getPath());
        try {
            FileOutputStream outStream = new FileOutputStream(file);
            bitmap.compress(Bitmap.CompressFormat.PNG, 100, outStream);
            outStream.flush();
            outStream.close();
            return file.getPath();
        } catch (IOException e) {
            Log.d(TAG, " input Path Not found"+ e.getMessage());
            return null;
        }
    }

    private Drawable readDrawableFromBitMap(String drawablePath) {
        Log.d(TAG, " input Path "+ drawablePath);
        File filePath = new File(drawablePath);
        try {
            FileInputStream fi = new FileInputStream(filePath);
            Bitmap bitmap = BitmapFactory.decodeStream(fi);
            Drawable DrawableResource  = new BitmapDrawable(mContext.getResources(),bitmap);
            return DrawableResource;
        } catch (IOException e) {
            Log.d(TAG, " input Path Not found"+ e.getMessage());
            return null;
        }
    }

    private boolean deleteBitMapfromFile(String drawablePath) {
        Log.d(TAG, " delete Path "+ drawablePath);
        boolean deleted = false;
        File file = new File(drawablePath);
        if (file.exists()) {
            Log.d(TAG, " delete Path found"+ drawablePath);
            deleted = file.delete();
        }
        Log.d(TAG, "deleted "+deleted);
        return deleted;
    }
    // Register APDU Service
    public boolean registerApduService(int userId, int uid, String packageName, String serviceName, ApduServiceInfo apduService) {
        ComponentName componentName = new ComponentName(packageName, serviceName);
        Log.e(TAG,"registerApduService - incoming : " + apduService.toString());
        ApduServiceInfo cur = mApduServices.get(componentName);
        if(cur!=null)
            Log.e(TAG,"registerApduService - cur :" + cur.toString());

        mApduServices.put(componentName, apduService);

        cur = mApduServices.get(componentName);
        if(cur!=null)
            Log.e(TAG,"registerApduService - cur - after update :" + cur.toString());


        boolean status = writeDynamicApduService();
        if(status){
            mRegisteredServicesCache.invalidateCache(userId);
            return true;
        } else {
            Log.e(TAG,"Commit Failed due to writing failed to write in to the file");
            return false;
        }
    }

    // To Get the ApduServiceInfo List
    public ArrayList<ApduServiceInfo> getApduservicesList() {
        ArrayList<ApduServiceInfo> services = new ArrayList<ApduServiceInfo>();
        for (ApduServiceInfo value : mApduServices.values()) {
            services.add(value);
        }
        return services;
    }

    // To get the <ComponentName, ApduServiceInfo>  HashMap
    public HashMap<ComponentName, ApduServiceInfo> getApduservicesMaps() {
        return mApduServices;
    }

    public HashMap<ComponentName, ApduServiceInfo> getInstalledStaticServices() {
        return mRegisteredServicesCache.getAllStaticHashServices();
    }

    // update the Apduservice Info after uninstall the application.
    public void onPackageRemoved(String uninstalledpackageName) {
        if(uninstalledpackageName != null) {
            Log.d(TAG, "uninstall packageName:"+ uninstalledpackageName);
            for(Iterator<Map.Entry<ComponentName, ApduServiceInfo>>it=mApduServices.entrySet().iterator(); it.hasNext();){
                Map.Entry<ComponentName, ApduServiceInfo> entry = it.next();
                if(uninstalledpackageName.equals(entry.getKey().getPackageName())){
                    it.remove();
                    Log.d(TAG, "Removed packageName: "+ entry.getKey().getPackageName());
                    if(mApduBanner.containsKey(entry.getKey())) {
                        deleteBitMapfromFile(mApduBanner.get(entry.getKey()));
                        mApduBanner.remove(entry.getKey());
                    }
                }
            }
        } else {
            Log.d(TAG, "uninstall packageName is Null");
        }
    }

    // To Delete Apdu Service
    public boolean deleteApduService(int userId, int uid, String packageName, ApduServiceInfo apduService) {
         synchronized (mLock) {
             mApduServices.values().remove(apduService);
             writeDynamicApduService();
             if(mApduBanner.containsKey(apduService.getComponent())) {
                 deleteBitMapfromFile(mApduBanner.get(apduService.getComponent()));
                 mApduBanner.remove(apduService.getComponent());
             }
             mRegisteredServicesCache.invalidateCache(userId);
         }
         return true;
    }

    // To get Array of ApduServiceInfo
    public ArrayList<ApduServiceInfo> getApduServices(int userId, int uid, String packageName) {

        //ArrayList<ApduServiceInfo> apduInfo = new ArrayList<ApduServiceInfo>(mApduServices.values());
        ArrayList<ApduServiceInfo> apduInfo = new ArrayList<ApduServiceInfo>();
        for (Map.Entry<ComponentName, ApduServiceInfo> entry : mApduServices.entrySet()) {
            if(packageName.equals(entry.getKey().getPackageName())) {
                apduInfo.add(entry.getValue());
            }
        }
        ArrayList<ApduServiceInfo> staticServices = new ArrayList<ApduServiceInfo>(mRegisteredServicesCache.getAllServices());
        for(ApduServiceInfo service: staticServices ) {
            if(packageName.equals((service.getResolveInfo()).serviceInfo.packageName)) {
                apduInfo.add(service);
            }
        }
        return apduInfo;
    }

    public boolean writeDynamicApduService() {
        FileOutputStream fos = null;
        try {
            fos = mDynamicApduServiceFile.startWrite();
            XmlSerializer out = new FastXmlSerializer();
            out.setOutput(fos, "utf-8");
            out.startDocument(null, true);
            out.setFeature(XML_INDENT_OUTPUT_FEATURE, true);
            out.startTag(null, "apduservices");

            for(Iterator<Map.Entry<ComponentName, ApduServiceInfo>>it=mApduServices.entrySet().iterator(); it.hasNext();){
                Map.Entry<ComponentName, ApduServiceInfo> service = it.next();
                if( null != service) {
                    out.startTag(null, "service");
                    out.attribute(null, "component", service.getKey().flattenToString());
                    if(service.getValue().getBannerId() <= 0x00 && null != service.getValue().mBanner) {
                        Log.e(TAG, "writeDynamicApduService "+service.getValue().getBannerId());
                        String path = service.getKey().getPackageName() + "_"+ service.getKey().getClassName()+".png";
                        path = writeDrawableAsBitMap(service.getValue().mBanner ,path);
                        if(!mApduBanner.containsKey(service.getKey())) {
                            mApduBanner.put(service.getKey(),path);
                        }
                        out.attribute(null, "drawableResourcepath", path);
                    }
                    service.getValue().writeToXml(out);
                    out.endTag(null, "service");
                }
            }
            out.endTag(null, "apduservices");
            out.endDocument();
            mDynamicApduServiceFile.finishWrite(fos);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "Error writing dynamic APDU Service", e);
            if (fos != null) {
                mDynamicApduServiceFile.failWrite(fos);
            }
            return false;
        }
    }

    public void readDynamicApduService() {
        FileInputStream fis = null;
        try {
            if (!mDynamicApduServiceFile.getBaseFile().exists()) {
                Log.d(TAG, "Dynamic APDU Service file does not exist.");
                return;
            }
            fis = mDynamicApduServiceFile.openRead();
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(fis, null);
            int eventType = parser.getEventType();

            while (eventType != XmlPullParser.START_TAG &&
                    eventType != XmlPullParser.END_DOCUMENT) {
                eventType = parser.next();
            }
            boolean inService = false;
            ComponentName currentComponent = null;
            String drawbalePath = null;
            String description = null;
            boolean modifiable = false;
            int seId = 0;
            int userId = 0;
            int bannerId = 0;
            AidGroup aidGroup = null;
            Drawable DrawableResource = null;
            ApduServiceInfo apduService =null;
            ArrayList<AidGroup> dynamicAidGroup = new ArrayList<AidGroup>();

            String tagName = parser.getName();
            if ("apduservices".equals(tagName)) {
                while (eventType != XmlPullParser.END_DOCUMENT) {
                    tagName = parser.getName();
                    if (eventType == XmlPullParser.START_TAG) {
                        if ("service".equals(tagName) && parser.getDepth() == 2) {
                            String compString = parser.getAttributeValue(null, "component");
                            userId  = Integer.parseInt(parser.getAttributeValue(null, "uid"));
                            currentComponent = ComponentName.unflattenFromString(compString);
                            description = parser.getAttributeValue(null, "description");
                            bannerId  = Integer.parseInt(parser.getAttributeValue(null, "bannerId"));
                            String isModifiable = parser.getAttributeValue(null, "modifiable");
                            drawbalePath = parser.getAttributeValue(null, "drawableResourcepath");
                            if(isModifiable.equals("true")) {
                                modifiable = true;
                            } else {
                                modifiable = false;
                            }

                            String seIdString = parser.getAttributeValue(null, "seId");
                            seId = Integer.parseInt(seIdString);
                            inService = true;
                            Log.e(TAG, "readDynamicApduService "+bannerId);
                            if(bannerId <= 0x00) {
                                bannerId = -1;
                                DrawableResource = readDrawableFromBitMap(drawbalePath);
                                if(!mApduBanner.containsKey(currentComponent)) {
                                    mApduBanner.put(currentComponent, drawbalePath);
                                }
                            }
                        }

                        if ("aid-group".equals(tagName) && parser.getDepth() == 3 && inService) {
                            AidGroup group = AidGroup.createFromXml(parser);
                            if (group != null) {
                                dynamicAidGroup.add(group);
                            } else {
                                Log.e(TAG, "Could not parse AID group.");
                            }
                        }

                    } else if (eventType == XmlPullParser.END_TAG) {
                        if ("service".equals(tagName)) {
                            int powerstate = -1;
                            boolean onHost = false;
                            boolean requiresUnlock = false;
                            /* creating Resolveinfo object */
                            ResolveInfo resolveInfo = new ResolveInfo();
                            resolveInfo.serviceInfo = new ServiceInfo();
                            resolveInfo.serviceInfo.applicationInfo = new ApplicationInfo();
                            resolveInfo.serviceInfo.packageName = currentComponent.getPackageName();
                            resolveInfo.serviceInfo.name = currentComponent.getClassName();
                            ApduServiceInfo.ESeInfo mEseInfo = new ApduServiceInfo.ESeInfo(seId,powerstate);
                            ArrayList<android.nfc.cardemulation.AidGroup> staticAidGroups = null;
                            apduService = new ApduServiceInfo(resolveInfo,onHost,description,staticAidGroups, dynamicAidGroup,
                                                               requiresUnlock,bannerId,userId, "Fixme: NXP:<Activity Name>", mEseInfo,null, DrawableResource, modifiable);
                            mApduServices.put(currentComponent, apduService);
                            Log.d(TAG,"mApduServices size= "+ mApduServices.size());
                            dynamicAidGroup.clear();
                            inService = false;
                            currentComponent = null;
                            drawbalePath = null;
                            description = null;
                            modifiable = false;
                            seId = 0;
                            userId = 0;
                            aidGroup = null;
                            DrawableResource = null;
                            apduService =null;
                        }
                    }
                    eventType = parser.next();
                }
            }
        }  catch (Exception e) {
            Log.e(TAG, "Could not parse dynamic APDU Service file, trashing.");
            mDynamicApduServiceFile.delete();
        } finally {
            if (fis != null) {
                try {
                    fis.close();
                } catch (IOException e) {
                }
            }
        }
        Log.d(TAG,"readDynamicApduService End:   "+ mApduServices.size());
        return;
    }
}
