/* 7zBuf.h -- Byte Buffer
2008-05-01
Igor Pavlov
Public domain */

#ifndef __7Z_BUF_H
#define __7Z_BUF_H

#include "Types.h"

typedef struct
{
  Byte *data;
  size_t size;
} CBuf;

void Buf_Init(CBuf *p);
int Buf_Create(CBuf *p, size_t size, ISzAlloc *alloc);
void Buf_Free(CBuf *p, ISzAlloc *alloc);

#endif
