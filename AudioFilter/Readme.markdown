# AudioFilter Code

This code provides an abstract interface for managing various audio formats (raw, speex, ogg).

Refer to the code for examples of incorporating it into your existing code.

## Useful #defines

If you do not want support for a particular codec, the following defines are available :
*NO_AUDIOFILTER* - Removes all AudioFilter's
*NO_VORBISVORBIS* - Removes ogg vorbis support
*NO_SPEEXFILTER* - Removes speex support

## Converting audio (via script)

Use the filterConvert function to convert some audio (last two arguements are optional) :

    filterConvert(%in_file, %out_file, %out_quality, %out_vbrquality);

e.g. :

    filterConvert("my_raw_audio.raw", "out_speex_audio.spx", 5, 0.5);

