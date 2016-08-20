package android.security.keystore;

import android.security.KeyStore;
import java.security.Key;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.UnrecoverableKeyException;

public class SoterKeyStoreSpi extends AndroidKeyStoreSpi {
    private KeyStore mKeyStore;

    public SoterKeyStoreSpi() {
        this.mKeyStore = null;
        this.mKeyStore = KeyStore.getInstance();
    }

    public Key engineGetKey(String alias, char[] password) throws NoSuchAlgorithmException, UnrecoverableKeyException {
        if (isPrivateKeyEntry(alias)) {
            String privateKeyAlias = "USRPKEY_" + alias;
            if (password == null || !"from_soter_ui".equals(String.valueOf(password))) {
                return SoterKeyStoreProvider.loadAndroidKeyStorePrivateKeyFromKeystore(this.mKeyStore, privateKeyAlias);
            }
            return SoterKeyStoreProvider.loadJsonPublicKeyFromKeystore(this.mKeyStore, privateKeyAlias);
        } else if (!isSecretKeyEntry(alias)) {
            return null;
        } else {
            return AndroidKeyStoreProvider.loadAndroidKeyStoreSecretKeyFromKeystore(this.mKeyStore, "USRSKEY_" + alias);
        }
    }

    private boolean isPrivateKeyEntry(String alias) {
        if (alias != null) {
            return this.mKeyStore.contains("USRPKEY_" + alias);
        }
        throw new NullPointerException("alias == null");
    }

    private boolean isSecretKeyEntry(String alias) {
        if (alias != null) {
            return this.mKeyStore.contains("USRSKEY_" + alias);
        }
        throw new NullPointerException("alias == null");
    }

    public void engineDeleteEntry(String alias) throws KeyStoreException {
        boolean deletePkey = this.mKeyStore.delete("USRPKEY_" + alias);
        boolean deleteCert = this.mKeyStore.delete("USRCERT_" + alias);
        if (engineContainsAlias(alias) && !(deletePkey || deleteCert)) {
            throw new KeyStoreException("Failed to delete entry: " + alias);
        }
    }

    public boolean engineContainsAlias(String alias) {
        if (alias != null) {
            return this.mKeyStore.contains(new StringBuilder().append("USRPKEY_").append(alias).toString()) || this.mKeyStore.contains("USRCERT_" + alias);
        } else {
            throw new NullPointerException("alias == null");
        }
    }
}
