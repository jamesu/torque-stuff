// DynamicLibrary
// (C) 2006 James Urquhart (jameus@gmail.com), All Rights Reserved.

#include "platformWin32/platformWin32.h"
#include "platform/platformDynamicLibrary.h"
#include "console/console.h"

//-----------------------------------------------------------------------------
DynamicLibrary::DynamicLibrary()
{
}

//-----------------------------------------------------------------------------
DynamicLibrary::DynamicLibrary( const char *filename, bool reportErrors )
{
	char realname[1024];
	dSprintf(realname, 1024, "%s.dll", filename);
	mError = false;
	mErrorReport = reportErrors;
	
	mInst = LoadLibraryA( realname );
	if ( !mInst ) {
		Con::errorf( "DynamicLibrary: Failed to load %s.", realname );
		mInst = NULL;
	}
}

//-----------------------------------------------------------------------------
DynamicLibrary::~DynamicLibrary()
{
	if (mInst)
		FreeLibrary((HMODULE)mInst);
}

//-----------------------------------------------------------------------------
void *DynamicLibrary::getProcAddress( const char *function )
{
	if (mInst) {
		void *addr = GetProcAddress((HMODULE)mInst, function);
		if (addr == NULL) {
		   if (mErrorReport)
			   Con::errorf("DynamicLibrary: function '%s' not found!", function);
		   mError = true;
		}
		return addr;
	}
	else
		return NULL;
}

