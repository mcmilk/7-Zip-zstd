/* 7zDecode.c */

#include "7zDecode.h"
#ifdef _SZ_ONE_DIRECTORY
#include "LzmaDecode.h"
#else
#include "../../Compress/LZMA_C/LzmaDecode.h"
#endif

CMethodID k_Copy = { { 0x0 }, 1 };
CMethodID k_LZMA = { { 0x3, 0x1, 0x1 }, 3 };

#ifdef _LZMA_IN_CB

typedef struct _CLzmaInCallbackImp
{
  ILzmaInCallback InCallback;
  ISzInStream *InStream;
  size_t Size;
} CLzmaInCallbackImp;

int LzmaReadImp(void *object, unsigned char **buffer, UInt32 *size)
{
  CLzmaInCallbackImp *cb = (CLzmaInCallbackImp *)object;
  size_t processedSize;
  SZ_RESULT res;
  *size = 0;
  res = cb->InStream->Read((void *)cb->InStream, (void **)buffer, cb->Size, &processedSize);
  *size = (UInt32)processedSize;
  if (processedSize > cb->Size)
    return (int)SZE_FAIL;
  cb->Size -= processedSize;
  if (res == SZ_OK)
    return 0;
  return (int)res;
}

#endif

SZ_RESULT SzDecode(CFileSize *packSizes, CFolder *folder,
    #ifdef _LZMA_IN_CB
    ISzInStream *inStream,
    #else
    Byte *inBuffer,
    #endif
    Byte *outBuffer, size_t outSize, 
    size_t *outSizeProcessed, ISzAlloc *allocMain)
{
  UInt32 si;
  size_t inSize = 0;
  CCoderInfo *coder;
  if (folder->NumPackStreams != 1)
    return SZE_NOTIMPL;
  if (folder->NumCoders != 1)
    return SZE_NOTIMPL;
  coder = folder->Coders;
  *outSizeProcessed = 0;

  for (si = 0; si < folder->NumPackStreams; si++)
    inSize += (size_t)packSizes[si];

  if (AreMethodsEqual(&coder->MethodID, &k_Copy))
  {
    size_t i;
    if (inSize != outSize)
      return SZE_DATA_ERROR;
    #ifdef _LZMA_IN_CB
    for (i = 0; i < inSize;)
    {
      size_t j;
      Byte *inBuffer;
      size_t bufferSize;
      RINOK(inStream->Read((void *)inStream,  (void **)&inBuffer, inSize - i, &bufferSize));
      if (bufferSize == 0)
        return SZE_DATA_ERROR;
      if (bufferSize > inSize - i)
        return SZE_FAIL;
      *outSizeProcessed += bufferSize;
      for (j = 0; j < bufferSize && i < inSize; j++, i++)
        outBuffer[i] = inBuffer[j];
    }
    #else
    for (i = 0; i < inSize; i++)
      outBuffer[i] = inBuffer[i];
    *outSizeProcessed = inSize;
    #endif
    return SZ_OK;
  }

  if (AreMethodsEqual(&coder->MethodID, &k_LZMA))
  {
    #ifdef _LZMA_IN_CB
    CLzmaInCallbackImp lzmaCallback;
    #endif

    int lc, lp, pb;
    size_t lzmaInternalSize;
    void *lzmaInternalData;
    int result;
    UInt32 outSizeProcessedLoc;

    #ifdef _LZMA_IN_CB
    lzmaCallback.Size = inSize;
    lzmaCallback.InStream = inStream;
    lzmaCallback.InCallback.Read = LzmaReadImp;
    #endif

    if (coder->Properties.Capacity < 5)
      return SZE_FAIL;
    lc = (unsigned char)coder->Properties.Items[0];
    if (lc >= (9 * 5 * 5))
      return SZE_FAIL;
    for (pb = 0; lc >= (9 * 5); pb++, lc -= (9 * 5));
    for (lp = 0; lc >= 9; lp++, lc -= 9);

    lzmaInternalSize = (LZMA_BASE_SIZE + (LZMA_LIT_SIZE << (lc + lp))) * sizeof(CProb);
    lzmaInternalData = allocMain->Alloc(lzmaInternalSize);
    if (lzmaInternalData == 0)
      return SZE_OUTOFMEMORY;

    result = LzmaDecode((Byte *)lzmaInternalData, (UInt32)lzmaInternalSize,
        lc, lp, pb,
        #ifdef _LZMA_IN_CB
        &lzmaCallback.InCallback,
        #else
        inBuffer, (UInt32)inSize,
        #endif
        outBuffer, (UInt32)outSize, 
        &outSizeProcessedLoc);
    *outSizeProcessed = (size_t)outSizeProcessedLoc;
    allocMain->Free(lzmaInternalData);
    /*
    NOT_ENOUGH_MEM error is impossible for this code
    if (result = LZMA_RESULT_NOT_ENOUGH_MEM)
      return SZE_OUTOFMEMORY;
    */
    if (result == LZMA_RESULT_DATA_ERROR)
      return SZE_DATA_ERROR;
    if (result != LZMA_RESULT_OK)
      return SZE_FAIL;
    return SZ_OK;
  }
  return SZE_NOTIMPL;
}
