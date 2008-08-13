/* 7zIn.c -- 7z Input functions
2008-08-05
Igor Pavlov
Copyright (c) 1999-2008 Igor Pavlov
Read 7zIn.h for license options */

#include "7zIn.h"
#include "7zDecode.h"
#include "../../7zCrc.h"

#define RINOM(x) { if((x) == 0) return SZ_ERROR_MEM; }

void SzArEx_Init(CSzArEx *p)
{
  SzAr_Init(&p->db);
  p->FolderStartPackStreamIndex = 0;
  p->PackStreamStartPositions = 0;
  p->FolderStartFileIndex = 0;
  p->FileIndexToFolderIndexMap = 0;
}

void SzArEx_Free(CSzArEx *p, ISzAlloc *alloc)
{
  IAlloc_Free(alloc, p->FolderStartPackStreamIndex);
  IAlloc_Free(alloc, p->PackStreamStartPositions);
  IAlloc_Free(alloc, p->FolderStartFileIndex);
  IAlloc_Free(alloc, p->FileIndexToFolderIndexMap);
  SzAr_Free(&p->db, alloc);
  SzArEx_Init(p);
}

/*
CFileSize GetFolderPackStreamSize(int folderIndex, int streamIndex) const
{
  return PackSizes[FolderStartPackStreamIndex[folderIndex] + streamIndex];
}

CFileSize GetFilePackSize(int fileIndex) const
{
  int folderIndex = FileIndexToFolderIndexMap[fileIndex];
  if (folderIndex >= 0)
  {
    const CSzFolder &folderInfo = Folders[folderIndex];
    if (FolderStartFileIndex[folderIndex] == fileIndex)
    return GetFolderFullPackSize(folderIndex);
  }
  return 0;
}
*/

#define MY_ALLOC(T, p, size, alloc) { if ((size) == 0) p = 0; else \
  if ((p = (T *)IAlloc_Alloc(alloc, (size) * sizeof(T))) == 0) return SZ_ERROR_MEM; }

static SRes SzArEx_Fill(CSzArEx *p, ISzAlloc *alloc)
{
  UInt32 startPos = 0;
  CFileSize startPosSize = 0;
  UInt32 i;
  UInt32 folderIndex = 0;
  UInt32 indexInFolder = 0;
  MY_ALLOC(UInt32, p->FolderStartPackStreamIndex, p->db.NumFolders, alloc);
  for (i = 0; i < p->db.NumFolders; i++)
  {
    p->FolderStartPackStreamIndex[i] = startPos;
    startPos += p->db.Folders[i].NumPackStreams;
  }

  MY_ALLOC(CFileSize, p->PackStreamStartPositions, p->db.NumPackStreams, alloc);

  for (i = 0; i < p->db.NumPackStreams; i++)
  {
    p->PackStreamStartPositions[i] = startPosSize;
    startPosSize += p->db.PackSizes[i];
  }

  MY_ALLOC(UInt32, p->FolderStartFileIndex, p->db.NumFolders, alloc);
  MY_ALLOC(UInt32, p->FileIndexToFolderIndexMap, p->db.NumFiles, alloc);

  for (i = 0; i < p->db.NumFiles; i++)
  {
    CSzFileItem *file = p->db.Files + i;
    int emptyStream = !file->HasStream;
    if (emptyStream && indexInFolder == 0)
    {
      p->FileIndexToFolderIndexMap[i] = (UInt32)-1;
      continue;
    }
    if (indexInFolder == 0)
    {
      /*
      v3.13 incorrectly worked with empty folders
      v4.07: Loop for skipping empty folders
      */
      for (;;)
      {
        if (folderIndex >= p->db.NumFolders)
          return SZ_ERROR_ARCHIVE;
        p->FolderStartFileIndex[folderIndex] = i;
        if (p->db.Folders[folderIndex].NumUnpackStreams != 0)
          break;
        folderIndex++;
      }
    }
    p->FileIndexToFolderIndexMap[i] = folderIndex;
    if (emptyStream)
      continue;
    indexInFolder++;
    if (indexInFolder >= p->db.Folders[folderIndex].NumUnpackStreams)
    {
      folderIndex++;
      indexInFolder = 0;
    }
  }
  return SZ_OK;
}


CFileSize SzArEx_GetFolderStreamPos(const CSzArEx *p, UInt32 folderIndex, UInt32 indexInFolder)
{
  return p->ArchiveInfo.DataStartPosition +
    p->PackStreamStartPositions[p->FolderStartPackStreamIndex[folderIndex] + indexInFolder];
}

int SzArEx_GetFolderFullPackSize(const CSzArEx *p, UInt32 folderIndex, CFileSize *resSize)
{
  UInt32 packStreamIndex = p->FolderStartPackStreamIndex[folderIndex];
  CSzFolder *folder = p->db.Folders + folderIndex;
  CFileSize size = 0;
  UInt32 i;
  for (i = 0; i < folder->NumPackStreams; i++)
  {
    CFileSize t = size + p->db.PackSizes[packStreamIndex + i];
    if (t < size) // check it
      return SZ_ERROR_FAIL;
    size = t;
  }
  *resSize = size;
  return SZ_OK;
}


/*
SRes SzReadTime(const CObjectVector<CBuf> &dataVector,
    CObjectVector<CSzFileItem> &files, UInt64 type)
{
  CBoolVector boolVector;
  RINOK(ReadBoolVector2(files.Size(), boolVector))

  CStreamSwitch streamSwitch;
  RINOK(streamSwitch.Set(this, &dataVector));

  for (int i = 0; i < files.Size(); i++)
  {
    CSzFileItem &file = files[i];
    CArchiveFileTime fileTime;
    bool defined = boolVector[i];
    if (defined)
    {
      UInt32 low, high;
      RINOK(SzReadUInt32(low));
      RINOK(SzReadUInt32(high));
      fileTime.dwLowDateTime = low;
      fileTime.dwHighDateTime = high;
    }
    switch(type)
    {
      case k7zIdCTime: file.IsCTimeDefined = defined; if (defined) file.CTime = fileTime; break;
      case k7zIdATime: file.IsATimeDefined = defined; if (defined) file.ATime = fileTime; break;
      case k7zIdMTime: file.IsMTimeDefined = defined; if (defined) file.MTime = fileTime; break;
    }
  }
  return SZ_OK;
}
*/

static SRes SafeReadDirect(ISzInStream *inStream, Byte *data, size_t size)
{
  while (size > 0)
  {
    void *inBufferSpec;
    size_t processedSize = size;
    const Byte *inBuffer;
    RINOK(inStream->Read(inStream, (void **)&inBufferSpec, &processedSize));
    inBuffer = (const Byte *)inBufferSpec;
    if (processedSize == 0)
      return SZ_ERROR_INPUT_EOF;
    size -= processedSize;
    do
      *data++ = *inBuffer++;
    while (--processedSize != 0);
  }
  return SZ_OK;
}

static SRes SafeReadDirectByte(ISzInStream *inStream, Byte *data)
{
  return SafeReadDirect(inStream, data, 1);
}

static SRes SafeReadDirectUInt32(ISzInStream *inStream, UInt32 *value, UInt32 *crc)
{
  int i;
  *value = 0;
  for (i = 0; i < 4; i++)
  {
    Byte b;
    RINOK(SafeReadDirectByte(inStream, &b));
    *value |= ((UInt32)b << (8 * i));
    *crc = CRC_UPDATE_BYTE(*crc, b);
  }
  return SZ_OK;
}

static SRes SafeReadDirectUInt64(ISzInStream *inStream, UInt64 *value, UInt32 *crc)
{
  int i;
  *value = 0;
  for (i = 0; i < 8; i++)
  {
    Byte b;
    RINOK(SafeReadDirectByte(inStream, &b));
    *value |= ((UInt64)b << (8 * i));
    *crc = CRC_UPDATE_BYTE(*crc, b);
  }
  return SZ_OK;
}

static int TestSignatureCandidate(Byte *testBytes)
{
  size_t i;
  for (i = 0; i < k7zSignatureSize; i++)
    if (testBytes[i] != k7zSignature[i])
      return 0;
  return 1;
}

typedef struct _CSzState
{
  Byte *Data;
  size_t Size;
}CSzData;

static SRes SzReadByte(CSzData *sd, Byte *b)
{
  if (sd->Size == 0)
    return SZ_ERROR_ARCHIVE;
  sd->Size--;
  *b = *sd->Data++;
  return SZ_OK;
}

static SRes SzReadBytes(CSzData *sd, Byte *data, size_t size)
{
  size_t i;
  for (i = 0; i < size; i++)
  {
    RINOK(SzReadByte(sd, data + i));
  }
  return SZ_OK;
}

static SRes SzReadUInt32(CSzData *sd, UInt32 *value)
{
  int i;
  *value = 0;
  for (i = 0; i < 4; i++)
  {
    Byte b;
    RINOK(SzReadByte(sd, &b));
    *value |= ((UInt32)(b) << (8 * i));
  }
  return SZ_OK;
}

static SRes SzReadNumber(CSzData *sd, UInt64 *value)
{
  Byte firstByte;
  Byte mask = 0x80;
  int i;
  RINOK(SzReadByte(sd, &firstByte));
  *value = 0;
  for (i = 0; i < 8; i++)
  {
    Byte b;
    if ((firstByte & mask) == 0)
    {
      UInt64 highPart = firstByte & (mask - 1);
      *value += (highPart << (8 * i));
      return SZ_OK;
    }
    RINOK(SzReadByte(sd, &b));
    *value |= ((UInt64)b << (8 * i));
    mask >>= 1;
  }
  return SZ_OK;
}

static SRes SzReadSize(CSzData *sd, CFileSize *value)
{
  UInt64 value64;
  RINOK(SzReadNumber(sd, &value64));
  *value = (CFileSize)value64;
  return SZ_OK;
}

static SRes SzReadNumber32(CSzData *sd, UInt32 *value)
{
  UInt64 value64;
  RINOK(SzReadNumber(sd, &value64));
  if (value64 >= 0x80000000)
    return SZ_ERROR_UNSUPPORTED;
  if (value64 >= ((UInt64)(1) << ((sizeof(size_t) - 1) * 8 + 2)))
    return SZ_ERROR_UNSUPPORTED;
  *value = (UInt32)value64;
  return SZ_OK;
}

static SRes SzReadID(CSzData *sd, UInt64 *value)
{
  return SzReadNumber(sd, value);
}

static SRes SzSkeepDataSize(CSzData *sd, UInt64 size)
{
  if (size > sd->Size)
    return SZ_ERROR_ARCHIVE;
  sd->Size -= (size_t)size;
  sd->Data += (size_t)size;
  return SZ_OK;
}

static SRes SzSkeepData(CSzData *sd)
{
  UInt64 size;
  RINOK(SzReadNumber(sd, &size));
  return SzSkeepDataSize(sd, size);
}

static SRes SzReadArchiveProperties(CSzData *sd)
{
  for (;;)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdEnd)
      break;
    SzSkeepData(sd);
  }
  return SZ_OK;
}

static SRes SzWaitAttribute(CSzData *sd, UInt64 attribute)
{
  for (;;)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if (type == attribute)
      return SZ_OK;
    if (type == k7zIdEnd)
      return SZ_ERROR_ARCHIVE;
    RINOK(SzSkeepData(sd));
  }
}

static SRes SzReadBoolVector(CSzData *sd, size_t numItems, Byte **v, ISzAlloc *alloc)
{
  Byte b = 0;
  Byte mask = 0;
  size_t i;
  MY_ALLOC(Byte, *v, numItems, alloc);
  for (i = 0; i < numItems; i++)
  {
    if (mask == 0)
    {
      RINOK(SzReadByte(sd, &b));
      mask = 0x80;
    }
    (*v)[i] = (Byte)(((b & mask) != 0) ? 1 : 0);
    mask >>= 1;
  }
  return SZ_OK;
}

static SRes SzReadBoolVector2(CSzData *sd, size_t numItems, Byte **v, ISzAlloc *alloc)
{
  Byte allAreDefined;
  size_t i;
  RINOK(SzReadByte(sd, &allAreDefined));
  if (allAreDefined == 0)
    return SzReadBoolVector(sd, numItems, v, alloc);
  MY_ALLOC(Byte, *v, numItems, alloc);
  for (i = 0; i < numItems; i++)
    (*v)[i] = 1;
  return SZ_OK;
}

static SRes SzReadHashDigests(
    CSzData *sd,
    size_t numItems,
    Byte **digestsDefined,
    UInt32 **digests,
    ISzAlloc *alloc)
{
  size_t i;
  RINOK(SzReadBoolVector2(sd, numItems, digestsDefined, alloc));
  MY_ALLOC(UInt32, *digests, numItems, alloc);
  for (i = 0; i < numItems; i++)
    if ((*digestsDefined)[i])
    {
      RINOK(SzReadUInt32(sd, (*digests) + i));
    }
  return SZ_OK;
}

static SRes SzReadPackInfo(
    CSzData *sd,
    CFileSize *dataOffset,
    UInt32 *numPackStreams,
    CFileSize **packSizes,
    Byte **packCRCsDefined,
    UInt32 **packCRCs,
    ISzAlloc *alloc)
{
  UInt32 i;
  RINOK(SzReadSize(sd, dataOffset));
  RINOK(SzReadNumber32(sd, numPackStreams));

  RINOK(SzWaitAttribute(sd, k7zIdSize));

  MY_ALLOC(CFileSize, *packSizes, (size_t)*numPackStreams, alloc);

  for (i = 0; i < *numPackStreams; i++)
  {
    RINOK(SzReadSize(sd, (*packSizes) + i));
  }

  for (;;)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdEnd)
      break;
    if (type == k7zIdCRC)
    {
      RINOK(SzReadHashDigests(sd, (size_t)*numPackStreams, packCRCsDefined, packCRCs, alloc));
      continue;
    }
    RINOK(SzSkeepData(sd));
  }
  if (*packCRCsDefined == 0)
  {
    MY_ALLOC(Byte, *packCRCsDefined, (size_t)*numPackStreams, alloc);
    MY_ALLOC(UInt32, *packCRCs, (size_t)*numPackStreams, alloc);
    for (i = 0; i < *numPackStreams; i++)
    {
      (*packCRCsDefined)[i] = 0;
      (*packCRCs)[i] = 0;
    }
  }
  return SZ_OK;
}

static SRes SzReadSwitch(CSzData *sd)
{
  Byte external;
  RINOK(SzReadByte(sd, &external));
  return (external == 0) ? SZ_OK: SZ_ERROR_UNSUPPORTED;
}

static SRes SzGetNextFolderItem(CSzData *sd, CSzFolder *folder, ISzAlloc *alloc)
{
  UInt32 numCoders;
  UInt32 numBindPairs;
  UInt32 numPackedStreams;
  UInt32 i;
  UInt32 numInStreams = 0;
  UInt32 numOutStreams = 0;
  RINOK(SzReadNumber32(sd, &numCoders));
  folder->NumCoders = numCoders;

  MY_ALLOC(CSzCoderInfo, folder->Coders, (size_t)numCoders, alloc);

  for (i = 0; i < numCoders; i++)
    SzCoderInfo_Init(folder->Coders + i);

  for (i = 0; i < numCoders; i++)
  {
    Byte mainByte;
    CSzCoderInfo *coder = folder->Coders + i;
    {
      unsigned idSize, j;
      Byte longID[15];
      RINOK(SzReadByte(sd, &mainByte));
      idSize = (unsigned)(mainByte & 0xF);
      RINOK(SzReadBytes(sd, longID, idSize));
      if (idSize > sizeof(coder->MethodID))
        return SZ_ERROR_UNSUPPORTED;
      coder->MethodID = 0;
      for (j = 0; j < idSize; j++)
        coder->MethodID |= (CMethodID)longID[idSize - 1 - j] << (8 * j);

      if ((mainByte & 0x10) != 0)
      {
        RINOK(SzReadNumber32(sd, &coder->NumInStreams));
        RINOK(SzReadNumber32(sd, &coder->NumOutStreams));
      }
      else
      {
        coder->NumInStreams = 1;
        coder->NumOutStreams = 1;
      }
      if ((mainByte & 0x20) != 0)
      {
        UInt64 propertiesSize = 0;
        RINOK(SzReadNumber(sd, &propertiesSize));
        if (!Buf_Create(&coder->Props, (size_t)propertiesSize, alloc))
          return SZ_ERROR_MEM;
        RINOK(SzReadBytes(sd, coder->Props.data, (size_t)propertiesSize));
      }
    }
    while ((mainByte & 0x80) != 0)
    {
      RINOK(SzReadByte(sd, &mainByte));
      RINOK(SzSkeepDataSize(sd, (mainByte & 0xF)));
      if ((mainByte & 0x10) != 0)
      {
        UInt32 n;
        RINOK(SzReadNumber32(sd, &n));
        RINOK(SzReadNumber32(sd, &n));
      }
      if ((mainByte & 0x20) != 0)
      {
        UInt64 propertiesSize = 0;
        RINOK(SzReadNumber(sd, &propertiesSize));
        RINOK(SzSkeepDataSize(sd, propertiesSize));
      }
    }
    numInStreams += (UInt32)coder->NumInStreams;
    numOutStreams += (UInt32)coder->NumOutStreams;
  }

  numBindPairs = numOutStreams - 1;
  folder->NumBindPairs = numBindPairs;


  MY_ALLOC(CBindPair, folder->BindPairs, (size_t)numBindPairs, alloc);

  for (i = 0; i < numBindPairs; i++)
  {
    CBindPair *bindPair = folder->BindPairs + i;;
    RINOK(SzReadNumber32(sd, &bindPair->InIndex));
    RINOK(SzReadNumber32(sd, &bindPair->OutIndex));
  }

  numPackedStreams = numInStreams - (UInt32)numBindPairs;

  folder->NumPackStreams = numPackedStreams;
  MY_ALLOC(UInt32, folder->PackStreams, (size_t)numPackedStreams, alloc);

  if (numPackedStreams == 1)
  {
    UInt32 j;
    UInt32 pi = 0;
    for (j = 0; j < numInStreams; j++)
      if (SzFolder_FindBindPairForInStream(folder, j) < 0)
      {
        folder->PackStreams[pi++] = j;
        break;
      }
  }
  else
    for (i = 0; i < numPackedStreams; i++)
    {
      RINOK(SzReadNumber32(sd, folder->PackStreams + i));
    }
  return SZ_OK;
}

static SRes SzReadUnpackInfo(
    CSzData *sd,
    UInt32 *numFolders,
    CSzFolder **folders,  /* for alloc */
    ISzAlloc *alloc,
    ISzAlloc *allocTemp)
{
  UInt32 i;
  RINOK(SzWaitAttribute(sd, k7zIdFolder));
  RINOK(SzReadNumber32(sd, numFolders));
  {
    RINOK(SzReadSwitch(sd));

    MY_ALLOC(CSzFolder, *folders, (size_t)*numFolders, alloc);

    for (i = 0; i < *numFolders; i++)
      SzFolder_Init((*folders) + i);

    for (i = 0; i < *numFolders; i++)
    {
      RINOK(SzGetNextFolderItem(sd, (*folders) + i, alloc));
    }
  }

  RINOK(SzWaitAttribute(sd, k7zIdCodersUnpackSize));

  for (i = 0; i < *numFolders; i++)
  {
    UInt32 j;
    CSzFolder *folder = (*folders) + i;
    UInt32 numOutStreams = SzFolder_GetNumOutStreams(folder);

    MY_ALLOC(CFileSize, folder->UnpackSizes, (size_t)numOutStreams, alloc);

    for (j = 0; j < numOutStreams; j++)
    {
      RINOK(SzReadSize(sd, folder->UnpackSizes + j));
    }
  }

  for (;;)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdEnd)
      return SZ_OK;
    if (type == k7zIdCRC)
    {
      SRes res;
      Byte *crcsDefined = 0;
      UInt32 *crcs = 0;
      res = SzReadHashDigests(sd, *numFolders, &crcsDefined, &crcs, allocTemp);
      if (res == SZ_OK)
      {
        for (i = 0; i < *numFolders; i++)
        {
          CSzFolder *folder = (*folders) + i;
          folder->UnpackCRCDefined = crcsDefined[i];
          folder->UnpackCRC = crcs[i];
        }
      }
      IAlloc_Free(allocTemp, crcs);
      IAlloc_Free(allocTemp, crcsDefined);
      RINOK(res);
      continue;
    }
    RINOK(SzSkeepData(sd));
  }
}

static SRes SzReadSubStreamsInfo(
    CSzData *sd,
    UInt32 numFolders,
    CSzFolder *folders,
    UInt32 *numUnpackStreams,
    CFileSize **unpackSizes,
    Byte **digestsDefined,
    UInt32 **digests,
    ISzAlloc *allocTemp)
{
  UInt64 type = 0;
  UInt32 i;
  UInt32 si = 0;
  UInt32 numDigests = 0;

  for (i = 0; i < numFolders; i++)
    folders[i].NumUnpackStreams = 1;
  *numUnpackStreams = numFolders;

  for (;;)
  {
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdNumUnpackStream)
    {
      *numUnpackStreams = 0;
      for (i = 0; i < numFolders; i++)
      {
        UInt32 numStreams;
        RINOK(SzReadNumber32(sd, &numStreams));
        folders[i].NumUnpackStreams = numStreams;
        *numUnpackStreams += numStreams;
      }
      continue;
    }
    if (type == k7zIdCRC || type == k7zIdSize)
      break;
    if (type == k7zIdEnd)
      break;
    RINOK(SzSkeepData(sd));
  }

  if (*numUnpackStreams == 0)
  {
    *unpackSizes = 0;
    *digestsDefined = 0;
    *digests = 0;
  }
  else
  {
    *unpackSizes = (CFileSize *)IAlloc_Alloc(allocTemp, (size_t)*numUnpackStreams * sizeof(CFileSize));
    RINOM(*unpackSizes);
    *digestsDefined = (Byte *)IAlloc_Alloc(allocTemp, (size_t)*numUnpackStreams * sizeof(Byte));
    RINOM(*digestsDefined);
    *digests = (UInt32 *)IAlloc_Alloc(allocTemp, (size_t)*numUnpackStreams * sizeof(UInt32));
    RINOM(*digests);
  }

  for (i = 0; i < numFolders; i++)
  {
    /*
    v3.13 incorrectly worked with empty folders
    v4.07: we check that folder is empty
    */
    CFileSize sum = 0;
    UInt32 j;
    UInt32 numSubstreams = folders[i].NumUnpackStreams;
    if (numSubstreams == 0)
      continue;
    if (type == k7zIdSize)
    for (j = 1; j < numSubstreams; j++)
    {
      CFileSize size;
      RINOK(SzReadSize(sd, &size));
      (*unpackSizes)[si++] = size;
      sum += size;
    }
    (*unpackSizes)[si++] = SzFolder_GetUnpackSize(folders + i) - sum;
  }
  if (type == k7zIdSize)
  {
    RINOK(SzReadID(sd, &type));
  }

  for (i = 0; i < *numUnpackStreams; i++)
  {
    (*digestsDefined)[i] = 0;
    (*digests)[i] = 0;
  }


  for (i = 0; i < numFolders; i++)
  {
    UInt32 numSubstreams = folders[i].NumUnpackStreams;
    if (numSubstreams != 1 || !folders[i].UnpackCRCDefined)
      numDigests += numSubstreams;
  }

 
  si = 0;
  for (;;)
  {
    if (type == k7zIdCRC)
    {
      int digestIndex = 0;
      Byte *digestsDefined2 = 0;
      UInt32 *digests2 = 0;
      SRes res = SzReadHashDigests(sd, numDigests, &digestsDefined2, &digests2, allocTemp);
      if (res == SZ_OK)
      {
        for (i = 0; i < numFolders; i++)
        {
          CSzFolder *folder = folders + i;
          UInt32 numSubstreams = folder->NumUnpackStreams;
          if (numSubstreams == 1 && folder->UnpackCRCDefined)
          {
            (*digestsDefined)[si] = 1;
            (*digests)[si] = folder->UnpackCRC;
            si++;
          }
          else
          {
            UInt32 j;
            for (j = 0; j < numSubstreams; j++, digestIndex++)
            {
              (*digestsDefined)[si] = digestsDefined2[digestIndex];
              (*digests)[si] = digests2[digestIndex];
              si++;
            }
          }
        }
      }
      IAlloc_Free(allocTemp, digestsDefined2);
      IAlloc_Free(allocTemp, digests2);
      RINOK(res);
    }
    else if (type == k7zIdEnd)
      return SZ_OK;
    else
    {
      RINOK(SzSkeepData(sd));
    }
    RINOK(SzReadID(sd, &type));
  }
}


static SRes SzReadStreamsInfo(
    CSzData *sd,
    CFileSize *dataOffset,
    CSzAr *p,
    UInt32 *numUnpackStreams,
    CFileSize **unpackSizes, /* allocTemp */
    Byte **digestsDefined,   /* allocTemp */
    UInt32 **digests,        /* allocTemp */
    ISzAlloc *alloc,
    ISzAlloc *allocTemp)
{
  for (;;)
  {
    UInt64 type;
    RINOK(SzReadID(sd, &type));
    if ((UInt64)(int)type != type)
      return SZ_ERROR_UNSUPPORTED;
    switch((int)type)
    {
      case k7zIdEnd:
        return SZ_OK;
      case k7zIdPackInfo:
      {
        RINOK(SzReadPackInfo(sd, dataOffset, &p->NumPackStreams,
            &p->PackSizes, &p->PackCRCsDefined, &p->PackCRCs, alloc));
        break;
      }
      case k7zIdUnpackInfo:
      {
        RINOK(SzReadUnpackInfo(sd, &p->NumFolders, &p->Folders, alloc, allocTemp));
        break;
      }
      case k7zIdSubStreamsInfo:
      {
        RINOK(SzReadSubStreamsInfo(sd, p->NumFolders, p->Folders,
            numUnpackStreams, unpackSizes, digestsDefined, digests, allocTemp));
        break;
      }
      default:
        return SZ_ERROR_UNSUPPORTED;
    }
  }
}

Byte kUtf8Limits[5] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

static SRes SzReadFileNames(CSzData *sd, UInt32 numFiles, CSzFileItem *files, ISzAlloc *alloc)
{
  UInt32 i;
  for (i = 0; i < numFiles; i++)
  {
    UInt32 len = 0;
    UInt32 pos = 0;
    CSzFileItem *file = files + i;
    while(pos + 2 <= sd->Size)
    {
      int numAdds;
      UInt32 value = (UInt32)(sd->Data[pos] | (((UInt32)sd->Data[pos + 1]) << 8));
      pos += 2;
      len++;
      if (value == 0)
        break;
      if (value < 0x80)
        continue;
      if (value >= 0xD800 && value < 0xE000)
      {
        UInt32 c2;
        if (value >= 0xDC00)
          return SZ_ERROR_ARCHIVE;
        if (pos + 2 > sd->Size)
          return SZ_ERROR_ARCHIVE;
        c2 = (UInt32)(sd->Data[pos] | (((UInt32)sd->Data[pos + 1]) << 8));
        pos += 2;
        if (c2 < 0xDC00 || c2 >= 0xE000)
          return SZ_ERROR_ARCHIVE;
        value = ((value - 0xD800) << 10) | (c2 - 0xDC00);
      }
      for (numAdds = 1; numAdds < 5; numAdds++)
        if (value < (((UInt32)1) << (numAdds * 5 + 6)))
          break;
      len += numAdds;
    }

    MY_ALLOC(char, file->Name, (size_t)len, alloc);

    len = 0;
    while(2 <= sd->Size)
    {
      int numAdds;
      UInt32 value = (UInt32)(sd->Data[0] | (((UInt32)sd->Data[1]) << 8));
      SzSkeepDataSize(sd, 2);
      if (value < 0x80)
      {
        file->Name[len++] = (char)value;
        if (value == 0)
          break;
        continue;
      }
      if (value >= 0xD800 && value < 0xE000)
      {
        UInt32 c2 = (UInt32)(sd->Data[0] | (((UInt32)sd->Data[1]) << 8));
        SzSkeepDataSize(sd, 2);
        value = ((value - 0xD800) << 10) | (c2 - 0xDC00);
      }
      for (numAdds = 1; numAdds < 5; numAdds++)
        if (value < (((UInt32)1) << (numAdds * 5 + 6)))
          break;
      file->Name[len++] = (char)(kUtf8Limits[numAdds - 1] + (value >> (6 * numAdds)));
      do
      {
        numAdds--;
        file->Name[len++] = (char)(0x80 + ((value >> (6 * numAdds)) & 0x3F));
      }
      while(numAdds > 0);

      len += numAdds;
    }
  }
  return SZ_OK;
}

static SRes SzReadHeader2(
    CSzArEx *p,   /* allocMain */
    CSzData *sd,
    CFileSize **unpackSizes,  /* allocTemp */
    Byte **digestsDefined,    /* allocTemp */
    UInt32 **digests,         /* allocTemp */
    Byte **emptyStreamVector, /* allocTemp */
    Byte **emptyFileVector,   /* allocTemp */
    Byte **lwtVector,         /* allocTemp */
    ISzAlloc *allocMain,
    ISzAlloc *allocTemp)
{
  UInt64 type;
  UInt32 numUnpackStreams = 0;
  UInt32 numFiles = 0;
  CSzFileItem *files = 0;
  UInt32 numEmptyStreams = 0;
  UInt32 i;

  RINOK(SzReadID(sd, &type));

  if (type == k7zIdArchiveProperties)
  {
    RINOK(SzReadArchiveProperties(sd));
    RINOK(SzReadID(sd, &type));
  }
 
 
  if (type == k7zIdMainStreamsInfo)
  {
    RINOK(SzReadStreamsInfo(sd,
        &p->ArchiveInfo.DataStartPosition,
        &p->db,
        &numUnpackStreams,
        unpackSizes,
        digestsDefined,
        digests, allocMain, allocTemp));
    p->ArchiveInfo.DataStartPosition += p->ArchiveInfo.StartPositionAfterHeader;
    RINOK(SzReadID(sd, &type));
  }

  if (type == k7zIdEnd)
    return SZ_OK;
  if (type != k7zIdFilesInfo)
    return SZ_ERROR_ARCHIVE;
  
  RINOK(SzReadNumber32(sd, &numFiles));
  p->db.NumFiles = numFiles;

  MY_ALLOC(CSzFileItem, files, (size_t)numFiles, allocMain);

  p->db.Files = files;
  for (i = 0; i < numFiles; i++)
    SzFile_Init(files + i);

  for (;;)
  {
    UInt64 type;
    UInt64 size;
    RINOK(SzReadID(sd, &type));
    if (type == k7zIdEnd)
      break;
    RINOK(SzReadNumber(sd, &size));

    if ((UInt64)(int)type != type)
    {
      RINOK(SzSkeepDataSize(sd, size));
    }
    else
    switch((int)type)
    {
      case k7zIdName:
      {
        RINOK(SzReadSwitch(sd));
        RINOK(SzReadFileNames(sd, numFiles, files, allocMain))
        break;
      }
      case k7zIdEmptyStream:
      {
        RINOK(SzReadBoolVector(sd, numFiles, emptyStreamVector, allocTemp));
        numEmptyStreams = 0;
        for (i = 0; i < numFiles; i++)
          if ((*emptyStreamVector)[i])
            numEmptyStreams++;
        break;
      }
      case k7zIdEmptyFile:
      {
        RINOK(SzReadBoolVector(sd, numEmptyStreams, emptyFileVector, allocTemp));
        break;
      }
      case k7zIdMTime:
      {
        RINOK(SzReadBoolVector2(sd, numFiles, lwtVector, allocTemp));
        RINOK(SzReadSwitch(sd));
        for (i = 0; i < numFiles; i++)
        {
          CSzFileItem *f = &files[i];
          Byte defined = (*lwtVector)[i];
          f->MTimeDefined = defined;
          f->MTime.Low = f->MTime.High = 0;
          if (defined)
          {
            RINOK(SzReadUInt32(sd, &f->MTime.Low));
            RINOK(SzReadUInt32(sd, &f->MTime.High));
          }
        }
        break;
      }
      default:
      {
        RINOK(SzSkeepDataSize(sd, size));
      }
    }
  }

  {
    UInt32 emptyFileIndex = 0;
    UInt32 sizeIndex = 0;
    for (i = 0; i < numFiles; i++)
    {
      CSzFileItem *file = files + i;
      file->IsAnti = 0;
      if (*emptyStreamVector == 0)
        file->HasStream = 1;
      else
        file->HasStream = (Byte)((*emptyStreamVector)[i] ? 0 : 1);
      if(file->HasStream)
      {
        file->IsDir = 0;
        file->Size = (*unpackSizes)[sizeIndex];
        file->FileCRC = (*digests)[sizeIndex];
        file->FileCRCDefined = (Byte)(*digestsDefined)[sizeIndex];
        sizeIndex++;
      }
      else
      {
        if (*emptyFileVector == 0)
          file->IsDir = 1;
        else
          file->IsDir = (Byte)((*emptyFileVector)[emptyFileIndex] ? 0 : 1);
        emptyFileIndex++;
        file->Size = 0;
        file->FileCRCDefined = 0;
      }
    }
  }
  return SzArEx_Fill(p, allocMain);
}

static SRes SzReadHeader(
    CSzArEx *p,
    CSzData *sd,
    ISzAlloc *allocMain,
    ISzAlloc *allocTemp)
{
  CFileSize *unpackSizes = 0;
  Byte *digestsDefined = 0;
  UInt32 *digests = 0;
  Byte *emptyStreamVector = 0;
  Byte *emptyFileVector = 0;
  Byte *lwtVector = 0;
  SRes res = SzReadHeader2(p, sd,
      &unpackSizes, &digestsDefined, &digests,
      &emptyStreamVector, &emptyFileVector, &lwtVector,
      allocMain, allocTemp);
  IAlloc_Free(allocTemp, unpackSizes);
  IAlloc_Free(allocTemp, digestsDefined);
  IAlloc_Free(allocTemp, digests);
  IAlloc_Free(allocTemp, emptyStreamVector);
  IAlloc_Free(allocTemp, emptyFileVector);
  IAlloc_Free(allocTemp, lwtVector);
  return res;
}

static SRes SzReadAndDecodePackedStreams2(
    ISzInStream *inStream,
    CSzData *sd,
    CBuf *outBuffer,
    CFileSize baseOffset,
    CSzAr *p,
    CFileSize **unpackSizes,
    Byte **digestsDefined,
    UInt32 **digests,
    ISzAlloc *allocTemp)
{

  UInt32 numUnpackStreams = 0;
  CFileSize dataStartPos;
  CSzFolder *folder;
  CFileSize unpackSize;
  SRes res;

  RINOK(SzReadStreamsInfo(sd, &dataStartPos, p,
      &numUnpackStreams,  unpackSizes, digestsDefined, digests,
      allocTemp, allocTemp));
  
  dataStartPos += baseOffset;
  if (p->NumFolders != 1)
    return SZ_ERROR_ARCHIVE;

  folder = p->Folders;
  unpackSize = SzFolder_GetUnpackSize(folder);
  
  RINOK(inStream->Seek(inStream, dataStartPos, SZ_SEEK_SET));

  if (!Buf_Create(outBuffer, (size_t)unpackSize, allocTemp))
    return SZ_ERROR_MEM;
  
  res = SzDecode(p->PackSizes, folder,
          inStream, dataStartPos,
          outBuffer->data, (size_t)unpackSize, allocTemp);
  RINOK(res);
  if (folder->UnpackCRCDefined)
    if (CrcCalc(outBuffer->data, (size_t)unpackSize) != folder->UnpackCRC)
      return SZ_ERROR_CRC;
  return SZ_OK;
}

static SRes SzReadAndDecodePackedStreams(
    ISzInStream *inStream,
    CSzData *sd,
    CBuf *outBuffer,
    CFileSize baseOffset,
    ISzAlloc *allocTemp)
{
  CSzAr p;
  CFileSize *unpackSizes = 0;
  Byte *digestsDefined = 0;
  UInt32 *digests = 0;
  SRes res;
  SzAr_Init(&p);
  res = SzReadAndDecodePackedStreams2(inStream, sd, outBuffer, baseOffset,
    &p, &unpackSizes, &digestsDefined, &digests,
    allocTemp);
  SzAr_Free(&p, allocTemp);
  IAlloc_Free(allocTemp, unpackSizes);
  IAlloc_Free(allocTemp, digestsDefined);
  IAlloc_Free(allocTemp, digests);
  return res;
}

static SRes SzArEx_Open2(
    CSzArEx *p,
    ISzInStream *inStream,
    ISzAlloc *allocMain,
    ISzAlloc *allocTemp)
{
  Byte signature[k7zSignatureSize];
  Byte version;
  UInt32 crcFromArchive;
  UInt64 nextHeaderOffset;
  UInt64 nextHeaderSize;
  UInt32 nextHeaderCRC;
  UInt32 crc = 0;
  CFileSize pos = 0;
  CBuf buffer;
  CSzData sd;
  SRes res;

  if (SafeReadDirect(inStream, signature, k7zSignatureSize) != SZ_OK)
    return SZ_ERROR_NO_ARCHIVE;

  if (!TestSignatureCandidate(signature))
    return SZ_ERROR_NO_ARCHIVE;

  /*
  p.Clear();
  p.ArchiveInfo.StartPosition = _arhiveBeginStreamPosition;
  */
  RINOK(SafeReadDirectByte(inStream, &version));
  if (version != k7zMajorVersion)
    return SZ_ERROR_UNSUPPORTED;
  RINOK(SafeReadDirectByte(inStream, &version));

  RINOK(SafeReadDirectUInt32(inStream, &crcFromArchive, &crc));

  crc = CRC_INIT_VAL;
  RINOK(SafeReadDirectUInt64(inStream, &nextHeaderOffset, &crc));
  RINOK(SafeReadDirectUInt64(inStream, &nextHeaderSize, &crc));
  RINOK(SafeReadDirectUInt32(inStream, &nextHeaderCRC, &crc));

  pos = k7zStartHeaderSize;
  p->ArchiveInfo.StartPositionAfterHeader = pos;
  
  if (CRC_GET_DIGEST(crc) != crcFromArchive)
    return SZ_ERROR_CRC;

  if (nextHeaderSize == 0)
    return SZ_OK;

  RINOK(inStream->Seek(inStream, (CFileSize)(pos + nextHeaderOffset), SZ_SEEK_SET));

  if (!Buf_Create(&buffer, (size_t)nextHeaderSize, allocTemp))
    return SZ_ERROR_MEM;

  res = SafeReadDirect(inStream, buffer.data, (size_t)nextHeaderSize);
  if (res == SZ_OK)
  {
    res = SZ_ERROR_ARCHIVE;
    if (CrcCalc(buffer.data, (size_t)nextHeaderSize) == nextHeaderCRC)
    {
      for (;;)
      {
        UInt64 type;
        sd.Data = buffer.data;
        sd.Size = buffer.size;
        res = SzReadID(&sd, &type);
        if (res != SZ_OK)
          break;
        if (type == k7zIdHeader)
        {
          res = SzReadHeader(p, &sd, allocMain, allocTemp);
          break;
        }
        if (type != k7zIdEncodedHeader)
        {
          res = SZ_ERROR_UNSUPPORTED;
          break;
        }
        {
          CBuf outBuffer;
          Buf_Init(&outBuffer);
          res = SzReadAndDecodePackedStreams(inStream, &sd, &outBuffer,
              p->ArchiveInfo.StartPositionAfterHeader,
              allocTemp);
          if (res != SZ_OK)
          {
            Buf_Free(&outBuffer, allocTemp);
            break;
          }
          Buf_Free(&buffer, allocTemp);
          buffer.data = outBuffer.data;
          buffer.size = outBuffer.size;
        }
      }
    }
  }
  Buf_Free(&buffer, allocTemp);
  return res;
}

SRes SzArEx_Open(CSzArEx *p, ISzInStream *inStream, ISzAlloc *allocMain, ISzAlloc *allocTemp)
{
  SRes res = SzArEx_Open2(p, inStream, allocMain, allocTemp);
  if (res != SZ_OK)
    SzArEx_Free(p, allocMain);
  return res;
}
