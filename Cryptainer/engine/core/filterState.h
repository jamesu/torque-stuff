//-----------------------------------------------------------------------------
// (C) 2004 - 2006, Stuart James Urquhart (jamesu@gmail.com). All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef _FILTERSTATE_H_
#define _FILTERSTATE_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#include "core/hash.h"

#define BLOCKREAD_SIZE 4096
#define BLOCKWRITE_SIZE 2048 * 1024

#define NO_BLOCKSIZE

/// FilterState
///
/// This class is a wrapper round the many compression and encryption library's available; It can also incorporate lossy/lossless audio compression, though it currently does not posess the neccesary functions to facilitate data required for that purpose.
///
/// Using the filter is quite simple; you supply input data via dataIn(), and tell it where to output via dataOut(). One then uses either process() or reverseProcess() to perform the corresponding operations on the input data.
/// e.g.
/// @code
/// // Get handler
/// FilterState *myFilter = findHandler(FilterState::PROCESS_BASIC);
/// // Initialize instance of handler;
/// // Flags sent again in case FilerState implements multiple techniques.
/// myFilter = myFilter->init(FilterState::PROCESS_BASIC);
/// 
/// myFilter->dataIn(myData, myDataSize);
/// myFilter->dataOut(myOutput, myOuputSize);
///
/// while (myFilter->process()) {
///    // FilterState may take multiple passes to
///    // process due to implementation.
///    // You can use dataIn()/dataOut() to check if you need to refresh your data.
/// }
/// @endcode
/// 
/// @note For encryption to work, you NEED to set a CryptHash via setCryptHash().
///
/// @warning FilterState does not take into account endian issues with the input data.
///
/// Adding a FilterState is relatively easy, as they are linked together by a static linked list, which is initialized at runtime via static constructors on each FilterState derivative.
/// @see BasicState for an example of a basic FilterState.
class FilterState
{
public:
	// Filter flags
	/* List of filters */
	enum {
		// Processors...
		PROCESS_BASIC = BIT(0),
		PROCESS_DELTA8 = BIT(1),
		PROCESS_DELTA16 = BIT(2),
		PROCESS_DELTA32 = BIT(3),
		PROCESS_ONLY = PROCESS_BASIC | PROCESS_DELTA8 | PROCESS_DELTA16 | PROCESS_DELTA32,
		// Compressors...
		COMPRESS_ZLIB = BIT(10),
		COMPRESS_BZIP2 = BIT(11),
		COMPRESS_ONLY = COMPRESS_ZLIB | COMPRESS_BZIP2,
		// Compressors and Processors are grouped together (for ResFilter code, aswell as other potential uses)
		PROCESS_ALL = PROCESS_ONLY | COMPRESS_ONLY,
		// Encryptors
		ENCRYPT_BLOWFISH = BIT(20),
		ENCRYPT_TWOFISH = BIT(21),
		ENCRYPT_RIJNDAEL = BIT(22),
		ENCRYPT_XTEA = BIT(23),
		ENCRYPT_RC6 = BIT(24),
		ENCRYPT_DES = BIT(25),
		ENCRYPT_ALL = ENCRYPT_BLOWFISH | ENCRYPT_TWOFISH | ENCRYPT_RIJNDAEL | ENCRYPT_XTEA | ENCRYPT_RC6 | ENCRYPT_DES,

		// Extra Capabilities
		FILTER_WRITE = BIT(30), // Read is always implied
	};

	static const char *toString(U32 flags);
	static U32         fromString(const char *name, bool write);

	FilterState(){;}
	FilterState(U32 flags){mFlags = flags;}
	virtual ~FilterState(){;}

	virtual FilterState *init(U32 flags) {return new FilterState(flags);}

	/// @name Quick reference
	/// @{
	virtual U32 dataIn(){return 0;}							///< Data left to read in
	virtual void dataIn(U8 *buff, U32 size){;}			///< Set data to read in
	virtual U32 dataOut(){return 0;}							///< Compressed data left to write out
	virtual void dataOut(U8 *buff, U32 size){;}			///< Set data in out buffer
	virtual U32 chunkSize() {return BLOCKREAD_SIZE;}	///< Default chunk size (to read in)
	/// @}

protected:
	/// @name Linking variables
	/// @{
	static FilterState *mRoot;	///< Linked list for root
	FilterState        *mNext;	///< Next object in linked list
	U32 mTag;						///< Tag of filter
	const char *mShortName;		///< Name of filter
	const char *mInfoString;	///< Description of filter
	/// @}

public:
	/// @name Static handler functions
	/// These are used to manage or search the list of filter handlers
	/// @{
	static void registerHandler(FilterState *aHandler);	///< Adds handler to linked list of handlers
	static FilterState *findHandler(U32 aTag);				///< Finds handler that matches bits with supplied tag
	static void printHandlers();									///< Prints a list of handlers to the console
	/// @}

	/// @name Compression / Decompression routines
	/// @{
	/// Override these in derivative classes
	virtual bool process() {return false;}				///< Process dataIn(), outputting to dataOut()
	virtual bool reverseProcess() {return false;}	///< The same as process, but the routine goes in reverse
	virtual void reset() {;}				///< Causes filter to read in headers / write out headers again (on next *Process)
	virtual bool end() {return true;}	///< Tells the filter to dump out any remaining data, including any EOS markers (used by compressors)
	/// @}

	 /// @name Encryption functions
	/// Encryption keys
	/// @{
	virtual void setHash(CryptHash *hash){;}		///< Set crypt hash
	/// @}

protected:
	U32 mFlags;

};

#endif //_FILTERSTATE_H_
