#include "audio/audioFilter.h"

#ifndef NO_AUDIOFILTER
// Include Filter includes here

#ifndef NO_VORBISFILTER
#include "audio/vorbisFilter.h"
#endif

#ifndef NO_SPEEXFILTER
#include "audio/speexFilter.h"
#endif

#include "audio/rawFilter.h"

#endif

#include "audio/audioFilterManager.h"
#include "audio/audioBuffer.h"
#include "console/console.h"

// Static variables
Vector<FilterInfo*> AudioFilterManager::filterList;

// Plonk filter creation functions here
//--------------------------------------------------------------------------

#ifndef NO_AUDIOFILTER

#ifndef NO_SPEEXFILTER
AudioFilter* createSpeexFilter()
{
	return new SpeexFilter();
}
#endif

#ifndef NO_VORBISFILTER
AudioFilter *createVorbisFilter()
{
	return new VorbisFilter();
}
#endif

AudioFilter *createRawFilter()
{
	return new RawFilter();
}
#endif

//--------------------------------------------------------------------------
void AudioFilterManager::init()
{
	FilterInfo *info;
	 
	#ifndef NO_AUDIOFILTER
	// Register the standard AudioFilter's here...
	#ifndef NO_SPEEXFILTER
	info = new FilterInfo;
	info->audioCapability = AudioFilter::AudioEncode;
	info->name = StringTable->insert("SPEEX");
	info->extension = ".spx";
	info->createFunc = &createSpeexFilter;
	registerFilter(info);
	ResourceManager->registerExtension(".spx", AudioBuffer::construct);
	#endif
	
	#ifndef NO_VORBISFILTER
	info = new FilterInfo;
	info->audioCapability = AudioFilter::AudioSeek | AudioFilter::AudioTime;
	info->name = StringTable->insert("VORBIS");
	info->extension = ".ogg";
	info->createFunc = &createVorbisFilter;
	registerFilter(info);
	ResourceManager->registerExtension(".ogg", AudioBuffer::construct);
	#endif
	
	info = new FilterInfo;
	info->audioCapability = AudioFilter::AudioSeek | AudioFilter::AudioTime | AudioFilter::AudioEncode;
	info->name = StringTable->insert("RAW");
	info->extension = ".raw";
	info->createFunc = &createRawFilter;
	registerFilter(info);
	ResourceManager->registerExtension(".raw", AudioBuffer::construct);
	
	// NOTE: .wav filter has not been implemented. 
	// This is here to prevent any fatal assert's as a result of the .wav extension not being registered.
	ResourceManager->registerExtension(".wav", AudioBuffer::construct);
	#endif
}

//--------------------------------------------------------------------------
void AudioFilterManager::destroy()
{
	// Destroy all the FilterInfo's
	for (U8 i=0;i<filterList.size();i++)
	{
		delete filterList[i];
	}
	filterList.clear();
}

//--------------------------------------------------------------------------
AudioFilter * AudioFilterManager::getFilterFromFile(const char *filename, U8 mode)
{
	S32 len = dStrlen(filename);
	
	const char *extension = extractFileExtension(filename);
	
	// Loop through the FilterInfo's and find appropriate filter for extension
	for (U8 i=0;i<filterList.size();i++)
	{
		// Find out if the extensions match...
		const char *strPtr=filterList[i]->extension;
		while (strPtr != NULL) 
		{
			if (!dStricmp(strPtr, extension))
			{
				return initializeFilter(filterList[i], mode);
			}
			strPtr = extractListToken(strPtr, ' ');
		}
	}
	return NULL;
}

//--------------------------------------------------------------------------
void AudioFilterManager::closeFilter(AudioFilter *stream)
{
	delete stream;
}

//--------------------------------------------------------------------------
U8 AudioFilterManager::registerFilter(FilterInfo *info)
{
	U8 id = filterList.size();
	while (getFilterInfo(id)) // while the id is still being used...
	{
		id++; // increment it
		// (goes on till we find a free id)
	}
	// By now we have and id, so assign it...
	info->id = id;
	filterList.push_back(info);
}

// Removes info for a particular filter
//--------------------------------------------------------------------------
void AudioFilterManager::unRegisterFilter(U8 id)
{
	// Find EncoderInfo where id == id in list
	// remove that EncoderInfo from list
	for (U8 i=0;i<filterList.size();i++)
	{
		if (filterList[i]->id == id)
		{
			delete filterList[i];
			filterList.erase(i);
			break;
		}
	}
}

// Creates a list of Encoder info's we have,
// independant of the main list.
// Note: you must delete this list afterwards.
//--------------------------------------------------------------------------
void AudioFilterManager::enumFilters(U8 *numFilters, FilterInfo *list)
{
	// Create the *list the size of the encoder List
	// Store amount of encoders in numEncoders
	// Loop through encoder list
	// Copy encoder info to list
	list = new FilterInfo[filterList.size()];
	*numFilters = filterList.size();
	for (U8 i=0;i<filterList.size();i++)
	{
		list[i].id = filterList[i]->id;
		list[i].audioCapability = filterList[i]->audioCapability;
		list[i].name = filterList[i]->name;
		list[i].extension = filterList[i]->extension;
		list[i].createFunc = NULL; // Don't copy it
	}
}

//--------------------------------------------------------------------------
FilterInfo *AudioFilterManager::findFilterInfo(const char *filterName)
{
	for (U8 i=0;i<filterList.size();i++)
	{
		// Note: we HAVE to use dStrcmp since filterName is not
		// known to be part of the stringtable
		// (However, this could easily be changed)
		if (!dStricmp(filterList[i]->name, filterName))
		{
			return filterList[i];
		}
	}
	return NULL;
}

//--------------------------------------------------------------------------
FilterInfo *AudioFilterManager::getFilterInfo(U8 id)
{
	for (U8 i=0;i<filterList.size();i++)
	{
		if (filterList[i]->id == id)
		{
			return filterList[i];
		}
	}
	return NULL;
}

// Creates an AudioFilter based upon internal FilterInfo.
//--------------------------------------------------------------------------
AudioFilter *AudioFilterManager::initializeFilter(FilterInfo *info, U8 mode)
{
	AudioFilter *filter;
	// Check to see if this filter can write if we asked for it
	if (mode == AudioWrite)
	{
		if (!(info->audioCapability & AudioFilter::AudioEncode))
		{
			Con::errorf("Error: could not open filter %s for write", info->name);
			return NULL;
		}
	}
			
	filter = (*info->createFunc)();
	AssertFatal(filter, "Got NULL AudioFilter!");
	switch (mode)
	{
		case AudioRead:
			filter->enableRead(true);
			filter->enableWrite(false);
		break;
		case AudioWrite:
			filter->enableRead(false);
			filter->enableWrite(true);
		break;
		default:
			AssertFatal(false, "createFilter() recieved odd mode!");
		break;
	}
	return filter;
}

// Creates an instance of an AudioFilter in default encoding mode,
// using a memorybuffer of bufferSize length
//--------------------------------------------------------------------------
AudioFilter *AudioFilterManager::createFilter(U8 id, U8 mode)
{
	// Look in list, find Encoder, create using createFunc, return instance
	for (U8 i=0;i<filterList.size();i++)
	{
		if (filterList[i]->id == id)
		{
			return initializeFilter(filterList[i], mode);
		}
	}
	return NULL;
}

//--------------------------------------------------------------------------
AudioFilter *AudioFilterManager::createFilter(FilterInfo *info, U8 mode)
{
	return createFilter(info->id, mode);
}

//--------------------------------------------------------------------------
AudioFilter *AudioFilterManager::createFilter(const char*name, U8 mode)
{
	// Look in list, find Encoder, create using createFunc, return instance
	for (U8 i=0;i<filterList.size();i++)
	{
		// NOTE: we do not know if name is in the stringtable,
		// so we *have* to use dStricmp!
		if (!dStricmp(filterList[i]->name, name))
		{
			return initializeFilter(filterList[i], mode);
		}
	}
	return NULL;
}

/// @name Utility Functions for AudioFilterManager
/// @{

//--------------------------------------------------------------------------

/// Converts 1 file to another
/// @param 3 Input File
/// @param 4 Output File
/// @param 5 Output Quality (Optional)
/// @param 6 Output VBR Quality
///
/// Example usage :
/// @code
/// filterConvert("mymusic.ogg", "myvoicemusic.spx", 5, 0.5);
/// @endcode
///
/// @note Any errors will be reported in the console; Beware that this function may cause torque to hang for 
/// an extended period of time depending on the size of the audio you are converting.
ConsoleFunction(filterConvert, void, 3, 5, "filterConvert(filename in, filename out [quality, vbrquality])")
{
   FileStream *in;
   FileStream *out;
   AudioFilter *in_filter;
   AudioFilter *out_filter;
   char *decoded_buffer;
   
   U8 quality=8;
   F32 vbrquality=-1;
   
   if (argc > 3)
   {
   	quality = dAtoi(argv[3]);
   	if (argc > 4)
   		vbrquality = dAtof(argv[4]);
   }

   U32 samplingRate;
   
   // Set up the filters and the reading buffer
   in_filter = AudioFilterManager::getFilterFromFile(argv[1],AudioFilterManager::AudioRead);
   if (!in_filter)
   {
      Con::errorf("Error: could not open filter to read!");
   }
   out_filter = AudioFilterManager::getFilterFromFile(argv[2],AudioFilterManager::AudioWrite);
   if (!out_filter)
   {
      Con::errorf("Error: could not open filter to write!");
      AudioFilterManager::closeFilter(in_filter);
      return;
   }

   // Read in input audio...
   in = new FileStream();
   in->open(argv[1], FileStream::Read);

   // Write out output audio...
   out = new FileStream();
   out->open(argv[2], FileStream::Write);

   // Filter Data
   Con::printf("Filtering Data...");
   in_filter->attachStream(in);
   out_filter->attachStream(out);
   U32 size = in_filter->getStreamSize();
   Con::printf("File Sampling Rate is : %d", in_filter->getSamplingRate());
   Con::printf("Reading %d bytes", size);
   decoded_buffer = new char[size];

   // read in Data
   Con::printf("Decoding Data...");
   in_filter->read(size, decoded_buffer);

   // write decoded data
   Con::printf("Encoding Data to out, quality %d, VBR quality %f", vbrquality);
   Con::printf("Attached to stream, now writing...");
   out_filter->setSize(size);
   out_filter->setQuality(quality);
   out_filter->setVBRQuality(vbrquality);
   out_filter->setSamplingRate(in_filter->getSamplingRate());
   out_filter->writeHeader(); // Header is very important
   out_filter->write(size, decoded_buffer);

   // Cleanup
   Con::printf("Detatched from stream");
   in_filter->detachStream();

   Con::printf("Cleanup");
   out->close();
   in->close();
   delete in;
   AudioFilterManager::closeFilter(in_filter);
   AudioFilterManager::closeFilter(out_filter);
   delete out;

   delete [] decoded_buffer;
}

/// Lists available filters in the AudioFilterManager
ConsoleFunction(filterList, void, 1, 1, "filterList")
{
	Vector<FilterInfo*> *infos;
	Vector<FilterInfo*>::iterator info;
	infos = AudioFilterManager::getFilterList();
	Con::printf("%d Filters Available :", infos->size());
	
	for (info = infos->begin(); info != infos->end(); info++)
	{
		Con::printf("Filter %d : %s ", (*info)->id, (*info)->name);
		if ((*info)->audioCapability & AudioFilter::AudioEncode)
			Con::printf("   Supports Encoding");
		Con::printf("   Extensions supported : %s",(*info)->extension);
	}
}

/// @}
