#include "audio/audioFilter.h"
#include "audio/rawFilter.h"
#include "console/console.h"
#include "math/mMathFn.h"
   
//--------------------------------------------------------------------------
RawFilter::RawFilter()
 : m_pStream(NULL)
{
   //
   mDetectHeaders = true;
}

//--------------------------------------------------------------------------
RawFilter::~RawFilter()
{
   detachStream();
}

//--------------------------------------------------------------------------
bool RawFilter::attachStream(Stream* io_pSlaveStream)
{
   AssertFatal(io_pSlaveStream != NULL, "NULL Slave stream?");
   AssertFatal(m_pStream == NULL,       "Already attached!");

   m_pStream      = io_pSlaveStream;
   mSamplingRate = 44100; mDecodedSize = 0;
   mVBRQuality = 0;
   mChannelFormat = AudioFilter::CHANNEL_MONO_16;
   
   // Setup Capabilities for this stream
   mAudioCapability = AudioFilter::AudioSeek | AudioFilter::AudioTime | AudioFilter::AudioEncode;

   setStatus(Ok);
   return true;
}

//--------------------------------------------------------------------------
void RawFilter::detachStream()
{
   mAudioCapability = 0; // reset capability's

   m_pStream      = NULL;
   setStatus(Closed);
}

//--------------------------------------------------------------------------
Stream* RawFilter::getStream()
{
   return m_pStream;
}

//--------------------------------------------------------------------------
bool RawFilter::_read(const U32 numBytes, void *pBuffer)
{
   if (numBytes == 0)
      return true;

   AssertFatal(pBuffer != NULL, "NULL input buffer");
   if (getStatus() == Closed)
   {
      AssertFatal(false, "Attempted read from a closed stream");
      return false;
   }

   /* Read Bytes */

   // Direct read
   if (!m_pStream->read(numBytes, pBuffer))
   	return false;

   // Tell torque we're ok...
   setStatus(Ok);

   return true;
}

//--------------------------------------------------------------------------
bool RawFilter::_write(const U32 numBytes, const void *pBuffer)
{
   if (numBytes == 0)
      return true;

   AssertFatal(pBuffer != NULL, "NULL input buffer");
   if (getStatus() == Closed)
   {
      AssertFatal(false, "Attempted write to a closed stream");
      return false;
   }

   // Direct write
   if (!m_pStream->write(numBytes, pBuffer))
   	return false;
   
   // Tell torque we're ok...
   setStatus(Ok);

   return true;
}

//--------------------------------------------------------------------------
U32 RawFilter::getPosition() const
{
   AssertFatal(m_pStream != NULL, "Error, not attached");
   return m_pStream->getPosition();
}

//--------------------------------------------------------------------------
bool RawFilter::setPosition(const U32 newPosition)
{
   AssertFatal(m_pStream != NULL, "Error, not attached");
   return m_pStream->setPosition(newPosition);
}

//--------------------------------------------------------------------------
U32 RawFilter::getNumSamples()
{
	return U32(mCeil(m_pStream->getStreamSize() / getFormatSize((const_cast<RawFilter*>(this))->getChannelFormat())));
}

//--------------------------------------------------------------------------
bool RawFilter::seekTime(F32 time)
{
	// Calulate position considering time, then seek to it
	U32 sampleSize = getFormatSize((const_cast<RawFilter*>(this))->getChannelFormat());
	U32 calcPos = U32(mCeil(time * (mSamplingRate) * sampleSize));
	
	if (calcPos > m_pStream->getStreamSize())
		return false;
	return setPosition(calcPos);
}

//--------------------------------------------------------------------------
F32 RawFilter::getTime()
{
	U32 sampleSize = getFormatSize((const_cast<RawFilter*>(this))->getChannelFormat());
	return ((m_pStream->getPosition()/sampleSize) / mSamplingRate);
}

//--------------------------------------------------------------------------
F32 RawFilter::getTimeLength()
{
	return (getNumSamples()/mSamplingRate);
}

//--------------------------------------------------------------------------
U8 RawFilter::getChannelFormat()
{
	return mChannelFormat;
}

// Note : setChannelFormat is only useful for setting the channel format of the audio, e.g. on encoders.
// It is not implemented on Filter's with only Read capability.
//--------------------------------------------------------------------------
bool RawFilter::setChannelFormat(U8 format)
{
	mChannelFormat = format;
}

//--------------------------------------------------------------------------
U32 RawFilter::getStreamSize()
{
	return m_pStream->getStreamSize();
}
