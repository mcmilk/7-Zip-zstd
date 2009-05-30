/* 7zDecode.h -- Decoding from 7z folder
2009-02-07 : Igor Pavlov : Public domain */

#ifndef __7Z_DECODE_H
#define __7Z_DECODE_H

#include "7zItem.h"

#ifdef __cplusplus
extern "C" {
#endif

SRes SzDecode(const UInt64 *packSizes, const CSzFolder *folder,
    ILookInStream *stream, UInt64 startPos,
    Byte *outBuffer, size_t outSize, ISzAlloc *allocMain);

#ifdef __cplusplus
}
#endif

#endif
