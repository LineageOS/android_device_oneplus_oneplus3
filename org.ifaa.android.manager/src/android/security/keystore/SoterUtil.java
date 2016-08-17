package android.security.keystore;

import android.util.Base64;
import android.util.Log;
import org.json.JSONException;
import org.json.JSONObject;

public class SoterUtil {
    public static final String JSON_KEY_PUBLIC = "pub_key";
    private static final String PARAM_NEED_AUTO_ADD_COUNTER_WHEN_GET_PUBLIC_KEY = "addcounter";
    private static final String PARAM_NEED_AUTO_ADD_SECMSG_FID_COUNTER_WHEN_SIGN = "secmsg_and_counter_signed_when_sign";
    private static final String PARAM_NEED_AUTO_SIGNED_WITH_ATTK_WHEN_GET_PUBLIC_KEY = "auto_signed_when_get_pubkey_attk";
    private static final String PARAM_NEED_AUTO_SIGNED_WITH_COMMON_KEY_WHEN_GET_PUBLIC_KEY = "auto_signed_when_get_pubkey";
    private static final String PARAM_NEED_NEXT_ATTK = "next_attk";
    private static final int RAW_LENGTH_PREFIX = 4;
    public static final String TAG = "Soter.Util";

    public static SoterRSAKeyGenParameterSpec convertKeyNameToParameterSpec(String name) {
        if (isNullOrNil(name)) {
            Log.e(TAG, "hy: null or nil when convert key name to parameter");
            return null;
        }
        String[] splits = name.split("\\.");
        if (splits == null || splits.length <= 1) {
            Log.w(TAG, "hy: pure alias, no parameter");
            return null;
        }
        boolean isAutoSignedWithAttkWhenGetPublicKey = false;
        boolean isAutoSignedWithCommonkWhenGetPublicKey = false;
        String mAutoSignedKeyNameWhenGetPublicKey = "";
        boolean isSecmsgFidCounterSignedWhenSign = false;
        boolean isAutoAddCounterWhenGetPublicKey = false;
        boolean isNeedNextAttk = false;
        if (contains(PARAM_NEED_AUTO_SIGNED_WITH_ATTK_WHEN_GET_PUBLIC_KEY, splits)) {
            isAutoSignedWithAttkWhenGetPublicKey = true;
        } else {
            String entireCommonKeyExpr = containsPrefix(PARAM_NEED_AUTO_SIGNED_WITH_COMMON_KEY_WHEN_GET_PUBLIC_KEY, splits);
            if (!isNullOrNil(entireCommonKeyExpr)) {
                String commonKeyName = retrieveKeyNameFromExpr(entireCommonKeyExpr);
                if (!isNullOrNil(commonKeyName)) {
                    isAutoSignedWithCommonkWhenGetPublicKey = true;
                    mAutoSignedKeyNameWhenGetPublicKey = commonKeyName;
                }
            }
        }
        if (contains(PARAM_NEED_AUTO_ADD_SECMSG_FID_COUNTER_WHEN_SIGN, splits)) {
            isSecmsgFidCounterSignedWhenSign = true;
        }
        if (contains(PARAM_NEED_AUTO_ADD_COUNTER_WHEN_GET_PUBLIC_KEY, splits)) {
            isAutoAddCounterWhenGetPublicKey = true;
            if (contains(PARAM_NEED_NEXT_ATTK, splits)) {
                isNeedNextAttk = true;
            }
        }
        SoterRSAKeyGenParameterSpec spec = new SoterRSAKeyGenParameterSpec(true, isAutoSignedWithAttkWhenGetPublicKey, isAutoSignedWithCommonkWhenGetPublicKey, mAutoSignedKeyNameWhenGetPublicKey, isSecmsgFidCounterSignedWhenSign, isAutoAddCounterWhenGetPublicKey, isNeedNextAttk);
        Log.i(TAG, "hy: spec: " + spec.toString());
        return spec;
    }

    private static String retrieveKeyNameFromExpr(String expr) {
        if (isNullOrNil(expr)) {
            Log.e(TAG, "hy: expr is null");
            return null;
        }
        int startPos = expr.indexOf("(");
        int endPos = expr.indexOf(")");
        if (startPos >= 0 && endPos > startPos) {
            return expr.substring(startPos + 1, endPos);
        }
        Log.e(TAG, "hy: no key name");
        return null;
    }

    private static boolean contains(String target, String[] src) {
        if (src == null || src.length == 0 || isNullOrNil(target)) {
            Log.e(TAG, "hy: param error");
            throw new IllegalArgumentException("param error");
        }
        for (String item : src) {
            if (target.equals(item)) {
                return true;
            }
        }
        return false;
    }

    private static String containsPrefix(String prefix, String[] src) {
        if (src == null || src.length == 0 || isNullOrNil(prefix)) {
            Log.e(TAG, "hy: param error");
            throw new IllegalArgumentException("param error");
        }
        for (String item : src) {
            if (!isNullOrNil(item) && item.startsWith(prefix)) {
                return item;
            }
        }
        return null;
    }

    public static String getPureKeyAliasFromKeyName(String name) {
        Log.i(TAG, "hy: retrieving pure name from: " + name);
        if (isNullOrNil(name)) {
            Log.e(TAG, "hy: null or nil when get pure key alias");
            return null;
        }
        String[] splits = name.split("\\.");
        if (splits != null && splits.length > 1) {
            return splits[0];
        }
        Log.d(TAG, "hy: pure alias");
        return name;
    }

    public static boolean isNullOrNil(String str) {
        return str == null || str.equals("");
    }

    public static byte[] getDataFromRaw(byte[] origin, String jsonKey) throws JSONException {
        if (isNullOrNil(jsonKey)) {
            Log.e("Soter", "hy: json keyname error");
            return null;
        } else if (origin == null) {
            Log.e("Soter", "hy: json origin null");
            return null;
        } else {
            JSONObject jsonObj = retriveJsonFromExportedData(origin);
            if (jsonObj == null || !jsonObj.has(jsonKey)) {
                return null;
            }
            return Base64.decode(jsonObj.getString(jsonKey).replace("-----BEGIN PUBLIC KEY-----", "").replace("-----END PUBLIC KEY-----", "").replace("\\n", ""), 0);
        }
    }

    private static JSONObject retriveJsonFromExportedData(byte[] origin) {
        if (origin == null) {
            Log.e("Soter", "raw data is null");
            return null;
        }
        if (origin.length < RAW_LENGTH_PREFIX) {
            Log.e("Soter", "raw data length smaller than 4");
        }
        byte[] lengthBytes = new byte[RAW_LENGTH_PREFIX];
        System.arraycopy(origin, 0, lengthBytes, 0, RAW_LENGTH_PREFIX);
        int rawLength = toInt(lengthBytes);
        byte[] rawJsonBytes = new byte[rawLength];
        if (origin.length <= rawLength + RAW_LENGTH_PREFIX) {
            Log.e("Soter", "length not correct 2");
            return null;
        }
        System.arraycopy(origin, RAW_LENGTH_PREFIX, rawJsonBytes, 0, rawLength);
        try {
            return new JSONObject(new String(rawJsonBytes));
        } catch (JSONException e) {
            Log.e("Soter", "hy: can not convert to json");
            return null;
        }
    }

    public static int toInt(byte[] bRefArr) {
        int iOutcome = 0;
        for (int i = 0; i < bRefArr.length; i++) {
            iOutcome += (bRefArr[i] & 255) << (i * 8);
        }
        return iOutcome;
    }
}
