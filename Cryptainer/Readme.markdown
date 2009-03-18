# Cryptainer


## How to install

1) Grab the code

2) Patch your sources. Refer to Applying_HOWTO.markdown for details on this.

3) Incorporate new project files

You will need to copy the following files into your engine/core/ folder, and add them to your project:

    engine/core/resContainer.*
    engine/core/resFilter.*
    engine/core/filterState.*
    engine/core/hash.h
    engine/core/resourceFilters (whole directory)
    lib/bzip2 (whole directory)
    lib/libtomcrypt (whole directory)

*NOTE*: with the exception of E_tomcrypt.cc, P_none.cc, and C_zlib.cc, all the files in core/resourceFilters are optional.

Also remove the following files from your project:

    core/zip*

Add the following to your include paths:

    lib/libtomcrypt/headers
    lib/bzip2 (if including bzip2 compression)

And add the following static libs to your torque target:

    libtomcrypt
    bzip2

## Quick overview of the new console functions

    setContainerKey(container, key) - sets the crypto key to use to decrypt encrypted assets from container.
    setContainerHash(container, hash) - same as setContainerKey(), but uses a precomputed hash.
    getHash(key) - returns computed hash for string "key".
    setFilterFlags(flags) - sets the filter flags used for containers/files created from now onwards.
    touchContainer(name) - creates an empty container file.
    dumpCompressionHandlers() - prints a list of compression handlers compiled in.

    * Processing flags (1 and 1 only required) *
    $Container::PROCESS_BASIC
    $Container::PROCESS_DELTA8
    $Container::PROCESS_DELTA16
    $Container::PROCESS_DELTA32
    $Container::COMPRESS_ZLIB
    $Container::COMPRESS_BZIP2
    $Container::PROCESS_ALL (Value with all PROCESS_* and COMPRESS_* flags set)

    * Encryption flags (optional, others can be added by modifying enum) *
    $Container::ENCRYPT_BLOWFISH
    $Container::ENCRYPT_TWOFISH
    $Container::ENCRYPT_RIJNDAEL
    $Container::ENCRYPT_XTEA
    $Container::ENCRYPT_RC6
    $Container::ENCRYPT_DES
    $Container::ENCRYPT_ALL (Value with all ENCRYPT_* flags set)

To add files to a container, merely make sure the container exists, and write to it as if it was a directory :

    // Set up container
    setFilterFlags($Container::COMPRESS_ZLIB);
    touchContainer("starter.fps/myContainer.dmf");
    
    // Enable encryption (optional)
    setFilterFlags($Container::COMPRESS_ZLIB | $Container::ENCRYPT_BLOWFISH);
    setContainerKey("starter.fps/myContainer.dmf","pies taste good");
    
    // Write something
    $player = localclientconnection.player;
    $player.write("starter.fps/myContainer.dmf/myScript.cs");

*NOTE*: Containers can also contain directories, or rather, paths. However, unlike files, directory entries can only be compressed.

To load files from a container, merely load them directly from the container; Containers collapse themselves to the root of the directory they are in, so you can also load the files that way (provided you restart torque, or use setModPaths() again). e.g:

    // Try and execute the script in the container
    exec("starter.fps/myScript.cs");
    // If exec() fails, make sure you set the correct key for the container, aka:
    setContainerKey("starter.fps/myContainer.dmf","pies taste good");
    exec("starter.fps/myScript.cs");

As for deleting files, this is not exposed to script; The only time a container will delete a file is when it is replacing it with a new copy.

## The archiver tool, dmfar

dmfar is a simple commandline tool for creating and extracting .dmf archives. These can be compressed and encrypted, according to which options are set. There is a quick reference printed out when you run the tool without any valid options.

Examples of Usage:

    dmfar -a -v -f bzip2 -w ./source_folder dest_container.dmf

(Creates a container using all the files and subdirectories from the folder "source_folder" in the current working directory)

    dmfar -l -v dest_container.dmf

(Lists all files stored in the container)

    dmfar -e -v -w ./extract_folder dest_container.dmf

(Extracts all files from the container into the folder "extract_folder" in the current working directory)

    dmfar -a -v -c blowfish -l mykey.txt -h myhash.txt -w ./source_folder dest_cryptainer.dmf

(Creates a container using all the files and subdirectories from the folder "source_folder" in the current working directory. Files will be encrypted using blowfish, using a key from "mykey.txt". In addition, a hash generated from the key will be stored in "myhash.txt", which can be reused later instead of specifying the key).

    dmfar -e -v -h myhash.txt -w ./extract_cryptfolder dest_cryptainer.dmf

(Extracts all files from the container, using the supplied hash in "myhash.txt" to decrypt the encrypted files present. Files will be placed in "extract_cryptfolder" in the current working directory)

(Also note that files can be both encrypted and compressed. Encrypted files decrypted with an invalid key will return invalid data, and if these are additionally compressed, then the decompression process will fail)

Have fun!
