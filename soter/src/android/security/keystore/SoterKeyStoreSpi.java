package android.security.keystore;

import android.security.Credentials;
import android.security.KeyStore;
import android.security.keystore.AndroidKeyStoreProvider;

import java.security.Key;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableKeyException;

/**
 * Created by henryye on 15/10/13.
 * If need any edit necessary, please contact him by RTX
 *
 * @hide
 */
public class SoterKeyStoreSpi extends android.security.keystore.AndroidKeyStoreSpi {

    private KeyStore mKeyStore = null;

    public SoterKeyStoreSpi() {
        mKeyStore = KeyStore.getInstance();//chenlingyun move init mKeyStore to here
    }

    @Override
    public Key engineGetKey(String alias, char[] password) throws NoSuchAlgorithmException,
            UnrecoverableKeyException {
        //mKeyStore = KeyStore.getInstance();
        if (isPrivateKeyEntry(alias)) {
            String privateKeyAlias = Credentials.USER_PRIVATE_KEY + alias;

            //chenlingyun we should continue our native layer work,
            //so we use an ugly special password "from_soter_ui"
            //to distinguish need private key or json string public key
            if (password != null && "from_soter_ui".equals(String.valueOf(password))) {
                return SoterKeyStoreProvider.loadJsonPublicKeyFromKeystore(
                        mKeyStore, privateKeyAlias);
            } else {
                return SoterKeyStoreProvider.loadAndroidKeyStorePrivateKeyFromKeystore(
                        mKeyStore, privateKeyAlias);
            }
        } else if (isSecretKeyEntry(alias)) {
            String secretKeyAlias = Credentials.USER_SECRET_KEY + alias;
            return AndroidKeyStoreProvider.loadAndroidKeyStoreSecretKeyFromKeystore(
                    mKeyStore, secretKeyAlias, KeyStore.UID_SELF);
        } else {
            // Key not found
            return null;
        }
    }

    private boolean isPrivateKeyEntry(String alias) {
        if (alias == null) {
            throw new NullPointerException("alias == null");
        }

        return mKeyStore.contains(Credentials.USER_PRIVATE_KEY + alias);
    }

    private boolean isSecretKeyEntry(String alias) {
        if (alias == null) {
            throw new NullPointerException("alias == null");
        }
        return mKeyStore.contains(Credentials.USER_SECRET_KEY + alias);
    }

    @Override
    public void engineDeleteEntry(String alias) throws KeyStoreException {
        if (!engineContainsAlias(alias)) {
            return;
        }
        // At least one entry corresponding to this alias exists in keystore

        if (!(mKeyStore.delete(Credentials.USER_PRIVATE_KEY + alias) | mKeyStore.delete(Credentials.USER_CERTIFICATE + alias))) {
            throw new KeyStoreException("Failed to delete entry: " + alias);
        }
    }

    @Override
    public boolean engineContainsAlias(String alias) {
        if (alias == null) {
            throw new NullPointerException("alias == null");
        }

        return mKeyStore.contains(Credentials.USER_PRIVATE_KEY + alias) ||
                mKeyStore.contains(Credentials.USER_CERTIFICATE + alias);
    }
}
