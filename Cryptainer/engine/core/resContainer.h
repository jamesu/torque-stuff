#ifndef _DCONTAINER_H_
#define _DCONTAINER_H_

//-----------------------------------------------------------------------------
// (C) 2004 - 2006, Stuart James Urquhart (jamesu@gmail.com). All Rights Reserved.
//-----------------------------------------------------------------------------


#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

#ifndef _RESMANAGER_H_
#include "core/resManager.h"
#endif

#ifndef _FILTERSTREAM_H_
#include "core/filterStream.h"
#endif

#ifndef _MEMSTREAM_H_
#include "core/memstream.h"
#endif

#ifndef _RESFILTER_H_
#include "core/resFilter.h"
#endif

#define DIRECTORY_SIZE 128
#define FILENAME_SIZE 128
#define CHUNK_PROCSIZE 4096  // How big the dummy buffer for file deletion is

/// DirectoryEntry
///
/// This stores lists of FileInfo, along with a path (relative to the ResContainer root).
///
/// The FileInfo list can be Processed, and/or Encrypted like file data in ResContainer, which is advantageous when one does not want a user to easily see information about files in the directory.
///
/// @note The directory name is NOT passed though ResFilter.
class DirectoryEntry
{
/// @name File and Directory structures
/// @{
public:
	/// FileInfo
	///
	/// This stores attributes about a file. This includes a static name buffer, the location of the file, and the flags that tell us how the file data is compressed.
	typedef struct FileInfo
	{
		char name[FILENAME_SIZE];	///< Filename of file (excluding path)
		U32 compressedSize;			///< Size of file when compressed
		U32 decompressedSize;		///< Size of file before compression
		U32 fileOffset;				///< Offset of start of data for file in the Container
		U32 flags;					///< ResFilter flags for file data
	};
protected:
	/// DirectoryInfo
	///
	/// This is a stripped down FileInfo struct that tells us how the list of FileInfo struct's is compressed (for ResFilter)
	/// @note It it recommended that you compresses the FileInfo data
	typedef struct DirectoryInfo
	{
		U32 compressedSize;			///< Size of directory info when compressed
		U32 numFiles;				///< Number of files in directory
		U32 flags;					///< ResFilter flags for directory info data
	};
	DirectoryInfo mDirectoryInfo;	///< Data for our directory, packed in for easy read/write
/// @}
private:
	/// @name Internal data
	/// @{
	Vector<FileInfo> files;			///< Vector of FileInfo - Simple, and to the point
	char mDirname[DIRECTORY_SIZE];	///< name of directory
	StringTableEntry mFullPath;		///< full path to the directory from container root
	ResContainer *mObject;			///< Container object
	/// @}
public:
	/// @name Useful File iterators & tools
	/// @{
	const char *getName()                            { return mDirname;}
	U32 numFiles() const                             { return files.size(); }

	typedef Vector<FileInfo>::iterator iterator;
	const FileInfo& operator[](const U32 idx) const  { return files[idx]; }
	iterator begin() const                           { return (iterator)files.begin(); }
	iterator end() const                             { return (iterator)files.end(); }
	/// @}

	/// @name White flags
	/// @{
	void setFlags(U32 flags) {mDirectoryInfo.flags = flags;}
	void setFlags(const char *name, U32 flags);
	U32 getFlags()          {return mDirectoryInfo.flags;}
	U32 getFlags(const char *name);
	/// @}

	/// @name For ResContainer
	/// @{
	void setFullPath(const char *entry) {mFullPath = entry;}
	const char *getFullPath() {return mFullPath;}
	/// @}

	/// @name Stream I/O
	/// @{
	void read(Stream &s);	///< Reads DirectoryEntry from Stream
	void write(Stream &s);	///< Writes DirectoryEntry to Stream
	/// @}

	/// @name Management of file records in directory
	/// @{
	void addFileEntry(const char *name, U32 compressedSize, U32 decompressedSize, U32 fileOffset, U32 flags);
	bool delFileEntry(const char *name);
	iterator findFileEntry(const char *name);
	/// @}

	/// @name Constructors, Destructor
	/// @{
	DirectoryEntry(ResContainer *obj);
	DirectoryEntry(ResContainer *obj, const char *name, U32 flags);
	/// @}
};

/// ResContainer
///
/// This is a generic container interface that handles storing files in other files (e.g. zip, tar).
///
/// The file format used in a custom designed one, taking into account simplicity, and security. File data, as well as directory data can be compressed and encrypted.
///
/// For encryption, the container uses a specific hash assigned by the ResourceManager upon load, or via the script functions setContainerKey()/setContainerHash(). All data in a container shares the same crypt key.
///
/// A helper script object, ContainerHelper is provided to quickly generate container files in torque; However, the container system has not specifically been designed for realtime use at runtime(e.g. deleting existing files requires moving file data to avoid fragmentation, which is a potentially expensive operation). Though it should suffice for thing such as savegame storage.
class ResContainer : public ResourceInstance
{
	typedef ResourceObject Parent;
protected:
	friend class DirectoryEntry;
	/// @name Internal data
	/// @{
	Vector<DirectoryEntry*> directorys;	///< List of Directory's which contain file information.
	Stream *cStream;							///< Container Stream.
	CryptHash *mHash;							///< Hash'd key
	U32 mDirectoryOffset;					///< Location in file of directory list
	bool mEnableWrite;						///< Should we allow write operations?
	/// @}
public:
	/// @name Generic I/O for headers in container
	/// @{
	bool initNew(Stream *s);
	bool open(bool enableWrite);				///< Loads container
	bool openExisting(Stream *s, bool enableWrite); ///< Loads container using existing stream
	bool read(Stream &s);						///< Loads container explicitly from a stream
	bool read() {return read(*cStream);}	///< Reads in a container file (header and Directory Info)
	bool write(Stream &s);						///< Explicit write to stream
	bool write() {return write(*cStream);}	///< Writes out container (header and Directory Info)
	bool close();									///< Closes container. Deletes stream, and removes entry's

	bool isOpen() {return cStream;}	///< Opened stream?
	/// @}

	///@name Shortcuts
	/// @{
	inline bool createContainer(Stream *s) {return write(*s);}	///< Initializes a container to stream
	bool canWrite() {return mEnableWrite;}								///< Tells us if we can modify container
	/// @}

	/// @name  File Management
	/// @{
	void addDirectory(const char *name, U32 flags);			///< Deletes directory. "name" is added to StringTable.
	bool delDirectory(const char *name);						///< Removes directory. "name" must be in StringTable.
	DirectoryEntry *getDirectory(const char *filename);	///< Finds a directory from a filename and returns the according object

	bool addFilteredFile(const char *name, U8 *ptr, U32 compressedSize, U32 size, U32 flags);	///< Adds a file with already processed data
	bool addFile(const char *name, U8 *ptr, U32 size, U32 flags);				///< Adds a file to the container (replaces if exists)
	bool delFile(const char *name);	///< Removes a file from the container

	void setHash(CryptHash *hash);			///< Sets the key of the container via a hash object
	CryptHash *getHash() {return mHash;}	///< Gets the key of the container in the form of a hash
	/// @}

	/// @name For the resource manager & co
	/// @{
	void setFullPath(const char *path);							///< Sets the full path to the container
	DirectoryEntry::iterator getFile(ResourceObject *obj);///< Gets file entry from ResourceObject
	DirectoryEntry::iterator getFile(const char *path, const char *name); ///< Gets file entry from path and name
	DirectoryEntry::iterator getFile(const char *filename); ///< Gets file entry from filename
	void setStream(Stream *stream) {cStream = stream;}		///< Sets the containers stream

	static ResFilter *getFilter(U32 flags);		///< Wrapper to get filter according to flags
	ResFilter *getFileStream(ResourceObject *obj);	///< Opens a READ ONLY Stream of file from container
	/// @}

	ResContainer();
	virtual ~ResContainer() {close();}

	/// @name Directory iterators
	/// @{
	typedef Vector<DirectoryEntry*>::iterator iterator;

	U32 numDirectorys() const                       { return directorys.size(); }
	DirectoryEntry* operator[](const U32 idx) const { return directorys[idx]; }
	iterator begin()                                { return directorys.begin(); }
	iterator end()                                  { return directorys.end(); }
	/// @}
};

#endif
