/* 7zItem.h -- 7z Items
2008-05-01
Igor Pavlov
Copyright (c) 1999-2008 Igor Pavlov
Read LzmaDec.h for license options */

#ifndef __7Z_ITEM_H
#define __7Z_ITEM_H

#include "../../7zBuf.h"

/* #define _SZ_FILE_SIZE_32 */
/* You can define _SZ_FILE_SIZE_32, if you don't need support for files larger than 4 GB*/

#ifdef _SZ_FILE_SIZE_32
typedef UInt32 CFileSize; 
#else
typedef UInt64 CFileSize; 
#endif

typedef UInt64 CMethodID;

typedef struct
{
  UInt32 NumInStreams;
  UInt32 NumOutStreams;
  CMethodID MethodID;
  CBuf Props;
} CSzCoderInfo;

void SzCoderInfo_Init(CSzCoderInfo *p);
void SzCoderInfo_Free(CSzCoderInfo *p, ISzAlloc *alloc);

typedef struct
{
  UInt32 InIndex;
  UInt32 OutIndex;
} CBindPair;

typedef struct
{
  CSzCoderInfo *Coders;
  CBindPair *BindPairs;
  UInt32 *PackStreams;
  CFileSize *UnPackSizes;
  UInt32 NumCoders;
  UInt32 NumBindPairs;
  UInt32 NumPackStreams; 
  int UnPackCRCDefined;
  UInt32 UnPackCRC;

  UInt32 NumUnPackStreams;
} CSzFolder;

void SzFolder_Init(CSzFolder *p);
CFileSize SzFolder_GetUnPackSize(CSzFolder *p);
int SzFolder_FindBindPairForInStream(CSzFolder *p, UInt32 inStreamIndex);
UInt32 SzFolder_GetNumOutStreams(CSzFolder *p);
CFileSize SzFolder_GetUnPackSize(CSzFolder *p);

typedef struct
{
  UInt32 Low;
  UInt32 High;
} CNtfsFileTime;

typedef struct
{
  CNtfsFileTime LastWriteTime;
  /*
  CFileSize StartPos;
  UInt32 Attributes; 
  */
  CFileSize Size;
  char *Name;
  UInt32 FileCRC;

  Byte IsFileCRCDefined;
  Byte HasStream;
  Byte IsDirectory;
  Byte IsAnti;
  Byte IsLastWriteTimeDefined;
  /*
  int AreAttributesDefined;
  int IsLastWriteTimeDefined;
  int IsStartPosDefined;
  */
} CSzFileItem;

void SzFile_Init(CSzFileItem *p);

typedef struct
{
  CFileSize *PackSizes;
  Byte *PackCRCsDefined;
  UInt32 *PackCRCs;
  CSzFolder *Folders;
  CSzFileItem *Files;
  UInt32 NumPackStreams;
  UInt32 NumFolders;
  UInt32 NumFiles;
} CSzAr;

void SzAr_Init(CSzAr *p);
void SzAr_Free(CSzAr *p, ISzAlloc *alloc);

#endif
