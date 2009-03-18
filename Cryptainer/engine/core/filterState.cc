//-----------------------------------------------------------------------------
// (C) 2004 - 2006, Stuart James Urquhart (jamesu@gmail.com). All Rights Reserved.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "core/fileStream.h"
#include "core/filterState.h"

FilterState *FilterState::mRoot = NULL;

static const char *FilterStateString[] = {
		"basic",
		"delta8",
		"delta16",
		"delta32",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		"zlib",
		"bzip2",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		"blowfish",
		"twofish",
		"rijndael",
		"xtea",
		"rc6",
		"des",
		NULL,
		NULL,
		NULL,
		NULL,
		"writes",
		NULL,
};

static void printHandlerNames(char *buffer, U32 bufferSize, U32 flags)
{
	int i=0;
	char *ptr = buffer;
	U32 charLeft = bufferSize;
	*ptr = '\0';

	U32 calc = flags;
	for (i=0; i<31; i++)
	{
		if (calc & 0x1) {
			charLeft -= dSprintf(ptr, charLeft, "%s ", FilterStateString[i]);
			ptr = buffer + dStrlen(buffer);
		}
		if (charLeft <= 1)
			break;
		calc = calc >> 1;
	}
	buffer[bufferSize-1] = '\0';
}

void FilterState::printHandlers()
{
	dPrintf("Processing handlers\n");
	dPrintf("======================\n");

	FilterState *walker;
	walker = mRoot;
	while (walker)
	{
		char buffer[2048];
		printHandlerNames(buffer, 2048, walker->mTag);
		dPrintf("Handler\t%s : [ %s]\n\tFunction: ", walker->mShortName, buffer);
		if (walker->mTag & ENCRYPT_ALL)
			dPrintf("Encrypts ");
		if (walker->mTag & PROCESS_ONLY)
			dPrintf("Processes ");
		if (walker->mTag & COMPRESS_ONLY)
			dPrintf("Compresses ");

		dPrintf("\n\t%s\n\n", walker->mInfoString);
		walker = walker->mNext;
	}
	dPrintf("======================\n");
}

const char *FilterState::toString(U32 flags)
{
	int i=0;
	U32 calc = flags;
	for (i=0; i<32; i++)
	{
		if (calc & 0x1)
			return FilterStateString[i];
		calc = calc >> 1;
	}
	return "unknown";
}

U32 FilterState::fromString(const char *name, bool write)
{
	int i=0;
	U32 calc = 0;
	for (i=0; i<31; i++)
	{
		if (FilterStateString[i] && !dStrcmp(FilterStateString[i], name))
		{
			calc = 1 << i;
			break;
		}
	}
	if (write)
		calc |= FILTER_WRITE;
	return calc;
}

void FilterState::registerHandler(FilterState *aHandler)
{
	aHandler->mNext = mRoot;
	mRoot = aHandler;
}

FilterState *FilterState::findHandler(U32 aTag)
{
	FilterState *walker;
	walker = mRoot;
	while (walker)
	{
		if (aTag & walker->mTag)
			return walker;
		walker = walker->mNext;
	}
	return NULL;
}

// Various useful ConsoleFunction's
//------------------------------------------------------------------------------

ConsoleFunction(dumpCompressionHandlers, void, 1, 1, "Print a list of compression handlers")
{
	FilterState::printHandlers();
}

#ifdef TORQUE_DEBUG
#include "core/resManager.h"
ConsoleFunction(testFilterState, void, 1, 1, "Tests filter state code")
{
	// Our filter states...
	FilterState *state1 = FilterState::findHandler(FilterState::PROCESS_BASIC)->init(FilterState::FILTER_WRITE);
	FilterState *state2 = FilterState::findHandler(FilterState::COMPRESS_ZLIB)->init(FilterState::FILTER_WRITE);

	U32 readSize = state2->chunkSize()*2;
	U8 *inData = new U8[readSize];
	U8 *outData = new U8[readSize];
	dStrcpy((char*)inData, "Welcome to the sample data to filter. Please put on your seatbelts, and prepare for takeoff!");

	// Setup states
	state1->dataIn(inData, readSize);
	state1->dataOut(outData, readSize);
	state2->dataIn(outData, readSize);
	state2->dataOut(inData, readSize);

	// Do initial dump
	FileStream fs;
	Con::printf("DBG: processing %d bytes, %d out buffer", state1->dataIn(), state1->dataOut());
	fs.open("initial_dump", FileStream::Write);
	fs.write(readSize, inData);
	fs.close();

	// Keep processing until state1 has finished writing data to out
	while (state1->process()) {;}
	Con::printf("1> DBG:  %d bytes, %d out buffer left", state1->dataIn(), state1->dataOut());
	fs.open("process1_dump", FileStream::Write);
	fs.write(readSize, outData);
	fs.close();

	// For sanity, clear buffer
	dMemset(inData, 0, readSize);

	// Keep processing until state2 has finished writing data to out
	while (state2->process()) {;}
	Con::printf("2> DBG:  %d bytes, %d out buffer left", state2->dataIn(), state2->dataOut());
	fs.open("process2_dump", FileStream::Write);
	fs.write(readSize, inData);
	fs.close();

	// For sanity, clear buffer
	dMemset(outData, 0, readSize);

	// Now run state1 in reverse
	delete state2;
	state2 = FilterState::findHandler(FilterState::COMPRESS_ZLIB)->init(0);
	state2->dataIn(inData, readSize);
	state2->dataOut(outData, readSize);
	while (state2->reverseProcess()) {;}
	Con::printf("3> DBG:  %d bytes, %d out buffer left", state2->dataIn(), state2->dataOut());
	fs.open("process2_orig_dump", FileStream::Write);
	fs.write(readSize, outData);
	fs.close();

	// Cleanup
	delete state1;
	delete state2;
	delete [] inData;
	delete [] outData;
}

ConsoleFunction(testCryptFilterState, void, 1, 1, "Tests filter state code (encryption)")
{
	// Our filter states...
	FilterState *state1 = FilterState::findHandler(FilterState::PROCESS_BASIC)->init(FilterState::FILTER_WRITE);
	FilterState *state2 = FilterState::findHandler(FilterState::ENCRYPT_BLOWFISH)->init(FilterState::ENCRYPT_BLOWFISH);
	CryptHash *myHash = new CryptHash(ResManager::defaultHash);

	myHash->hash("Bobollipi");
	state2->setHash(myHash);

	U32 readSize = state2->chunkSize();
	U8 *inData = new U8[readSize+8];
	U8 *outData = new U8[readSize+8];
	dStrcpy((char*)inData, "Welcome to the sample data to filter. Please put on your seatbelts, and prepare for takeoff!\nKeep in mind, we're going crypto!");

	// Setup states
	state1->dataIn(inData, readSize);
	state1->dataOut(outData, readSize+8);
	state2->dataIn(outData, readSize);
	state2->dataOut(inData, readSize+8);

	// Do initial dump
	FileStream fs;
	Con::printf("DBG: processing %d bytes, %d out buffer", state1->dataIn(), state1->dataOut());
	fs.open("einitial_dump", FileStream::Write);
	fs.write(readSize, inData);
	fs.close();

	// Keep processing until state1 has finished writing data to out
	while (state1->process()) {;}
	Con::printf("1> DBG:  %d bytes, %d out buffer left", state1->dataIn(), state1->dataOut());
	fs.open("eprocess1_dump", FileStream::Write);
	fs.write(readSize, outData);
	fs.close();

	// For sanity, clear buffer (our process data is in outData)
	dMemset(inData, 0, readSize);

	// Keep processing until state2 has finished writing data to out
	while (state2->process()) {;}
	Con::printf("2> DBG:  %d bytes, %d out buffer left", state2->dataIn(), state2->dataOut());
	fs.open("eprocess2_dump", FileStream::Write);
	fs.write(readSize, inData);
	fs.close();

	// For sanity, clear buffer (our process data is in inData)
	dMemset(outData, 0, readSize);

	// Now run state1 in reverse
	delete state2;
	state2 = FilterState::findHandler(FilterState::ENCRYPT_BLOWFISH)->init(FilterState::ENCRYPT_BLOWFISH);
	//myHash->hash("Other Key");
	state2->setHash(myHash);
	state2->dataIn(inData, readSize+8);
	state2->dataOut(outData, readSize);
	while (state2->reverseProcess()) {;}
	Con::printf("3> DBG:  %d bytes, %d out buffer left", state2->dataIn(), state2->dataOut());
	fs.open("eprocess2_orig_dump", FileStream::Write);
	fs.write(readSize, outData);
	fs.close();

	// Cleanup
	delete state1;
	delete state2;
	delete [] inData;
	delete [] outData;
	delete myHash;
}
#endif
