// 7z/ItemInfo.h

#pragma once

#ifndef __7Z_ITEMINFO_H
#define __7Z_ITEMINFO_H

#include "Common/Types.h"
#include "Common/String.h"
#include "Common/Buffer.h"
#include "MethodInfo.h"
#include "Header.h"

namespace NArchive {
namespace N7z {

struct CCoderInfo
{
  CMethodID DecompressionMethod;
  UINT64 NumInStreams;
  UINT64 NumOutStreams;
  CByteBuffer Properties;
  bool IsSimpleCoder() const 
    { return (NumInStreams == 1) && (NumOutStreams == 1); }
};

struct CPackStreamInfo
{
  UINT64 Index;
};

struct CBindPair
{
  UINT64 InIndex;
  UINT64 OutIndex;
};

struct CFolderItemInfo
{
  CObjectVector<CCoderInfo> CodersInfo;
  CRecordVector<CBindPair> BindPairs;
  CRecordVector<CPackStreamInfo> PackStreams;
  CRecordVector<UINT64> UnPackSizes;
  bool UnPackCRCDefined;
  UINT32 UnPackCRC;

  CFolderItemInfo(): UnPackCRCDefined(false) {}

  UINT64 GetUnPackSize() const // test it
  { 
    if (UnPackSizes.IsEmpty())
      return 0;
    for (int i = UnPackSizes.Size() - 1; i >= 0; i--)
      if (FindBindPairForOutStream(i) < 0)
        return UnPackSizes[i];
    throw 1;
  }
  UINT64 GetNumOutStreams() const
  {
    UINT64 result = 0;
    for (int i = 0; i < CodersInfo.Size(); i++)
      result += CodersInfo[i].NumOutStreams;
    return result;
  }


  int FindBindPairForInStream(int inStreamIndex) const
  {
    for(int i = 0; i < BindPairs.Size(); i++)
      if (BindPairs[i].InIndex == inStreamIndex)
        return i;
    return -1;
  }
  int FindBindPairForOutStream(int outStreamIndex) const
  {
    for(int i = 0; i < BindPairs.Size(); i++)
      if (BindPairs[i].OutIndex == outStreamIndex)
        return i;
    return -1;
  }
  int FindPackStreamArrayIndex(int inStreamIndex) const
  {
    for(int i = 0; i < PackStreams.Size(); i++)
      if (PackStreams[i].Index == inStreamIndex)
        return i;
    return -1;
  }
};

typedef FILETIME CArchiveFileTime;

class CFileItemInfo
{
public:
  bool IsDirectory;
  bool IsAnti;
  CArchiveFileTime CreationTime;
  CArchiveFileTime LastWriteTime;
  CArchiveFileTime LastAccessTime;
  UINT32 Attributes;
  UINT64 UnPackSize;
  
  bool FileCRCIsDefined;
  UINT32 FileCRC;

  UString Name;
  bool AreAttributesDefined;
  bool IsCreationTimeDefined;
  bool IsLastWriteTimeDefined;
  bool IsLastAccessTimeDefined;
  CFileItemInfo(): 
    AreAttributesDefined(false), 
    IsCreationTimeDefined(false), 
    IsLastWriteTimeDefined(false), 
    IsLastAccessTimeDefined(false),
    IsDirectory(false),
    FileCRCIsDefined(false),
    IsAnti(false)
      {}
  void SetAttributes(UINT32 attributes) 
  { 
    AreAttributesDefined = true;
    Attributes = attributes;
  }
  void SetCreationTime(CArchiveFileTime creationTime) 
  { 
    IsCreationTimeDefined = true;
    CreationTime = creationTime;
  }
  void SetLastWriteTime(CArchiveFileTime lastWriteTime) 
  {
    IsLastWriteTimeDefined = true;
    LastWriteTime = lastWriteTime;
  }
  void SetLastAccessTime(CArchiveFileTime lastAccessTime) 
  { 
    IsLastAccessTimeDefined = true;
    LastAccessTime = lastAccessTime;
  }
};

struct CArchiveDatabase
{
  CRecordVector<UINT64> PackSizes;
  CRecordVector<bool> PackCRCsDefined;
  CRecordVector<UINT32> PackCRCs;
  CObjectVector<CFolderItemInfo> Folders;
  CRecordVector<UINT64> NumUnPackStreamsVector;
  CObjectVector<CFileItemInfo> Files;
  void Clear()
  {
    PackSizes.Clear();
    PackCRCsDefined.Clear();
    PackCRCs.Clear();
    Folders.Clear();
    NumUnPackStreamsVector.Clear();
    Files.Clear();
  }
};

struct CArchiveHeaderDatabase
{
  CRecordVector<UINT64> PackSizes;
  CObjectVector<CFolderItemInfo> Folders;
  CRecordVector<UINT32> CRCs;
  void Clear()
  {
    PackSizes.Clear();
    Folders.Clear();
    CRCs.Clear();
  }
};

}}

#endif
