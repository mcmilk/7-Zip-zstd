// 7z/OutEngine.h

#pragma once

#ifndef __7Z_OUTENGINE_H
#define __7Z_OUTENGINE_H

#include "Header.h"
#include "ItemInfo.h"

#include "Stream/OutByte.h"

#include "Interface/IInOutStreams.h"

#include "CompressionMethod.h"
#include "Encode.h"

namespace NArchive {
namespace N7z {

class CWriteBufferLoc
{
  BYTE *m_Data;
  UINT32 m_Size;
  UINT32 m_Pos;
public:
  CWriteBufferLoc(): m_Size(0), m_Pos(0) {}
  void Init(BYTE *aData, UINT32 aSize)  
  { 
    m_Pos = 0;
    m_Data = aData;
    m_Size = aSize; 
  }
  HRESULT Write(const void *aData, UINT32 aSize)
  {
    if (m_Pos + aSize > m_Size)
      return E_FAIL;
    memmove(m_Data + m_Pos, aData, aSize);
    m_Pos += aSize;
    return S_OK; 
  }
};


class COutArchive
{
  UINT64 m_PrefixHeaderPos;

  HRESULT WriteBytes(const void *aData, UINT32 aSize);
  HRESULT WriteBytes2(const void *aData, UINT32 aSize);
  HRESULT WriteBytes2(const CByteBuffer &aData);
  HRESULT WriteByte2(BYTE aByte);
  HRESULT WriteNumber(UINT64 aValue);
  HRESULT WriteFolderHeader(const CFolderItemInfo &anItemInfo);
  HRESULT WriteFileHeader(const CFileItemInfo &anItemInfo);
  HRESULT WriteBoolVector(const CBoolVector &aVector);
  HRESULT WriteHashDigests(
      const CRecordVector<bool> &aDigestsDefined,
      const CRecordVector<UINT32> &aHashDigests);

  HRESULT WritePackInfo(
      UINT64 aDataOffset,
      const CRecordVector<UINT64> &aPackSizes,
      const CRecordVector<bool> &aPackCRCsDefined,
      const CRecordVector<UINT32> &aPackCRCs);

  HRESULT WriteUnPackInfo(
      bool anExternalFolders,
      UINT64 anExternalFoldersStreamIndex,
      const CObjectVector<CFolderItemInfo> &aFolders);

  HRESULT WriteSubStreamsInfo(
      const CObjectVector<CFolderItemInfo> &aFolders,
      const CRecordVector<UINT64> &aNumUnPackStreamsInFolders,
      const CRecordVector<UINT64> &anUnPackSizes,
      const CRecordVector<bool> &aDigestsDefined,
      const CRecordVector<UINT32> &aHashDigests);

  HRESULT WriteStreamsInfo(
      UINT64 aDataOffset,
      const CRecordVector<UINT64> &aPackSizes,
      const CRecordVector<bool> &aPackCRCsDefined,
      const CRecordVector<UINT32> &aPackCRCs,
      bool anExternalFolders,
      UINT64 anExternalFoldersStreamIndex,
      const CObjectVector<CFolderItemInfo> &aFolders,
      const CRecordVector<UINT64> &aNumUnPackStreamsInFolders,
      const CRecordVector<UINT64> &anUnPackSizes,
      const CRecordVector<bool> &aDigestsDefined,
      const CRecordVector<UINT32> &aHashDigests);


  HRESULT WriteTime(const CObjectVector<CFileItemInfo> &aFiles, BYTE aType,
      bool anIsExternal, int anExternalDataIndex);

  HRESULT EncodeStream(CEncoder &anEncoder, const CByteBuffer &aData, 
      CRecordVector<UINT64> &aPackSizes, CObjectVector<CFolderItemInfo> &aFolders);
  HRESULT WriteHeader(const CArchiveDatabase &aDatabase,
      const CCompressionMethodMode *anOptions, UINT64 &aHeaderOffset);
public:
  CComPtr<IOutStream> m_Stream;
  bool m_MainMode;
  bool m_CountMode;
  UINT32 m_CountSize;
  NStream::COutByte m_OutByte;
  CWriteBufferLoc m_OutByte2;
  CCRC m_CRC;

  HRESULT Create(IOutStream *aStream);
  void Close();
  HRESULT SkeepPrefixArchiveHeader();
  HRESULT WriteDatabase(const CArchiveDatabase &aDatabase,
      const CCompressionMethodMode *anOptions);
};

}}

#endif
