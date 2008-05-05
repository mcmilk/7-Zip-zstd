/* Lzma86Dec.c -- LZMA + x86 (BCJ) Filter Decoder
2008-04-07
Igor Pavlov
Public domain */

#include "Lzma86Dec.h"

#include "../Alloc.h"
#include "../Bra.h"
#include "../LzmaDec.h"

#define LZMA86_SIZE_OFFSET (1 + LZMA_PROPS_SIZE)
#define LZMA86_HEADER_SIZE (LZMA86_SIZE_OFFSET + 8)

static void *SzAlloc(void *p, size_t size) { p = p; return MyAlloc(size); }
static void SzFree(void *p, void *address) { p = p; MyFree(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

SRes Lzma86_GetUnpackSize(const Byte *src, SizeT srcLen, UInt64 *unpackSize)
{
  unsigned i;
  if (srcLen < LZMA86_HEADER_SIZE)
    return SZ_ERROR_INPUT_EOF;
  *unpackSize = 0;
  for (i = 0; i < sizeof(UInt64); i++)
    *unpackSize += ((UInt64)src[LZMA86_SIZE_OFFSET + i]) << (8 * i);
  return SZ_OK;
}

SRes Lzma86_Decode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen)
{
  SRes res;
  int useFilter;
  SizeT inSizePure;
  ELzmaStatus status;

  if (*srcLen < LZMA86_HEADER_SIZE)
    return SZ_ERROR_INPUT_EOF;

  useFilter = src[0];

  if (useFilter > 1)
  {
    *destLen = 0;
    return SZ_ERROR_UNSUPPORTED;
  }

  inSizePure = *srcLen - LZMA86_HEADER_SIZE;
  res = LzmaDecode(dest, destLen, src + LZMA86_HEADER_SIZE, &inSizePure,
      src + 1, LZMA_PROPS_SIZE, LZMA_FINISH_ANY, &status, &g_Alloc);
  *srcLen = inSizePure + LZMA86_HEADER_SIZE;
  if (res != SZ_OK)
    return res;
  if (useFilter == 1)
  {
    UInt32 x86State;
    x86_Convert_Init(x86State);
    x86_Convert(dest, *destLen, 0, &x86State, 0);
  }
  return SZ_OK;
}
