# How to incorporate:

## To start

Basically, the key difference with this code is that instead of the FileStream, 
you need to use the Stream class to process files.

e.g. Instead of:

    FileStream st;
    if(!ResourceManager->openFileForWrite(st, codeFileName)) 
       return false;
    
    // ...
    
    st.write(0xDEADBEEF);
    
    // ...
    
    st.close();

Do this:

    Stream *st = ResourceManager->openFileForWrite(codeFileName);
    if(!st) 
        return false;
    
    // ...
    
    st->write(0xDEADBEEF);
    
    // ...
    
    ResourceManager->closeStream(st);

This difference also affects the scriptable FileObject class. It should also be noted 
that this basically replaces the zip code functionality.

## The next thing

The code makes exclusive use of a Dynamic MemStream class to process data. The code for this is in DynMemStream.cc/.h. 

(Personally i'd incorporate it into the MemStream source.)

It also heavily modifies the ResManager in order to hook in the container file streams. A reference diff of these changes 
is located at *resman.diff*.

## Everything else

There are a few more bits of code you'll need. These are:

    // extra memory function
    
    void* dCalloc_r(dsize_t in_num, size_t in_size, const char* fileName, const dsize_t line)
    {
       return Memory::alloc(in_num * in_size, false, fileName, line); 
    }

And:

    // Somewhere in your setup code...
    extern ResourceInstance *constructContainer(Stream &);
    
    // ...
    
    // ContainerHandler variables
    Con::setIntVariable("$Container::PROCESS_BASIC",   FilterState::PROCESS_BASIC);
    Con::setIntVariable("$Container::PROCESS_DELTA8",  FilterState::PROCESS_BASIC);
    Con::setIntVariable("$Container::PROCESS_DELTA16", FilterState::PROCESS_BASIC);
    Con::setIntVariable("$Container::PROCESS_DELTA32", FilterState::PROCESS_BASIC);
    Con::setIntVariable("$Container::PROCESS_ALL",     FilterState::PROCESS_BASIC);
    Con::setIntVariable("$Container::COMPRESS_ZLIB",   FilterState::COMPRESS_ZLIB);
    Con::setIntVariable("$Container::COMPRESS_BZIP2",  FilterState::COMPRESS_BZIP2);
    Con::setIntVariable("$Container::ENCRYPT_BLOWFISH",FilterState::ENCRYPT_BLOWFISH);
    Con::setIntVariable("$Container::ENCRYPT_TWOFISH", FilterState::ENCRYPT_TWOFISH);
    Con::setIntVariable("$Container::ENCRYPT_RIJNDAEL",FilterState::ENCRYPT_RIJNDAEL);
    Con::setIntVariable("$Container::ENCRYPT_XTEA",    FilterState::ENCRYPT_XTEA);
    Con::setIntVariable("$Container::ENCRYPT_RC6",     FilterState::ENCRYPT_RC6);
    Con::setIntVariable("$Container::ENCRYPT_DES",     FilterState::ENCRYPT_DES);
    Con::setIntVariable("$Container::ENCRYPT_ALL",     FilterState::ENCRYPT_ALL);
    
    ResourceManager->registerExtension(".dmf", constructContainer);

Well, that's pretty much it.

Good luck.
