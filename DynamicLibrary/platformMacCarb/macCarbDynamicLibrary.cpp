// DynamicLibrary
// (C) 2006 James Urquhart (jamesu@gmail.com), All Rights Reserved.

#include "platformMacCarb/platformMacCarb.h"
#include "platform/platformDynamicLibrary.h"
#include "console/console.h"

#if defined(TORQUE_OS_MAC_CARB)
#if defined(TORQUE_OS_MAC_OSX)
#include <CoreFoundation/CFBundle.h>
#else
#include <CFBundle.h>
#endif
extern OSStatus LoadFrameworkBundle(CFStringRef framework, CFBundleRef *bundlePtr);
#endif

#include <CodeFragments.h>

//-----------------------------------------------------------------------------
DynamicLibrary::DynamicLibrary()
{
}

//-----------------------------------------------------------------------------
DynamicLibrary::DynamicLibrary( const char *filename, bool reportErrors )
{
	char realname[1024];
	hInst = NULL;

#if defined(TORQUE_OS_MAC_CARB)
	if (platState.osX)
	{
		dSprintf(realname, 1024, "%s.framework", filename);
		err = LoadFrameworkBundle(CFSTR(realname), &((CFBundleRef)hInst));
	}
	else
#endif
	{
		dSprintf(realname, 1024, "\p%s", filename);
		err = GetSharedLibrary (realname,
		                      kAnyCFragArch, // as we want whatever form it is in
		                      kReferenceCFrag, // the new name for the kLoadLib flag
		                      &((CFragConnectionID)hInst), // holder of codefrag connection identifier
		                      NULL, // we don't care
		                      NULL); // we don't care
	}

	mError = false;
	mErrorReport = reportErrors;
	
	if ( !mInst ) {
		Con::errorf( "DynamicLibrary: Failed to load %s.", realname );
		mInst = NULL;
	}
}

//-----------------------------------------------------------------------------
DynamicLibrary::~DynamicLibrary()
{
	if (!hInst)
		return;

#if defined(TORQUE_OS_MAC_CARB)
	if (platState.osX)
	{
		// NOTE: seems like nothing special is required here
		hInst = NULL;
	}
	else
#endif
	{
		CloseConnection(&((CFragConnectionID)hInst));
	}
}

//-----------------------------------------------------------------------------
void *DynamicLibrary::getProcAddress( const char *function )
{
	OSErr err = noErr;
	void *fnAddress = NULL;
#if defined(TORQUE_OS_MAC_CARB)
	if (platState.osX)
	{
		CFStringRef str = CFStringCreateWithCString(NULL, function, kCFStringEncodingMacRoman);
		fnAddress = CFBundleGetFunctionPointerForName( (CFBundleRef)hInst, str );
		if (fnAddress == NULL)
			err = cfragNoSymbolErr;
		CFRelease(str);
	}
	else
#endif
	{
		err = FindSymbol((CFragConnectionID)hInst, str2p(function), (char**)&fnAddress, NULL);
	}

	if (!fnAddress) {
		if (mErrorReport)
			Con::errorf("DynamicLibrary: function '%s' not found!", function);
		mError = true;		
	}
	return fnAddress;
}

