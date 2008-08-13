/* Lzma86Dec.h -- LZMA + x86 (BCJ) Filter Decoder
2008-08-05
Igor Pavlov
Public domain */

#ifndef __LZMA86DEC_H
#define __LZMA86DEC_H

#include "../Types.h"

/*
Lzma86_GetUnpackSize:
  In:
    src      - input data
    srcLen   - input data size
  Out:
    unpackSize - size of uncompressed stream
  Return code:
    SZ_OK               - OK
    SZ_ERROR_INPUT_EOF  - Error in headers
*/

SRes Lzma86_GetUnpackSize(const Byte *src, SizeT srcLen, UInt64 *unpackSize);

/*
Lzma86_Decode:
  In:
    dest     - output data
    destLen  - output data size
    src      - input data
    srcLen   - input data size
  Out:
    destLen  - processed output size
    srcLen   - processed input size
  Return code:
    SZ_OK           - OK
    SZ_ERROR_DATA  - Data error
    SZ_ERROR_MEM   - Memory allocation error
    SZ_ERROR_UNSUPPORTED - unsupported file
    SZ_ERROR_INPUT_EOF - it needs more bytes in input buffer
*/

SRes Lzma86_Decode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen);

#endif
