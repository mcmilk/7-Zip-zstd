// 7zIn.h

#pragma once

#ifndef __7Z_IN_H
#define __7Z_IN_H

#include "../../IStream.h"
#include "../../IPassword.h"
#include "../../../Common/MyCom.h"
#include "../../Common/InBuffer.h"

#include "7zHeader.h"
#include "7zItem.h"
 
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
  CInArchiveException(CCauseType cause);
};

struct CInArchiveInfo
{
  CArchiveVersion Version;
  UINT64 StartPosition;
  UINT64 StartPositionAfterHeader;
  UINT64 DataStartPosition;
  UINT64 DataStartPosition2;
  CRecordVector<UINT64> FileInfoPopIDs;
  void Clear()
  {
    FileInfoPopIDs.Clear();
  }
};


struct CArchiveDatabaseEx: public CArchiveDatabase
{
  CInArchiveInfo ArchiveInfo;
  CRecordVector<UINT64> PackStreamStartPositions;
  CRecordVector<UINT32> FolderStartPackStreamIndex;
  CRecordVector<UINT64> FolderStartFileIndex;
  CRecordVector<int> FileIndexToFolderIndexMap;

  void Clear()
  {
    CArchiveDatabase::Clear();
    ArchiveInfo.Clear();
    PackStreamStartPositions.Clear();
    FolderStartPackStreamIndex.Clear();
    FolderStartFileIndex.Clear();
    FolderStartFileIndex.Clear();
  }

  void FillFolderStartPackStream();
  void FillStartPos();
  void FillFolderStartFileIndex();
  
  UINT64 GetFolderStreamPos(int folderIndex, int indexInFolder) const
  {
    return ArchiveInfo.DataStartPosition +
        PackStreamStartPositions[FolderStartPackStreamIndex[folderIndex] +
        indexInFolder];
  }
  
  UINT64 GetFolderFullPackSize(int folderIndex) const 
  {
    UINT32 packStreamIndex = FolderStartPackStreamIndex[folderIndex];
    const CFolder &folder = Folders[folderIndex];
    UINT64 size = 0;
    for (int i = 0; i < folder.PackStreams.Size(); i++)
      size += PackSizes[packStreamIndex + i];
    return size;
  }
  
  UINT64 GetFolderPackStreamSize(int folderIndex, int streamIndex) const 
  {
    return PackSizes[FolderStartPackStreamIndex[folderIndex] + streamIndex];
  }
};

class CInByte2
{
  const BYTE *_buffer;
  UINT32 _size;
  UINT32 _pos;
public:
  void Init(const BYTE *buffer, UINT32 size)
  {
    _buffer = buffer;
    _size = size;
    _pos = 0;
  }
  bool ReadByte(BYTE &b)
  {
    if(_pos >= _size)
      return false;
    b = _buffer[_pos++];
    return true;
  }
  void ReadBytes(void *data, UINT32 size, UINT32 &processedSize)
  {
    for(processedSize = 0; processedSize < size && _pos < _size; processedSize++)
      ((BYTE *)data)[processedSize] = _buffer[_pos++];
  }
  
  bool ReadBytes(void *data, UINT32 size)
  {
    UINT32 processedSize;
    ReadBytes(data, size, processedSize);
    return (processedSize == size);
  }
  
  UINT32 GetProcessedSize() const { return _pos; }
};

class CStreamSwitch;
class CInArchive
{
  friend class CStreamSwitch;

  CMyComPtr<IInStream> _stream;

  CObjectVector<CInByte2> _inByteVector;
  CInByte2 *_inByteBack;
 
  UINT64 _arhiveBeginStreamPosition;
  UINT64 _position;

  void AddByteStream(const BYTE *buffer, UINT32 size)
  {
    _inByteVector.Add(CInByte2());
    _inByteBack = &_inByteVector.Back();
    _inByteBack->Init(buffer, size);
  }
  
  void DeleteByteStream()
  {
    _inByteVector.DeleteBack();
    if (!_inByteVector.IsEmpty())
      _inByteBack = &_inByteVector.Back();
  }

private:
  HRESULT FindAndReadSignature(IInStream *stream, const UINT64 *searchHeaderSizeLimit); // S_FALSE means is not archive
  
  HRESULT ReadFileNames(CObjectVector<CFileItem> &files);
  
  HRESULT ReadBytes(IInStream *stream, void *data, UINT32 size, 
      UINT32 *processedSize);
  HRESULT ReadBytes(void *data, UINT32 size, UINT32 *processedSize);
  HRESULT SafeReadBytes(void *data, UINT32 size);

  HRESULT SafeReadBytes2(void *data, UINT32 size)
  {
    if (!_inByteBack->ReadBytes(data, size))
      return E_FAIL;
    return S_OK;
  }

  HRESULT SafeReadByte2(BYTE &b)
  {
    if (!_inByteBack->ReadByte(b))
      return E_FAIL;
    return S_OK;
  }

  HRESULT SafeReadWideCharLE(wchar_t &c)
  {
    BYTE b1;
    if (!_inByteBack->ReadByte(b1))
      return E_FAIL;
    BYTE b2;
    if (!_inByteBack->ReadByte(b2))
      return E_FAIL;
    c = (int(b2) << 8) + b1;
    return S_OK;
  }

  HRESULT ReadNumber(UINT64 &value);

  HRESULT ReadID(UINT64 &value)
  {
    return ReadNumber(value);
  }
  
  HRESULT SkeepData(UINT64 size);
  HRESULT SkeepData();
  HRESULT WaitAttribute(UINT64 attribute);

  HRESULT ReadArchiveProperties(CInArchiveInfo &archiveInfo);
  HRESULT GetNextFolderItem(CFolder &itemInfo);
  HRESULT ReadHashDigests(int numItems,
      CRecordVector<bool> &digestsDefined, CRecordVector<UINT32> &digests);
  
  HRESULT ReadPackInfo(
      UINT64 &dataOffset,
      CRecordVector<UINT64> &packSizes,
      CRecordVector<bool> &packCRCsDefined,
      CRecordVector<UINT32> &packCRCs);
  
  HRESULT ReadUnPackInfo(
      const CObjectVector<CByteBuffer> *dataVector,
      CObjectVector<CFolder> &folders);
  
  HRESULT ReadSubStreamsInfo(
      const CObjectVector<CFolder> &folders,
      CRecordVector<UINT64> &numUnPackStreamsInFolders,
      CRecordVector<UINT64> &unPackSizes,
      CRecordVector<bool> &digestsDefined, 
      CRecordVector<UINT32> &digests);

  HRESULT CInArchive::ReadStreamsInfo(
      const CObjectVector<CByteBuffer> *dataVector,
      UINT64 &dataOffset,
      CRecordVector<UINT64> &packSizes,
      CRecordVector<bool> &packCRCsDefined,
      CRecordVector<UINT32> &packCRCs,
      CObjectVector<CFolder> &folders,
      CRecordVector<UINT64> &numUnPackStreamsInFolders,
      CRecordVector<UINT64> &unPackSizes,
      CRecordVector<bool> &digestsDefined, 
      CRecordVector<UINT32> &digests);



  HRESULT GetNextFileItem(CFileItem &itemInfo);
  HRESULT ReadBoolVector(UINT32 numItems, CBoolVector &vector);
  HRESULT ReadBoolVector2(UINT32 numItems, CBoolVector &vector);
  HRESULT ReadTime(const CObjectVector<CByteBuffer> &dataVector,
      CObjectVector<CFileItem> &files, UINT64 type);
  HRESULT ReadAndDecodePackedStreams(UINT64 baseOffset, UINT64 &dataOffset,
      CObjectVector<CByteBuffer> &dataVector
      #ifndef _NO_CRYPTO
      , ICryptoGetTextPassword *getTextPassword
      #endif
      );
  HRESULT ReadHeader(CArchiveDatabaseEx &database
      #ifndef _NO_CRYPTO
      ,ICryptoGetTextPassword *getTextPassword
      #endif
      );
public:
  HRESULT Open(IInStream *stream, const UINT64 *searchHeaderSizeLimit); // S_FALSE means is not archive
  void Close();

  HRESULT ReadDatabase(CArchiveDatabaseEx &database 
      #ifndef _NO_CRYPTO
      ,ICryptoGetTextPassword *getTextPassword
      #endif
      );
  HRESULT CheckIntegrity();
};
  
}}
  
#endif
