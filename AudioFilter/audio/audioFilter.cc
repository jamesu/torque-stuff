#include "audio/audioFilter.h"
#include "math/mMathFn.h"

//--------------------------------------------------------------------------
AudioFilter::AudioFilter()
 : m_pStream(NULL),
 m_pInputBuffer(NULL),
 m_pOutputBuffer(NULL)
{
   //
   mDetectHeaders = false;
   mEnableRead = true; // It is presumed we will mainly be reading
   mEnableWrite = false;
   mDecodedSize = mSamplingRate = 0;
   mVBRQuality = .0;
   mAudioCapability = 0;
}

//--------------------------------------
AudioFilter::~AudioFilter()
{
   detachStream();
}

//--------------------------------------
bool AudioFilter::attachStream(Stream* io_pSlaveStream)
{
   AssertFatal(io_pSlaveStream != NULL, "NULL Slave stream?");
   AssertFatal(m_pStream == NULL,       "Already attached!");

   m_pStream      = io_pSlaveStream;

   setStatus(Ok);
   return true;
}

//--------------------------------------
void AudioFilter::detachStream()
{
   m_pStream      = NULL;
   setStatus(Closed);
}

//--------------------------------------
Stream* AudioFilter::getStream()
{
   return m_pStream;
}

//--------------------------------------
bool AudioFilter::_read(const U32 numBytes, void *pBuffer)
{
   if (numBytes == 0)
      return true;

   AssertFatal(pBuffer != NULL, "NULL input buffer");
   if (getStatus() == Closed)
   {
      AssertFatal(false, "Attempted read from a closed stream");
      return false;
   }

   AssertFatal(false, "Cannot directly call AudioFilter::_read");
   return true;
}

//--------------------------------------
bool AudioFilter::_write(const U32 numBytes, const void *pBuffer)
{
   if (numBytes == 0)
      return true;

   AssertFatal(pBuffer != NULL, "NULL input buffer");
   if (getStatus() == Closed)
   {
      AssertFatal(false, "Attempted write to a closed stream");
      return false;
   }

   AssertFatal(false, "Cannot directly call AudioFilter::_write");

   return true;
}

//--------------------------------------
bool AudioFilter::hasCapability(const Capability cap) const
{
   if (m_pStream)
   	return m_pStream->hasCapability(cap);
   return false;
}

//--------------------------------------
bool AudioFilter::hasAudioCapability(const AudioCapability cap) const
{
   return mAudioCapability & cap;
}


//--------------------------------------
U32 AudioFilter::getPosition() const
{
   AssertFatal(m_pStream != NULL, "Error, not attached");
   return m_pStream->getPosition();
}

//--------------------------------------
bool AudioFilter::setPosition(const U32 newPosition)
{
   AssertFatal(m_pStream != NULL, "Error, not attached");
   return m_pStream->setPosition(newPosition);
}

//--------------------------------------
U32 AudioFilter::getStreamSize()
{
   return mDecodedSize;
}

//--------------------------------------
void AudioFilter::setQuality(U8 num)
{
}

//--------------------------------------
bool AudioFilter::readHeader()
{
	return false;
}

//--------------------------------------
bool AudioFilter::writeHeader()
{
	return false;
}

//--------------------------------------
U8 AudioFilter::getChannelFormat()
{
	return 255;
}

//--------------------------------------
bool AudioFilter::seekTime(F32 time)
{
	return false;
}

//--------------------------------------
F32 AudioFilter::getTime()
{
	return .0;
}

//--------------------------------------
F32 AudioFilter::getTimeLength()
{
	return .0;
}

//--------------------------------------
U32 AudioFilter::getNumSamples()
{
	return 0;
}


