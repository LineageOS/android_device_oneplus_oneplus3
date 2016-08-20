package android.security.keystore;

import android.app.ActivityThread;
import android.app.Application;
import android.content.Context;
import android.os.Process;
import android.security.Credentials;
import android.security.KeyPairGeneratorSpec;
import android.security.KeyStore;
import android.security.KeyStore.State;
import android.security.keymaster.KeyCharacteristics;
import android.security.keymaster.KeymasterArguments;
import android.security.keystore.KeyProperties.BlockMode;
import android.security.keystore.KeyProperties.Digest;
import android.security.keystore.KeyProperties.EncryptionPadding;
import android.security.keystore.KeyProperties.KeyAlgorithm;
import android.security.keystore.KeyProperties.Purpose;
import android.security.keystore.KeyProperties.SignaturePadding;
import android.util.Log;
import com.android.internal.util.ArrayUtils;
import com.android.org.bouncycastle.asn1.ASN1EncodableVector;
import com.android.org.bouncycastle.asn1.ASN1InputStream;
import com.android.org.bouncycastle.asn1.ASN1Integer;
import com.android.org.bouncycastle.asn1.DERBitString;
import com.android.org.bouncycastle.asn1.DERInteger;
import com.android.org.bouncycastle.asn1.DERNull;
import com.android.org.bouncycastle.asn1.DERSequence;
import com.android.org.bouncycastle.asn1.pkcs.PKCSObjectIdentifiers;
import com.android.org.bouncycastle.asn1.x509.AlgorithmIdentifier;
import com.android.org.bouncycastle.asn1.x509.Certificate;
import com.android.org.bouncycastle.asn1.x509.SubjectPublicKeyInfo;
import com.android.org.bouncycastle.asn1.x509.TBSCertificate;
import com.android.org.bouncycastle.asn1.x509.Time;
import com.android.org.bouncycastle.asn1.x509.V3TBSCertificateGenerator;
import com.android.org.bouncycastle.asn1.x9.X9ObjectIdentifiers;
import com.android.org.bouncycastle.jce.X509Principal;
import com.android.org.bouncycastle.jce.provider.X509CertificateObject;
import com.android.org.bouncycastle.x509.X509V3CertificateGenerator;
import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.security.InvalidAlgorithmParameterException;
import java.security.KeyPair;
import java.security.KeyPairGeneratorSpi;
import java.security.PrivateKey;
import java.security.ProviderException;
import java.security.PublicKey;
import java.security.SecureRandom;
import java.security.UnrecoverableKeyException;
import java.security.cert.CertificateEncodingException;
import java.security.cert.X509Certificate;
import java.security.spec.AlgorithmParameterSpec;
import java.security.spec.RSAKeyGenParameterSpec;
import java.util.HashSet;
import java.util.Set;
import libcore.util.EmptyArray;

public class SoterKeyStoreKeyPairRSAGeneratorSpi extends KeyPairGeneratorSpi {
    private static final int RSA_DEFAULT_KEY_SIZE = 2048;
    private static final int RSA_MAX_KEY_SIZE = 8192;
    private static final int RSA_MIN_KEY_SIZE = 512;
    public static final long UINT32_MAX_VALUE = 4294967295L;
    private static final long UINT32_RANGE = 4294967296L;
    private static final BigInteger UINT64_RANGE = BigInteger.ONE.shiftLeft(64);
    public static final BigInteger UINT64_MAX_VALUE = UINT64_RANGE.subtract(BigInteger.ONE);
    private static volatile SecureRandom sRng;
    private boolean isAutoAddCounterWhenGetPublicKey = false;
    private boolean isAutoSignedWithAttkWhenGetPublicKey = false;
    private boolean isAutoSignedWithCommonkWhenGetPublicKey = false;
    private boolean isForSoter = false;
    private boolean isNeedNextAttk = false;
    private boolean isSecmsgFidCounterSignedWhenSign = false;
    private String mAutoSignedKeyNameWhenGetPublicKey = "";
    private boolean mEncryptionAtRestRequired;
    private String mEntryAlias;
    private String mJcaKeyAlgorithm;
    private int mKeySizeBits;
    private KeyStore mKeyStore;
    private int mKeymasterAlgorithm = -1;
    private int[] mKeymasterBlockModes;
    private int[] mKeymasterDigests;
    private int[] mKeymasterEncryptionPaddings;
    private int[] mKeymasterPurposes;
    private int[] mKeymasterSignaturePaddings;
    private final int mOriginalKeymasterAlgorithm = 1;
    private BigInteger mRSAPublicExponent;
    private SecureRandom mRng;
    private KeyGenParameterSpec mSpec;

    public void initialize(int keysize, SecureRandom random) {
        throw new IllegalArgumentException(KeyGenParameterSpec.class.getName() + " required to initialize this KeyPairGenerator");
    }

    public void initialize(AlgorithmParameterSpec params, SecureRandom random) throws InvalidAlgorithmParameterException {
        resetAll();
        if (params == null) {
            try {
                throw new InvalidAlgorithmParameterException("Must supply params of type " + KeyGenParameterSpec.class.getName() + " or " + KeyPairGeneratorSpec.class.getName());
            } catch (Throwable th) {
                if (!false) {
                    resetAll();
                }
            }
        } else {
            int keymasterAlgorithm = this.mOriginalKeymasterAlgorithm;
            if (params instanceof KeyGenParameterSpec) {
                KeyGenParameterSpec spec = (KeyGenParameterSpec) params;
                this.mEntryAlias = SoterUtil.getPureKeyAliasFromKeyName(spec.getKeystoreAlias());
                this.mSpec = spec;
                this.mKeymasterAlgorithm = keymasterAlgorithm;
                this.mEncryptionAtRestRequired = false;
                this.mKeySizeBits = spec.getKeySize();
                initAlgorithmSpecificParameters();
                if (this.mKeySizeBits == -1) {
                    this.mKeySizeBits = getDefaultKeySize(keymasterAlgorithm);
                }
                checkValidKeySize(keymasterAlgorithm, this.mKeySizeBits);
                if (spec.getKeystoreAlias() == null) {
                    throw new InvalidAlgorithmParameterException("KeyStore entry alias not provided");
                }
                try {
                    String jcaKeyAlgorithm = KeyAlgorithm.fromKeymasterAsymmetricKeyAlgorithm(keymasterAlgorithm);
                    this.mKeymasterPurposes = Purpose.allToKeymaster(spec.getPurposes());
                    this.mKeymasterBlockModes = BlockMode.allToKeymaster(spec.getBlockModes());
                    this.mKeymasterEncryptionPaddings = EncryptionPadding.allToKeymaster(spec.getEncryptionPaddings());
                    if ((spec.getPurposes() & 1) != 0 && spec.isRandomizedEncryptionRequired()) {
                        int[] arr$ = this.mKeymasterEncryptionPaddings;
                        int len$ = arr$.length;
                        int i$ = 0;
                        while (i$ < len$) {
                            int keymasterPadding = arr$[i$];
                            if (isKeymasterPaddingSchemeIndCpaCompatibleWithAsymmetricCrypto(keymasterPadding)) {
                                i$++;
                            } else {
                                throw new InvalidAlgorithmParameterException("Randomized encryption (IND-CPA) required but may be violated by padding scheme: " + EncryptionPadding.fromKeymaster(keymasterPadding) + ". See " + KeyGenParameterSpec.class.getName() + " documentation.");
                            }
                        }
                    }
                    this.mKeymasterSignaturePaddings = SignaturePadding.allToKeymaster(spec.getSignaturePaddings());
                    if (spec.isDigestsSpecified()) {
                        this.mKeymasterDigests = Digest.allToKeymaster(spec.getDigests());
                    } else {
                        this.mKeymasterDigests = EmptyArray.INT;
                    }
                    KeymasterUtils.addUserAuthArgs(new KeymasterArguments(), this.mSpec.isUserAuthenticationRequired(), this.mSpec.getUserAuthenticationValidityDurationSeconds());
                    this.mJcaKeyAlgorithm = jcaKeyAlgorithm;
                    this.mRng = random;
                    this.mKeyStore = KeyStore.getInstance();
                    if (!true) {
                        resetAll();
                        return;
                    }
                    return;
                } catch (RuntimeException e) {
                    throw new InvalidAlgorithmParameterException(e);
                }
            }
            throw new InvalidAlgorithmParameterException("Unsupported params class: " + params.getClass().getName() + ". Supported: " + KeyGenParameterSpec.class.getName() + ", " + KeyPairGeneratorSpec.class.getName());
        }
    }

    private void resetAll() {
        this.mEntryAlias = null;
        this.mJcaKeyAlgorithm = null;
        this.mKeymasterAlgorithm = -1;
        this.mKeymasterPurposes = null;
        this.mKeymasterBlockModes = null;
        this.mKeymasterEncryptionPaddings = null;
        this.mKeymasterSignaturePaddings = null;
        this.mKeymasterDigests = null;
        this.mKeySizeBits = 0;
        this.mSpec = null;
        this.mRSAPublicExponent = null;
        this.mEncryptionAtRestRequired = false;
        this.mRng = null;
        this.mKeyStore = null;
        this.isForSoter = false;
        this.isAutoSignedWithAttkWhenGetPublicKey = false;
        this.isAutoSignedWithCommonkWhenGetPublicKey = false;
        this.mAutoSignedKeyNameWhenGetPublicKey = "";
        this.isSecmsgFidCounterSignedWhenSign = false;
        this.isAutoAddCounterWhenGetPublicKey = false;
        this.isNeedNextAttk = false;
    }

    private void initAlgorithmSpecificParameters() throws InvalidAlgorithmParameterException {
        AlgorithmParameterSpec algSpecificSpec = this.mSpec.getAlgorithmParameterSpec();
        BigInteger publicExponent = RSAKeyGenParameterSpec.F4;
        if (algSpecificSpec instanceof RSAKeyGenParameterSpec) {
            RSAKeyGenParameterSpec rsaSpec = (RSAKeyGenParameterSpec) algSpecificSpec;
            if (this.mKeySizeBits == -1) {
                this.mKeySizeBits = rsaSpec.getKeysize();
            } else if (this.mKeySizeBits != rsaSpec.getKeysize()) {
                throw new InvalidAlgorithmParameterException("RSA key size must match  between " + this.mSpec + " and " + algSpecificSpec + ": " + this.mKeySizeBits + " vs " + rsaSpec.getKeysize());
            }
            publicExponent = rsaSpec.getPublicExponent();
            if (publicExponent.compareTo(BigInteger.ZERO) < 1) {
                throw new InvalidAlgorithmParameterException("RSA public exponent must be positive: " + publicExponent);
            } else if (publicExponent.compareTo(UINT64_MAX_VALUE) > 0) {
                throw new InvalidAlgorithmParameterException("Unsupported RSA public exponent: " + publicExponent + ". Maximum supported value: " + UINT64_MAX_VALUE);
            }
        }
        this.mRSAPublicExponent = publicExponent;
        SoterRSAKeyGenParameterSpec soterSpec = SoterUtil.convertKeyNameToParameterSpec(this.mSpec.getKeystoreAlias());
        if (soterSpec != null) {
            this.isForSoter = soterSpec.isForSoter();
            this.isAutoSignedWithAttkWhenGetPublicKey = soterSpec.isAutoSignedWithAttkWhenGetPublicKey();
            this.isAutoSignedWithCommonkWhenGetPublicKey = soterSpec.isAutoSignedWithCommonkWhenGetPublicKey();
            this.mAutoSignedKeyNameWhenGetPublicKey = soterSpec.getAutoSignedKeyNameWhenGetPublicKey();
            this.isSecmsgFidCounterSignedWhenSign = soterSpec.isSecmsgFidCounterSignedWhenSign();
            this.isAutoAddCounterWhenGetPublicKey = soterSpec.isAutoAddCounterWhenGetPublicKey();
            this.isNeedNextAttk = soterSpec.isNeedUseNextAttk();
        }
    }

    public KeyPair generateKeyPair() {
        if (this.mKeyStore == null || this.mSpec == null) {
            throw new IllegalStateException("Not initialized");
        }
        int flags = this.mEncryptionAtRestRequired ? 1 : 0;
        if ((flags & 1) == 0 || this.mKeyStore.state() == State.UNLOCKED) {
            KeymasterArguments args = new KeymasterArguments();
            args.addUnsignedInt(805306371, (long) this.mKeySizeBits);
            args.addEnum(268435458, this.mKeymasterAlgorithm);
            args.addEnums(536870913, this.mKeymasterPurposes);
            args.addEnums(536870916, this.mKeymasterBlockModes);
            args.addEnums(536870918, this.mKeymasterEncryptionPaddings);
            args.addEnums(536870918, this.mKeymasterSignaturePaddings);
            args.addEnums(536870917, this.mKeymasterDigests);
            KeymasterUtils.addUserAuthArgs(args, this.mSpec.isUserAuthenticationRequired(), this.mSpec.getUserAuthenticationValidityDurationSeconds());
            if (this.mSpec.getKeyValidityStart() != null) {
                args.addDate(1610613136, this.mSpec.getKeyValidityStart());
            }
            if (this.mSpec.getKeyValidityForOriginationEnd() != null) {
                args.addDate(1610613137, this.mSpec.getKeyValidityForOriginationEnd());
            }
            if (this.mSpec.getKeyValidityForConsumptionEnd() != null) {
                args.addDate(1610613138, this.mSpec.getKeyValidityForConsumptionEnd());
            }
            addAlgorithmSpecificParameters(args);
            byte[] additionalEntropy = getRandomBytesToMixIntoKeystoreRng(this.mRng, (this.mKeySizeBits + 7) / 8);
            String privateKeyAlias = "USRPKEY_" + this.mEntryAlias;
            try {
                Credentials.deleteAllTypesForAlias(this.mKeyStore, this.mEntryAlias);
                int errorCode = this.mKeyStore.generateKey(privateKeyAlias, args, additionalEntropy, flags, new KeyCharacteristics());
                if (errorCode != 1) {
                    throw new ProviderException("Failed to generate key pair", KeyStore.getKeyStoreException(errorCode));
                }
                KeyPair result = SoterKeyStoreProvider.loadAndroidKeyStoreKeyPairFromKeystore(this.mKeyStore, privateKeyAlias);
                if (this.mJcaKeyAlgorithm.equalsIgnoreCase(result.getPrivate().getAlgorithm())) {
                    if (this.mKeyStore.put("USRCERT_" + this.mEntryAlias, generateSelfSignedCertificate(result.getPrivate(), result.getPublic()).getEncoded(), -1, flags)) {
                        if (!true) {
                            Credentials.deleteAllTypesForAlias(this.mKeyStore, this.mEntryAlias);
                        }
                        return result;
                    }
                    throw new ProviderException("Failed to store self-signed certificate");
                }
                throw new ProviderException("Generated key pair algorithm does not match requested algorithm: " + result.getPrivate().getAlgorithm() + " vs " + this.mJcaKeyAlgorithm);
            } catch (CertificateEncodingException e) {
                throw new ProviderException("Failed to obtain encoded form of self-signed certificate", e);
            } catch (UnrecoverableKeyException e) {
                throw new ProviderException("Failed to load generated key pair from keystore", e);
            } catch (Exception e) {
                Credentials.deleteAllTypesForAlias(this.mKeyStore, this.mEntryAlias);
                throw new ProviderException("Failed to generate self-signed certificate", e);
            }
        } else {
            throw new IllegalStateException("Encryption at rest using secure lock screen credential requested for key pair, but the user has not yet entered the credential");
        }
    }

    static byte[] getRandomBytesToMixIntoKeystoreRng(SecureRandom rng, int sizeBytes) {
        if (sizeBytes <= 0) {
            return EmptyArray.BYTE;
        }
        if (rng == null) {
            rng = getRng();
        }
        byte[] result = new byte[sizeBytes];
        rng.nextBytes(result);
        return result;
    }

    private static SecureRandom getRng() {
        if (sRng == null) {
            sRng = new SecureRandom();
        }
        return sRng;
    }

    private void addAlgorithmSpecificParameters(KeymasterArguments keymasterArgs) {
        if (this.mRSAPublicExponent != null) {
            keymasterArgs.addUnsignedLong(1342177480, this.mRSAPublicExponent);
        }
        if (this.isForSoter) {
            keymasterArgs.addBoolean(1879059192);
            keymasterArgs.addUnsignedInt(805317375, (long) Process.myUid());
        }
        if (this.isAutoSignedWithAttkWhenGetPublicKey) {
            keymasterArgs.addBoolean(1879059193);
        }
        if (this.isAutoSignedWithCommonkWhenGetPublicKey) {
            keymasterArgs.addBoolean(1879059194);
            if (!SoterUtil.isNullOrNil(this.mAutoSignedKeyNameWhenGetPublicKey)) {
                keymasterArgs.addBytes(-1879037189, ("USRPKEY_" + this.mAutoSignedKeyNameWhenGetPublicKey).getBytes());
            }
        }
        if (this.isAutoAddCounterWhenGetPublicKey) {
            keymasterArgs.addBoolean(1879059196);
        }
        if (this.isSecmsgFidCounterSignedWhenSign) {
            keymasterArgs.addBoolean(1879059197);
        }
        if (this.isNeedNextAttk) {
            keymasterArgs.addBoolean(1879059198);
        }
    }

    private X509Certificate generateSelfSignedCertificate(PrivateKey privateKey, PublicKey publicKey) throws Exception {
        Log.d("Soter", "generateSelfSignedCertificate");
        String signatureAlgorithm = getCertificateSignatureAlgorithm(this.mKeymasterAlgorithm, this.mKeySizeBits, this.mSpec);
        if (signatureAlgorithm == null) {
            Log.d("Soter", "generateSelfSignedCertificateWithFakeSignature1");
            return generateSelfSignedCertificateWithFakeSignature(publicKey);
        }
        try {
            Log.d("Soter", "generateSelfSignedCertificateWithValidSignature");
            return generateSelfSignedCertificateWithValidSignature(privateKey, publicKey, signatureAlgorithm);
        } catch (Exception e) {
            Log.d("Soter", "generateSelfSignedCertificateWithFakeSignature2");
            return generateSelfSignedCertificateWithFakeSignature(publicKey);
        }
    }

    private byte[] getRealKeyBlobByKeyName(String keyName) {
        Log.d("Soter", "start retrieve key blob by key name: " + keyName);
        return this.mKeyStore.get("USRPKEY_" + keyName);
    }

    private X509Certificate generateSelfSignedCertificateWithValidSignature(PrivateKey privateKey, PublicKey publicKey, String signatureAlgorithm) throws Exception {
        X509V3CertificateGenerator certGen = new X509V3CertificateGenerator();
        certGen.setPublicKey(publicKey);
        certGen.setSerialNumber(this.mSpec.getCertificateSerialNumber());
        certGen.setSubjectDN(this.mSpec.getCertificateSubject());
        certGen.setIssuerDN(this.mSpec.getCertificateSubject());
        certGen.setNotBefore(this.mSpec.getCertificateNotBefore());
        certGen.setNotAfter(this.mSpec.getCertificateNotAfter());
        certGen.setSignatureAlgorithm(signatureAlgorithm);
        return certGen.generate(privateKey);
    }

    private X509Certificate generateSelfSignedCertificateWithFakeSignature(PublicKey publicKey) throws Exception {
        AlgorithmIdentifier sigAlgId;
        byte[] signature;
        V3TBSCertificateGenerator tbsGenerator = new V3TBSCertificateGenerator();
        switch (this.mKeymasterAlgorithm) {
            case 1:
                sigAlgId = new AlgorithmIdentifier(PKCSObjectIdentifiers.sha256WithRSAEncryption, DERNull.INSTANCE);
                signature = new byte[1];
                break;
            case 3:
                sigAlgId = new AlgorithmIdentifier(X9ObjectIdentifiers.ecdsa_with_SHA256);
                ASN1EncodableVector v = new ASN1EncodableVector();
                v.add(new DERInteger(0));
                v.add(new DERInteger(0));
                signature = new DERSequence().getEncoded();
                break;
            default:
                throw new ProviderException("Unsupported key algorithm: " + this.mKeymasterAlgorithm);
        }
        ASN1InputStream publicKeyInfoIn = new ASN1InputStream(publicKey.getEncoded());
        Throwable th2 = null;
        try {
            tbsGenerator.setSubjectPublicKeyInfo(SubjectPublicKeyInfo.getInstance(publicKeyInfoIn.readObject()));
            if (publicKeyInfoIn != null) {
                if (th2 != null) {
                    try {
                        publicKeyInfoIn.close();
                    } catch (Throwable x2) {
                        th2.addSuppressed(x2);
                    }
                } else {
                    publicKeyInfoIn.close();
                }
            }
            tbsGenerator.setSerialNumber(new ASN1Integer(this.mSpec.getCertificateSerialNumber()));
            X509Principal subject = new X509Principal(this.mSpec.getCertificateSubject().getEncoded());
            tbsGenerator.setSubject(subject);
            tbsGenerator.setIssuer(subject);
            tbsGenerator.setStartDate(new Time(this.mSpec.getCertificateNotBefore()));
            tbsGenerator.setEndDate(new Time(this.mSpec.getCertificateNotAfter()));
            tbsGenerator.setSignature(sigAlgId);
            TBSCertificate tbsCertificate = tbsGenerator.generateTBSCertificate();
            ASN1EncodableVector result = new ASN1EncodableVector();
            result.add(tbsCertificate);
            result.add(sigAlgId);
            result.add(new DERBitString(signature));
            return new X509CertificateObject(Certificate.getInstance(new DERSequence(result)));
        } finally {
            if (publicKeyInfoIn != null) {
                publicKeyInfoIn.close();
            }
        }
    }

    private static int getDefaultKeySize(int keymasterAlgorithm) {
        return RSA_DEFAULT_KEY_SIZE;
    }

    private static void checkValidKeySize(int keymasterAlgorithm, int keySize) throws InvalidAlgorithmParameterException {
        if (keySize < RSA_MIN_KEY_SIZE || keySize > RSA_MAX_KEY_SIZE) {
            throw new InvalidAlgorithmParameterException("RSA key size must be >= 512 and <= 8192");
        }
    }

    private static String getCertificateSignatureAlgorithm(int keymasterAlgorithm, int keySizeBits, KeyGenParameterSpec spec) {
        if ((spec.getPurposes() & 4) == 0) {
            return null;
        }
        if (spec.isUserAuthenticationRequired()) {
            return null;
        }
        if (!spec.isDigestsSpecified()) {
            return null;
        }
        if (!ArrayUtils.contains(SignaturePadding.allToKeymaster(spec.getSignaturePaddings()), 5)) {
            return null;
        }
        int maxDigestOutputSizeBits = keySizeBits - 240;
        int bestKeymasterDigest = -1;
        int bestDigestOutputSizeBits = -1;
        for (Integer intValue : getAvailableKeymasterSignatureDigests(spec.getDigests(), getSupportedEcdsaSignatureDigests())) {
            int keymasterDigest = intValue.intValue();
            int outputSizeBits = getDigestOutputSizeBits(keymasterDigest);
            if (outputSizeBits <= maxDigestOutputSizeBits) {
                if (bestKeymasterDigest == -1) {
                    bestKeymasterDigest = keymasterDigest;
                    bestDigestOutputSizeBits = outputSizeBits;
                } else if (outputSizeBits > bestDigestOutputSizeBits) {
                    bestKeymasterDigest = keymasterDigest;
                    bestDigestOutputSizeBits = outputSizeBits;
                }
            }
        }
        if (bestKeymasterDigest == -1) {
            return null;
        }
        return Digest.fromKeymasterToSignatureAlgorithmDigest(bestKeymasterDigest) + "WithRSA";
    }

    private static String[] getSupportedEcdsaSignatureDigests() {
        return new String[]{"NONE", "SHA-1", "SHA-224", "SHA-256", "SHA-384", "SHA-512"};
    }

    private static Set<Integer> getAvailableKeymasterSignatureDigests(String[] authorizedKeyDigests, String[] supportedSignatureDigests) {
        Set<Integer> authorizedKeymasterKeyDigests = new HashSet();
        for (int keymasterDigest : Digest.allToKeymaster(authorizedKeyDigests)) {
            authorizedKeymasterKeyDigests.add(Integer.valueOf(keymasterDigest));
        }
        Set<Integer> supportedKeymasterSignatureDigests = new HashSet();
        for (int keymasterDigest2 : Digest.allToKeymaster(supportedSignatureDigests)) {
            supportedKeymasterSignatureDigests.add(Integer.valueOf(keymasterDigest2));
        }
        Set<Integer> result = new HashSet(supportedKeymasterSignatureDigests);
        result.retainAll(authorizedKeymasterKeyDigests);
        return result;
    }

    public static int getDigestOutputSizeBits(int keymasterDigest) {
        switch (keymasterDigest) {
            case 0:
                return -1;
            case 1:
                return 128;
            case 2:
                return 160;
            case 3:
                return 224;
            case 4:
                return 256;
            case 5:
                return 384;
            case 6:
                return RSA_MIN_KEY_SIZE;
            default:
                throw new IllegalArgumentException("Unknown digest: " + keymasterDigest);
        }
    }

    public static BigInteger toUint64(long value) {
        if (value >= 0) {
            return BigInteger.valueOf(value);
        }
        return BigInteger.valueOf(value).add(UINT64_RANGE);
    }

    public static boolean isKeymasterPaddingSchemeIndCpaCompatibleWithAsymmetricCrypto(int keymasterPadding) {
        switch (keymasterPadding) {
            case 1:
                return false;
            case 2:
            case 4:
                return true;
            default:
                throw new IllegalArgumentException("Unsupported asymmetric encryption padding scheme: " + keymasterPadding);
        }
    }

    public static Context getApplicationContext() {
        Application application = ActivityThread.currentApplication();
        if (application != null) {
            return application;
        }
        throw new IllegalStateException("Failed to obtain application Context from ActivityThread");
    }

    public static byte[] intToByteArray(int value) {
        ByteBuffer converter = ByteBuffer.allocate(4);
        converter.order(ByteOrder.nativeOrder());
        converter.putInt(value);
        return converter.array();
    }
}
