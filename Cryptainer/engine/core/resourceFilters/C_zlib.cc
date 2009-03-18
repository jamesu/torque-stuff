//-----------------------------------------------------------------------------
// (C) 2004 - 2006, Stuart James Urquhart (jamesu@gmail.com). All Rights Reserved.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "core/filterState.h"

#include "zlib.h"

class ZipState : public FilterState
{
	private:
		z_stream_s *m_pZipStream;
		static ZipState moMyself;
	public:

	virtual FilterState *init(U32 flags) {return new ZipState(flags);}

	ZipState()
	{
		static const char * const sn = "Zlib";
		static const char * const ln = "Compress with Zlib";
		mTag = COMPRESS_ZLIB;
		mShortName = (char*)sn;
		mInfoString = (char*)ln;
		registerHandler(this);

		m_pZipStream = NULL;
	}

	ZipState(U32 flags)
	{
		m_pZipStream = new z_stream_s;

		m_pZipStream->zalloc = Z_NULL;//myMalloc;
		m_pZipStream->zfree  = Z_NULL;//myFree;
		m_pZipStream->opaque = Z_NULL;

		m_pZipStream->next_in  = NULL;
		m_pZipStream->avail_in = 0;
		m_pZipStream->total_in = 0;
		m_pZipStream->next_out  = NULL;
		m_pZipStream->avail_out = 0;
		m_pZipStream->total_out = 0;

		mFlags = flags;
		if (mFlags & FilterState::FILTER_WRITE)
			deflateInit2(m_pZipStream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
		else
			inflateInit2(m_pZipStream, -MAX_WBITS);
	}

	void reset()
	{
		if (mFlags & FilterState::FILTER_WRITE)
			deflateEnd(m_pZipStream);
		else
			inflateEnd(m_pZipStream);

		m_pZipStream->zalloc = Z_NULL;//myMalloc;
		m_pZipStream->zfree  = Z_NULL;//myFree;
		m_pZipStream->opaque = Z_NULL;

		m_pZipStream->next_in  = NULL;
		m_pZipStream->avail_in = 0;
		m_pZipStream->total_in = 0;
		m_pZipStream->next_out  = NULL;
		m_pZipStream->avail_out = 0;
		m_pZipStream->total_out = 0;

		if (mFlags & FilterState::FILTER_WRITE)
			deflateInit2(m_pZipStream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
		else
			inflateInit2(m_pZipStream, -MAX_WBITS);
	}

	~ZipState()
	{
		if (m_pZipStream == NULL) return;
		if (mFlags & FilterState::FILTER_WRITE)
			deflateEnd(m_pZipStream);
		else
			inflateEnd(m_pZipStream);
		delete m_pZipStream;
	}

	U32 dataIn()
	{
		return m_pZipStream->avail_in;
	}

	void dataIn(U8 *buff, U32 size)
	{
		m_pZipStream->next_in = buff;
		m_pZipStream->avail_in = size;
		m_pZipStream->total_in = 0;
	}

	U32 dataOut()
	{
		return m_pZipStream->avail_out;
	}

	void dataOut(U8 *buff, U32 size)
	{
		m_pZipStream->next_out = buff;
		m_pZipStream->avail_out = size;
		m_pZipStream->total_out = 0;
	}

	bool end()
	{
		// NOTE: filter will be unusable following success!
		return deflate(m_pZipStream, Z_FINISH) == Z_STREAM_END;
	}

	bool process()
	{
		// Ok, we need to call deflate() until the output buffer is full.
		// First check if we are out of data - return false if none to signal end
		if (m_pZipStream->avail_in == 0)
		{
			deflate(m_pZipStream, Z_SYNC_FLUSH);
			if (m_pZipStream->avail_out != 0)
				return false;
		}

		// Keep using deflate() until we have filled up our current buffer
		while(m_pZipStream->avail_in != 0)
		{
			if(m_pZipStream->avail_out == 0)
				return false; // No more data to read, return false to indicate we need more data

			S32 retVal = deflate(m_pZipStream, Z_NO_FLUSH);
			AssertFatal(retVal !=  Z_BUF_ERROR, "ZlibSubWStream::_write: invalid buffer");
			if (retVal != Z_OK) {
				Con::warnf("ZipState:: error in process (%s)!", m_pZipStream->msg);
				return false;
			}
		}

		return true;
	}

	bool reverseProcess()
	{
		// Ok, we need to call inflate() until the output buffer is full.

		while (m_pZipStream->avail_out != 0)
		{
			S32 retVal = Z_OK;

			if(m_pZipStream->avail_in == 0)
			{
				// check if there is more output pending. Then bail out
				retVal = inflate(m_pZipStream, Z_SYNC_FLUSH);
				//if(m_pZipStream->avail_in == 0)
				return false;
			}
			else
			// need to get more?
				retVal = inflate(m_pZipStream, Z_SYNC_FLUSH);

			AssertFatal(retVal != Z_BUF_ERROR, "Should never run into a buffer error");
			AssertFatal(retVal == Z_OK || retVal == Z_STREAM_END, "error in the stream");

			// The end is nigh...
			if (retVal == Z_STREAM_END)
			{
				// If we haven't managed to fill in the output
				// buffer, there may still be data present, so
				// continue on.
				//if (m_pZipStream->avail_out != 0)
					return false;
			}
		}
		//AssertFatal(m_pZipStream->total_out == aDataOutSize,	"Error, didn't finish the decompression!");
		return true;
	}

};

ZipState ZipState::moMyself;

