// 7zIn.h

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
  UInt64 StartPosition;
  UInt64 StartPositionAfterHeader;
  UInt64 DataStartPosition;
  UInt64 DataStartPosition2;
  CRecordVector<UInt64> FileInfoPopIDs;
  void Clear()
  {
    FileInfoPopIDs.Clear();
  }
};


struct CArchiveDatabaseEx: public CArchiveDatabase
{
  CInArchiveInfo ArchiveInfo;
  CRecordVector<UInt64> PackStreamStartPositions;
  CRecordVector<UInt32> FolderStartPackStreamIndex;
  CRecordVector<UInt64> FolderStartFileIndex;
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

  void Fill()
  {
    FillFolderStartPackStream();
    FillStartPos();
    FillFolderStartFileIndex();
  }
  
  UInt64 GetFolderStreamPos(int folderIndex, int indexInFolder) const
  {
    return ArchiveInfo.DataStartPosition +
        PackStreamStartPositions[FolderStartPackStreamIndex[folderIndex] +
        indexInFolder];
  }
  
  UInt64 GetFolderFullPackSize(int folderIndex) const 
  {
    UInt32 packStreamIndex = FolderStartPackStreamIndex[folderIndex];
    const CFolder &folder = Folders[folderIndex];
    UInt64 size = 0;
    for (int i = 0; i < folder.PackStreams.Size(); i++)
      size += PackSizes[packStreamIndex + i];
    return size;
  }
  
  UInt64 GetFolderPackStreamSize(int folderIndex, int streamIndex) const 
  {
    return PackSizes[FolderStartPackStreamIndex[folderIndex] + streamIndex];
  }

  UInt64 GetFilePackSize(int fileIndex) const
  {
    int folderIndex = FileIndexToFolderIndexMap[fileIndex];
    if (folderIndex >= 0)
    {
      const CFolder &folderInfo = Folders[folderIndex];
      if (FolderStartFileIndex[folderIndex] == fileIndex)
        return GetFolderFullPackSize(folderIndex);
    }
    return 0;
  }
};

class CInByte2
{
  const Byte *_buffer;
  UInt32 _size;
  UInt32 _pos;
public:
  void Init(const Byte *buffer, UInt32 size)
  {
    _buffer = buffer;
    _size = size;
    _pos = 0;
  }
  bool ReadByte(Byte &b)
  {
    if(_pos >= _size)
      return false;
    b = _buffer[_pos++];
    return true;
  }
  void ReadBytes(void *data, UInt32 size, UInt32 &processedSize)
  {
    for(processedSize = 0; processedSize < size && _pos < _size; processedSize++)
      ((Byte *)data)[processedSize] = _buffer[_pos++];
  }
  
  bool ReadBytes(void *data, UInt32 size)
  {
    UInt32 processedSize;
    ReadBytes(data, size, processedSize);
    return (processedSize == size);
  }
  
  UInt32 GetProcessedSize() const { return _pos; }
};

class CStreamSwitch;
class CInArchive
{
  friend class CStreamSwitch;

  CMyComPtr<IInStream> _stream;
  #ifdef _7Z_VOL
  bool _finishSignature;
  #endif

  CObjectVector<CInByte2> _inByteVector;
  CInByte2 *_inByteBack;
 
  UInt64 _arhiveBeginStreamPosition;
  UInt64 _position;

  void AddByteStream(const Byte *buffer, UInt32 size)
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
  HRESULT FindAndReadSignature(IInStream *stream, const UInt64 *searchHeaderSizeLimit); // S_FALSE means is not archive
  #ifdef _7Z_VOL
  HRESULT FindFinishSignature(IInStream *stream, const UInt64 *searchHeaderSizeLimit); // S_FALSE means is not archive
  #endif
  
  HRESULT ReadFileNames(CObjectVector<CFileItem> &files);
  
  HRESULT ReadDirect(IInStream *stream, void *data, UInt32 size, 
      UInt32 *processedSize);
  HRESULT ReadDirect(void *data, UInt32 size, UInt32 *processedSize);
  HRESULT SafeReadDirect(void *data, UInt32 size);
  HRESULT SafeReadDirectByte(Byte &b);
  HRESULT SafeReadDirectUInt32(UInt32 &value);
  HRESULT SafeReadDirectUInt64(UInt64 &value);

  HRESULT ReadBytes(void *data, UInt32 size)
  {
    if (!_inByteBack->ReadBytes(data, size))
      return E_FAIL;
    return S_OK;
  }

  HRESULT ReadByte(Byte &b)
  {
    if (!_inByteBack->ReadByte(b))
      return E_FAIL;
    return S_OK;
  }

  HRESULT ReadWideCharLE(wchar_t &c)
  {
    Byte b1;
    if (!_inByteBack->ReadByte(b1))
      return E_FAIL;
    Byte b2;
    if (!_inByteBack->ReadByte(b2))
      return E_FAIL;
    c = (wchar_t(b2) << 8) + b1;
    return S_OK;
  }

  HRESULT ReadNumber(UInt64 &value);
  HRESULT ReadID(UInt64 &value) { return ReadNumber(value); }
  HRESULT ReadUInt32(UInt32 &value);
  HRESULT ReadUInt64(UInt64 &value);
  
  HRESULT SkeepData(UInt64 size);
  HRESULT SkeepData();
  HRESULT WaitAttribute(UInt64 attribute);

  HRESULT ReadArchiveProperties(CInArchiveInfo &archiveInfo);
  HRESULT GetNextFolderItem(CFolder &itemInfo);
  HRESULT ReadHashDigests(int numItems,
      CRecordVector<bool> &digestsDefined, CRecordVector<UInt32> &digests);
  
  HRESULT ReadPackInfo(
      UInt64 &dataOffset,
      CRecordVector<UInt64> &packSizes,
      CRecordVector<bool> &packCRCsDefined,
      CRecordVector<UInt32> &packCRCs);
  
  HRESULT ReadUnPackInfo(
      const CObjectVector<CByteBuffer> *dataVector,
      CObjectVector<CFolder> &folders);
  
  HRESULT ReadSubStreamsInfo(
      const CObjectVector<CFolder> &folders,
      CRecordVector<UInt64> &numUnPackStreamsInFolders,
      CRecordVector<UInt64> &unPackSizes,
      CRecordVector<bool> &digestsDefined, 
      CRecordVector<UInt32> &digests);

  HRESULT CInArchive::ReadStreamsInfo(
      const CObjectVector<CByteBuffer> *dataVector,
      UInt64 &dataOffset,
      CRecordVector<UInt64> &packSizes,
      CRecordVector<bool> &packCRCsDefined,
      CRecordVector<UInt32> &packCRCs,
      CObjectVector<CFolder> &folders,
      CRecordVector<UInt64> &numUnPackStreamsInFolders,
      CRecordVector<UInt64> &unPackSizes,
      CRecordVector<bool> &digestsDefined, 
      CRecordVector<UInt32> &digests);


  HRESULT GetNextFileItem(CFileItem &itemInfo);
  HRESULT ReadBoolVector(UInt32 numItems, CBoolVector &v);
  HRESULT ReadBoolVector2(UInt32 numItems, CBoolVector &v);
  HRESULT ReadTime(const CObjectVector<CByteBuffer> &dataVector,
      CObjectVector<CFileItem> &files, UInt64 type);
  HRESULT ReadAndDecodePackedStreams(UInt64 baseOffset, UInt64 &dataOffset,
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
  HRESULT Open(IInStream *stream, const UInt64 *searchHeaderSizeLimit); // S_FALSE means is not archive
  void Close();

  HRESULT ReadDatabase(CArchiveDatabaseEx &database 
      #ifndef _NO_CRYPTO
      ,ICryptoGetTextPassword *getTextPassword
      #endif
      );
};
  
}}
  
#endif
