/* 7zDecode.c  Decoding from 7z folder
2008-08-05
Igor Pavlov
Copyright (c) 1999-2008 Igor Pavlov
Read 7zDecode.h for license options */

#include <string.h>

#include "7zDecode.h"
#include "../../LzmaDec.h"
#include "../../Bra.h"
#include "../../Bcj2.h"

#define k_Copy 0
#define k_LZMA 0x30101
#define k_BCJ 0x03030103
#define k_BCJ2 0x0303011B

/*
#ifdef _LZMA_IN_CB
*/

static SRes SzDecodeLzma(CSzCoderInfo *coder, CFileSize inSize, ISzInStream *inStream,
    Byte *outBuffer, size_t outSize, ISzAlloc *allocMain)
{
  CLzmaDec state;
  int res = SZ_OK;
  size_t _inSize;
  Byte *inBuf = NULL;

  LzmaDec_Construct(&state);
  RINOK(LzmaDec_AllocateProbs(&state, coder->Props.data, (unsigned)coder->Props.size, allocMain));
  state.dic = outBuffer;
  state.dicBufSize = outSize;
  LzmaDec_Init(&state);

  _inSize = 0;

  for (;;)
  {
    if (_inSize == 0)
    {
      _inSize = (1 << 18);
      if (_inSize > inSize)
        _inSize = (size_t)(inSize);
      res = inStream->Read((void *)inStream, (void **)&inBuf, &_inSize);
      if (res != SZ_OK)
        break;
      inSize -= _inSize;
    }

    {
      SizeT inProcessed = _inSize, dicPos = state.dicPos;
      ELzmaStatus status;
      res = LzmaDec_DecodeToDic(&state, outSize, inBuf, &inProcessed, LZMA_FINISH_END, &status);
      _inSize -= inProcessed;
      inBuf = (Byte *)inBuf + inProcessed;
      if (res != SZ_OK)
        break;
      if (state.dicPos == state.dicBufSize || (inProcessed == 0 && dicPos == state.dicPos))
      {
        if (state.dicBufSize != outSize || _inSize != 0 ||
            (status != LZMA_STATUS_FINISHED_WITH_MARK &&
             status != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK))
          res = SZ_ERROR_DATA;
        break;
      }
    }
  }

  LzmaDec_FreeProbs(&state, allocMain);

  return res;
}

static SRes SzDecodeCopy(CFileSize inSize, ISzInStream *inStream, Byte *outBuffer)
{
  while (inSize > 0)
  {
    void *inBuffer;
    size_t curSize = (1 << 18);
    if (curSize > inSize)
      curSize = (size_t)(inSize);
    RINOK(inStream->Read((void *)inStream, (void **)&inBuffer, &curSize));
    if (curSize == 0)
      return SZ_ERROR_INPUT_EOF;
    memcpy(outBuffer, inBuffer, curSize);
    outBuffer += curSize;
    inSize -= curSize;
  }
  return SZ_OK;
}
/*
#endif
*/

#define IS_UNSUPPORTED_METHOD(m) ((m) != k_Copy && (m) != k_LZMA)
#define IS_UNSUPPORTED_CODER(c) (IS_UNSUPPORTED_METHOD(c.MethodID) || c.NumInStreams != 1 || c.NumOutStreams != 1)
#define IS_NO_BCJ(c) (c.MethodID != k_BCJ || c.NumInStreams != 1 || c.NumOutStreams != 1)
#define IS_NO_BCJ2(c) (c.MethodID != k_BCJ2 || c.NumInStreams != 4 || c.NumOutStreams != 1)

SRes CheckSupportedFolder(const CSzFolder *f)
{
  if (f->NumCoders < 1 || f->NumCoders > 4)
    return SZ_ERROR_UNSUPPORTED;
  if (IS_UNSUPPORTED_CODER(f->Coders[0]))
    return SZ_ERROR_UNSUPPORTED;
  if (f->NumCoders == 1)
  {
    if (f->NumPackStreams != 1 || f->PackStreams[0] != 0 || f->NumBindPairs != 0)
      return SZ_ERROR_UNSUPPORTED;
    return SZ_OK;
  }
  if (f->NumCoders == 2)
  {
    if (IS_NO_BCJ(f->Coders[1]) ||
        f->NumPackStreams != 1 || f->PackStreams[0] != 0 ||
        f->NumBindPairs != 1 ||
        f->BindPairs[0].InIndex != 1 || f->BindPairs[0].OutIndex != 0)
      return SZ_ERROR_UNSUPPORTED;
    return SZ_OK;
  }
  if (f->NumCoders == 4)
  {
    if (IS_UNSUPPORTED_CODER(f->Coders[1]) ||
        IS_UNSUPPORTED_CODER(f->Coders[2]) ||
        IS_NO_BCJ2(f->Coders[3]))
      return SZ_ERROR_UNSUPPORTED;
    if (f->NumPackStreams != 4 ||
        f->PackStreams[0] != 2 ||
        f->PackStreams[1] != 6 ||
        f->PackStreams[2] != 1 ||
        f->PackStreams[3] != 0 ||
        f->NumBindPairs != 3 ||
        f->BindPairs[0].InIndex != 5 || f->BindPairs[0].OutIndex != 0 ||
        f->BindPairs[1].InIndex != 4 || f->BindPairs[1].OutIndex != 1 ||
        f->BindPairs[2].InIndex != 3 || f->BindPairs[2].OutIndex != 2)
      return SZ_ERROR_UNSUPPORTED;
    return SZ_OK;
  }
  return SZ_ERROR_UNSUPPORTED;
}

CFileSize GetSum(const CFileSize *values, UInt32 index)
{
  CFileSize sum = 0;
  UInt32 i;
  for (i = 0; i < index; i++)
    sum += values[i];
  return sum;
}

SRes SzDecode2(const CFileSize *packSizes, const CSzFolder *folder,
    /*
    #ifdef _LZMA_IN_CB
    */
    ISzInStream *inStream, CFileSize startPos,
    /*
    #else
    const Byte *inBuffer,
    #endif
    */
    Byte *outBuffer, size_t outSize, ISzAlloc *allocMain,
    Byte *tempBuf[])
{
  UInt32 ci;
  size_t tempSizes[3] = { 0, 0, 0};
  size_t tempSize3 = 0;
  Byte *tempBuf3 = 0;

  RINOK(CheckSupportedFolder(folder));

  for (ci = 0; ci < folder->NumCoders; ci++)
  {
    CSzCoderInfo *coder = &folder->Coders[ci];

    if (coder->MethodID == k_Copy || coder->MethodID == k_LZMA)
    {
      UInt32 si = 0;
      CFileSize offset;
      CFileSize inSize;
      Byte *outBufCur = outBuffer;
      size_t outSizeCur = outSize;
      if (folder->NumCoders == 4)
      {
        UInt32 indices[] = { 3, 2, 0 };
        CFileSize unpackSize = folder->UnpackSizes[ci];
        si = indices[ci];
        if (ci < 2)
        {
          Byte *temp;
          outSizeCur = (size_t)unpackSize;
          if (outSizeCur != unpackSize)
            return SZ_ERROR_MEM;
          temp = (Byte *)IAlloc_Alloc(allocMain, outSizeCur);
          if (temp == 0 && outSizeCur != 0)
            return SZ_ERROR_MEM;
          outBufCur = tempBuf[1 - ci] = temp;
          tempSizes[1 - ci] = outSizeCur;
        }
        else if (ci == 2)
        {
          if (unpackSize > outSize) // check it
            return SZ_ERROR_PARAM; // check it
          tempBuf3 = outBufCur = outBuffer + (outSize - (size_t)unpackSize);
          tempSize3 = outSizeCur = (size_t)unpackSize;
        }
        else
          return SZ_ERROR_UNSUPPORTED;
      }
      offset = GetSum(packSizes, si);
      inSize = packSizes[si];
      /*
      #ifdef _LZMA_IN_CB
      */
      RINOK(inStream->Seek(inStream, startPos + offset, SZ_SEEK_SET));
      /*
      #endif
      */

      if (coder->MethodID == k_Copy)
      {
        if (inSize != outSizeCur) // check it
          return SZ_ERROR_DATA;
        
        /*
        #ifdef _LZMA_IN_CB
        */
        RINOK(SzDecodeCopy(inSize, inStream, outBufCur));
        /*
        #else
        memcpy(outBufCur, inBuffer + (size_t)offset, (size_t)inSize);
        #endif
        */
      }
      else
      {
        /*
        #ifdef _LZMA_IN_CB
        */
        SRes res = SzDecodeLzma(coder, inSize,
            inStream,
            outBufCur, outSizeCur, allocMain);
        /*
        #else
        SizeT lzmaOutSizeT = outSizeCur;
        SizeT lzmaInSizeT = (SizeT)inSize;
        SRes res = LzmaDecode(outBufCur, &lzmaOutSizeT,
            inBuffer + (size_t)offset, &lzmaInSizeT,
            coder->Props.Items, (unsigned)coder->Props.size, LZMA_FINISH_BYTE, allocMain);
        #endif
        */

        RINOK(res);
      }
    }
    else if (coder->MethodID == k_BCJ)
    {
      UInt32 state;
      if (ci != 1)
        return SZ_ERROR_UNSUPPORTED;
      x86_Convert_Init(state);
      x86_Convert(outBuffer, outSize, 0, &state, 0);
    }
    else if (coder->MethodID == k_BCJ2)
    {
      CFileSize offset = GetSum(packSizes, 1);
      CFileSize s3Size = packSizes[1];
      SRes res;
      if (ci != 3)
        return SZ_ERROR_UNSUPPORTED;

      /*
      #ifdef _LZMA_IN_CB
      */
      RINOK(inStream->Seek(inStream, startPos + offset, SZ_SEEK_SET));
      tempSizes[2] = (size_t)s3Size;
      if (tempSizes[2] != s3Size)
        return SZ_ERROR_MEM;
      tempBuf[2] = (Byte *)IAlloc_Alloc(allocMain, tempSizes[2]);
      if (tempBuf[2] == 0 && tempSizes[2] != 0)
        return SZ_ERROR_MEM;
      res = SzDecodeCopy(s3Size, inStream, tempBuf[2]);
      RINOK(res)
      /*
      #endif
      */

      res = Bcj2_Decode(
          tempBuf3, tempSize3,
          tempBuf[0], tempSizes[0],
          tempBuf[1], tempSizes[1],
          /*
          #ifdef _LZMA_IN_CB
          */
          tempBuf[2], tempSizes[2],
          /*
          #else
          inBuffer + (size_t)offset, (size_t)s3Size,
          #endif
          */
          outBuffer, outSize);
      RINOK(res)
    }
    else
      return SZ_ERROR_UNSUPPORTED;
  }
  return SZ_OK;
}

SRes SzDecode(const CFileSize *packSizes, const CSzFolder *folder,
    /*
    #ifdef _LZMA_IN_CB
    */
    ISzInStream *inStream, CFileSize startPos,
    /*
    #else
    const Byte *inBuffer,
    #endif
    */
    Byte *outBuffer, size_t outSize, ISzAlloc *allocMain)
{
  Byte *tempBuf[3] = { 0, 0, 0};
  int i;
  SRes res = SzDecode2(packSizes, folder,
      /*
      #ifdef _LZMA_IN_CB
      */
      inStream, startPos,
      /*
      #else
      inBuffer,
      #endif
      */
      outBuffer, outSize, allocMain, tempBuf);
  for (i = 0; i < 3; i++)
    IAlloc_Free(allocMain, tempBuf[i]);
  return res;
}
