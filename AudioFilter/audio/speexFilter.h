#ifndef _SPEEXFILTER_H_
#define _SPEEXFILTER_H_

#ifndef _FILTERSTREAM_H_
#include "core/filterStream.h"
#endif
#ifndef _AUDIOFILTER_H_
#include "audio/audioFilter.h"
#endif

#include "speex.h" // Speex header

//----------------------------------------------------------------------
/// This class implements Speex Support.
///
/// When attached to a Stream (using attachStream), this class can be used to read and write raw PCM data from and to speex encoded streams.
///
/// @note The file format this filter outputs is NOT the same as the official speex file format,
/// use the ConsoleFunction filterConvert to convert audio to the format this filter uses.
///
/// @see AudioFilter for example usage.
/// 
class SpeexFilter : public AudioFilter
{
   typedef AudioFilter Parent;

   Stream*      m_pStream; ///< Slave Stream

  public:
   SpeexFilter();
   virtual ~SpeexFilter();

   /// @name Overrides of NFilterStream
   /// @{
  public:
   bool    attachStream(Stream* io_pSlaveStream);
   void    detachStream();
   Stream* getStream();
   /// @}

   // Mandatory overrides.  By default, these are simply passed to
   //  whatever is returned from getStream();
   /// @name Mandatory overrides 
   /// @{
  protected:
   bool _read(const U32 in_numBytes,  void* out_pBuffer);
   bool _write(const U32 in_numBytes, const void* in_pBuffer);
  public:
   U32  getPosition() const;
   bool setPosition(const U32 in_newPosition);
   /// @}

   /// @name AudioFilter Management 
   /// @{
   // Functions that we can use
  bool readHeader();		///< Forced user supply of data header
  bool writeHeader();		///< Write the header
  U8 getChannelFormat();	///< Get Formats for each channel used
  U32 getNumSamples();		///< Get Number of Samples
  void setSamplingRate(U32 val);///< Set Sampling rate
  void setQuality(U8 num);	///< Set Quality level
  void setVBRQuality(F32 val);	///< Set VBR Quality in speex
  bool seekTime(F32 time);	///< Seek to time in stream
  F32 getTime();		///< Get current time in stream
  F32 getTimeLength();		///< Get current time length
  /// @}

  public:
  /// @name Speex Stuff 
  /// @{
   SpeexBits bits_encode;	///< Bits for encoding
   SpeexBits bits_decode;	///< Bits for decoding
   void *dec_state;		///< decoder state
   void *enc_state;		///< encoder state
   U32 speex_size;		///< Buffer size for speex data encoding
   U32 speex_samplesize;	///< Size of samples in speex data
   void refreshStreams(U32);	///< Recreates Streams of size speex_size
   void refreshCoder(U8 mode);	///< Recreates encoder
   U8 currentEncoder;		///< Current Encoder
   U32 trackedPosition;		///< Position we tracked in stream
  /// @}
  
  /// @name Simple enum of speex modes 
  /// @{
  enum {
	SPEEX_NONE=0,
	SPEEX_NB,
	SPEEX_WB,
	SPEEX_UWB
  };
  /// @}
};

#endif //_SPEEXFILTER_H_
