//-----------------------------------------------------------------------------
// (C) 2004 - 2006, Stuart James Urquhart (jamesu@gmail.com). All Rights Reserved.
//-----------------------------------------------------------------------------

// Generic libtomcrypt encryptor
#include "platform/platform.h"
#include "console/console.h"
#include "core/filterState.h"

#include "tomcrypt.h"

// Tomcrypt helper for obtaining corresponding crypt names
const char *getCryptName(U32 flags)
{
	return FilterState::toString(flags & FilterState::ENCRYPT_ALL);
}


/// LibTomCrypt handler
///
/// This class implements encryption handling via libtomcrypt.
/// Since this is the only implementation of an encryptor currently available,
/// the crypto interface for FilterState  is based upon the ideals of this interface;
/// the main one being, the handler implements multiple hash's and encyrption standards,
/// as opposed to other filters such as ZlibState which only implements one way of filtering.
class TomcryptState : public FilterState
{
	private:
	U8 *mDataIn;
	U32 mDataInSize;
	U8 *mDataOut;
	U32 mDataOutSize;

	/// @name Tomcrypt status
	/// @{
	S32 cipher_idx;			///< Number of cipher
	CryptHash *key;			///< Hash'd key
	unsigned long ivsize;	///< Block length
	U8 IV[MAXBLOCKSIZE];		///< IV
	bool IVactive;				///< IV is valid?
	prng_state prng;			///< Stste used in encryption
	symmetric_CTR ctr;		///< State used in encrypt/decryption
	///  }

	static TomcryptState mMyself;
	public:

	virtual FilterState *init(U32 flags)
	{
		TomcryptState  *state = new TomcryptState(mFlags);
		state->cipher_idx = find_cipher(getCryptName(flags));
		state->key = NULL;
		AssertFatal(state->cipher_idx, "TomcryptState::init : Cipher not found!");

		state->ivsize = cipher_descriptor[state->cipher_idx].block_length;
		state->IVactive = false;
		return state;
	}

	TomcryptState()
	{
		static const char * const sn = "TomCrypt";
		static const char * const ln = "Generic encryptor using libtomcrypt";
		mTag = ENCRYPT_ALL; // We provide implementations of all the encryption algo's
		mShortName = (char*)sn;
		mInfoString = (char*)ln;

		// Register encryptors...
		#ifdef RIJNDAEL
		register_cipher (&aes_desc);
		#endif
		#ifdef BLOWFISH
		register_cipher (&blowfish_desc);
		#endif
		#ifdef XTEA
		register_cipher (&xtea_desc);
		#endif
		#ifdef RC5
		register_cipher (&rc5_desc);
		#endif
		#ifdef RC6
		register_cipher (&rc6_desc);
		#endif
		#ifdef SAFERP
		register_cipher (&saferp_desc);
		#endif
		#ifdef TWOFISH
		register_cipher (&twofish_desc);
		#endif
		#ifdef SAFER
		register_cipher (&safer_k64_desc);
		register_cipher (&safer_sk64_desc);
		register_cipher (&safer_k128_desc);
		register_cipher (&safer_sk128_desc);
		#endif
		#ifdef RC2
		register_cipher (&rc2_desc);
		#endif
		#ifdef DES
		register_cipher (&des_desc);
		register_cipher (&des3_desc);
		#endif
		#ifdef CAST5
		register_cipher (&cast5_desc);
		#endif
		#ifdef NOEKEON
		register_cipher (&noekeon_desc);
		#endif
		#ifdef SKIPJACK
		register_cipher (&skipjack_desc);
		#endif

		if (register_hash(&sha256_desc) == -1)
			return;

		if (register_prng(&yarrow_desc) == -1)
			return;

		//if (register_prng(&sprng_desc) == -1)
		//	return;

		// If all went well, register our encryptor
		registerHandler(this);
	}

	TomcryptState(U32 flags)
	{
		mDataInSize = 0;
		mDataOutSize = 0;
		mDataIn = 0;
		mDataOut = 0;
	}

	~TomcryptState()
	{
		// Nothing to do here
	}

	U32 dataIn()
	{
		return mDataInSize;
	}

	void dataIn(U8 *buff, U32 size)
	{
		mDataIn = buff;
		mDataInSize = size;
	}

	U32 dataOut()
	{
		return mDataOutSize;
	}

	void dataOut(U8 *buff, U32 size)
	{
		mDataOut = buff;
		mDataOutSize = size;
	}

	bool process()
	{
		if ((mDataInSize == 0) || (mDataOutSize == 0)) return false;
		U32 read = mDataInSize >= mDataOutSize ? mDataOutSize : mDataInSize;

		AssertFatal(key, "TomcryptState::process : No key!");
		S32 errorno;
		// Firstly, if IV isn't valid, write it to the output stream
		if (!IVactive)
		{
			// Setup prng for encryption (write)
			if ((rng_make_prng(128, find_prng("yarrow"), &prng, NULL)) != CRYPT_OK) {
				Con::errorf("TomcryptState::process : Error setting up PRNG!");
				return false;
			}

			S32 x = yarrow_read(IV,ivsize,&prng);
			AssertFatal(x == ivsize, "TomcryptState::process : Error reading PRNG for IV required.");
			// Copy IV and update offsets
			read -= ivsize;
			dMemcpy(mDataOut, IV, ivsize); mDataOut += ivsize; mDataOutSize -= ivsize;
			errorno = ctr_start(cipher_idx, IV, key->data, key->size, 0, CTR_COUNTER_LITTLE_ENDIAN, &ctr);

			AssertFatal(errorno == CRYPT_OK, "TomcryptState::reverseProcess : ctr_start error!");
			IVactive = true;
		}
		// Now, encypt the buffer
		errorno = ctr_encrypt(mDataIn, mDataOut,read,&ctr);
		AssertFatal(errorno == CRYPT_OK, "TomcryptState::reverseProcess : ctr_encrypt error!");

		// Less to read, less left in output
		mDataInSize -= read;
		mDataIn += read;
		mDataOut += read;
		mDataOutSize -= read;
		return true;
	}

	bool reverseProcess()
	{
		if ((mDataInSize == 0) || (mDataOutSize == 0)) return false;
		U32 read = mDataInSize >= mDataOutSize ? mDataOutSize : mDataInSize;

		AssertFatal(key, "TomcryptState::reverseProcess : No key!");
		
		S32 errorno;
		// Firstly, if IV isn't valid, read it from the input stream
		if (!IVactive) {
			dMemcpy(IV, mDataIn, ivsize); mDataIn += ivsize; mDataInSize -= ivsize; read -= ivsize;

			errorno = ctr_start(cipher_idx, IV, key->data, key->size, 0, CTR_COUNTER_LITTLE_ENDIAN, &ctr);
			AssertFatal(errorno == CRYPT_OK, "TomcryptState::reverseProcess : ctr_start error!");
			IVactive = true;
		}
		// Now, decrypt the rest of the buffer
		errorno = ctr_decrypt(mDataIn,mDataOut,read,&ctr);
		AssertFatal(errorno == CRYPT_OK, "TomcryptState::reverseProcess : ctr_decrypt error!");

		// Less to read, less left in output
		mDataInSize -= read;
		mDataIn += read;
		mDataOut += read;
		mDataOutSize -= read;
		return true;
	}

	virtual void setHash(CryptHash *hash)
	{
		key = hash;
		AssertFatal(cipher_descriptor[cipher_idx].keysize(&key->size) == CRYPT_OK, "EncryptTom::setCryptKey : Invalid keysize!");
		IVactive = false;
	}

	virtual U32 chunkSize()
	{
		return 512;
	}

	virtual void reset()
	{
		IVactive = false;
	}
};

TomcryptState TomcryptState::mMyself;

// Hash code
//------------------------------------------------------------------------------
CryptHash::CryptHash(const char *hashName)
{
	id = find_hash(hashName);
	data = NULL;
	size = 0;
	AssertFatal(id != -1, "CryptHash: hash not found!");
	if (id == -1) return;
	size = hash_descriptor[id].hashsize;
	data = new U8[size];
}

bool CryptHash::hash(const char *aKey)
{
	if (id == -1) return false;

	unsigned long maxSize = size;
	// Hash key to state data
	S32 errorno = hash_memory(id,(const unsigned char*)aKey,dStrlen(aKey),data,(long unsigned int*)&size);
	AssertFatal(errorno == CRYPT_OK, "EncryptTom::setCryptKey : Error hashing key!");
	return errorno == CRYPT_OK;
}

void CryptHash::setData(U8 *ptr)
{
	dMemcpy(data, ptr, size);
}

CryptHash::~CryptHash()
{
	if (id != -1)
		delete [] data;
}

// Hash ConsoleFunction's
//------------------------------------------------------------------------------
ConsoleFunction(getHash, const char*, 3, 3, "(hashname, key)")
{
	CryptHash *myHash = new CryptHash(argv[1]);
	U32 sz = myHash->getSize();
	char* returnString = Con::getReturnBuffer( sz+1 );
	if (!myHash->hash(argv[2])) {
		dStrcpy(returnString, "");
		return returnString;
	}
	dMemcpy(returnString, myHash->getData(), sz);
	returnString[sz] = '\0';
	return returnString;
}
