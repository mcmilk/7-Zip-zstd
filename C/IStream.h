/* IStream.h */

#ifndef __C_ISTREAM_H
#define __C_ISTREAM_H

#include "Types.h"

typedef struct _ISeqInStream
{
  HRes (*Read)(void *object, void *data, UInt32 size, UInt32 *processedSize);
} ISeqInStream;

typedef struct _ISzAlloc
{
  void *(*Alloc)(size_t size);
  void (*Free)(void *address); /* address can be 0 */
} ISzAlloc;

#endif
