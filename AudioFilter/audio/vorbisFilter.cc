#include "audio/audioFilter.h"
#include "audio/vorbisFilter.h"
#include "console/console.h"

#if defined(TORQUE_BIG_ENDIAN)
	#define ENDIAN 1
#else
	#define ENDIAN 0
#endif

//--------------------------------------------------------------------------
VorbisFilter::VorbisFilter()
 : m_pStream(NULL)
{
   //
   mDetectHeaders = true;
   vi = NULL;
}

//--------------------------------------------------------------------------
VorbisFilter::~VorbisFilter()
{
   detachStream();
}

//--------------------------------------------------------------------------
bool VorbisFilter::attachStream(Stream* io_pSlaveStream)
{
   AssertFatal(io_pSlaveStream != NULL, "NULL Slave stream?");
   AssertFatal(m_pStream == NULL,       "Already attached!");

   m_pStream      = io_pSlaveStream;
   
   // Setup Capabilities for this stream
   mAudioCapability = AudioFilter::AudioSeek | AudioFilter::AudioTime;

   // Open vorbis file
   if(vf.ov_open(m_pStream , NULL, 0) < 0) {
      setStatus(IOError);
      return false;
   }

   // Success!
   // Now setup our buffers

   if ( mEnableWrite && (m_pStream->hasCapability(StreamWrite)))
   	m_pOutputBuffer = new U8[VORBIS_BUFFSIZE];
   else
   	m_pOutputBuffer = NULL;

   vi = NULL;

   if (mDetectHeaders)
     readHeader();	//Read Vorbis File Info

   setStatus(Ok);
   return true;
}

//--------------------------------------------------------------------------
void VorbisFilter::detachStream()
{
   if (mEnableWrite) {
   	if (m_pOutputBuffer != NULL)
   	delete [] m_pOutputBuffer;
   }
   
   mAudioCapability = 0; // reset capability's
   
   vf.ov_clear();

   m_pOutputBuffer = NULL;

   m_pStream      = NULL;
   setStatus(Closed);
}

//--------------------------------------------------------------------------
Stream* VorbisFilter::getStream()
{
   return m_pStream;
}

//--------------------------------------------------------------------------
long VorbisFilter::oggRead(char *buffer,int length,
		    int bigendianp,int *bitstream) {
	long bytesRead = 0;
	long totalBytes = 0;
	long offset = 0;
	long bytesToRead = 0;
	while((offset) < length) {
		if((length - offset) < VORBIS_CHUNKSIZE)
			bytesToRead = length - offset;
		else
			bytesToRead = VORBIS_CHUNKSIZE;

		bytesRead = vf.ov_read(buffer, bytesToRead, bigendianp, bitstream);
		// Might fix mac audio issue and possibly others...based on references, this looks like correct action
		// linux ver will hang on exit after a stream loop if we don't
		// do this
		if (bytesRead == OV_HOLE)
		  // retry, see:
		  // http://www.xiph.org/archives/vorbis-dev/200102/0163.html
		  // http://www.mit.edu/afs/sipb/user/xiphmont/ogg-sandbox/vorbis/doc/vorbis-errors.txt
		  continue;

		if(bytesRead <= 0)
			break;
		offset += bytesRead;
		buffer += bytesRead;
	}
	return offset;
}

//--------------------------------------------------------------------------
bool VorbisFilter::_read(const U32 numBytes, void *pBuffer)
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

    // Read into temp buffer
    if (oggRead((char*)pBuffer, numBytes, ENDIAN, &current_section) == 0) {
       return false;
    }

   // Tell torque we're ok...
   setStatus(Ok);

   return true;
}

//--------------------------------------------------------------------------
bool VorbisFilter::_write(const U32 numBytes, const void *pBuffer)
{
  AssertFatal(0, "Write on VorbisFilter not permitted!");
   if (numBytes == 0)
      return true;

   U32 writtenBytes = 0;	// Bytes we have written so far
   U32 bytesToWrite = 0;	// Bytes we have yet to write
   U8 *buffer;			// Pointer to where we write

   AssertFatal(pBuffer != NULL, "NULL input buffer");
   if (getStatus() == Closed)
   {
      AssertFatal(false, "Attempted write to a closed stream");
      return false;
   }

   // Tell torque we're ok...
   setStatus(Ok);

   return true;
}

//--------------------------------------------------------------------------
U32 VorbisFilter::getPosition() const
{
   AssertFatal(m_pStream != NULL, "Error, not attached");
   return vf.vf->pcm_offset * getFormatSize((const_cast<VorbisFilter*>(this))->getChannelFormat());
}

//--------------------------------------------------------------------------
bool VorbisFilter::setPosition(const U32 newPosition)
{
   AssertFatal(m_pStream != NULL, "Error, not attached");
   return vf.ov_pcm_seek(newPosition) >= 0;
}

//--------------------------------------------------------------------------
U32 VorbisFilter::getNumSamples()
{
	return vf.ov_pcm_total(-1);
}

//--------------------------------------------------------------------------
bool VorbisFilter::seekTime(F32 time)
{
	return vf.ov_time_seek(time) < 0 ? false : true; 
}

//--------------------------------------------------------------------------
F32 VorbisFilter::getTime()
{
	return vf.ov_time_tell();
}

//--------------------------------------------------------------------------
F32 VorbisFilter::getTimeLength()
{
	return vf.ov_time_total(-1);
}

//--------------------------------------------------------------------------
bool VorbisFilter::readHeader()
{
	vi = vf.ov_info(-1);
	mSamplingRate = vi->rate;
	mDecodedSize = vf.ov_pcm_total(-1) * getFormatSize((const_cast<VorbisFilter*>(this))->getChannelFormat());
	return true;
}

//--------------------------------------------------------------------------
U8 VorbisFilter::getChannelFormat()
{
	if(vi->channels == 1) {
		return CHANNEL_MONO_16;
	} else {
		return CHANNEL_STEREO_16;
	}
}
