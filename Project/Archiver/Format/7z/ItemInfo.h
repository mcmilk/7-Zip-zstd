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
  int GetNumOutStreams() const
  {
    int aResult = 0;
    for (int i = 0; i < CodersInfo.Size(); i++)
      aResult += CodersInfo[i].NumOutStreams;
    return aResult;
  }


  int FindBindPairForInStream(int anInStreamIndex) const
  {
    for(int i = 0; i < BindPairs.Size(); i++)
      if (BindPairs[i].InIndex == anInStreamIndex)
        return i;
    return -1;
  }
  int FindBindPairForOutStream(int anOutStreamIndex) const
  {
    for(int i = 0; i < BindPairs.Size(); i++)
      if (BindPairs[i].OutIndex == anOutStreamIndex)
        return i;
    return -1;
  }
  int FindPackStreamArrayIndex(int anInStreamIndex) const
  {
    for(int i = 0; i < PackStreams.Size(); i++)
      if (PackStreams[i].Index == anInStreamIndex)
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
  void SetAttributes(UINT32 anAttributes) 
  { 
    AreAttributesDefined = true;
    Attributes = anAttributes;
  }
  void SetCreationTime(CArchiveFileTime aCreationTime) 
  { 
    IsCreationTimeDefined = true;
    CreationTime = aCreationTime;
  }
  void SetLastWriteTime(CArchiveFileTime aLastWriteTime) 
  {
    IsLastWriteTimeDefined = true;
    LastWriteTime = aLastWriteTime;
  }
  void SetLastAccessTime(CArchiveFileTime aLastAccessTime) 
  { 
    IsLastAccessTimeDefined = true;
    LastAccessTime = aLastAccessTime;
  }
};

struct CArchiveDatabase
{
  CRecordVector<UINT64> m_PackSizes;
  CRecordVector<bool> m_PackCRCsDefined;
  CRecordVector<UINT32> m_PackCRCs;
  CObjectVector<CFolderItemInfo> m_Folders;
  CRecordVector<UINT64> m_NumUnPackStreamsVector;
  CObjectVector<CFileItemInfo> m_Files;
  void Clear()
  {
    m_PackSizes.Clear();
    m_PackCRCsDefined.Clear();
    m_PackCRCs.Clear();
    m_Folders.Clear();
    m_NumUnPackStreamsVector.Clear();
    m_Files.Clear();
  }
};

struct CArchiveHeaderDatabase
{
  CRecordVector<UINT64> m_PackSizes;
  CObjectVector<CFolderItemInfo> m_Folders;
  CRecordVector<UINT32> m_CRCs;
  void Clear()
  {
    m_PackSizes.Clear();
    m_Folders.Clear();
    m_CRCs.Clear();
  }
};

}}

#endif
