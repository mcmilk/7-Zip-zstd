/* 
LzmaTest.c
Test application for LZMA Decoder
LZMA SDK 4.16 Copyright (c) 1999-2004 Igor Pavlov (2005-03-18)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LzmaDecode.h"

size_t MyReadFile(FILE *file, void *data, size_t size)
{
  return (fread(data, 1, size, file) == size);
}

#ifdef _LZMA_IN_CB
typedef struct _CBuffer
{
  ILzmaInCallback InCallback;
  unsigned char *Buffer;
  unsigned int Size;
} CBuffer;

int LzmaReadCompressed(void *object, unsigned char **buffer, unsigned int *size)
{
  CBuffer *bo = (CBuffer *)object;
  *size = bo->Size; /* You can specify any available size here */
  *buffer = bo->Buffer;
  bo->Buffer += *size; 
  bo->Size -= *size;
  return LZMA_RESULT_OK;
}
#endif

int main2(int numargs, const char *args[], char *rs)
{
  FILE *inputHandle, *outputHandle;
  unsigned int length, processedSize;
  unsigned int compressedSize, outSize, outSizeProcessed, lzmaInternalSize;
  void *inStream, *outStream, *lzmaInternalData;
  unsigned char properties[5];
  unsigned char prop0;
  int ii;
  int lc, lp, pb;
  int res;
  #ifdef _LZMA_IN_CB
  CBuffer bo;
  #endif

  sprintf(rs + strlen(rs), "\nLZMA Decoder 4.16 Copyright (c) 1999-2005 Igor Pavlov  2005-03-18\n");
  if (numargs < 2 || numargs > 3)
  {
    sprintf(rs + strlen(rs), "\nUsage:  lzmaDec file.lzma [outFile]\n");
    return 1;
  }

  inputHandle = fopen(args[1], "rb");
  if (inputHandle == 0)
  {
    sprintf(rs + strlen(rs), "\n Open input file error");
    return 1;
  }

  fseek(inputHandle, 0, SEEK_END);
  length = ftell(inputHandle);
  fseek(inputHandle, 0, SEEK_SET);

  if (!MyReadFile(inputHandle, properties, sizeof(properties)))
    return 1;
  
  outSize = 0;
  for (ii = 0; ii < 4; ii++)
  {
    unsigned char b;
    if (!MyReadFile(inputHandle, &b, sizeof(b)))
      return 1;
    outSize += (unsigned int)(b) << (ii * 8);
  }

  if (outSize == 0xFFFFFFFF)
  {
    sprintf(rs + strlen(rs), "\nstream version is not supported");
    return 1;
  }

  for (ii = 0; ii < 4; ii++)
  {
    unsigned char b;
    if (!MyReadFile(inputHandle, &b, sizeof(b)))
      return 1;
    if (b != 0)
    {
      sprintf(rs + strlen(rs), "\n too long file");
      return 1;
    }
  }

  compressedSize = length - 13;
  inStream = malloc(compressedSize);
  if (inStream == 0)
  {
    sprintf(rs + strlen(rs), "\n can't allocate");
    return 1;
  }
  if (!MyReadFile(inputHandle, inStream, compressedSize))
  {
    sprintf(rs + strlen(rs), "\n can't read");
    return 1;
  }

  fclose(inputHandle);

  prop0 = properties[0];
  if (prop0 >= (9*5*5))
  {
    sprintf(rs + strlen(rs), "\n Properties error");
    return 1;
  }
  for (pb = 0; prop0 >= (9 * 5); 
    pb++, prop0 -= (9 * 5));
  for (lp = 0; prop0 >= 9; 
    lp++, prop0 -= 9);
  lc = prop0;

  lzmaInternalSize = 
    (LZMA_BASE_SIZE + (LZMA_LIT_SIZE << (lc + lp)))* sizeof(CProb);

  #ifdef _LZMA_OUT_READ
  lzmaInternalSize += 100;
  #endif

  outStream = malloc(outSize);
  lzmaInternalData = malloc(lzmaInternalSize);
  if (outStream == 0 || lzmaInternalData == 0)
  {
    sprintf(rs + strlen(rs), "\n can't allocate");
    return 1;
  }

  #ifdef _LZMA_IN_CB
  bo.InCallback.Read = LzmaReadCompressed;
  bo.Buffer = (unsigned char *)inStream;
  bo.Size = compressedSize;
  #endif

  #ifdef _LZMA_OUT_READ
  {
    UInt32 nowPos;
    unsigned char *dictionary;
    UInt32 dictionarySize = 0;
    int i;
    for (i = 0; i < 4; i++)
      dictionarySize += (UInt32)(properties[1 + i]) << (i * 8);
    if (dictionarySize == 0)
      dictionarySize = 1; /* LZMA decoder can not work with dictionarySize = 0 */
    dictionary = (unsigned char *)malloc(dictionarySize);
    if (dictionary == 0)
    {
      sprintf(rs + strlen(rs), "\n can't allocate");
      return 1;
    }
    res = LzmaDecoderInit((unsigned char *)lzmaInternalData, lzmaInternalSize,
        lc, lp, pb,
        dictionary, dictionarySize,
        #ifdef _LZMA_IN_CB
        &bo.InCallback
        #else
        (unsigned char *)inStream, compressedSize
        #endif
        );
    if (res == 0)
    for (nowPos = 0; nowPos < outSize;)
    {
      UInt32 blockSize = outSize - nowPos;
      UInt32 kBlockSize = 0x10000;
      if (blockSize > kBlockSize)
        blockSize = kBlockSize;
      res = LzmaDecode((unsigned char *)lzmaInternalData, 
      ((unsigned char *)outStream) + nowPos, blockSize, &outSizeProcessed);
      if (res != 0)
        break;
      if (outSizeProcessed == 0)
      {
        outSize = nowPos;
        break;
      }
      nowPos += outSizeProcessed;
    }
    free(dictionary);
  }

  #else
  res = LzmaDecode((unsigned char *)lzmaInternalData, lzmaInternalSize,
      lc, lp, pb,
      #ifdef _LZMA_IN_CB
      &bo.InCallback,
      #else
      (unsigned char *)inStream, compressedSize,
      #endif
      (unsigned char *)outStream, outSize, &outSizeProcessed);
  outSize = outSizeProcessed;
  #endif

  if (res != 0)
  {
    sprintf(rs + strlen(rs), "\nerror = %d\n", res);
    return 1;
  }

  if (numargs > 2)
  {
    outputHandle = fopen(args[2], "wb+");
    if (outputHandle == 0)
    {
      sprintf(rs + strlen(rs), "\n Open output file error");
      return 1;
    }
    processedSize = fwrite(outStream, 1, outSize, outputHandle);
    if (processedSize != outSize)
    {
      sprintf(rs + strlen(rs), "\n can't write");
      return 1;
    }
    fclose(outputHandle);
  }
  free(lzmaInternalData);
  free(outStream);
  free(inStream);
  return 0;
}

int main(int numargs, const char *args[])
{
  char sz[800] = { 0 };
  int code = main2(numargs, args, sz);
  printf(sz);
  return code;
}
