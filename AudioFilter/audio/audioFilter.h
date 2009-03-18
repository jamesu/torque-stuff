#ifndef _AUDIOFILTER_H_
#define _AUDIOFILTER_H_

//Includes
#ifndef _FILTERSTREAM_H_
#include "core/filterStream.h"
#endif

//----------------------------------------------------------------------
/// This class provides a universal interface for various audio formats.
///
/// When attached to a Stream (using attachStream), this class can be used to read raw PCM data
/// from a stream.  It can also be used to encode and write data (depending if the filter supports encoding audio)
///
/// Additionally, you can also obtain and set various variables to aid with the reading and writing of the encoded audio data in the stream, such as sampling rate, quality level, vbr quality level, decoded size, and channel format.
///
/// @note The creation of the filter's is managed by AudioFilterManager. This class allows one to easily create instances of filters for playing back audio files, or creating specific encoders/decoders by name.
/// 
/// Attaching an audiofilter to a stream couldn't be simpler. The following example attaches an AudioFilter to a FileStream for reading, and reads all data out of the stream into an array :
/// @code
///  FileStream *fs = new FileStream();
///  fs->open("~/myfile.ogg", FileStream::Read);
///  AudioFilter *filter = AudioFilterManager::createFilterFromFile("~/myfile.ogg", AudioFilterManager::AudioRead);
///  filter->attachStream(fs);
///  U32 size = filter->getStreamSize();
///  U8 *data = new U8[size];
///  filter->read(size, data);
/// @endcode
///
/// An additional thing to remember is that there is no specific limit to the number of quality levels you can specify (except the limit of the U8 data type). 
/// To be on the safe side, assume there is a fixed amount of quality levels, equal to the number of possible quality levels in SpeexFilter (24).
/// As for VBRQuality, that should only go from 0-1.0. Any value less than 0 means the VBR option is turned off.
///
class AudioFilter : public FilterStream
{
   typedef FilterStream Parent;
   
   public:
   enum AudioCapability {
      AudioSeek    = BIT(0),	///< Can this stream seek?
      AudioTime    = BIT(1),	///< Can this stream seek in time?
      AudioEncode  = BIT(2)	///< Can we encode audio with this filter?
   };

   private:
   Stream*      m_pStream; ///< Slave Stream

   protected:
   /// @name Buffers for i/o 
   /// @{
   U8*          m_pOutputBuffer; ///< Buffer for writing
   U8*          m_pInputBuffer; ///< Buffer for reading
   /// @}

   U8 mAudioCapability;

  public:
   AudioFilter();
   virtual ~AudioFilter();

   /// @name Overrides of FilterStream 
   /// @{
  public:
   bool    attachStream(Stream* io_pSlaveStream);
   void    detachStream();
   Stream* getStream();
   /// @}

   /// @name Mandatory overrides 
   /// (Mandatory overrides - By default, these are simply passed to
   ///  whatever is returned from getStream();)
   /// @{
  protected:
   bool _read(const U32 in_numBytes,  void* out_pBuffer);
   bool _write(const U32 in_numBytes, const void* in_pBuffer);
  public:
   bool hasCapability(const Capability) const;
   bool hasAudioCapability(const AudioCapability) const;

   U32  getPosition() const;
   bool setPosition(const U32 in_newPosition);

   U32  getStreamSize();
   /// @}

   protected:
   /// @name AudioFilter Variables 
   /// @{
   /// (Covers basic terms used in different formats)
   U32 mSamplingRate;		///< Sampling rate for audio
   bool mDetectHeaders;		///< Read in headers from stream on attach?
   bool mEnableWrite;		///< Enable write buffer?
   bool mEnableRead;		///< Enable read buffer?
   U32 mDecodedSize;		///< Size of PCM Data (total)
   U32 mQuality;	///< Compression Factor
   F32 mVBRQuality;		///< Variable Bitrate Quality
   /// @}
   public:
   /// @name AudioFilter Quality 
   /// @{  
   virtual void setQuality(U8 num); ///< Sets quality factor
   U32 getQuality() {return mQuality;}
   virtual void setVBRQuality(F32 val) {mVBRQuality=val;}
   F32 getVBRQuality() {return mVBRQuality;}
   /// @}
  
   /// @name AudioFilter I/O 
   /// @{
   void detectHeaders(bool val) {mDetectHeaders = val;} ///< Enabled / Disables auto detection of headers
   void enableRead(bool val) {mEnableRead = val;}
   void enableWrite(bool val) {mEnableWrite = val;}
   virtual bool readHeader(); ///< Read in header
   virtual bool writeHeader(); ///< Write out header
   U32 setSize(U32 val) {mDecodedSize = val;}
   /// @}
  
   /// @name PCM Data and Advanced Seeking 
   /// @{
   U32 getSamplingRate() {return mSamplingRate;} ///< Get Sampling Rate, or Frequency
   virtual void setSamplingRate(U32 val) {mSamplingRate = val;} ///< Set Sampling Rate
   
   virtual U8 getChannelFormat(); ///< Get Channel Format
   virtual bool setChannelFormat(){return true;} ///< Sets Channels Format. Not implemented by all Filters.
   virtual U32 getNumSamples(); ///< Get total number of samples (per channel)

   virtual F32 getTime(); ///< Get current time in stream
   virtual bool seekTime(F32 time); ///< Seek to specific time in stream
   virtual F32 getTimeLength(); ///< Get current time length
   /// @}

   /// @name enum for stream formats 
   /// @{
   enum {
	CHANNEL_MONO_8=1,
	CHANNEL_MONO_16,
	CHANNEL_STEREO_8,
	CHANNEL_STEREO_16,
	CHANNEL_UNKNOWN=255
   };
   /// @}
};

/// Function to get size of sample in format
inline U8 getFormatSize(U8 format)
{
	switch (format)
	{
		case AudioFilter::CHANNEL_MONO_16:
			return 2;
		break;
		case AudioFilter::CHANNEL_STEREO_16:
			return 4;
		break;
		default:
			return 0;
		break;
	};
}

#endif //_AUDIOFILTER_H_
