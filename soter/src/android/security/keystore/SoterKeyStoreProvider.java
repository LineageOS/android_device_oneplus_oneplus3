package android.security.keystore;

import android.annotation.NonNull;
import android.security.KeyStore;
import android.security.keymaster.ExportResult;
import android.security.keymaster.KeyCharacteristics;
import android.security.keymaster.KeymasterDefs;
import android.security.keystore.AndroidKeyStoreECPrivateKey;
import android.security.keystore.AndroidKeyStoreECPublicKey;
import android.security.keystore.AndroidKeyStorePrivateKey;
import android.security.keystore.AndroidKeyStorePublicKey;
import android.security.keystore.AndroidKeyStoreRSAPrivateKey;
import android.security.keystore.AndroidKeyStoreRSAPublicKey;
import android.util.Log;

import org.json.JSONException;

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

/**
 * Created by henryye on 15/9/14.
 * The provider which supports SOTER-RSA-Generation method
 *
 * @hide
 */
public class SoterKeyStoreProvider extends Provider {
    public static final String PROVIDER_NAME = "SoterKeyStore";

    private static final String SOTER_PACKAGE_NAME = "android.security.keystore";
    private static final String ANDROID_PACKAGE_NAME = "android.security.keystore";

    public SoterKeyStoreProvider() {
        super(PROVIDER_NAME, 1.0, "provider for soter");
        put("KeyPairGenerator.RSA", SOTER_PACKAGE_NAME + ".SoterKeyStoreKeyPairRSAGeneratorSpi");
        put("KeyStore.SoterKeyStore", SOTER_PACKAGE_NAME + ".SoterKeyStoreSpi");
        putKeyFactoryImpl("RSA");
    }

    private void putKeyFactoryImpl(String algorithm) {
        put("KeyFactory." + algorithm, ANDROID_PACKAGE_NAME + ".AndroidKeyStoreKeyFactorySpi");
    }

    public static void install() {
        Security.addProvider(new SoterKeyStoreProvider());
    }


    @NonNull
    public static AndroidKeyStorePrivateKey getAndroidKeyStorePrivateKey(
            @NonNull AndroidKeyStorePublicKey publicKey) {
        String keyAlgorithm = publicKey.getAlgorithm();
        if (KeyProperties.KEY_ALGORITHM_EC.equalsIgnoreCase(keyAlgorithm)) {
            return new AndroidKeyStoreECPrivateKey(
                    publicKey.getAlias(), KeyStore.UID_SELF, ((ECKey) publicKey).getParams());
        } else if (KeyProperties.KEY_ALGORITHM_RSA.equalsIgnoreCase(keyAlgorithm)) {
            return new AndroidKeyStoreRSAPrivateKey(
                    publicKey.getAlias(), KeyStore.UID_SELF, ((RSAKey) publicKey).getModulus());
        } else {
            throw new ProviderException("Unsupported Android Keystore public key algorithm: "
                    + keyAlgorithm);
        }
    }

    @NonNull
    public static AndroidKeyStorePublicKey loadAndroidKeyStorePublicKeyFromKeystore(
            @NonNull KeyStore keyStore, @NonNull String privateKeyAlias)
            throws UnrecoverableKeyException {
        KeyCharacteristics keyCharacteristics = new KeyCharacteristics();
        int errorCode = keyStore.getKeyCharacteristics(
                privateKeyAlias, null, null, keyCharacteristics);
        if (errorCode != KeyStore.NO_ERROR) {
            throw (UnrecoverableKeyException)
                    new UnrecoverableKeyException("Failed to obtain information about private key")
                            .initCause(KeyStore.getKeyStoreException(errorCode));
        }
        ExportResult exportResult = keyStore.exportKey(
                privateKeyAlias, KeymasterDefs.KM_KEY_FORMAT_X509, null, null);
        if (exportResult.resultCode != KeyStore.NO_ERROR) {
            throw (UnrecoverableKeyException)
                    new UnrecoverableKeyException("Failed to obtain X.509 form of public key")
                            .initCause(KeyStore.getKeyStoreException(errorCode));
        }
        final byte[] x509EncodedPublicKey = exportResult.exportData;

        Integer keymasterAlgorithm = keyCharacteristics.getEnum(KeymasterDefs.KM_TAG_ALGORITHM);
        if (keymasterAlgorithm == null) {
            throw new UnrecoverableKeyException("Key algorithm unknown");
        }

        String jcaKeyAlgorithm;
        try {
            jcaKeyAlgorithm = KeyProperties.KeyAlgorithm.fromKeymasterAsymmetricKeyAlgorithm(
                    keymasterAlgorithm);
        } catch (IllegalArgumentException e) {
            throw (UnrecoverableKeyException)
                    new UnrecoverableKeyException("Failed to load private key")
                            .initCause(e);
        }

        return getAndroidKeyStorePublicKey(
                privateKeyAlias, jcaKeyAlgorithm, x509EncodedPublicKey);
    }


    @NonNull
    public static KeyPair loadAndroidKeyStoreKeyPairFromKeystore(
            @NonNull KeyStore keyStore, @NonNull String privateKeyAlias)
            throws UnrecoverableKeyException {
        AndroidKeyStorePublicKey publicKey =
                loadAndroidKeyStorePublicKeyFromKeystore(keyStore, privateKeyAlias);
        AndroidKeyStorePrivateKey privateKey =
                getAndroidKeyStorePrivateKey(publicKey);
        return new KeyPair(publicKey, privateKey);
    }

    @NonNull
    public static AndroidKeyStorePublicKey getAndroidKeyStorePublicKey(
            @NonNull String alias,
            @NonNull String keyAlgorithm,
            @NonNull byte[] x509EncodedForm) {
        PublicKey publicKey;
        try {
            KeyFactory keyFactory = KeyFactory.getInstance(keyAlgorithm);
            byte[] realPublicKey = SoterUtil.getDataFromRaw(x509EncodedForm, SoterUtil.JSON_KEY_PUBLIC);
            if (realPublicKey != null) {
                publicKey = keyFactory.generatePublic(new X509EncodedKeySpec(realPublicKey));
            } else {
                throw new NullPointerException("invalid soter public key");
            }
        } catch (NoSuchAlgorithmException e) {
            throw new ProviderException(
                    "Failed to obtain " + keyAlgorithm + " KeyFactory", e);
        } catch (InvalidKeySpecException e) {
            throw new ProviderException("Invalid X.509 encoding of public key", e);
        } catch (JSONException e) {
            throw new ProviderException("Not in json format");
        }
        if (KeyProperties.KEY_ALGORITHM_EC.equalsIgnoreCase(keyAlgorithm)) {
            Log.d("Soter", "AndroidKeyStoreECPublicKey");
            return new AndroidKeyStoreECPublicKey(alias, KeyStore.UID_SELF, (ECPublicKey) publicKey);
        } else if (KeyProperties.KEY_ALGORITHM_RSA.equalsIgnoreCase(keyAlgorithm)) {
            Log.d("Soter", "AndroidKeyStoreRSAPublicKey");
            RSAPublicKey rsaPubKey = (RSAPublicKey) publicKey;
            //generate certificate need decoded realPublicKey, because need give the to
            //android certificate generator
            //but Soter ui get key need encode json string, soter ui need
            //resolve all information from json string
            //so maybe we should have two ways to meet all request
            //en, this in soter provider, so it's under control
            return new AndroidKeyStoreRSAPublicKey(alias, KeyStore.UID_SELF, rsaPubKey);//x509EncodedForm, rsaPubKey.getModulus(), rsaPubKey.getPublicExponent());
        } else {
            throw new ProviderException("Unsupported Android Keystore public key algorithm: "
                    + keyAlgorithm);
        }
    }

    //chenlingyun copied begin
    @NonNull
    public static AndroidKeyStorePrivateKey loadAndroidKeyStorePrivateKeyFromKeystore(
            @NonNull KeyStore keyStore, @NonNull String privateKeyAlias)
            throws UnrecoverableKeyException {
        KeyPair keyPair = loadAndroidKeyStoreKeyPairFromKeystore(keyStore, privateKeyAlias);
        return (AndroidKeyStorePrivateKey) keyPair.getPrivate();
    }

    @NonNull
    public static AndroidKeyStorePublicKey loadJsonPublicKeyFromKeystore(
            @NonNull KeyStore keyStore, @NonNull String privateKeyAlias)
            throws UnrecoverableKeyException {
        KeyCharacteristics keyCharacteristics = new KeyCharacteristics();
        int errorCode = keyStore.getKeyCharacteristics(
                privateKeyAlias, null, null, keyCharacteristics);
        if (errorCode != KeyStore.NO_ERROR) {
            throw (UnrecoverableKeyException)
                    new UnrecoverableKeyException("Failed to obtain information about private key")
                            .initCause(KeyStore.getKeyStoreException(errorCode));
        }
        ExportResult exportResult = keyStore.exportKey(
                privateKeyAlias, KeymasterDefs.KM_KEY_FORMAT_X509, null, null);
        if (exportResult.resultCode != KeyStore.NO_ERROR) {
            throw (UnrecoverableKeyException)
                    new UnrecoverableKeyException("Failed to obtain X.509 form of public key")
                            .initCause(KeyStore.getKeyStoreException(errorCode));
        }
        final byte[] x509EncodedPublicKey = exportResult.exportData;

        Integer keymasterAlgorithm = keyCharacteristics.getEnum(KeymasterDefs.KM_TAG_ALGORITHM);
        if (keymasterAlgorithm == null) {
            throw new UnrecoverableKeyException("Key algorithm unknown");
        }

        String jcaKeyAlgorithm;
        try {
            jcaKeyAlgorithm = KeyProperties.KeyAlgorithm.fromKeymasterAsymmetricKeyAlgorithm(
                    keymasterAlgorithm);
        } catch (IllegalArgumentException e) {
            throw (UnrecoverableKeyException)
                    new UnrecoverableKeyException("Failed to load private key")
                            .initCause(e);
        }

        return getJsonPublicKey(
                privateKeyAlias, jcaKeyAlgorithm, x509EncodedPublicKey);
    }

    @NonNull
    public static AndroidKeyStorePublicKey getJsonPublicKey(
            @NonNull String alias,
            @NonNull String keyAlgorithm,
            @NonNull byte[] x509EncodedForm) {
        PublicKey publicKey;
        try {
            KeyFactory keyFactory = KeyFactory.getInstance(keyAlgorithm);
            byte[] realPublicKey = SoterUtil.getDataFromRaw(x509EncodedForm, SoterUtil.JSON_KEY_PUBLIC);
            if (realPublicKey != null) {
                publicKey = keyFactory.generatePublic(new X509EncodedKeySpec(realPublicKey));
            } else {
                throw new NullPointerException("invalid soter public key");
            }
        } catch (NoSuchAlgorithmException e) {
            throw new ProviderException(
                    "Failed to obtain " + keyAlgorithm + " KeyFactory", e);
        } catch (InvalidKeySpecException e) {
            throw new ProviderException("Invalid X.509 encoding of public key", e);
        } catch (JSONException e) {
            throw new ProviderException("Not in json format");
        }
        if (KeyProperties.KEY_ALGORITHM_EC.equalsIgnoreCase(keyAlgorithm)) {
            Log.d("Soter", "AndroidKeyStoreECPublicKey");
            return new AndroidKeyStoreECPublicKey(alias, KeyStore.UID_SELF, (ECPublicKey) publicKey);
        } else if (KeyProperties.KEY_ALGORITHM_RSA.equalsIgnoreCase(keyAlgorithm)) {
            Log.d("Soter", "getJsonPublicKey");
            RSAPublicKey rsaPubKey = (RSAPublicKey) publicKey;
            //generate certificate need decoded realPublicKey, because need give the to
            //android certificate generator
            //but Soter ui get key need encode json string, soter ui need
            //resolve all information from json string
            //so maybe we should have two ways to meet all request
            //en, this in soter provider, so it's under control
            return new AndroidKeyStoreRSAPublicKey(alias, KeyStore.UID_SELF, x509EncodedForm, rsaPubKey.getModulus(), rsaPubKey.getPublicExponent());
        } else {
            throw new ProviderException("Unsupported Android Keystore public key algorithm: "
                    + keyAlgorithm);
        }
    }
    //chenlingyun copied end
}
