package android.security.keystore;

import android.security.KeyStore;
import android.security.keymaster.ExportResult;
import android.security.keymaster.KeyCharacteristics;
import android.security.keystore.KeyProperties.KeyAlgorithm;
import android.util.Log;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.NoSuchAlgorithmException;
import java.security.Provider;
import java.security.ProviderException;
import java.security.PublicKey;
import java.security.Security;
import java.security.UnrecoverableKeyException;
import java.security.interfaces.ECKey;
import java.security.interfaces.ECPublicKey;
import java.security.interfaces.RSAKey;
import java.security.interfaces.RSAPublicKey;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.X509EncodedKeySpec;
import org.json.JSONException;

public class SoterKeyStoreProvider extends Provider {
    private static final String ANDROID_PACKAGE_NAME = "android.security.keystore";
    public static final String PROVIDER_NAME = "SoterKeyStore";
    private static final String SOTER_PACKAGE_NAME = "android.security.keystore";

    public SoterKeyStoreProvider() {
        super(PROVIDER_NAME, 1.0d, "provider for soter");
        put("KeyPairGenerator.RSA", "android.security.keystore.SoterKeyStoreKeyPairRSAGeneratorSpi");
        put("KeyStore.SoterKeyStore", "android.security.keystore.SoterKeyStoreSpi");
        putKeyFactoryImpl("RSA");
    }

    private void putKeyFactoryImpl(String algorithm) {
        put("KeyFactory." + algorithm, "android.security.keystore.AndroidKeyStoreKeyFactorySpi");
    }

    public static void install() {
        Security.addProvider(new SoterKeyStoreProvider());
    }

    public static AndroidKeyStorePrivateKey getAndroidKeyStorePrivateKey(AndroidKeyStorePublicKey publicKey) {
        String keyAlgorithm = publicKey.getAlgorithm();
        if ("EC".equalsIgnoreCase(keyAlgorithm)) {
            return new AndroidKeyStoreECPrivateKey(publicKey.getAlias(), ((ECKey) publicKey).getParams());
        }
        if ("RSA".equalsIgnoreCase(keyAlgorithm)) {
            return new AndroidKeyStoreRSAPrivateKey(publicKey.getAlias(), ((RSAKey) publicKey).getModulus());
        }
        throw new ProviderException("Unsupported Android Keystore public key algorithm: " + keyAlgorithm);
    }

    public static AndroidKeyStorePublicKey loadAndroidKeyStorePublicKeyFromKeystore(KeyStore keyStore, String privateKeyAlias) throws UnrecoverableKeyException {
        KeyCharacteristics keyCharacteristics = new KeyCharacteristics();
        int errorCode = keyStore.getKeyCharacteristics(privateKeyAlias, null, null, keyCharacteristics);
        if (errorCode != 1) {
            throw ((UnrecoverableKeyException) new UnrecoverableKeyException("Failed to obtain information about private key").initCause(KeyStore.getKeyStoreException(errorCode)));
        }
        ExportResult exportResult = keyStore.exportKey(privateKeyAlias, 0, null, null);
        if (exportResult.resultCode != 1) {
            throw ((UnrecoverableKeyException) new UnrecoverableKeyException("Failed to obtain X.509 form of public key").initCause(KeyStore.getKeyStoreException(errorCode)));
        }
        byte[] x509EncodedPublicKey = exportResult.exportData;
        Integer keymasterAlgorithm = keyCharacteristics.getEnum(268435458);
        if (keymasterAlgorithm == null) {
            throw new UnrecoverableKeyException("Key algorithm unknown");
        }
        try {
            return getAndroidKeyStorePublicKey(privateKeyAlias, KeyAlgorithm.fromKeymasterAsymmetricKeyAlgorithm(keymasterAlgorithm.intValue()), x509EncodedPublicKey);
        } catch (IllegalArgumentException e) {
            throw ((UnrecoverableKeyException) new UnrecoverableKeyException("Failed to load private key").initCause(e));
        }
    }

    public static KeyPair loadAndroidKeyStoreKeyPairFromKeystore(KeyStore keyStore, String privateKeyAlias) throws UnrecoverableKeyException {
        AndroidKeyStorePublicKey publicKey = loadAndroidKeyStorePublicKeyFromKeystore(keyStore, privateKeyAlias);
        return new KeyPair(publicKey, getAndroidKeyStorePrivateKey(publicKey));
    }

    public static AndroidKeyStorePublicKey getAndroidKeyStorePublicKey(String alias, String keyAlgorithm, byte[] x509EncodedForm) {
        try {
            KeyFactory keyFactory = KeyFactory.getInstance(keyAlgorithm);
            byte[] realPublicKey = SoterUtil.getDataFromRaw(x509EncodedForm, SoterUtil.JSON_KEY_PUBLIC);
            if (realPublicKey != null) {
                PublicKey publicKey = keyFactory.generatePublic(new X509EncodedKeySpec(realPublicKey));
                if ("EC".equalsIgnoreCase(keyAlgorithm)) {
                    Log.d("Soter", "AndroidKeyStoreECPublicKey");
                    return new AndroidKeyStoreECPublicKey(alias, (ECPublicKey) publicKey);
                } else if ("RSA".equalsIgnoreCase(keyAlgorithm)) {
                    Log.d("Soter", "AndroidKeyStoreRSAPublicKey");
                    return new AndroidKeyStoreRSAPublicKey(alias, (RSAPublicKey) publicKey);
                } else {
                    throw new ProviderException("Unsupported Android Keystore public key algorithm: " + keyAlgorithm);
                }
            }
            throw new NullPointerException("invalid soter public key");
        } catch (NoSuchAlgorithmException e) {
            throw new ProviderException("Failed to obtain " + keyAlgorithm + " KeyFactory", e);
        } catch (InvalidKeySpecException e2) {
            throw new ProviderException("Invalid X.509 encoding of public key", e2);
        } catch (JSONException e3) {
            throw new ProviderException("Not in json format");
        }
    }

    public static AndroidKeyStorePrivateKey loadAndroidKeyStorePrivateKeyFromKeystore(KeyStore keyStore, String privateKeyAlias) throws UnrecoverableKeyException {
        return (AndroidKeyStorePrivateKey) loadAndroidKeyStoreKeyPairFromKeystore(keyStore, privateKeyAlias).getPrivate();
    }

    public static AndroidKeyStorePublicKey loadJsonPublicKeyFromKeystore(KeyStore keyStore, String privateKeyAlias) throws UnrecoverableKeyException {
        KeyCharacteristics keyCharacteristics = new KeyCharacteristics();
        int errorCode = keyStore.getKeyCharacteristics(privateKeyAlias, null, null, keyCharacteristics);
        if (errorCode != 1) {
            throw ((UnrecoverableKeyException) new UnrecoverableKeyException("Failed to obtain information about private key").initCause(KeyStore.getKeyStoreException(errorCode)));
        }
        ExportResult exportResult = keyStore.exportKey(privateKeyAlias, 0, null, null);
        if (exportResult.resultCode != 1) {
            throw ((UnrecoverableKeyException) new UnrecoverableKeyException("Failed to obtain X.509 form of public key").initCause(KeyStore.getKeyStoreException(errorCode)));
        }
        byte[] x509EncodedPublicKey = exportResult.exportData;
        Integer keymasterAlgorithm = keyCharacteristics.getEnum(268435458);
        if (keymasterAlgorithm == null) {
            throw new UnrecoverableKeyException("Key algorithm unknown");
        }
        try {
            return getJsonPublicKey(privateKeyAlias, KeyAlgorithm.fromKeymasterAsymmetricKeyAlgorithm(keymasterAlgorithm.intValue()), x509EncodedPublicKey);
        } catch (IllegalArgumentException e) {
            throw ((UnrecoverableKeyException) new UnrecoverableKeyException("Failed to load private key").initCause(e));
        }
    }

    public static AndroidKeyStorePublicKey getJsonPublicKey(String alias, String keyAlgorithm, byte[] x509EncodedForm) {
        try {
            KeyFactory keyFactory = KeyFactory.getInstance(keyAlgorithm);
            byte[] realPublicKey = SoterUtil.getDataFromRaw(x509EncodedForm, SoterUtil.JSON_KEY_PUBLIC);
            if (realPublicKey != null) {
                PublicKey publicKey = keyFactory.generatePublic(new X509EncodedKeySpec(realPublicKey));
                if ("EC".equalsIgnoreCase(keyAlgorithm)) {
                    Log.d("Soter", "AndroidKeyStoreECPublicKey");
                    return new AndroidKeyStoreECPublicKey(alias, (ECPublicKey) publicKey);
                } else if ("RSA".equalsIgnoreCase(keyAlgorithm)) {
                    Log.d("Soter", "getJsonPublicKey");
                    RSAPublicKey rsaPubKey = (RSAPublicKey) publicKey;
                    return new AndroidKeyStoreRSAPublicKey(alias, x509EncodedForm, rsaPubKey.getModulus(), rsaPubKey.getPublicExponent());
                } else {
                    throw new ProviderException("Unsupported Android Keystore public key algorithm: " + keyAlgorithm);
                }
            }
            throw new NullPointerException("invalid soter public key");
        } catch (NoSuchAlgorithmException e) {
            throw new ProviderException("Failed to obtain " + keyAlgorithm + " KeyFactory", e);
        } catch (InvalidKeySpecException e2) {
            throw new ProviderException("Invalid X.509 encoding of public key", e2);
        } catch (JSONException e3) {
            throw new ProviderException("Not in json format");
        }
    }
}
