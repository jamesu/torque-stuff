//-----------------------------------------------------------------------------
// (C) 2004 - 2006, Stuart James Urquhart (jamesu@gmail.com). All Rights Reserved.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "core/fileStream.h"
#include "core/memstream.h"
#include "core/resFilter.h"
#include "core/resManager.h"

ResFilter::ResFilter(U32 aTag)
 : m_pStream(NULL),
   m_startOffset(0),
   m_streamLen(0),
   m_currOffset(0),
   m_decompressedOffset(0),
   compressedCache(NULL),
   hasWrit(false)
{
	mWriteCompressState = mCompressState = mEncryptState = NULL;
	mTag = aTag;
	mResource = NULL;
}

ResFilter::~ResFilter()
{
	detachStream();
}

bool ResFilter::allocCache(bool enableWrite)
{
	deallocCache();
	compressedCache = new U8[BLOCKWRITE_SIZE];
	return compressedCache != NULL;
}

void ResFilter::deallocCache()
{
	if (compressedCache) delete [] compressedCache;
	compressedCache = NULL;
}

bool ResFilter::attachStream(Stream* io_pSlaveStream)
{
	return attachStream(io_pSlaveStream, true);
}

bool ResFilter::attachStream(Stream* io_pSlaveStream, bool enableWrite)
{
	AssertFatal(io_pSlaveStream != NULL, "NULL Slave stream?");

	m_pStream     = io_pSlaveStream;
	m_startOffset = 0;
	m_streamLen   = m_pStream->getStreamSize();
	m_currOffset  = 0;
	m_decompressedOffset = 0;
	hasWrit = false;

	// Setup state's
	FilterState *handler = NULL;
	mCompressState = mWriteCompressState = mEncryptState = NULL;

	// Compression...
	if (handler = FilterState::findHandler(mTag & FilterState::PROCESS_ALL))
	{
		mCompressState = handler->init(mTag & FilterState::PROCESS_ALL);
		mCompressState->dataIn(NULL, 0);

		// Compressors DO require seperate write states
		// since the compression modifies the filesize, and therefor requires the state to be specifically configured.
		if (enableWrite) {
			mWriteCompressState =  handler->init((mTag & FilterState::PROCESS_ALL) | FilterState::FILTER_WRITE);
		}
	}
	else {
		Con::errorf("You need to have at least a process handler for filters!");
		return false;
	}

	// Encryption...
	if (handler = FilterState::findHandler(mTag & FilterState::ENCRYPT_ALL))
	{
		// mEncryptState is a generic encryptor, which requires one to set the key, though we leave that to the external interface
		mEncryptState = handler->init(mTag);

		// Cryptor's via libtomcrypt do not require seperate write states
		// typically, they will write a RNG state upon the first write(), or read one upon the first read()
		// It is therefor important that the filter is reset when the position is changed in the stream (when starting again from 0).
	}

	allocCache(enableWrite);

	setStatus(Ok);

	if (mWriteCompressState) mWriteCompressState->dataOut(compressedCache, BLOCKWRITE_SIZE);
	return true;
}

void ResFilter::detachStream()
{
	// If we are still writing to the cache, just dump it to the stream
	//if (mWriteCompressState) Con::warnf("could flushWrite(), out == %d)", mWriteCompressState->dataOut());
	//else Con::warnf("No write stream, so no flushWrite())");
	if (mWriteCompressState && hasWrit)
	{
		bool success = true;
		if (m_pStream->getPosition() != (m_startOffset + m_currOffset))  // only change if stream position is not identical
			success = m_pStream->setPosition(m_startOffset + m_currOffset);
		
		if (success)
		{
			// We need to instruct the FilterState compressor to end the stream.
			// However, we also need to take into account if the cache is already full.
			// To save hastle, we flush the cache, then keep telling the compressor to end,
			// until it is happy.
			flushWrite();
			mWriteCompressState->dataIn(NULL, 0);
			while (!mWriteCompressState->end())
			{
				if (mWriteCompressState->dataOut() == 0)
					flushWrite();
		    }
			if (mWriteCompressState->dataOut() < BLOCKWRITE_SIZE)
				flushWrite();
		}
	}

	// Kill state's
	if (mCompressState) delete mCompressState;
	if (mWriteCompressState) delete mWriteCompressState;
	if (mEncryptState) delete mEncryptState;

	mWriteCompressState = mCompressState = mEncryptState = NULL; // For you, valgrind

	m_pStream     = NULL;
	m_startOffset = 0;
	m_currOffset  = 0;
	m_decompressedOffset = 0;
	deallocCache();

	setStatus(Closed);
}

Stream* ResFilter::getStream()
{
   return m_pStream;
}

bool ResFilter::setStreamOffset(const U32 in_startOffset, const U32 in_streamLen)
{
	AssertFatal(m_pStream != NULL, "stream not attached!");
	if (m_pStream == NULL)
		return false;

	U32 start  = in_startOffset;
	U32 end    = in_startOffset + in_streamLen;
	U32 actual = m_pStream->getStreamSize();
	
	if (start > actual)
	{
		Con::errorf("ResFilter: Stream position invalid (%d->%d / %d)", start, end, actual);
		return false;
	}

	m_startOffset = start;
	m_currOffset  = 0;
	m_streamLen   = in_streamLen;

	if (m_streamLen != 0)
		setStatus(Ok);
	else
		setStatus(EOS);

	return true;
}

U32 ResFilter::getPosition() const
{
	return m_decompressedOffset;
}

bool ResFilter::setPosition(const U32 in_newPosition)
{
	AssertFatal(m_pStream != NULL, "Error, stream not attached");
	if (m_pStream == NULL)
		return false;

	/*
		How we seek :

		1) If the position is < than our current compressed position, we go to the beginning(reset compression buffer), then do 2)
		2) If the position is > than our compressed position, we read in the difference with read()
	*/

	U8 seekCache[512];
	if (in_newPosition < m_decompressedOffset)
	{
		
		// Set everything to the beginning
		m_currOffset = 0;
		m_decompressedOffset = 0;
		// Reset cryptor if available
		if (mEncryptState) mEncryptState->reset();
		mCompressState->reset();

		// Then read to the position by dumping to seekCache every time (sloow)
		U32 seekPos = 0;
		while (seekPos != in_newPosition)
		{
			U32 toRead = seekPos + 512 > in_newPosition ? in_newPosition - seekPos : 512;
			if (!_read(toRead, &seekCache)) return false;
			seekPos += toRead;
		}
	}
	else if (in_newPosition != m_decompressedOffset)
		return _read(in_newPosition - m_decompressedOffset, NULL);
	return true;
}

U32 ResFilter::getStreamSize()
{
	return m_streamLen;
}

bool ResFilter::_read(const U32 in_numBytes, void* out_pBuffer)
{
	AssertFatal(m_pStream != NULL, "Error, stream not attached");

	if (in_numBytes == 0)
		return true;

	if (getStatus() != Ok)
	{
		AssertFatal(getStatus() != Closed, "Attempted read from closed stream");
		return false;
	}

	if (mWriteCompressState && mWriteCompressState->dataOut() != BLOCKWRITE_SIZE)
	{
		// Write needs to be flushed, read needs to be filled
		Con::warnf("ResFilter::_read() : data was sent to write, but read() called after. Data may be lost!");
		flushWrite();
		fillRead(); // read() after just written data
	}

	// On the first read, we need to fill up the buffer,
	// or of course if anything weird happens.
	if (mCompressState->dataIn() == 0)
		fillRead();

	U8 *ptr = (U8*)out_pBuffer;        // Pointer to where we are writing to in out_pBuffer
	U8 *finishRead = ptr + in_numBytes;// Pointer to where we should stop reading

	// We'd better make sure we don't read past the end of the stream,
	// (otherwise non-EOS aware processors such as basic read will not
	// be able to figure out if the end has been reached).
	if ( (m_decompressedOffset + in_numBytes) > m_streamLen )
	{
		U32 realRead = in_numBytes - ((m_decompressedOffset + in_numBytes) - (m_streamLen));
		if (realRead == 0)
		{
			setStatus(EOS);
			return false;
		}
		finishRead = ptr + realRead;
	}

	// Keep reading utill we have got the desired actualSize of bytes
	while (ptr != finishRead)
	{
		// Straight forward decompress from compressedCache
		mCompressState->dataOut(ptr, finishRead - ptr);

		while (mCompressState->dataOut() != 0) {
			// NOTE: we are reliant on the FilterState to return false when no data is available any more
			if (!mCompressState->reverseProcess())  break;
		}

		// Update ptr, determined by the amount of data left to read in
		U32 decompressedRead =  ((finishRead - ptr) - mCompressState->dataOut());
		ptr += decompressedRead;
		m_decompressedOffset += decompressedRead;

		//Con::printf("in : %d, out : %d", mCompressState->dataIn(), mCompressState->dataOut());
		// We will likely succeed on this condition if dataIn() == 0
		if (mCompressState->dataOut() > 0)
		{
			if (mCompressState->dataIn() > 0) {
				// Crap! State must have failed
				//AssertFatal(false, "ResFilter::_read() : process() must have failed!");
				setStatus(EOS);
				return false;
			}
			// We need more data!
			if (mCompressState->dataIn() == 0)
			{
				if (!fillRead()) {
					setStatus(EOS);
					return false;
				}
			}
		}
	}

	setStatus(m_pStream->getStatus());
	return true;
}

// Temporary buffer where encrypted data goes to get decrypted
static U8 cryptCache[BLOCKWRITE_SIZE+32];

bool ResFilter::fillRead()
{
	// Read in *compressed* data
	U32 streamSize = m_pStream->getStreamSize();
	U32 currPos    = m_pStream->getPosition();

	// Go to the current position
	if (currPos != (m_startOffset + m_currOffset))
	{
		currPos = m_startOffset + m_currOffset;
		if (!m_pStream->setPosition(currPos))
			return false;
	}

	U8 *apprCache = mEncryptState ? cryptCache : compressedCache;

	U32 actualReadSize = BLOCKREAD_SIZE + currPos > streamSize ? streamSize - currPos : BLOCKREAD_SIZE;
	if (actualReadSize == 0) return false;
	if (m_pStream->read(actualReadSize, apprCache) == true)
	{
		m_currOffset += actualReadSize;
		// Setup crypt and compress states
		if (mEncryptState)
		{
			// Data is encrypted, so decrypt it first to we have an easy time in _read()
			mEncryptState->dataIn(cryptCache, actualReadSize);
			mEncryptState->dataOut(compressedCache, actualReadSize);
			while (mEncryptState->reverseProcess()) {;}
			dMemset(cryptCache, 0, actualReadSize); // potential security measure
			AssertFatal(mEncryptState->dataIn() == 0, "ResFilter::fillRead() : Crap, we missed out on some data, or there was an abrupt error!");
			actualReadSize -= mEncryptState->dataOut(); // negate any overheads due to headers

			mCompressState->dataIn(compressedCache, actualReadSize);
		}
		else if (mCompressState)
		{
			// In any case, compressor will be the first and only
			mCompressState->dataIn(compressedCache, actualReadSize);
		}
		else
			AssertFatal(false, "ResFilter::fillRead() : no filter state!");

		return true;
	}
	return false;
}

bool ResFilter::flushWrite()
{
	bool success = false;
	U32 actualSize = 0;

	// Go to the current position
	if (m_pStream->getPosition() != (m_startOffset + m_currOffset))
	{
		if (!m_pStream->setPosition(m_startOffset + m_currOffset))
			return false;
	}

	// Compressed data in compressedCache...
	if (mEncryptState)
	{
		mEncryptState->dataIn(compressedCache, BLOCKWRITE_SIZE - mWriteCompressState->dataOut());
		mEncryptState->dataOut(cryptCache, BLOCKWRITE_SIZE+32);

		while (mEncryptState->process()) {;}

		actualSize = (BLOCKWRITE_SIZE+32) - mEncryptState->dataOut();
		success = m_pStream->write(actualSize, cryptCache);
		m_currOffset += actualSize;
		dMemset(cryptCache, 0, actualSize); // potential security measure
	}
	else
	{
		actualSize = BLOCKWRITE_SIZE - mWriteCompressState->dataOut();
		success = m_pStream->write(actualSize, compressedCache);
		m_currOffset += actualSize;
	}

	mWriteCompressState->dataOut(compressedCache, BLOCKWRITE_SIZE);
	return success;
}

bool ResFilter::_write(const U32 in_numBytes, const void* in_pBuffer)
{
	if (!mWriteCompressState) return false;
	AssertFatal(m_pStream != NULL, "Error, stream not attached");

	if (in_numBytes == 0)
		return true;

	if (getStatus() != Ok)
	{
		AssertFatal(getStatus() != Closed, "Attempted write to closed stream");
		return false;
	}

	U8 *ptr = (U8*)in_pBuffer;
	U8 *finishWrite = ptr + in_numBytes;
	hasWrit = true;

	// Keep writing utill we have written the desired number of in_numBytes
	while (ptr != finishWrite)
	{
		// Straight forward decompress from compressedCache
		mWriteCompressState->dataIn(ptr, finishWrite - ptr);

		while (mWriteCompressState->dataOut() != 0)
		{
			if (!mWriteCompressState->process())  break;
		}

		// Update ptr, determined by the amount of data left to read in
		// Ideally should be full size in one pass, unless we ran out of out, or failed
		U32 decompressedWrite =  ((finishWrite - ptr) - mWriteCompressState->dataIn());
		ptr += decompressedWrite;
		m_decompressedOffset += decompressedWrite;

		//Con::printf("in : %d, out : %d", mWriteCompressState->dataIn(), mWriteCompressState->dataOut());

		// We will likely succeed on this condition if dataIn() == 0
		if (mWriteCompressState->dataIn() > 0)
		{
			if (mWriteCompressState->dataOut() != 0) {
				// Crap! State must have failed
				AssertFatal(false, "ResFilter::_write() : process() must have failed!");
				setStatus(EOS);
				return false;
			}
			// Dump out data
			if (!flushWrite()) {
				setStatus(EOS);
				return false;
			}
		}
	}

	if (m_decompressedOffset > m_streamLen)
		m_streamLen = m_decompressedOffset;

	setStatus(m_pStream->getStatus());
	return true;
}

bool ResFilter::hasCapability(const Capability caps)
{
	bool res = true;
	if ((caps & StreamWrite) && !mWriteCompressState)
		res = false;
	return res;
}


ConsoleFunction(dumpCompressionHandlers, void, 1, 1, "Print a list of compression handlers")
{
	FilterState::printHandlers();
}

