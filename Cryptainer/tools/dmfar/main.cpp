//-----------------------------------------------------------------------------
// (C) 2004 - 2006, Stuart James Urquhart (jamesu@gmail.com). All Rights Reserved.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "platform/event.h"
#include "platform/platformAssert.h"
#include "platform/platformVideo.h"
#include "math/mMath.h"
#include "console/console.h"
#include "core/tVector.h"
#include "core/fileStream.h"
#include "console/consoleTypes.h"
#include "math/mathTypes.h"
#include "interior/interior.h"
#include "interior/floorPlanRes.h"
#include "dmfar/dmfArGame.h"
#include "core/frameAllocator.h"
#include "gui/core/guiCanvas.h"
#include "core/resManager.h"
#include "core/resContainer.h"
#include "core/resFilter.h"

#ifdef TORQUE_DEBUG
#include <stdio.h>
#endif

DMFArGame GameObject;
extern ResourceInstance *constructContainer(Stream &);

// FOR SILLY LINK DEPENDANCY
bool gEditingMission = false;

#if defined(TORQUE_DEBUG)
const char* const gProgramVersion = "1.0d";
#else
const char* const gProgramVersion = "1.0r";
#endif

//
static bool initLibraries()
{
   // asserts should be created FIRST
   //PlatformAssert::create();
   FrameAllocator::init(2 << 20);

   _StringTable::create();

   ResManager::create();

   // Register known file types here
   ResourceManager->registerExtension(".dmf", constructContainer);

   //RegisterCoreTypes();
   //RegisterMathTypes();

   Con::init();

   Math::init();
   Platform::init();    // platform specific initialization

   // Create a log file
   Con::setLogMode(6);

   return(true);
}

static void shutdownLibraries()
{
   // shut down
   Platform::shutdown();
   Con::shutdown();

   _StringTable::destroy();

   // asserts should be destroyed LAST
   FrameAllocator::destroy();
   PlatformAssert::destroy();
}

void cleanSlashes(char* path)
{
   // Clean up path char.
   for (char* ptr = path; *ptr != '\0'; ptr++)
      if (*ptr == '\\')
         *ptr = '/';
}

void terminate(char* path)
{
   // Check termination
   char* end = &path[dStrlen(path) - 1];
   if (*end != '/')
   {
      end[1] = '/';
      end[2] = 0;      
   }
}

char* cleanPath(const char* _path)
{
   char* path = new char[dStrlen(_path) + 2];
   dStrcpy(path, _path);

   cleanSlashes(path);

   terminate(path);

   return path;
}   

char* getPath(const char* file)
{
   char* path = "\0";

   if (!dStrchr(file, '/') && !dStrchr(file, '\\'))
      return path;
   else
   {
      path = new char[dStrlen(file) + 2];
      dStrcpy(path, file);
   }

   // Strip back to first path char.
   char* slash = dStrrchr(path, '/');
   if (!slash)
      slash = dStrrchr(path, '\\');
   if (slash)
      *slash = 0;

   cleanSlashes(path);

   terminate(path);

   return path;
}

char* getBaseName(const char* file)
{
   // Get rid of path
   const char* slash = dStrrchr(file, '/');
   if (!slash)
      slash = dStrrchr(file, '\\');
   if (!slash)
      slash = file;
   else
      slash++;
   char* name = new char[dStrlen(slash) + 1];
   dStrcpy(name, slash);

   // Strip extension & trailing _N
   char* dot = dStrrchr(name, '.') - 2;
   if (dot[0] == '_' && (dot[1] >= '0' && dot[1] <= '9'))
      dot[0] = '\0';
   else
      dot[2] = '\0';

   return name;
}

CryptHash *getCryptParams(const char *method, const char *keyfile, const char *hashfile)
{
	if (!keyfile && !hashfile) {
		if (method)
			dPrintf("Warning: No hash or key file specified for '%s', encryption will be disabled!\n", method);
		return NULL;
	}

	CryptHash *hash = new CryptHash(ResManager::defaultHash);
	FileStream fs;

	if (!fs.open(keyfile ? keyfile : hashfile, FileStream::Read))
	{
		delete hash;
		dPrintf("Warning: could not open hash or key file, encryption will be disabled!\n");
		return NULL;
	}

	if (keyfile)
	{
		// Compute hash of key stored in keyfile
		char data[1024];
		U32 size;
		size = fs.getStreamSize();
		if (size > 1024) size = 1024;
		fs.read(size, &data);
		data[size == 1024 ? size-1 : size] = '\0';
		if (!hash->hash(data))
		{
			delete hash;
			hash = NULL;
			dPrintf("Warning: could not hash key from '%s', encryption will be disabled!\n", keyfile);
		}
		else if (hashfile)
		{
			// Write hash'd key to specified hash file
			fs.close();
			if (fs.open(hashfile, FileStream::Write))
				fs.write(hash->getSize(), hash->getData());
		}
	}
	else if (hashfile)
	{
		// Read directly into CryptHash
		U32 size = hash->getSize();
		U8 *data = new U8[size];
		fs.read(size, data);
		hash->setData(data);
		delete [] data;
	}

	fs.close();
	return hash;
}

U32 getFilterParams(const char *process, const char *crypt)
{
	U32 res;
	if (process)
		res = FilterState::fromString(process, true);
	else
		res = FilterState::PROCESS_BASIC;

	if (crypt)
		res |= FilterState::fromString(crypt, true);

	return res;
}

bool touchPath(const char *cwd, const char *dir)
{
	char buffer[4096];
	
	dSprintf(buffer, 4096, "%s/%s/", cwd, dir);
	return Platform::createPath(buffer);
}

typedef enum {
	DMF_DISPLAYHELP,
	DMF_LISTFILES,
	DMF_EXTRACTFILES,
	DMF_ADDFILES,
	DMF_BAD,
} DMFMode;

S32 DMFArGame::main(int argc, const char** argv)
{
   // Set the memory manager page size to 64 megs...
   setMinimumAllocUnit(64 << 20);

   int success = 0;

   if(!initLibraries())
      return 0;

   // Set up the command line args for the console scripts...
   Con::setIntVariable("Game::argc", argc);
   for (S32 i = 0; i < argc; i++)
      Con::setVariable(avar("Game::argv%d", i), argv[i]);

   const char *gProcessMethod = "basic";
   const char *gCryptMethod = NULL;
   const char *gCryptKeyFile = NULL;
   const char *gCryptHashFile = NULL;
   const char *gWorkingDirectory = "./";
     
   bool gVerbose = false;
   bool gModeAppend = false;
   DMFMode gMode = DMF_DISPLAYHELP;

   dPrintf("\ndmfar - Torque .DMF archiver\n"
              "  Copyright (C) James Urquhart.\n"
              "  Program version: %s\n"
			  "  Programmers: James Urquhart\n"
              "  Built: %s at %s\n\n", gProgramVersion, __DATE__, __TIME__);

   // Parse command line args...
   S32 i = 1;
   for (; i < argc; i++) {
      if (argv[i][0] != '-')
         break;
      switch(dToupper(argv[i][1])) {
         case 'E':
            gMode = DMF_EXTRACTFILES;
            break;
         case 'L':
            gMode = DMF_LISTFILES;
            break;
         case 'A':
            gMode = DMF_ADDFILES;
            break;
         case 'R':
            gMode = DMF_ADDFILES;
			gModeAppend = true;
            break;
         case 'F':
            gProcessMethod = argv[++i];
            break;
         case 'C':
            gCryptMethod = argv[++i];
            break;
         case 'K':
            gCryptKeyFile = argv[++i];
            break;
         case 'H':
            gCryptHashFile = argv[++i];
            break;
         case 'W':
            gWorkingDirectory = argv[++i];
           break;
         case 'V':
            gVerbose = true;
            break;
      }
   }
   U32 args = argc - i;
   if (gMode == DMF_DISPLAYHELP || (args < 1 || gMode == DMF_BAD) ) {
      dPrintf("Usage: dmfar [-lear] [-f <filter>] [-c <crypt name>] [-k <key file>] [-h <hash file>] [-w <directory>] <file>.dmf\n"
			  "        -e : extract files from archive\n"
			  "        -l : list files in archive\n"
			  "        -a : append files to archive\n"
			  "        -r : overwrite files in archive\n"
			  "        -f : name of the filter used to compress new files & new directories\n"
			  "        -c : encryption method (default is none)\n"
			  "        -k : file in which encryption key is stored\n"
			  "        -h : file in which encryption hash is stored\n"
			  "        -w : directory in which files are extracted or archived\n"
			  "        -v : verbose output\n"
			  "<file>.dmf : container file\n\n");
      
	  // Print more options here
	  FilterState::printHandlers();

	  shutdownLibraries();
	  return gMode == DMF_DISPLAYHELP ? 0 : 1;
   }

   if (gMode == DMF_LISTFILES)
   {
		// List all files in the container
		const char *archive = argv[i++];
		FileStream fs;

		if (fs.open(archive, FileStream::Read))
		{
			ResContainer *inst = new ResContainer();
			CryptHash *myHash = getCryptParams(gCryptMethod, gCryptKeyFile, gCryptHashFile);
			if (myHash)
				inst->setHash(myHash);

			if (inst->read(fs))
			{
				for (ResContainer::iterator ditr = inst->begin(); ditr != inst->end(); ditr++)
				{
					DirectoryEntry *ent = *ditr;
					dPrintf("/%s:\n", ent->getName());
					for (DirectoryEntry::iterator fitr = ent->begin(); fitr != ent->end(); fitr++)
					{
						dPrintf("\t%s\n", fitr->name);
					}
				}
			}

			if (myHash)
				delete myHash;
			delete inst;
		}
		else {
			dPrintf("Error: could not open archive %s.", archive);
			success = 1;
		}

   }
   else if (gMode == DMF_EXTRACTFILES)
   {
	   // Extract all files from the container
	   const char *archive = argv[i++];
	   char buffer[2048];
	   FileStream fs;

		// First step, ensure working directory exists
		if (!Platform::isDirectory(gWorkingDirectory))
		{
			dSprintf(buffer, 2048, "%s/", gWorkingDirectory);
			if (!Platform::createPath(buffer))
			{
				dPrintf("Error: directory '%s' does not exist, or could not be created!\n", buffer);
				shutdownLibraries();
				return 1;
			}
		}

	   if (fs.open(archive, FileStream::Read))
	   {
		   ResContainer *inst = new ResContainer();
		   CryptHash *myHash = getCryptParams(gCryptMethod, gCryptKeyFile, gCryptHashFile);
		   if (myHash)
			   inst->setHash(myHash);

		   if (inst->read(fs))
		   {
			   //inst->setFullPath(gWorkingDirectory);
			   for (ResContainer::iterator ditr = inst->begin(); ditr != inst->end(); ditr++)
			   {
				   DirectoryEntry *ent = *ditr;
				   if (gVerbose) dPrintf("Extracting /%s...\n", ent->getName());

				   if (!touchPath(gWorkingDirectory, ent->getName()))
				   {
					   dPrintf("Error: directory could not be created.\n");
					   continue;
				   }

				   for (DirectoryEntry::iterator fitr = ent->begin(); fitr != ent->end(); fitr++)
				   {
					   if (gVerbose) dPrintf("\t%s...", fitr->name);
					   FileStream out;
					   if (*ent->getName() == '\0') // root
					       dSprintf(buffer, 1024, "%s", gWorkingDirectory);
					   else
						   dSprintf(buffer, 1024, "%s/%s", gWorkingDirectory, ent->getName());

					   if (out.open(ResManager::buildPath(buffer, fitr->name), FileStream::Write))
					   {
						   if ((fitr->flags & FilterState::ENCRYPT_ALL) && (myHash == NULL))
						   {
							   if (gVerbose)
								   dPrintf("\tFAILED (Encrypted)\n");
							   continue;
						   }

						   ResFilter *filter = new ResFilter(fitr->flags);
						   filter->attachStream(&fs, false);
						   filter->setHash(myHash);
						   filter->setStreamOffset(fitr->fileOffset, fitr->decompressedSize);

						   U8 *obuf = new U8[fitr->decompressedSize];
						   filter->read(fitr->decompressedSize, obuf);
						   out.write(fitr->decompressedSize, obuf);

						   delete filter;
						   delete [] obuf;

						   if (gVerbose) dPrintf("\tOK\n");
						   out.close();
					   }
					   else if (gVerbose)
					   {
						   dPrintf("\tFAILED (IO Error)\n");
					   }
				   }
			   }
		   }

		   if (myHash)
			   delete myHash;
		   delete inst;
	   }
   }
   else if (gMode == DMF_ADDFILES)
   {
		const char *archive = argv[i++];
		char buffer[2048];
		ResContainer *inst;
		FileStream fs;

		// First step, ensure working directory exists
		if (!Platform::isDirectory(gWorkingDirectory))
		{
			dPrintf("Error: directory '%s' does not exist!\n", gWorkingDirectory);
			shutdownLibraries();
			return 1;
		}

		// Next step, open the file
		inst = new ResContainer();

		if (gModeAppend)
		{
			if (!fs.open(archive, FileStream::ReadWrite) && !inst->read(fs))
			{
				dPrintf("Error: could not open input file '%s' for append operation!\n", archive);
				delete inst;
				shutdownLibraries();
				return 1;
			}
		}
		else
		{
			if (!fs.open(archive, FileStream::Write))
			{
				dPrintf("Error: could not open input file '%s'!\n", archive);
				delete inst;
				shutdownLibraries();
				return 1;
			}
		}

		// Set up container & co

		CryptHash *myHash = getCryptParams(gCryptMethod, gCryptKeyFile, gCryptHashFile);
		U32 tag = getFilterParams(gProcessMethod, myHash ? gCryptMethod : NULL);
		
		if (myHash)
			inst->setHash(myHash);
		if (gModeAppend) {
			if (!inst->openExisting(&fs, true))
			{
				dPrintf("Error: invalid container file '%s'!\n", archive);
				delete inst;
				if (myHash)
					delete myHash;
				shutdownLibraries();
				return 1;
			}
			inst->setFullPath(gWorkingDirectory);
		}
		else
			inst->initNew(&fs);

		// Now scan for files and add them!

		Vector < Platform::FileInfo > fileInfoVec;
		Platform::dumpPath (gWorkingDirectory, fileInfoVec);
		FileStream in;
		U32 wdname = dStrlen(gWorkingDirectory);
		for (U32 i = 0; i < fileInfoVec.size (); i++)
		{
			Platform::FileInfo & rInfo = fileInfoVec[i];

			const char *ext = dStrrchr(rInfo.pFileName, '.');
			if (!ext)
				continue;
			
			dSprintf(buffer, 2048, "%s/%s", rInfo.pFullPath, rInfo.pFileName);
			if (in.open(buffer, FileStream::Read))
			{
				// TODO: cut off cwd?
				U8 *data;
				U32 datasize = in.getStreamSize();

				data = new U8[datasize];
				in.read(datasize, data);
				in.close();

				// Strip off working directory from path
				const char *realPath = rInfo.pFullPath + wdname;
				if (*realPath == '\0')
					dSprintf(buffer, 2048, "%s", rInfo.pFileName);
				else {
					if (*realPath == '/') realPath++;
					dSprintf(buffer, 2048, "%s/%s", realPath, rInfo.pFileName);
				}

				inst->addFile(buffer, data, datasize, tag ^ FilterState::FILTER_WRITE);
				if (gVerbose) dPrintf("%s\n", buffer);
				delete [] data;
			}
		}

	   inst->write(fs);
	   inst->openExisting(NULL, false);
	   fs.close();
	   delete inst;
	   if (myHash)
	      delete myHash;
   }

#ifdef TORQUE_DEBUG
   dPrintf("\nDone, press any key to exit.\n");
   getchar();
#endif

   shutdownLibraries();
   return success;
}

void GameReactivate()
{

}

void GameDeactivate( bool )
{

}
