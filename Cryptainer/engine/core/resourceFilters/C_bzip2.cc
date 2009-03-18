//-----------------------------------------------------------------------------
// (C) 2004 - 2006, Stuart James Urquhart (jamesu@gmail.com). All Rights Reserved.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "core/filterState.h"

#include "bzlib.h"

extern "C" void bz_internal_error(int errorcode)
{
	Con::errorf("BZLIB: internal error %i", errorcode); 
}

class BzipState : public FilterState
{
	private:
		bz_stream *m_pBZipStream;
		S32        m_lastRetVal;
		static BzipState mMyself;
	public:

	virtual FilterState *init(U32 flags) {return new BzipState(flags);}

	BzipState()
	{
		static const char * const sn = "Bzip2";
		static const char * const ln = "Compress with Bzip2";
		mTag = COMPRESS_BZIP2;
		mShortName = (char*)sn;
		mInfoString = (char*)ln;
		registerHandler(this);

		m_pBZipStream = NULL;
	}

	BzipState(U32 flags)
	{
		m_pBZipStream = new bz_stream;

		m_pBZipStream->bzalloc = NULL;
		m_pBZipStream->bzfree  = NULL;
		m_pBZipStream->opaque = NULL;

		m_pBZipStream->next_in  = NULL;
		m_pBZipStream->avail_in = 0;
		m_pBZipStream->next_out  = NULL;
		m_pBZipStream->avail_out = 0;

		mFlags = flags;
		if (mFlags & FilterState::FILTER_WRITE)
			m_lastRetVal = BZ2_bzCompressInit(m_pBZipStream, 1, 0, 30);
		else
			m_lastRetVal = BZ2_bzDecompressInit(m_pBZipStream, 0, true); // use memory saving decompression scheme
	}

	~BzipState()
	{
		if (mFlags & FilterState::FILTER_WRITE)
			m_lastRetVal = BZ2_bzCompressEnd(m_pBZipStream);
		else
			m_lastRetVal = BZ2_bzDecompressEnd(m_pBZipStream);
		delete m_pBZipStream;
	}

	void reset()
	{
		if (mFlags & FilterState::FILTER_WRITE)
			BZ2_bzCompressEnd(m_pBZipStream);
		else
			BZ2_bzDecompressEnd(m_pBZipStream);

		m_pBZipStream->bzalloc = NULL;
		m_pBZipStream->bzfree  = NULL;
		m_pBZipStream->opaque = NULL;

		m_pBZipStream->next_in  = NULL;
		m_pBZipStream->avail_in = 0;
		m_pBZipStream->next_out  = NULL;
		m_pBZipStream->avail_out = 0;

		if (mFlags & FilterState::FILTER_WRITE)
			m_lastRetVal = BZ2_bzCompressInit(m_pBZipStream, 1, 0, 30);
		else
			m_lastRetVal = BZ2_bzDecompressInit(m_pBZipStream, 0, true); 
	}

	U32 dataIn()
	{
		return m_pBZipStream->avail_in;
	}

	void dataIn(U8 *buff, U32 size)
	{
		m_pBZipStream->next_in = (char*)buff;
		m_pBZipStream->avail_in = size;
	}

	U32 dataOut()
	{
		return m_pBZipStream->avail_out;
	}

	void dataOut(U8 *buff, U32 size)
	{
		m_pBZipStream->next_out = (char*)buff;
		m_pBZipStream->avail_out = size;
	}

	bool end()
	{
		// NOTE: filter will be unusable following success!
		return BZ2_bzCompress(m_pBZipStream, BZ_FINISH) == BZ_STREAM_END;
	}

	bool process()
	{
		// Ok, we need to call deflate() until the output buffer is full.
		// First check if we are out of data - return false if none to signal end
		if (m_pBZipStream->avail_in == 0) return false;

		// Keep using deflate() until we have filled up our current buffer
		while(m_pBZipStream->avail_in != 0)
		{
			if(m_pBZipStream->avail_out == 0)
				return false; // No more data to read, return false to indicate we need more data

			S32 retVal = BZ2_bzCompress(m_pBZipStream, BZ_RUN);
			//AssertFatal(retVal !=  Z_BUF_ERROR, "ZlibSubWStream::_write: invalid buffer"); TODO
			if (retVal != BZ_RUN_OK) {
				dPrintf("BzipState:: error in process!\n");
				return false;
			}
		}

		return true;
	}

	bool reverseProcess()
	{
		// Ok, we need to call inflate() until the output buffer is full.
		if (m_lastRetVal != BZ_OK)
			return false;

		while (m_pBZipStream->avail_out != 0)
		{
			if(m_pBZipStream->avail_in == 0)
			{
				// check if there is more output pending. Then bail out
				m_lastRetVal = BZ2_bzDecompress(m_pBZipStream);
				return false;
			}
			else
			// need to get more?
				m_lastRetVal = BZ2_bzDecompress(m_pBZipStream);

			//AssertFatal(retVal != Z_BUF_ERROR, "Should never run into a buffer error"); TODO
			AssertFatal(m_lastRetVal == BZ_OK || m_lastRetVal == BZ_STREAM_END, "error in the stream");

			// The end is nigh...
			if (m_lastRetVal == BZ_STREAM_END)
				return false;
		}
		//AssertFatal(m_pBZipStream->total_out == aDataOutSize,	"Error, didn't finish the decompression!");
		return true;
	}

};

BzipState BzipState::mMyself;

