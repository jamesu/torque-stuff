#ifndef _VORBISFILTER_H_
#define _VORBISFILTER_H_

#ifndef _FILTERSTREAM_H_
#include "core/filterStream.h"
#endif
#ifndef _AUDIOFILTER_H_
#include "audio/audioFilter.h"
#endif

#include "audio/vorbisStream.h"

#define VORBIS_BUFFSIZE 32768
#define VORBIS_CHUNKSIZE 4096

//----------------------------------------------------------------------
/// This class implements Ogg Vorbis Support.
///
/// When attached to a Stream (using attachStream), this class can be used to read raw PCM data from ogg vorbis files (.ogg)
///
/// @note This filter does not support encoding ogg vorbis data. 
///
/// @see AudioFilter for example usage.
/// 
class VorbisFilter : public AudioFilter
{
   typedef AudioFilter Parent;

   Stream*      m_pStream; ///< Slave Stream
   
  public:
   VorbisFilter();
   virtual ~VorbisFilter();

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
  bool readHeader(); ///< Forced user supply of data header
  U8 getChannelFormat(); ///< Get Formats for each channel used
  U32 getNumSamples(); ///< Get Number of Samples
  bool seekTime(F32 time);
  F32 getTime();
  F32 getTimeLength();
  /// @}

  public:
  /// @name Vorbis Stuff
  /// @{
  int current_section;
  OggVorbisFile vf;
  vorbis_info *vi;
  long oggRead(char *buffer,int length, int bigendianp,int *bitstream);
  /// @}
};

#endif //_VORBISFILTER_H_
