//-----------------------------------------------------------------------------
// (C) 2004 - 2006, Stuart James Urquhart (jamesu@gmail.com). All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef _RESFILTER_H_
#define _RESFILTER_H_

//Includes
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _FILTERSTREAM_H_
#include "core/filterStream.h"
#endif
#ifndef _FILTERSTATE_H_
#include "core/filterState.h"
#endif

#define BLOCKREAD_SIZE 4096
#define BLOCKWRITE_SIZE 2048 * 1024

class ResourceObject;

/// Resource filter main class
///
/// The data in this FilterStream stream goes through two steps (all optional, though a Basic Process is at least required) :
///
///	* Processing - Compression or plain simple processing methods; Can alter size of input data
///	* Encryption - Does not alter the size of the data; Similar to Processing in that it mangles the input, though uses a key to do so.
///
/// This class is pretty much tied to the FilterState class; up to 3 FilterState's can be initialized in one instance at any one time (processing, processing(write version) and encryption).
///
/// @note Encryption does not require a seperate write process due to how the underlying implementation works.
class ResFilter : public FilterStream
{
	typedef FilterStream Parent;
	
	/// @name Parent stream details
	/// @{
	Stream* m_pStream;		///< The parent stream
	U32     m_startOffset;	///< Offset which we call 0
	U32     m_streamLen;		///< Maximum distance from start offset we are allowed to go
	U32     m_currOffset;	///< Current offset we are at in parent stream
	/// @}
	
	/// @name Process, Compression, and Encryption handlers
	/// @{
	FilterState *mCompressState;			///< Compress Handler (typically reduces the data size)
	FilterState *mWriteCompressState;	///< Compress Handler (write state)
	FilterState *mEncryptState;			///< Encrypt Handler (crypt's the stream with a key)
	bool hasWrit;								///< Tells us if _write() has been called. Used on stream detach
	U32 mTag;									///< Tag that specifies which set of FilterState's to use
	/// @}
	
	/// @name Details for this stream
	/// @{
	U32     m_decompressedOffset;			///< Offset from startOffset decompressed
	/// @}
	
	/// @name I/O buffer
	/// @{
	U8 *compressedCache;	///< Compressed data cache
	
	bool allocCache(bool enableWrite);	///< Allocates new cache data
	void deallocCache();						///< Free's allocated cache data
	/// @}
	
	public:

	ResFilter(U32 aTag);	///< Dynamic constructor
	~ResFilter();			///< Destructor
	
	bool    attachStream(Stream* io_pSlaveStream);						///< Attach to stream; Assume read-only
	bool    attachStream(Stream* io_pSlaveStream, bool readMode);	///< Attach to stream; Optionally write
	void    detachStream();														///< Detatch slave stream
	Stream* getStream();															///< Get slave stream
	U32     getFlags() {return mTag;}										///< Get tags used to create filter
	
	bool setStreamOffset(const U32 in_startOffset, const U32 in_streamLen);	///< Set offset and length(decompressed) of master in slave stream
	
	/// @name State Settings
	/// Changes and maintains the state of the filter
	/// @{
	FilterState *initState();															///< Initialize state of a compressor or processor
	FilterState *initState(const char *cryptName, const char *hashName);	///< Init state of an encryptor (requires name of cryptor)
	
	bool doesCrypt(){return mEncryptState != NULL;}			///< Tells us if filter has an encryptor set
	bool doesCompress(){return mCompressState != NULL;}	///< Tells us if filter has an encryptor set
	
	void setHash(CryptHash *hash) {if (mEncryptState) mEncryptState->setHash(hash);}

	static void printHandlers();	///< Prints a list of available FilterState's to the console
	
	bool flushWrite();	///< Flush write buffer to slave stream's current position
	bool fillRead();		///< Fill read buffer, reading in new data from slave stream's current position
	/// @}
	
	// Mandatory overrides.
	protected:
	
	bool _read(const U32 in_numBytes,  void* out_pBuffer);
	bool _write(const U32 in_numBytes, const void* in_pBuffer);

	ResourceObject *mResource; ///< Pointer to owner resource for tracking (on write)

	public:
	
	U32  getPosition() const;
	U32  getRealPosition() const {return m_startOffset + m_currOffset;}
	U32  getOffsetPosition() const {return m_currOffset;}
	bool setPosition(const U32 in_newPosition);

	bool hasCapability(const Capability);

	void setResource(ResourceObject *obj) {mResource = obj;}
	ResourceObject *getResource() {return mResource;}
	
	U32  getStreamSize();
};

#endif //_RESSFILTER_H_
