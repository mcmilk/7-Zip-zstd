// 7z/InEngine.h

#pragma once

#ifndef __7Z_INENGINE_H
#define __7Z_INENGINE_H

#include "Interface/IInOutStreams.h"

#include "Header.h"
#include "ItemInfo.h"
#include "Stream/InByte.h"

namespace NArchive {
namespace N7z {
  
class CInArchiveException
{
public:
  enum CCauseType
  {
    kUnsupportedVersion = 0,
    kUnexpectedEndOfArchive = 0,
    kIncorrectHeader,
  } Cause;
  CInArchiveException(CCauseType aCause);
};

struct CInArchiveInfo
{
  CArchiveVersion Version;
  UINT64 StartPosition;
  UINT64 StartPositionAfterHeader;
  UINT64 DataStartPosition;
  UINT64 DataStartPosition2;
  CRecordVector<UINT32> FileInfoPopIDs;
};


struct CArchiveDatabaseEx: public CArchiveDatabase
{
  CInArchiveInfo m_ArchiveInfo;
  CRecordVector<UINT64> m_PackStreamStartPositions;
  CRecordVector<UINT64> m_FolderStartPackStreamIndex;
  CRecordVector<UINT64> m_FolderStartFileIndex;
  CRecordVector<int> m_FileIndexToFolderIndexMap;

  void FillFolderStartPackStream();
  void FillStartPos();
  void FillFolderStartFileIndex();
  UINT64 GetFolderStreamPos(int aFolderIndex, int anIndexInFolder) const
  {
    return m_ArchiveInfo.DataStartPosition +
        m_PackStreamStartPositions[m_FolderStartPackStreamIndex[aFolderIndex] +
        anIndexInFolder];
  }
  UINT64 GetFolderFullPackSize(int aFolderIndex) const 
  {
    UINT64 aPackStreamIndex = m_FolderStartPackStreamIndex[aFolderIndex];
    const CFolderItemInfo &aFolder = m_Folders[aFolderIndex];
    UINT64 aSize = 0;
    for (int i = 0; i < aFolder.PackStreams.Size(); i++)
      aSize += m_PackSizes[aPackStreamIndex + i];
    return aSize;
  }
  UINT64 GetFolderPackStreamSize(int aFolderIndex, int aStreamIndex) const 
  {
    return m_PackSizes[m_FolderStartPackStreamIndex[aFolderIndex] + aStreamIndex];
  }
};

class CInByte2
{
  const BYTE *m_Buffer;
  UINT32 m_Size;
  UINT32 m_Pos;
public:
  void Init(const BYTE *aBuffer, UINT32 aSize)
  {
    m_Buffer = aBuffer;
    m_Size = aSize;
    m_Pos = 0;
  }
  bool ReadByte(BYTE &aByte)
  {
    if(m_Pos >= m_Size)
      return false;
    aByte = m_Buffer[m_Pos++];
    return true;
  }
  void ReadBytes(void *aData, UINT32 aSize, UINT32 &aProcessedSize)
  {
    for(aProcessedSize = 0; aProcessedSize < aSize && m_Pos < m_Size; aProcessedSize++)
      ((BYTE *)aData)[aProcessedSize] = m_Buffer[m_Pos++];
  }
  bool ReadBytes(void *aData, UINT32 aSize)
  {
    UINT32 aProcessedSize;
    ReadBytes(aData, aSize, aProcessedSize);
    return (aProcessedSize == aSize);
  }
  UINT32 GetProcessedSize() const { return m_Pos; }
};

class CStreamSwitch;
class CInArchive
{
  friend CStreamSwitch;

  CComPtr<IInStream> m_Stream;

  CObjectVector<CInByte2> m_InByteVector;
  CInByte2 *m_InByteBack;
 
  UINT64 m_ArhiveBeginStreamPosition;
  UINT64 m_Position;

  void AddByteStream(const BYTE *aBuffer, UINT32 aSize)
  {
    m_InByteVector.Add(CInByte2());
    m_InByteBack = &m_InByteVector.Back();
    m_InByteBack->Init(aBuffer, aSize);
  }
  void DeleteByteStream()
  {
    m_InByteVector.DeleteBack();
    if (!m_InByteVector.IsEmpty())
      m_InByteBack = &m_InByteVector.Back();
  }

private:
  HRESULT FindAndReadSignature(IInStream *aStream, const UINT64 *aSearchHeaderSizeLimit); // S_FALSE means is not archive
  
  HRESULT ReadFileNames(CObjectVector<CFileItemInfo> &aFiles);
  
  HRESULT ReadBytes(IInStream *aStream, void *aData, UINT32 aSize, 
      UINT32 *aProcessedSize);
  HRESULT ReadBytes(void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  HRESULT SafeReadBytes(void *aData, UINT32 aSize);

  HRESULT SafeReadBytes2(void *aData, UINT32 aSize)
  {
    if (!m_InByteBack->ReadBytes(aData, aSize))
      return E_FAIL;
    return S_OK;
  }
  HRESULT SafeReadByte2(BYTE &aByte)
  {
    if (!m_InByteBack->ReadByte(aByte))
      return E_FAIL;
    return S_OK;
  }
  HRESULT SafeReadWideCharLE(wchar_t &aChar)
  {
    BYTE aByte1;
    if (!m_InByteBack->ReadByte(aByte1))
      return E_FAIL;
    BYTE aByte2;
    if (!m_InByteBack->ReadByte(aByte2))
      return E_FAIL;
    aChar = (int(aByte2) << 8) + aByte1;
    return S_OK;
  }

  HRESULT ReadNumber(UINT64 &aValue);
  
  HRESULT SkeepData(UINT64 aSize);
  HRESULT SkeepData();
  HRESULT WaitAttribute(BYTE anAttribute);

  HRESULT ReadArhiveProperties(CInArchiveInfo &anArchiveInfo);
  HRESULT GetNextFolderItem(CFolderItemInfo &anItemInfo);
  HRESULT ReadHashDigests(int aNumItems,
      CRecordVector<bool> &aDigestsDefined, CRecordVector<UINT32> &aDigests);
  
  HRESULT ReadPackInfo(
      UINT64 &aDataOffset,
      CRecordVector<UINT64> &aPackSizes,
      CRecordVector<bool> &aPackCRCsDefined,
      CRecordVector<UINT32> &aPackCRCs);
  
  HRESULT ReadUnPackInfo(
      const CObjectVector<CByteBuffer> *aDataVector,
      CObjectVector<CFolderItemInfo> &aFolders);
  
  HRESULT ReadSubStreamsInfo(
      const CObjectVector<CFolderItemInfo> &aFolders,
      CRecordVector<UINT64> &aNumUnPackStreamsInFolders,
      CRecordVector<UINT64> &anUnPackSizes,
      CRecordVector<bool> &aDigestsDefined, 
      CRecordVector<UINT32> &aDigests);

  HRESULT CInArchive::ReadStreamsInfo(
      const CObjectVector<CByteBuffer> *aDataVector,
      UINT64 &aDataOffset,
      CRecordVector<UINT64> &aPackSizes,
      CRecordVector<bool> &aPackCRCsDefined,
      CRecordVector<UINT32> &aPackCRCs,
      CObjectVector<CFolderItemInfo> &aFolders,
      CRecordVector<UINT64> &aNumUnPackStreamsInFolders,
      CRecordVector<UINT64> &anUnPackSizes,
      CRecordVector<bool> &aDigestsDefined, 
      CRecordVector<UINT32> &aDigests);



  HRESULT GetNextFileItem(CFileItemInfo &anItemInfo);
  HRESULT ReadBoolVector(UINT32 aNumItems, CBoolVector &aVector);
  HRESULT ReadBoolVector2(UINT32 aNumItems, CBoolVector &aVector);
  HRESULT ReadTime(const CObjectVector<CByteBuffer> &aDataVector,
      CObjectVector<CFileItemInfo> &aFiles, BYTE aType);
  HRESULT ReadHeader(CArchiveDatabaseEx &aDatabase);
public:
  HRESULT Open(IInStream *aStream, const UINT64 *aSearchHeaderSizeLimit); // S_FALSE means is not archive
  void Close();

  HRESULT ReadDatabase(CArchiveDatabaseEx &aDatabase);
  HRESULT CheckIntegrity();
};
  
}}
  
#endif
