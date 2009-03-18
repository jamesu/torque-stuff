#ifndef _RAWFILTER_H_
#define _RAWFILTER_H_

#ifndef _FILTERSTREAM_H_
#include "core/filterStream.h"
#endif
#ifndef _AUDIOFILTER_H_
#include "audio/audioFilter.h"
#endif

//----------------------------------------------------------------------
/// This class implements Raw audio support.
///
/// When attached to a Stream (using attachStream), this class can be used to read and write raw PCM data.
///
/// Please note that the default sampling rate is 44100, and the channel format is 16bit mono pcm.
/// If you wish to change the filter's sampling rate or channel format, use setSamplingRate/setChannelFormat AFTER attaching the stream with attachStream()
///
/// @see AudioFilter for example usage.
/// 
class RawFilter : public AudioFilter
{
   typedef AudioFilter Parent;

   Stream*      m_pStream; ///< Slave Stream
   
  public:
   RawFilter();
   virtual ~RawFilter();

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
   
   U32 getStreamSize();
   /// @}

   /// @name AudioFilter Management 
   /// @{
  U8 getChannelFormat();		///< Get Formats for each channel used
  bool setChannelFormat(U8 format);	///< Sets Channel format
  U32 getNumSamples();			///< Get Number of Samples
  bool seekTime(F32 time);		///< Seeks to time
  F32 getTime();			///< Gets current time
  F32 getTimeLength();			///< Gets length of stream in time
  /// @}

  private:
  	U8 mChannelFormat;
};

#endif
