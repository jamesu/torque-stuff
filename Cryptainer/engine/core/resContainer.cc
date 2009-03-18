//-----------------------------------------------------------------------------
// (C) 2004 - 2006, Stuart James Urquhart (jamesu@gmail.com). All Rights Reserved.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "console/console.h"
#include "core/frameAllocator.h"
#include "core/resContainer.h"

// ResourceManager Hook
//------------------------------------------------------------------------------
ResourceInstance* constructContainer(Stream& stream)
{
   ResContainer* pResource = new ResContainer();

   if (pResource->read(stream) == true)
      return pResource;
   else {
      delete pResource;
      return NULL;
   }
}

// DirectoryEntry
//------------------------------------------------------------------------------
void DirectoryEntry::read(Stream &s)
{
	s.readString(mDirname);
	//s.read(sizeof(DirectoryInfo), &mDirectoryInfo);
	// Endian safe read
	s.read(&mDirectoryInfo.compressedSize);
	s.read(&mDirectoryInfo.numFiles);
	s.read(&mDirectoryInfo.flags);

	// Special case : if we have no files, or no size, skip this unneccesary setup
	if (mDirectoryInfo.numFiles == 0 || mDirectoryInfo.compressedSize == 0)
		return;

	// Create temp memory
	U8 *tmpBuffer = new U8[sizeof(FileInfo)*mDirectoryInfo.numFiles];
	ResFilter *filter = ResContainer::getFilter(mDirectoryInfo.flags);

	U32 origOffset = s.getPosition();
	filter->attachStream(&s, false);
	if (mObject->mHash) filter->setHash(mObject->mHash);
	filter->setStreamOffset(origOffset, sizeof(FileInfo)*mDirectoryInfo.numFiles);

	FileInfo *ptr = (FileInfo*)tmpBuffer;
	// Read every FileInfo
	for (U32 i=0;i<mDirectoryInfo.numFiles;i++) {
		//filter->read(sizeof(FileInfo),ptr);
		// Endian Safe read
		filter->read(FILENAME_SIZE, &ptr->name);
		filter->read(&ptr->compressedSize);
		filter->read(&ptr->decompressedSize);
		filter->read(&ptr->fileOffset);
		filter->read(&ptr->flags);
		//Con::printf(">>DirectoryEntry[%s]::read : name == %s, compressedSize == %d (real == %d), offset == %d", mDirname, ptr->name, ptr->compressedSize, ptr->decompressedSize, ptr->fileOffset);
		ptr++;
	}

	// Now we can just directly read the vector in from the buffer
	files.set(tmpBuffer, mDirectoryInfo.numFiles);
	// Also revert the stream position (since the cache reading nature of ResFilter will likely read too much)
	s.setPosition(origOffset + mDirectoryInfo.compressedSize);

	delete filter;
	delete [] tmpBuffer;
}

//------------------------------------------------------------------------------
void DirectoryEntry::write(Stream &s)
{
	U8 *tmpBuffer = NULL;
	MemStream *mem = NULL;
	ResFilter *filter = NULL;

	// Setup mDirectoryInfo...
	mDirectoryInfo.numFiles = files.size();

	// Special case : if we have no files, skip this unneccesary setup
	if (mDirectoryInfo.numFiles != 0) {
		// Create temp memory
		tmpBuffer = new U8[sizeof(FileInfo)*(mDirectoryInfo.numFiles+1)];
		mem = new MemStream(sizeof(FileInfo)*(mDirectoryInfo.numFiles+1), tmpBuffer, true, true);

		// Get the filter to compress...
		filter = ResContainer::getFilter(mDirectoryInfo.flags);
		
		filter->attachStream(mem, true);
		if (mObject->mHash) filter->setHash(mObject->mHash);
		filter->setStreamOffset(0, sizeof(FileInfo) * files.size());

		// Write every FileInfo to the tmpBuffer
		for (DirectoryEntry::iterator itr = begin(); itr != end(); itr++) {
			//filter->write(sizeof(FileInfo),&*itr);
			// Endian Safe write
			const FileInfo *entry = &*itr;
			filter->write(FILENAME_SIZE, entry->name);
			filter->write(entry->compressedSize);
			filter->write(entry->decompressedSize);
			filter->write(entry->fileOffset);
			filter->write(entry->flags);
		}
		delete filter;
	}

	// Write everything to file
	mDirectoryInfo.compressedSize = mem ? mem->getPosition() : 0;
	s.writeString(mDirname);
	//s.write(sizeof(DirectoryInfo), &mDirectoryInfo);
	// Endian Safe write
	s.write(mDirectoryInfo.compressedSize);
	s.write(mDirectoryInfo.numFiles);
	s.write(mDirectoryInfo.flags);
	if (tmpBuffer) {
		s.write(mDirectoryInfo.compressedSize,tmpBuffer);
		delete mem;
		delete [] tmpBuffer;
	}

	//Con::printf(">>DirectoryEntry::write: dirName == %s, numFiles == %d, compressedSize == %d (real == %d)", mDirname, mDirectoryInfo.numFiles, mDirectoryInfo.compressedSize, sizeof(FileInfo)*mDirectoryInfo.numFiles);
}

//------------------------------------------------------------------------------
DirectoryEntry::DirectoryEntry(ResContainer *obj)
{
	mObject = obj;
	mDirectoryInfo.compressedSize = 0;
	mDirectoryInfo.numFiles = 0;
	mDirectoryInfo.flags = 0;
	mFullPath = NULL;
}

//------------------------------------------------------------------------------
DirectoryEntry::DirectoryEntry(ResContainer *obj, const char *name, U32 flags)
{
	mObject = obj;
	mDirectoryInfo.compressedSize = 0;
	mDirectoryInfo.numFiles = 0;
	mDirectoryInfo.flags = flags;
	mFullPath = NULL;
	dStrncpy(mDirname, name, DIRECTORY_SIZE);
	mDirname[DIRECTORY_SIZE-1] = '\0';
	VECTOR_SET_ASSOCIATION(directorys);
}

//------------------------------------------------------------------------------
void DirectoryEntry::addFileEntry(const char *name, U32 compressedSize, U32 decompressedSize, U32 fileOffset, U32 flags)
{
	files.increment();
	FileInfo *info = &files.last();
	dStrncpy(info->name, name, FILENAME_SIZE);
	info->name[FILENAME_SIZE-1] = '\0'; // Make sure its terminated
	info->compressedSize = compressedSize;
	info->decompressedSize = decompressedSize;
	info->fileOffset = fileOffset;
	info->flags = flags;

	//Con::printf(">^DirectoryEntry::addFileEntry : name == %s, compressedSize == %d (real == %d), offset == %d", name, compressedSize, decompressedSize, fileOffset);
}

//------------------------------------------------------------------------------
bool DirectoryEntry::delFileEntry(const char *name)
{
	DirectoryEntry::iterator itr = findFileEntry(name);
	if (itr)
		files.erase((Vector<FileInfo>::iterator)itr);
	return itr ? true : false;
}

//------------------------------------------------------------------------------
DirectoryEntry::iterator DirectoryEntry::findFileEntry(const char *name)
{
	for (DirectoryEntry::iterator file = begin(); file != end(); file++) {
		if (!dStricmp(name, file->name))
			return file;
	}
	return NULL;
}

//------------------------------------------------------------------------------
void DirectoryEntry::setFlags(const char *name, U32 flags)
{
	Vector<FileInfo>::iterator file = (Vector<FileInfo>::iterator)findFileEntry(name);
	if (file)
		file->flags = flags &~ FilterState::ENCRYPT_ALL;
}

//------------------------------------------------------------------------------
U32 DirectoryEntry::getFlags(const char *name)
{
	DirectoryEntry::iterator file = findFileEntry(name);
	if (file)
		return file->flags;
	return (U32)-1;
}


// ResContainer
//------------------------------------------------------------------------------
ResContainer::ResContainer()
{
	cStream=NULL;
	mHash = NULL;
	mEnableWrite = false;
	mDirectoryOffset = sizeof(U32)*2;
	VECTOR_SET_ASSOCIATION(files);
}

//------------------------------------------------------------------------------
bool ResContainer::read(Stream &s)
{
	// Header...
	U32 num;
	s.setPosition(0);
	s.read(&num);
	if (num != 0x44434f4e) // "NOCD"
		return false;
	s.read(&mDirectoryOffset);

	// Directory's
	U16 numDirs;
	s.setPosition(mDirectoryOffset);
	s.read(&numDirs); directorys.setSize(numDirs);

	//Con::printf(">>ResContainer::read : dirOffset == %d, dirs = %d", mDirectoryOffset, numDirs);

	for (Vector<DirectoryEntry*>::iterator itr = directorys.begin(); itr != directorys.end(); itr++) {
		DirectoryEntry *entry = new DirectoryEntry(this);
		*itr = entry;
		entry->read(s);
	}

	return true;
}

//------------------------------------------------------------------------------
bool ResContainer::write(Stream &s)
{
	// Header...
	U32 num;
	num = 0x44434f4e; // "NOCD"
	s.setPosition(0);
	s.write(num);
	s.write(mDirectoryOffset);

	//Con::printf(">>ResContainer::write : dirOffset == %d", mDirectoryOffset);

	// Directory's...
	U16 numDirs;
	s.setPosition(mDirectoryOffset);
	numDirs = directorys.size(); s.write(numDirs);
	for (Vector<DirectoryEntry*>::iterator itr = directorys.begin(); itr != directorys.end(); itr++) {
		DirectoryEntry *entry = *itr;
		entry->write(s);
	}

	return true;
}

//------------------------------------------------------------------------------
bool ResContainer::open(bool enableWrite)
{
	// Close existing stream (if any), and load a new one with ResourceManager
	AssertFatal(this->mSourceResource, "ResContainer::open : No Resource!");
	if (cStream) {
		// We assume we have opened the container before, and therefor still have the file info present
		// Don't close existing stream unless neccesary
		if (enableWrite && (!cStream->hasCapability(Stream::StreamWrite))) {
			ResourceManager->closeStream(cStream);
			cStream = ResourceManager->openResourceForWrite(this->mSourceResource, FileStream::ReadWrite);
		}
		mEnableWrite = enableWrite;
		return true;
	}
	mEnableWrite = enableWrite;
	cStream = enableWrite ? ResourceManager->openResourceForWrite(this->mSourceResource, FileStream::ReadWrite) : ResourceManager->openStream(this->mSourceResource);

	// Re-load container headers if neccesary
	if (cStream && directorys.size() == 0)
		read(*cStream);
	return cStream != NULL;
}

//------------------------------------------------------------------------------
bool ResContainer::openExisting(Stream *s, bool enableWrite)
{
	// Close existing stream (if any), and load a new one with ResourceManager
	/*if (cStream) {
		// We assume we have opened the container before, and therefor still have the file info present
		// Don't close existing stream unless neccesary
		ResourceManager->closeStream(cStream);
	}*/
	cStream = s;
	mEnableWrite = enableWrite;

	// Re-load container headers if neccesary
	if (cStream && directorys.size() == 0)
		read(*cStream);
	return cStream != NULL;
}

//------------------------------------------------------------------------------
bool ResContainer::initNew(Stream *s)
{
	cStream = s;
	mEnableWrite = true;

	if (cStream)
		write(*cStream);

	// Re-load container headers if neccesary
	return cStream != NULL;
}

//------------------------------------------------------------------------------
bool ResContainer::close()
{
	// Finish with main stream...
	if (cStream)
	{
		if (mEnableWrite)
			write(*cStream);
		ResourceManager->closeStream(cStream);
		cStream = NULL;
	}
	// Clear internal data
	for (Vector<DirectoryEntry*>::iterator ptr = directorys.begin();ptr != directorys.end();ptr++)
		if (*ptr) delete *ptr;
	directorys.setSize(0);
	directorys.compact(); // Compact will free any memory used by our container object (if we want to use it again)

	return true;
};

//------------------------------------------------------------------------------
ResFilter *ResContainer::getFilter(U32 flags)
{
	ResFilter *strm = new ResFilter(flags);
	AssertFatal(strm != NULL, "ResContainer::getFilter : Could not get filter!");
	return strm;
}

//------------------------------------------------------------------------------
DirectoryEntry::iterator ResContainer::getFile(ResourceObject *obj)
{
	// filePath size should be higher than zipPath size, since it includes that
	// NOTE: it should also exclude '/' at the start (+1)).
	const char *filePath = obj->path;
	if (obj->zipPath != NULL) filePath = filePath + dStrlen(obj->zipPath);
	if (*filePath == '/') filePath++;
	const char *fileName = obj->name;

	return getFile(filePath, fileName);
}

//------------------------------------------------------------------------------
DirectoryEntry::iterator ResContainer::getFile(const char *path, const char *name)
{
	for (Vector<DirectoryEntry*>::iterator itr = directorys.begin();itr != directorys.end();itr++)
	{
		DirectoryEntry *entry = *itr;
		// Do we have the correct directory?
		if (!dStricmp(entry->getName(), path))
		{
			// Yes, so look at files
			DirectoryEntry::iterator file = entry->findFileEntry(name);
			if (file) return file;
		}
	}
	return NULL;
}

//------------------------------------------------------------------------------
DirectoryEntry::iterator ResContainer::getFile(const char *filename)
{
	const char *name = dStrrchr(filename, '/');
	if (!name) name = filename;
	else name++;

	// Copy path into another buffer
	U32 mark = FrameAllocator::getWaterMark ();
	U32 len = name - filename;
	char *buffer = "";
	if (len != 0) {
		buffer = (char *) FrameAllocator::alloc(len);
		dStrncpy(buffer, filename, len);
		buffer[len-1] = '\0';
	}

	DirectoryEntry::iterator itr = getFile(buffer, name);
	FrameAllocator::setWaterMark(mark);
	return itr;
}

//------------------------------------------------------------------------------
ResFilter *ResContainer::getFileStream(ResourceObject *obj)
{
	DirectoryEntry::iterator file = getFile(obj);
	if (file) {
		// Firstly, check if the file has crypto enabled, and if we have a hash assigned.
		// e.g. if we have encryption enabled, but we have no hash, the crypto will very likely fail!
		if ((file->flags & FilterState::ENCRYPT_ALL) && (mHash == NULL)) return NULL;
		
		// We have the file, so make a stream instance
		Stream *strm = ResourceManager->openStream(this->mSourceResource);
		// And attach the filter...
		ResFilter *filter = getFilter(file->flags);
		
		filter->attachStream(strm, false);
		if (mHash) filter->setHash(mHash);
		filter->setStreamOffset(file->fileOffset, file->decompressedSize);
		return filter;
	}

	return NULL;
}

//------------------------------------------------------------------------------
void ResContainer::addDirectory(const char *name, U32 flags)
{
	DirectoryEntry *entry = new DirectoryEntry(this, name, flags &~ FilterState::ENCRYPT_ALL);
	directorys.push_back(entry);
}

//------------------------------------------------------------------------------
bool ResContainer::delDirectory(const char *name)
{
	for (Vector<DirectoryEntry*>::iterator itr = directorys.begin(); itr != directorys.end(); itr++) {
		DirectoryEntry *entry = *itr;
		if (!dStrcmp(entry->getName(),name)) {
			delete *itr;
			directorys.erase(itr);
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------
DirectoryEntry *ResContainer::getDirectory(const char *filename)
{
	const char *ptr = dStrrchr(filename, '/');

	U32 pathLen = 0;
	bool success = false;
	if (ptr == NULL)
		pathLen = 0;
	else
		pathLen = ptr - filename;

	// Find according directory
	for (Vector<DirectoryEntry*>::iterator itr = directorys.begin();itr != directorys.end();itr++)
	{
		DirectoryEntry *entry = *itr;
		if (!dStrncmp(filename, entry->getName(), pathLen)) {
			return entry;
		}
	}
	return NULL;
}

//------------------------------------------------------------------------------
bool ResContainer::addFile(const char *name, U8 *ptr, U32 size, U32 flags)
{
	if (getFile(name)) delFile(name); // Delete any existing file
	
	const char *fileName = dStrrchr(name, '/');
	char filePath[FILENAME_SIZE];
	bool success = false;
	if (fileName == NULL) {
		filePath[0] = '\0'; // Root path, so terminate path string
		fileName = name;
	}
	else {
		dStrncpy(filePath, name, (fileName - name + 1));
		filePath[fileName - name] = '\0';
		fileName++; // Miss off first '/'
	}

	// Go to end of file data (mDirectoryOffset), and start writing...
	ResFilter *filter = getFilter(flags);
	
	if (!filter->attachStream(cStream, true)) {
		delete filter;
		return false;
	}
	if (mHash) filter->setHash(mHash);
	filter->setStreamOffset(mDirectoryOffset, size*2); // *2 to account for expansion
	filter->write(size, ptr);
	delete filter;

	// Append file to appropriate directory
	for (Vector<DirectoryEntry*>::iterator itr = directorys.begin();itr != directorys.end();itr++)
	{
		DirectoryEntry *entry = *itr;
		if (!dStrcmp(filePath, entry->getName())) {
			entry->addFileEntry(fileName, cStream->getPosition() - mDirectoryOffset, size, mDirectoryOffset, flags);
			success = true;
			break;
		}
	}

	if (!success) {
		// Directory doesn't exist!
		addDirectory(filePath, flags);
		directorys[directorys.size()-1]->addFileEntry(fileName, cStream->getPosition() - mDirectoryOffset, size, mDirectoryOffset, flags);
	}

	mDirectoryOffset = cStream->getPosition();
	return true;
}

bool ResContainer::addFilteredFile(const char *name, U8 *ptr, U32 compressedSize, U32 size, U32 flags)
{
	//Con::warnf("addFilteredFile(%s, %d, %d, %d, %d)", name, ptr, compressedSize, size, flags);
	if (getFile(name)) delFile(name); // Delete any existing file
	
	const char *fileName = dStrrchr(name, '/');
	char filePath[FILENAME_SIZE];
	bool success = false;
	if (fileName == NULL) {
		filePath[0] = '\0'; // Root path, so terminate path string
		fileName = name;
	}
	else {
		dStrncpy(filePath, name, (fileName - name + 1));
		filePath[fileName - name] = '\0';
		fileName++; // Miss off first '/'
	}

	cStream->setPosition(mDirectoryOffset);
	cStream->write(compressedSize, ptr);
	
	// Append file to appropriate directory
	for (Vector<DirectoryEntry*>::iterator itr = directorys.begin();itr != directorys.end();itr++)
	{
		DirectoryEntry *entry = *itr;
		if (!dStrcmp(filePath, entry->getName())) {
			entry->addFileEntry(fileName, compressedSize, size, mDirectoryOffset, flags);
			success = true;
			break;
		}
	}
	
	if (!success) {
		// Directory doesn't exist!
		addDirectory(filePath, flags);
		directorys[directorys.size()-1]->addFileEntry(fileName, compressedSize, size, mDirectoryOffset, flags);
	}

	mDirectoryOffset = cStream->getPosition();
	return success;
}

//------------------------------------------------------------------------------
bool ResContainer::delFile(const char *name)
{
	const char *fileName = dStrrchr(name, '/');
	char filePath[FILENAME_SIZE];
	U32 targetStart, targetEnd = 0;

	if (fileName == NULL) {
		fileName = name;
		filePath[0] = '\0'; // Root path, so terminate path string
	}
	else {
		dStrncpy(filePath, name, (fileName - name));
		filePath[(fileName - name)] = '\0';
        fileName++;
	}

	// Find file entry...
	Vector<DirectoryEntry*>::iterator itr;
	for (itr = directorys.begin();itr != directorys.end();itr++)
	{
		DirectoryEntry *entry = *itr;
		if (!dStrcmp(filePath, entry->getName()))
		{
			DirectoryEntry::iterator myFileEntry = entry->findFileEntry(fileName);
			if (!myFileEntry)
				continue;

			targetStart = myFileEntry->fileOffset;
			targetEnd = myFileEntry->fileOffset + myFileEntry->compressedSize;

			// Move file data from targetEnd+ to targetStart
			U8 myBuff[CHUNK_PROCSIZE];
			U32 dataLeft = cStream->getStreamSize() - targetEnd;
			targetEnd = targetStart;

			while (dataLeft)
			{
				U32 toRead = dataLeft > CHUNK_PROCSIZE ? CHUNK_PROCSIZE : dataLeft;
				
				cStream->setPosition(cStream->getStreamSize() - dataLeft);
				cStream->read(toRead, myBuff);

				cStream->setPosition(targetEnd);
				cStream->write(toRead, myBuff);

				targetEnd += toRead;
				dataLeft -= toRead;
			}

			targetStart = myFileEntry->compressedSize;
			targetEnd = myFileEntry->fileOffset + myFileEntry->compressedSize;

			if (!entry->delFileEntry(fileName))
				return false;

			break;
		}
	}

	if (itr == directorys.end())
	{
		return false;
	}

	// Final pass, move file offsets
	for (Vector<DirectoryEntry*>::iterator itr = directorys.begin();itr != directorys.end();itr++)
	{
		DirectoryEntry *entry = *itr;
		DirectoryEntry::iterator myFileEntry;
		for (myFileEntry = entry->begin(); myFileEntry != entry->end(); myFileEntry++)
		{
			if (myFileEntry->fileOffset > targetEnd)
			{
				myFileEntry->fileOffset -= targetStart;
			}
		}
	}
	return true;
}


//------------------------------------------------------------------------------
void ResContainer::setFullPath(const char *path)
{
	U32 pathLen = dStrlen(path);
	
	// Loop through directory's...
	for (Vector<DirectoryEntry*>::iterator itr = directorys.begin();itr != directorys.end();itr++)
	{
		DirectoryEntry *entry = *itr;
		const char *dirName = entry->getName();
		if (dirName[0] != '\0') {// Not root dir
			const char *buf = ResManager::buildPath(path, dirName);
			entry->setFullPath(StringTable->insert(buf));
		}
		else
			entry->setFullPath(StringTable->insert(path));
	}
}

//------------------------------------------------------------------------------
void ResContainer::setHash(CryptHash *hash)
{
	mHash = hash;
}
