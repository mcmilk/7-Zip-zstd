/* 7zIn.h -- 7z Input functions
2008-05-05
Igor Pavlov
Copyright (c) 1999-2008 Igor Pavlov
Read 7zItem.h for license options */

#ifndef __7Z_IN_H
#define __7Z_IN_H

#include "7zHeader.h"
#include "7zItem.h"

typedef struct
{
  CFileSize StartPositionAfterHeader; 
  CFileSize DataStartPosition;
} CInArchiveInfo;

typedef struct
{
  CSzAr db;
  CInArchiveInfo ArchiveInfo;
  UInt32 *FolderStartPackStreamIndex;
  CFileSize *PackStreamStartPositions;
  UInt32 *FolderStartFileIndex;
  UInt32 *FileIndexToFolderIndexMap;
} CSzArEx;

void SzArEx_Init(CSzArEx *p);
void SzArEx_Free(CSzArEx *p, ISzAlloc *alloc);
CFileSize SzArEx_GetFolderStreamPos(const CSzArEx *p, UInt32 folderIndex, UInt32 indexInFolder);
int SzArEx_GetFolderFullPackSize(const CSzArEx *p, UInt32 folderIndex, CFileSize *resSize);

typedef enum 
{
  SZ_SEEK_SET = 0,
  SZ_SEEK_CUR = 1,
  SZ_SEEK_END = 2
} ESzSeek;

typedef struct
{
  SRes (*Read)(void *object, void **buf, size_t *size);
    /* if (input(*size) != 0 && output(*size) == 0) means end_of_stream.
       (output(*size) < input(*size)) is allowed */
  SRes (*Seek)(void *object, CFileSize pos, ESzSeek origin);
} ISzInStream;

 
/*
Errors:
SZ_ERROR_NO_ARCHIVE
SZ_ERROR_ARCHIVE
SZ_ERROR_UNSUPPORTED
SZ_ERROR_MEM
SZ_ERROR_CRC
SZ_ERROR_INPUT_EOF
SZ_ERROR_FAIL
*/

SRes SzArEx_Open(CSzArEx *p, ISzInStream *inStream, ISzAlloc *allocMain, ISzAlloc *allocTemp);
 
#endif
