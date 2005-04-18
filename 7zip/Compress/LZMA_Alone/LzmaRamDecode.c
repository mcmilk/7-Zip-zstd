/* LzmaRamDecode.c */

#include "LzmaRamDecode.h"
#ifdef _SZ_ONE_DIRECTORY
#include "LzmaDecode.h"
#include "BranchX86.h"
#else
#include "../LZMA_C/LzmaDecode.h"
#include "../Branch/BranchX86.h"
#endif

#define LZMA_PROPS_SIZE 14
#define LZMA_SIZE_OFFSET 6

int LzmaRamGetUncompressedSize(
    unsigned char *inBuffer, 
    size_t inSize,
    size_t *outSize)
{
  unsigned int i;
  if (inSize < LZMA_PROPS_SIZE)
    return 1;
  *outSize = 0;
  for(i = 0; i < sizeof(size_t); i++)
    *outSize += ((size_t)inBuffer[LZMA_SIZE_OFFSET + i]) << (8 * i);
  for(; i < 8; i++)
    if (inBuffer[LZMA_SIZE_OFFSET + i] != 0)
      return 1;
  return 0;
}

#define SZE_DATA_ERROR (1)
#define SZE_OUTOFMEMORY (2)

int LzmaRamDecompress(
    unsigned char *inBuffer, 
    size_t inSize,
    unsigned char *outBuffer,
    size_t outSize,
    size_t *outSizeProcessed,
    void * (*allocFunc)(size_t size), 
    void (*freeFunc)(void *))
{
  int lc, lp, pb;
  size_t lzmaInternalSize;
  void *lzmaInternalData;
  int result;
  UInt32 outSizeProcessedLoc;
  
  int useFilter = inBuffer[0];

  *outSizeProcessed = 0;
  if (useFilter > 1)
    return 1;

  if (inSize < LZMA_PROPS_SIZE)
    return 1;
  lc = inBuffer[1];
  if (lc >= (9 * 5 * 5))
    return 1;
  for (pb = 0; lc >= (9 * 5); pb++, lc -= (9 * 5));
  for (lp = 0; lc >= 9; lp++, lc -= 9);
  
  lzmaInternalSize = (LZMA_BASE_SIZE + (LZMA_LIT_SIZE << (lc + lp))) * sizeof(CProb);
  lzmaInternalData = allocFunc(lzmaInternalSize);
  if (lzmaInternalData == 0)
    return SZE_OUTOFMEMORY;
  
  result = LzmaDecode((unsigned char *)lzmaInternalData, (UInt32)lzmaInternalSize,
    lc, lp, pb,
    inBuffer + LZMA_PROPS_SIZE, (UInt32)inSize - LZMA_PROPS_SIZE,
    outBuffer, (UInt32)outSize, 
    &outSizeProcessedLoc);
  freeFunc(lzmaInternalData);
  if (result != LZMA_RESULT_OK)
    return 1;
  *outSizeProcessed = (size_t)outSizeProcessedLoc;
  if (useFilter == 1)
  {
    UInt32 _prevMask;
    UInt32 _prevPos;
    x86_Convert_Init(_prevMask, _prevPos);
    x86_Convert(outBuffer, (UInt32)outSizeProcessedLoc, 0, &_prevMask, &_prevPos, 0);
  }
  return 0;
}


