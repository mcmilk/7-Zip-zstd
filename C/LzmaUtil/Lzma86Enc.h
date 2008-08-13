/* Lzma86Enc.h -- LZMA + x86 (BCJ) Filter Encoder
2008-08-05
Igor Pavlov
Public domain */

#ifndef __LZMA86ENC_H
#define __LZMA86ENC_H

#include "../Types.h"

/*
It's an example for LZMA + x86 Filter use.
You can use .lzma86 extension, if you write that stream to file.
.lzma86 header adds one additional byte to standard .lzma header.
.lzma86 header (14 bytes):
  Offset Size  Description
    0     1    = 0 - no filter,
               = 1 - x86 filter
    1     1    lc, lp and pb in encoded form
    2     4    dictSize (little endian)
    6     8    uncompressed size (little endian)


Lzma86_Encode
-------------
level - compression level: 0 <= level <= 9, the default value for "level" is 5.


dictSize - The dictionary size in bytes. The maximum value is
        128 MB = (1 << 27) bytes for 32-bit version
          1 GB = (1 << 30) bytes for 64-bit version
     The default value is 16 MB = (1 << 24) bytes, for level = 5.
     It's recommended to use the dictionary that is larger than 4 KB and
     that can be calculated as (1 << N) or (3 << N) sizes.
     For better compression ratio dictSize must be >= inSize.

filterMode:
    SZ_FILTER_NO   - no Filter
    SZ_FILTER_YES  - x86 Filter
    SZ_FILTER_AUTO - it tries both alternatives to select best.
              Encoder will use 2 or 3 passes:
              2 passes when FILTER_NO provides better compression.
              3 passes when FILTER_YES provides better compression.

Lzma86Encode allocates Data with MyAlloc functions.
RAM Requirements for compressing:
  RamSize = dictionarySize * 11.5 + 6MB + FilterBlockSize
      filterMode     FilterBlockSize
     SZ_FILTER_NO         0
     SZ_FILTER_YES      inSize
     SZ_FILTER_AUTO     inSize


Return code:
  SZ_OK               - OK
  SZ_ERROR_MEM        - Memory allocation error
  SZ_ERROR_PARAM      - Incorrect paramater
  SZ_ERROR_OUTPUT_EOF - output buffer overflow
  SZ_ERROR_THREAD     - errors in multithreading functions (only for Mt version)
*/

enum ESzFilterMode
{
  SZ_FILTER_NO,
  SZ_FILTER_YES,
  SZ_FILTER_AUTO
};

SRes Lzma86_Encode(Byte *dest, size_t *destLen, const Byte *src, size_t srcLen,
    int level, UInt32 dictSize, int filterMode);

#endif
