package android.security.keystore;

import java.math.BigInteger;
import java.security.spec.RSAKeyGenParameterSpec;

/**
 * Created by yexuan on 15/9/14.
 * @hide
 */
public class SoterRSAKeyGenParameterSpec extends RSAKeyGenParameterSpec {

    private boolean isForSoter = false;
    private boolean isAutoSignedWithAttkWhenGetPublicKey = false;
    private boolean isAutoSignedWithCommonkWhenGetPublicKey = false;
    private String mAutoSignedKeyNameWhenGetPublicKey = "";
    private boolean isSecmsgFidCounterSignedWhenSign = false;
    private boolean isAutoAddCounterWhenGetPublicKey = false;
    private boolean isNeedUseNextAttk = false;

    public SoterRSAKeyGenParameterSpec(
            int keysize, BigInteger publicExponent,
            boolean isForSoter,
            boolean isAutoSignedWithAttkWhenGetPublicKey,
            boolean isAutoSignedWithCommonkWhenGetPublicKey,
            String signedKeyNameWhenGetPublicKey,
            boolean isSecmsgFidCounterSignedWhenSign,
            boolean isAutoAddCounterWhenGetPublicKey,
            boolean isNeedNextAttk) {
        super(keysize, publicExponent);
        this.isForSoter = isForSoter;
        this.isAutoSignedWithAttkWhenGetPublicKey = isAutoSignedWithAttkWhenGetPublicKey;
        this.isAutoSignedWithCommonkWhenGetPublicKey = isAutoSignedWithCommonkWhenGetPublicKey;
        this.mAutoSignedKeyNameWhenGetPublicKey = signedKeyNameWhenGetPublicKey;
        this.isSecmsgFidCounterSignedWhenSign = isSecmsgFidCounterSignedWhenSign;
        this.isAutoAddCounterWhenGetPublicKey = isAutoAddCounterWhenGetPublicKey;
        this.isNeedUseNextAttk = isNeedNextAttk;
    }

    public SoterRSAKeyGenParameterSpec(
            boolean isForSoter,
            boolean isAutoSignedWithAttkWhenGetPublicKey,
            boolean isAutoSignedWithCommonkWhenGetPublicKey,
            String signedKeyNameWhenGetPublicKey,
            boolean isSecmsgFidCounterSignedWhenSign,
            boolean isAutoAddCounterWhenGetPubli,
            boolean isNeedNextAttkcKey) {
        this(2048, RSAKeyGenParameterSpec.F4, isForSoter, isAutoSignedWithAttkWhenGetPublicKey,
                isAutoSignedWithCommonkWhenGetPublicKey,
                signedKeyNameWhenGetPublicKey, isSecmsgFidCounterSignedWhenSign, isAutoAddCounterWhenGetPubli, isNeedNextAttkcKey);
    }


    public boolean isForSoter() {
        return isForSoter;
    }

    public void setIsForSoter(boolean isForSoter) {
        this.isForSoter = isForSoter;
    }

    public boolean isAutoSignedWithAttkWhenGetPublicKey() {
        return isAutoSignedWithAttkWhenGetPublicKey;
    }

    public void setIsAutoSignedWithAttkWhenGetPublicKey(boolean isAutoSignedWithAttkWhenGetPublicKey) {
        this.isAutoSignedWithAttkWhenGetPublicKey = isAutoSignedWithAttkWhenGetPublicKey;
    }

    public boolean isAutoSignedWithCommonkWhenGetPublicKey() {
        return isAutoSignedWithCommonkWhenGetPublicKey;
    }

    public void setIsAutoSignedWithCommonkWhenGetPublicKey(boolean isAutoSignedWithCommonkWhenGetPublicKey) {
        this.isAutoSignedWithCommonkWhenGetPublicKey = isAutoSignedWithCommonkWhenGetPublicKey;
    }

    public String getAutoSignedKeyNameWhenGetPublicKey() {
        return mAutoSignedKeyNameWhenGetPublicKey;
    }

    public void setAutoSignedKeyNameWhenGetPublicKey(String mAutoSignedKeyNameWhenGetPublicKey) {
        this.mAutoSignedKeyNameWhenGetPublicKey = mAutoSignedKeyNameWhenGetPublicKey;
    }

    public boolean isSecmsgFidCounterSignedWhenSign() {
        return isSecmsgFidCounterSignedWhenSign;
    }

    public void setIsSecmsgFidCounterSignedWhenSign(boolean isSecmsgFidCounterSignedWhenSign) {
        this.isSecmsgFidCounterSignedWhenSign = isSecmsgFidCounterSignedWhenSign;
    }

    public boolean isAutoAddCounterWhenGetPublicKey() {
        return isAutoAddCounterWhenGetPublicKey;
    }

    public void setIsAutoAddCounterWhenGetPublicKey(boolean isAutoAddCounterWhenGetPublicKey) {
        this.isAutoAddCounterWhenGetPublicKey = isAutoAddCounterWhenGetPublicKey;
    }

    public boolean isNeedUseNextAttk() {
        return isNeedUseNextAttk;
    }

    public void setIsNeedUseNextAttk(boolean isNeedUseNextAttk) {
        this.isNeedUseNextAttk = isNeedUseNextAttk;
    }

    @Override
    public String toString() {
        return "SoterRSAKeyGenParameterSpec{" +
        "isForSoter=" + isForSoter +
        ", isAutoSignedWithAttkWhenGetPublicKey=" + isAutoSignedWithAttkWhenGetPublicKey +
        ", isAutoSignedWithCommonkWhenGetPublicKey=" + isAutoSignedWithCommonkWhenGetPublicKey +
        ", mAutoSignedKeyNameWhenGetPublicKey='" + mAutoSignedKeyNameWhenGetPublicKey + '\'' +
        ", isSecmsgFidCounterSignedWhenSign=" + isSecmsgFidCounterSignedWhenSign +
        ", isAutoAddCounterWhenGetPublicKey=" + isAutoAddCounterWhenGetPublicKey +
        ", isNeedUseNextAttk=" + isNeedUseNextAttk +
        '}';
    }

}
