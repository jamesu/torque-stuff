// Encryption hash wrapper
#ifndef _HASH_H_
#define _HASH_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif


//#define NO_TOMCRYPT  // Define if E_tomcrypt is not compiled in!

#ifndef NO_TOMCRYPT
class TomcryptState;

// Hashing implemented by libtomcrypt in E_tomcrypt.cpp
class CryptHash
{
	friend class TomcryptState;

private:

	S32 id;		///< TomCrypt Hash ID
	S32 size;	///< TomCrypt Hash Size
	U8 *data;	///< TomCrupt Hash Data

public:

	CryptHash(const char *hashName);	///< Constructor. Must provide hash name
	bool hash(const char *aKey);		///< Creates a hash based on aKey

	/// @name get/set hash data
	/// @{
	U8 *getData() {return data;}		///< Obtain a pointer
	void setData(U8 *ptr);				///< Sets new hash (copying data)
	/// @}

	S32 getSize() {return size;}		///< Gets size of hash
	~CryptHash();
};

#else
// Dummy hashing class
class CryptHash
{
	CryptHash(const char *hashName) {;}
	bool hash(const char *aKey) {return false;};

	U8 *getData() {return NULL;}
	void setData(U8 *ptr) {;}

	S32 getSize() {return -1;}
};
#endif

#endif
