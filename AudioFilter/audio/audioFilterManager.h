#ifndef _AUDIOFILTERMANAGER_H_
#define _AUDIOFILTERMANAGER_H_

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _AUDIOFILTER_H_
#include "audio/audioFilter.h"
#endif

typedef AudioFilter* (*filterCreate)();

struct FilterInfo
{
	U8 id; // ID to initialize
	U8 audioCapability; // Capability's (up to 8)
	U8 qualityLevels; // Number of quality levels
	StringTableEntry name; // Name of encoder
	const char *extension; // Filename extensions supported
	filterCreate createFunc; // Creation function
};

//----------------------------------------------------------------------
/// Manages lists of FilterInfo objects
/// 
/// This static class manages a list of FilterInfo structures. These in turn are used to create AudioFilter instances.
/// There are two places to register your filter; Either add code to add your FilterInfo to AudioFilterManager::init(), or to another function elsewhere in a place after AudioFilterManager::init() has been called.
///
/// Example code to add a filter description is as follows.
///
/// Firstly, create a function that creates an instance of your AudioFilter, like so :
/// @code
/// AudioFilter *createPonyFilter()
/// {
///    return new PonyFilter();
/// }
/// @endcode
/// Next, add a description of your filter, like so :
/// @code
///  FilterInfo *info;
///  info = new FilterInfo;
///  info->audioCapability = AudioFilter::AudioSeek | AudioFilter::AudioTime | AudioFilter::AudioEncode;
///  info->name = StringTable->insert("PONY");
///  info->extension = ".pony";
///  info->createFunc = &createPonyFilter;
///  AudioFilterManager::registerFilter(info);
///  ResourceManager->registerExtension(".pony", AudioBuffer::construct);
/// @endcode
///
/// You should now be able to load audio files with your filter, in this case, ".pony" files. 
/// @note You can have more than 1 extension in the extension list of a FilterInfo. Seperate entries via spaces.
///
/// See AudioFilter for example usage.
class AudioFilterManager
{
	public:
		/// @name Enum's for filter creation
		/// @{
		enum {
			AudioRead=0, // NOTE: typically filter's are one way, so we won't have both read and write.
			AudioWrite=1,
		};
		/// @}
		
		/// @name Static Functions 
		/// @{
			static void init();
			static void destroy();
		/// @}
	
	private:
		/// @name Static Variables
		/// @{
			static Vector<FilterInfo*> filterList; //< List of FilterInfo's registered.
		/// @}
		
		/// @name Secret internal functions 
		/// @{
			static AudioFilter *initializeFilter(FilterInfo *info, U8 mode); ///< Creates a filter based upon FilterInfo and mode
		/// @}

	public:
		/// @name Filter Registration 
		/// @{
			static U8 registerFilter(FilterInfo *info); ///< Register an AudioFilter.
			static void unRegisterFilter(U8 id); ///< Removes registration for an AudioFilter.
			static void enumFilters(U8 *numFilters, FilterInfo *list); ///< Creates list of FilterInfo objects.
			static Vector<FilterInfo*> *getFilterList(){return &filterList;} ///< Returns reference to existing FilterInfo Vector.
			static FilterInfo *findFilterInfo(const char *filterName); ///< Finds a FilterInfo. that has a name, filterName
			static FilterInfo *getFilterInfo(U8 id); ///< Finds a FilterInfo with the specified id.
		/// @}
		
		/// @name Functions to open / close filter streams 
		/// @{
			static AudioFilter * getFilterFromFile(const char *filename, U8 mode); ///< Creates an AudioFilter based upon filename and mode/
			static void closeFilter(AudioFilter *stream); ///< Closes an instance of an AudioFilter.
			static AudioFilter *createFilter(U8 id, U8 mode); ///< Creates an AudioFilter based upon id and mode.
			static AudioFilter *createFilter(const char *name, U8 mode); ///< Creates an AudioFilter based upon name and mode.
			static AudioFilter *createFilter(FilterInfo *info, U8 mode); ///< Creates an AudioFilter based upon FilterInfo and mode.
		/// @}
};

//--------------------------------------------------------------------------
/// Extracts the file extension from a filename
/// @param filename Filename
inline const char* extractFileExtension(const char *filename)
{
	// Gets the last . from a filename
	const char *buffer = filename;
	const char *pointer = buffer;
	
	while (*buffer != '\0')
	{
		if ((*buffer == '.'))
			pointer = buffer; // Update latest pos of .
		buffer++;
	}
	return pointer;
}

//--------------------------------------------------------------------------
/// Gets the size of a token, assuming the seperator is ' '
/// @param ptr Input string
inline U32 getTokenSize(const char *ptr)
{
	// Gets size of a token in a list of ' ' seperated tokens
	const char *buffer = ptr;
	
	U32 size = 0;
	while (*buffer != '\0')
	{
		if (*buffer == ' ')
			break;
		buffer++;
		size++;
	}
	return size;
}

//--------------------------------------------------------------------------
/// Extracts the next token from a list of tokens
/// @param string Input string
/// @param seperator Token seperator
inline const char* extractListToken(const char*string, const char seperator)
{
	// Gets the next 
	const char *buffer = string;
	
	while (*buffer != '\0')
	{
		if ((*buffer == seperator))
			return buffer+1; // Update latest pos of ' '
		buffer++;
	}
	return NULL; // No more tokens
}

#endif
