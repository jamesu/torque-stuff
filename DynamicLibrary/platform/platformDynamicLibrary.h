#ifndef _PLATFORM_DYNAMICLIBRARY_H_
#define _PLATFORM_DYNAMICLIBRARY_H_

// DynamicLibrary
// (C) 2006 James Urquhart (jamesu@gmail.com), All Rights Reserved.

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

/// PlatformDynamicLibrary
///
/// Abstracts loading dynamic library's on any platform.
/// @note Filenames for library's to load should not include any extensions.
///
class DynamicLibrary
{
public:
	DynamicLibrary();
	DynamicLibrary( const char *filename, bool reportErrors=false ); ///< Loads specified dynamic library
	~DynamicLibrary();
	
	void *getProcAddress( const char *function );
	bool isLoaded() {return mInst != NULL;} ///< Determines if the library has loaded.
	bool isError() {if (mError) {mError = false; return true;} else return false;} ///< Determines if an error has occured

protected:
	void *mInst;       ///< Pointer to library handle
	bool mError;       ///< Error has occured
	bool mErrorReport; ///< Errors will be printed directly onto the console
};

#endif // _PLATFORMDYNAMICLIBRARY_H_
