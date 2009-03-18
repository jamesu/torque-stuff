#include "audio/audioFilter.h"
#include "audio/speexFilter.h"
#include "console/console.h"
#include "core/fileStream.h"
#include "math/mMath.h"

//--------------------------------------------------------------------------
SpeexFilter::SpeexFilter()
 : m_pStream(NULL)
{
   // Set some defaults
   mDetectHeaders = true;
   mEnableRead = true;
   mEnableWrite = false;
   mQuality = 8;
   mSamplingRate = 0;
   speex_size = 0;
   mDecodedSize = 0;
   mVBRQuality = -1;
   currentEncoder = SPEEX_NONE;
}

//--------------------------------------------------------------------------
SpeexFilter::~SpeexFilter()
{
	if (getStatus() != Closed)
		detachStream();
}

//--------------------------------------------------------------------------
bool SpeexFilter::attachStream(Stream* io_pSlaveStream)
{
   AssertFatal(io_pSlaveStream != NULL, "NULL Slave stream?");
   AssertFatal(m_pStream == NULL,       "Already attached!");

   m_pStream      = io_pSlaveStream;
   m_pOutputBuffer = m_pInputBuffer = NULL; // make sure these are NULL
   
   // Setup AudioCapability
   mAudioCapability = AudioFilter::AudioEncode | AudioFilter::AudioSeek | AudioFilter::AudioTime;

   // Determine if we should detect headers or not
   bool DetectHeaders = (mEnableRead and m_pStream->hasCapability(StreamRead)) ? mDetectHeaders : false;
  
  if (DetectHeaders)
     readHeader();	//Read Speex File Info
  else {
     // Quality must be set first in order for the speex coders to be created
     setQuality(mQuality);
     setSamplingRate(mSamplingRate);
     setVBRQuality(mVBRQuality);
  }

   trackedPosition = 0;
   setStatus(Ok);
   return true;
}

//--------------------------------------------------------------------------
void SpeexFilter::detachStream()
{  
   refreshStreams(0); // remove streams
   refreshCoder(SPEEX_NONE); // destroy encoder/decoder

   mAudioCapability = 0;
   m_pOutputBuffer = NULL;
   m_pInputBuffer = NULL;
   m_pStream      = NULL;
   setStatus(Closed);
}

//--------------------------------------------------------------------------
Stream* SpeexFilter::getStream()
{
   return m_pStream;
}

//--------------------------------------------------------------------------
bool SpeexFilter::_read(const U32 numBytes, void *pBuffer)
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
	U16 FrameSize;	// Size of encoded frame being read
	U32 DataLeft;	// Data left
	U32 DataToRead;	// Size to read into buffer
	DataLeft = numBytes;
	U8 *buffer = (U8*)pBuffer;
	
	if (speex_size == 0) {
		Con::warnf("Warning : speex frame size is 0!");
		return false;
	}

	// Read in until we have read numBytes of decoded audio
	while (1)
	{
		m_pStream->read(sizeof(U16), &FrameSize); // Read encoded size

		// Calculate how many more bytes are left to decode
		DataToRead = (DataLeft > (speex_size*speex_samplesize)) ? (speex_size*speex_samplesize) : DataLeft;

		if (FrameSize == 0) {
			m_pStream->setPosition(m_pStream->getPosition()-2); // Trick into reading frame size again
			break; // Canot read any more, or could be invalid
		}

		if (!m_pStream->read(FrameSize, m_pInputBuffer))  // Buffer Up
			return false; // Bail out
		
		// Speex read
		// We perform all speex operations in the buffers, since the user *may* miss out parts of packets to read.
		speex_bits_read_from(&bits_decode,(char*)m_pInputBuffer, FrameSize);

		// Then convert
		if (speex_decode(dec_state, &bits_decode, (short*)m_pOutputBuffer) < 0) {
			Con::warnf("speex_decode < 0!");
			return false;
		}
		
		// And copy into buffer
		dMemcpy(buffer, m_pOutputBuffer, DataToRead);

		// Increment pointers and check
		buffer += DataToRead;
		trackedPosition += DataToRead;
		
		if (DataLeft == DataToRead) {
			break; // We have finished the frames
		}
		DataLeft -= DataToRead; // We've done 1 frame worth of decoded data
	}
	
	// Tell torque we're ok...
	setStatus(Ok);

   return true;
}

//--------------------------------------------------------------------------
bool SpeexFilter::_write(const U32 numBytes, const void *pBuffer)
{
   U32 bytesToWrite = 0;// Bytes we have yet to write
   U32 bytesLeft = 0;	// Bytes remaining
   U16 EncodedSize = 0;	// Size of encoded data
   U8 *buffer;		// Pointer to where we write

   AssertFatal(pBuffer != NULL, "NULL input buffer");
   if (getStatus() == Closed)
   {
      AssertFatal(false, "Attempted write to a closed stream");
      return false;
   }

   // Get pointer to our data
   buffer = (U8*)pBuffer;
   bytesToWrite = 0;
   bytesLeft = numBytes; 

   // Now do a buffered write
   // Will continue until we have written all of the data
   while (1) {
      /* Write Bytes */
      if (bytesLeft == 0) {
      	// Write terminator
	EncodedSize = 0;
	m_pStream->write(sizeof(U16), &EncodedSize);
      	break;
      }

      // Encode in blocks of speex_size (in bytes)
      bytesToWrite = (bytesLeft > speex_size*speex_samplesize) ? speex_size*speex_samplesize : bytesLeft;

      // Encode this frame
      speex_bits_reset(&bits_encode); // Reset

      dMemcpy(m_pInputBuffer, buffer, bytesToWrite); // Copy decoded mem from buffer into raw_data
      speex_encode(enc_state, (short*)m_pInputBuffer, &bits_encode);
      EncodedSize = speex_bits_write(&bits_encode, (char*)m_pOutputBuffer, bytesToWrite); // (bytes)

      // Write the data
      m_pStream->write(sizeof(U16), &EncodedSize);  // Size in bytes of Frame
      m_pStream->write(EncodedSize, m_pOutputBuffer); // Write the speex_data
      
      // Increment pointers
      bytesLeft -= bytesToWrite;
      buffer += bytesToWrite;
      trackedPosition += bytesToWrite;
      
      
      if (bytesToWrite == bytesLeft)
      	bytesLeft = 0; // This will write terminator and break next loop 
   }

   // Tell torque we're ok...
   setStatus(Ok);

   return true;
}

//--------------------------------------------------------------------------
U32 SpeexFilter::getPosition() const
{
   AssertFatal(m_pStream != NULL, "Error, not attached");
   return trackedPosition;
}

//---------------------------------------------------------------------------
bool SpeexFilter::setPosition(const U32 newPosition)
{
   AssertFatal(m_pStream != NULL, "Error, not attached");
   m_pStream->setPosition(13); // Offset from header
   if (m_pStream->getStreamSize() >= mDecodedSize)
   	return false;

   // Now we need to keep going through the frame's till we get close to our position
   U32 newPos = 13;
   U16 encodedFrameSize = 0;
   U32 FrameSize = speex_size*getFormatSize(getChannelFormat());
   U32 decodedPos = FrameSize; // Where we are going to be, not where we are
   
   while (1)
   {
	// Check if this frame is within the decoded position
	if (decodedPos >= newPosition)
		break;
	
	// Otherwise, go to next frame
	m_pStream->read(sizeof(U16), &encodedFrameSize);
	decodedPos += FrameSize;
	
	newPos += encodedFrameSize+2; // 2 == sizeof(U16)
	m_pStream->setPosition(newPos);
   }
   
   trackedPosition = decodedPos-FrameSize; // Set tracked position to where we are

   if (mEnableRead and m_pStream->hasCapability(StreamRead))
   	speex_bits_reset(&bits_decode); // Reset decoding bits
   if (mEnableWrite and m_pStream->hasCapability(StreamWrite))
   	speex_bits_reset(&bits_encode); // Reset encoding bits

   return true;
}

//--------------------------------------------------------------------------
bool SpeexFilter::seekTime(F32 time)
{
	// Calulate position considering time, then seek to it
	U32 sampleSize = getFormatSize(getChannelFormat());
	U32 calcPos = U32(mCeil(time * (mSamplingRate) * sampleSize));
	
	return setPosition(calcPos);
}

//--------------------------------------------------------------------------
U32 SpeexFilter::getNumSamples()
{
	return mDecodedSize/getFormatSize(getChannelFormat()); // (assuming 16bit mono)
}

//--------------------------------------------------------------------------
void SpeexFilter::setQuality(U8 num)
{
	U32 realquality;
	// Recreate coders if neccesary
	if (num > 16) {
		realquality = num - 16;
		refreshCoder(SPEEX_UWB); // Ultra Wide Band
	}
	else if (num > 8) {
		realquality = num - 8;
		refreshCoder(SPEEX_WB); // Wide Band
	}
	else  {
		realquality = num;
		refreshCoder(SPEEX_NB); // Narrow Band
	}
	
	// Now set the quality
	U32 newsize;
	if (mEnableWrite and m_pStream->hasCapability(StreamWrite))
	{
		// Set quality
		speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY, &realquality);
		speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &newsize);
	}
	if (mEnableRead and m_pStream->hasCapability(StreamRead))
	{
		speex_decoder_ctl(dec_state, SPEEX_GET_FRAME_SIZE, &newsize);
	}
	
	// Rezsize buffers if neccesary
	refreshStreams(newsize);
	mQuality = num;
}

//--------------------------------------------------------------------------
void SpeexFilter::refreshStreams(U32 newsize)
{
	// This function recreates the buffers if the frame size has changed
	
	if (newsize == speex_size)
		return; // No need to resize
		
	speex_size = newsize; // size in samples of each frame
	speex_samplesize = getFormatSize(getChannelFormat());
	
	if (m_pInputBuffer != NULL)
		delete [] m_pInputBuffer;

	if (speex_size != 0)
		m_pInputBuffer = new U8[speex_size*speex_samplesize];
	else
		m_pInputBuffer = NULL;

	if (m_pOutputBuffer != NULL)
		delete [] m_pOutputBuffer;
	
	if (speex_size != 0)
		m_pOutputBuffer = new U8[speex_size*speex_samplesize];
	else
		m_pOutputBuffer = NULL;
}

//--------------------------------------------------------------------------
void SpeexFilter::setSamplingRate(U32 val)
{
	U32 newsize;
	mSamplingRate = val;
	if (mEnableWrite and m_pStream->hasCapability(StreamWrite)) {
		speex_encoder_ctl(enc_state, SPEEX_SET_SAMPLING_RATE, &mSamplingRate);
		speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &newsize);
	}
	if (mEnableRead and m_pStream->hasCapability(StreamRead)) {
		speex_decoder_ctl(dec_state, SPEEX_SET_SAMPLING_RATE, &mSamplingRate);
		speex_decoder_ctl(dec_state, SPEEX_GET_FRAME_SIZE, &newsize);
	}
	
	// Rezsize buffers if neccesary
	refreshStreams(newsize);
}

//--------------------------------------------------------------------------
bool SpeexFilter::readHeader()
{
	m_pStream->read(sizeof(U32), &mDecodedSize);
	m_pStream->read(sizeof(U32), &mSamplingRate);
	m_pStream->read(sizeof(U8), &mQuality);
	m_pStream->read(sizeof(F32), &mVBRQuality);
	setQuality(mQuality);
	setVBRQuality(mVBRQuality);
	setSamplingRate(mSamplingRate);
	return true;
}

//--------------------------------------------------------------------------
bool SpeexFilter::writeHeader()
{
	m_pStream->write(sizeof(U32), &mDecodedSize);
	m_pStream->write(sizeof(U32), &mSamplingRate);
	m_pStream->write(sizeof(U8), &mQuality);
	m_pStream->write(sizeof(F32), &mVBRQuality);
}

//--------------------------------------------------------------------------
void SpeexFilter::setVBRQuality(F32 val)
{
	U32 newsize = 0;
	if (mEnableWrite and m_pStream->hasCapability(StreamWrite))
	{
		U32 vbr;
		if (val < .0) // Disable VBR
			vbr = 0;
		else // Enable VBR
			vbr = 1;
	
		speex_encoder_ctl(enc_state, SPEEX_SET_VBR, &vbr);
		if (vbr == 1)
			speex_encoder_ctl(enc_state, SPEEX_SET_VBR_QUALITY, &val);
		speex_encoder_ctl(enc_state, SPEEX_GET_FRAME_SIZE, &newsize);
	}
	if (mEnableRead and m_pStream->hasCapability(StreamRead))
		speex_decoder_ctl(dec_state, SPEEX_GET_FRAME_SIZE, &newsize);
	
	refreshStreams(newsize);
}

//--------------------------------------------------------------------------
U8 SpeexFilter::getChannelFormat()
{
	return CHANNEL_MONO_16;
}

//--------------------------------------------------------------------------
void SpeexFilter::refreshCoder(U8 mode)
{
	U32 enh = 1;
	if (currentEncoder == mode)
		return;
	
	// Destroy current coders
	if (currentEncoder != SPEEX_NONE) {
		if (mEnableWrite and m_pStream->hasCapability(StreamWrite))
		{
			// Destroy Encoder
			speex_bits_destroy(&bits_encode);
			speex_encoder_destroy(enc_state);
		}
		if (mEnableRead and m_pStream->hasCapability(StreamRead))
		{
			// Destroy Decoder
			speex_bits_destroy(&bits_decode);
			speex_decoder_destroy(dec_state);
		}
	}
	
	// Determine which coders to create
	const SpeexMode *mode_ptr;
	switch (mode)
	{
		case SPEEX_NB:
			currentEncoder = SPEEX_NB;
			mode_ptr = &speex_nb_mode;
		break;
		case SPEEX_WB:
			currentEncoder = SPEEX_WB;
			mode_ptr = &speex_wb_mode;
		break;
		case SPEEX_UWB:
			currentEncoder = SPEEX_UWB;
			mode_ptr = &speex_uwb_mode;
		break;
		default:
		case SPEEX_NONE:
			currentEncoder = SPEEX_NONE;
			return;
		break;
	}
	
	// Recreate the encoder and decoder
	if (mEnableWrite and m_pStream->hasCapability(StreamWrite))
	{
		speex_bits_init(&bits_encode);
		enc_state = speex_encoder_init(mode_ptr);
	}

	if (mEnableRead and m_pStream->hasCapability(StreamRead))
	{
		speex_bits_init(&bits_decode);
		dec_state = speex_decoder_init(mode_ptr);
		speex_decoder_ctl(dec_state, SPEEX_SET_ENH, &enh);  
	}
	
	// Reinstate some settings
	setSamplingRate(mSamplingRate);
	setVBRQuality(mVBRQuality);
}

//--------------------------------------------------------------------------
F32 SpeexFilter::getTime()
{
	U32 sampleSize = getFormatSize(getChannelFormat());
	return ((trackedPosition/sampleSize) / mSamplingRate);
}

//--------------------------------------------------------------------------
F32 SpeexFilter::getTimeLength()
{
	U32 sampleSize = getFormatSize(getChannelFormat());
	return ((mDecodedSize/sampleSize) / mSamplingRate);
}
