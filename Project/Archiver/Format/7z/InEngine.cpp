// 7z/InEngine.cpp

#include "StdAfx.h"

#include "Windows/Defs.h"
#include "InEngine.h"
#include "RegistryInfo.h"
#include "Decode.h"

#include "../../../Compress/Interface/CompressInterface.h"

#include "Interface/StreamObjects.h"
#include "Interface/LimitedStreams.h"

namespace NArchive {
namespace N7z {

class CStreamSwitch
{
  CInArchive *_archive;
  bool _needRemove;
  void Remove();
public:
  CStreamSwitch(): _needRemove(false) {}
  ~CStreamSwitch() { Remove(); }
  void Set(CInArchive *archive, const BYTE *data, UINT32 size);
  void Set(CInArchive *archive, const CByteBuffer &byteBuffer);
  HRESULT Set(CInArchive *archive, const CObjectVector<CByteBuffer> *dataVector);
};

void CStreamSwitch::Remove()
{
  if (_needRemove)
  {
    _archive->DeleteByteStream();
    _needRemove = false;
  }
}

void CStreamSwitch::Set(CInArchive *archive, const BYTE *data, UINT32 size)
{
  Remove();
  _archive = archive;
  _archive->AddByteStream(data, size);
  _needRemove = true;
}

void CStreamSwitch::Set(CInArchive *archive, const CByteBuffer &byteBuffer)
{
  Set(archive, byteBuffer, byteBuffer.GetCapacity());
}

HRESULT CStreamSwitch::Set(CInArchive *archive, const CObjectVector<CByteBuffer> *dataVector)
{
  Remove();
  BYTE external;
  RETURN_IF_NOT_S_OK(archive->SafeReadByte2(external));
  if (external != 0)
  {
    UINT64 dataIndex;
    RETURN_IF_NOT_S_OK(archive->ReadNumber(dataIndex));
    Set(archive, (*dataVector)[dataIndex]);
  }
  return S_OK;
}

  
CInArchiveException::CInArchiveException(CCauseType cause):
  Cause(cause)
{}

HRESULT CInArchive::ReadBytes(IInStream *stream, void *data, UINT32 size, 
    UINT32 *processedSize)
{
  UINT32 realProcessedSize;
  HRESULT result = stream->Read(data, size, &realProcessedSize);
  if(processedSize != NULL)
    *processedSize = realProcessedSize;
  _position += realProcessedSize;
  return result;
}

HRESULT CInArchive::ReadBytes(void *data, UINT32 size, UINT32 *processedSize)
{
  return ReadBytes(_stream, data, size, processedSize);
}

HRESULT CInArchive::SafeReadBytes(void *data, UINT32 size)
{
  UINT32 realProcessedSize;
  RETURN_IF_NOT_S_OK(ReadBytes(data, size, &realProcessedSize));
  if (realProcessedSize != size)
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
  return S_OK;
}

HRESULT CInArchive::ReadNumber(UINT64 &value)
{
  BYTE firstByte;
  RETURN_IF_NOT_S_OK(SafeReadByte2(firstByte));
  BYTE mask = 0x80;
  for (int i = 0; i < 8; i++)
  {
    if ((firstByte & mask) == 0)
    {
      value = 0;
      RETURN_IF_NOT_S_OK(SafeReadBytes2(&value, i));
      UINT64 highPart = firstByte & (mask - 1);
      value += (highPart << (i * 8));
      return S_OK;
    }
    mask >>= 1;
  }
  return SafeReadBytes2(&value, 8);
}

static inline bool TestSignatureCandidate(const void *testBytes)
{
  // return (memcmp(testBytes, kSignature, kSignatureSize) == 0);
  for (UINT32 i = 0; i < kSignatureSize; i++)
    if (((const BYTE *)testBytes)[i] != kSignature[i])
      return false;
  return true;
}

HRESULT CInArchive::FindAndReadSignature(IInStream *stream, const UINT64 *searchHeaderSizeLimit)
{
  _position = _arhiveBeginStreamPosition;
  RETURN_IF_NOT_S_OK(stream->Seek(_arhiveBeginStreamPosition, STREAM_SEEK_SET, NULL));

  BYTE signature[kSignatureSize];
  UINT32 processedSize; 
  RETURN_IF_NOT_S_OK(ReadBytes(stream, signature, kSignatureSize, &processedSize));
  if(processedSize != kSignatureSize)
    return S_FALSE;
  if (TestSignatureCandidate(signature))
    return S_OK;

  CByteBuffer byteBuffer;
  static const UINT32 kSearchSignatureBufferSize = 0x10000;
  byteBuffer.SetCapacity(kSearchSignatureBufferSize);
  BYTE *buffer = byteBuffer;
  UINT32 numBytesPrev = kSignatureSize - 1;
  memmove(buffer, signature + 1, numBytesPrev);
  UINT64 curTestPos = _arhiveBeginStreamPosition + 1;
  while(true)
  {
    if (searchHeaderSizeLimit != NULL)
      if (curTestPos - _arhiveBeginStreamPosition > *searchHeaderSizeLimit)
        return false;
    UINT32 numReadBytes = kSearchSignatureBufferSize - numBytesPrev;
    RETURN_IF_NOT_S_OK(ReadBytes(stream, buffer + numBytesPrev, numReadBytes, &processedSize));
    UINT32 numBytesInBuffer = numBytesPrev + processedSize;
    if (numBytesInBuffer < kSignatureSize)
      return S_FALSE;
    UINT32 numTests = numBytesInBuffer - kSignatureSize + 1;
    for(UINT32 pos = 0; pos < numTests; pos++, curTestPos++)
    { 
      if (TestSignatureCandidate(buffer + pos))
      {
        _arhiveBeginStreamPosition = curTestPos;
        _position = curTestPos + kSignatureSize;
        return stream->Seek(_position, STREAM_SEEK_SET, NULL);
      }
    }
    numBytesPrev = numBytesInBuffer - numTests;
    memmove(buffer, buffer + numTests, numBytesPrev);
  }
}

// S_FALSE means that file is not archive
HRESULT CInArchive::Open(IInStream *stream, const UINT64 *searchHeaderSizeLimit)
{
  Close();
  RETURN_IF_NOT_S_OK(stream->Seek(0, STREAM_SEEK_CUR, &_arhiveBeginStreamPosition))
  _position = _arhiveBeginStreamPosition;
  RETURN_IF_NOT_S_OK(FindAndReadSignature(stream, searchHeaderSizeLimit));
  _stream = stream;
  return S_OK;
}
  
void CInArchive::Close()
{
  _stream.Release();
}

HRESULT CInArchive::SkeepData(UINT64 size)
{
  for (UINT64 i = 0; i < size; i++)
  {
    BYTE temp;
    RETURN_IF_NOT_S_OK(SafeReadByte2(temp));
  }
  return S_OK;
}

HRESULT CInArchive::SkeepData()
{
  UINT64 size;
  RETURN_IF_NOT_S_OK(ReadNumber(size));
  return SkeepData(size);
}

HRESULT CInArchive::ReadArhiveProperties(CInArchiveInfo &archiveInfo)
{
  while(true)
  {
    BYTE type;
    RETURN_IF_NOT_S_OK(SafeReadByte2(type));
    if (type == NID::kEnd)
      break;
    SkeepData();
  }
  return S_OK;
}

HRESULT CInArchive::GetNextFolderItem(CFolderItemInfo &itemInfo)
{
  UINT64 numCoders;
  RETURN_IF_NOT_S_OK(ReadNumber(numCoders));

  itemInfo.CodersInfo.Clear();
  itemInfo.CodersInfo.Reserve(numCoders);
  UINT32 numInStreams = 0;
  UINT32 numOutStreams = 0;
  for (int i = 0; i < numCoders; i++)
  {
    itemInfo.CodersInfo.Add(CCoderInfo());
    CCoderInfo &coderInfo = itemInfo.CodersInfo.Back();

    BYTE mainByte;
    RETURN_IF_NOT_S_OK(SafeReadByte2(mainByte));
    coderInfo.DecompressionMethod.IDSize = mainByte & 0xF;
    bool isComplex = (mainByte & 0x10) != 0;
    bool tereAreProperties = (mainByte & 0x20) != 0;
    RETURN_IF_NOT_S_OK(SafeReadBytes2(&coderInfo.DecompressionMethod.ID[0], 
        coderInfo.DecompressionMethod.IDSize));
    if (isComplex)
    {
      RETURN_IF_NOT_S_OK(ReadNumber(coderInfo.NumInStreams));
      RETURN_IF_NOT_S_OK(ReadNumber(coderInfo.NumOutStreams));
    }
    else
    {
      coderInfo.NumInStreams = 1;
      coderInfo.NumOutStreams = 1;
    }
    numInStreams += coderInfo.NumInStreams;
    numOutStreams += coderInfo.NumOutStreams;
    UINT64 propertiesSize = 0;
    if (tereAreProperties)
    {
      RETURN_IF_NOT_S_OK(ReadNumber(propertiesSize));
    }
    coderInfo.Properties.SetCapacity(propertiesSize);
    RETURN_IF_NOT_S_OK(SafeReadBytes2((BYTE *)coderInfo.Properties, propertiesSize));
  }

  UINT64 numBindPairs;
  // RETURN_IF_NOT_S_OK(ReadNumber(numBindPairs));
  numBindPairs = numOutStreams - 1;
  itemInfo.BindPairs.Clear();
  itemInfo.BindPairs.Reserve(numBindPairs);
  for (i = 0; i < numBindPairs; i++)
  {
    CBindPair bindPair;
    RETURN_IF_NOT_S_OK(ReadNumber(bindPair.InIndex));
    RETURN_IF_NOT_S_OK(ReadNumber(bindPair.OutIndex)); 
    itemInfo.BindPairs.Add(bindPair);
  }

  UINT32 numPackedStreams = numInStreams - numBindPairs;
  itemInfo.PackStreams.Reserve(numPackedStreams);
  if (numPackedStreams == 1)
  {
    for (int j = 0; j < numInStreams; j++)
      if (itemInfo.FindBindPairForInStream(j) < 0)
      {
        CPackStreamInfo packStreamInfo;
        packStreamInfo.Index = j;
        itemInfo.PackStreams.Add(packStreamInfo);
        break;
      }
  }
  else
    for(i = 0; i < numPackedStreams; i++)
    {
      CPackStreamInfo packStreamInfo;
      RETURN_IF_NOT_S_OK(ReadNumber(packStreamInfo.Index));
      itemInfo.PackStreams.Add(packStreamInfo);
    }

  return S_OK;
}

HRESULT CInArchive::WaitAttribute(BYTE attribute)
{
  while(true)
  {
    BYTE type;
    RETURN_IF_NOT_S_OK(SafeReadByte2(type));
    if (type == attribute)
      return S_OK;
    if (type == NID::kEnd)
      return S_FALSE;
    RETURN_IF_NOT_S_OK(SkeepData());
  }
}

HRESULT CInArchive::ReadHashDigests(int numItems,
    CRecordVector<bool> &digestsDefined, 
    CRecordVector<UINT32> &digests)
{
  RETURN_IF_NOT_S_OK(ReadBoolVector2(numItems, digestsDefined));
  digests.Clear();
  digests.Reserve(numItems);
  for(int i = 0; i < numItems; i++)
  {
    UINT32 crc;
    if (digestsDefined[i])
      RETURN_IF_NOT_S_OK(SafeReadBytes2(&crc, sizeof(crc)));
    digests.Add(crc);
  }
  return S_OK;
}

HRESULT CInArchive::ReadPackInfo(
    UINT64 &dataOffset,
    CRecordVector<UINT64> &packSizes,
    CRecordVector<bool> &packCRCsDefined,
    CRecordVector<UINT32> &packCRCs)
{
  RETURN_IF_NOT_S_OK(ReadNumber(dataOffset));
  UINT64 numPackStreams;
  RETURN_IF_NOT_S_OK(ReadNumber(numPackStreams));

  RETURN_IF_NOT_S_OK(WaitAttribute(NID::kSize));
  packSizes.Clear();
  packSizes.Reserve(numPackStreams);
  for(UINT64 i = 0; i < numPackStreams; i++)
  {
    UINT64 size;
    RETURN_IF_NOT_S_OK(ReadNumber(size));
    packSizes.Add(size);
  }

  BYTE type;
  while(true)
  {
    RETURN_IF_NOT_S_OK(SafeReadByte2(type));
    if (type == NID::kEnd)
      break;
    if (type == NID::kCRC)
    {
      RETURN_IF_NOT_S_OK(ReadHashDigests(
          numPackStreams, packCRCsDefined, packCRCs)); 
      continue;
    }
    RETURN_IF_NOT_S_OK(SkeepData());
  }
  if (packCRCsDefined.IsEmpty())
  {
    packCRCsDefined.Reserve(numPackStreams);
    packCRCsDefined.Clear();
    packCRCs.Reserve(numPackStreams);
    packCRCs.Clear();
    for(UINT64 i = 0; i < numPackStreams; i++)
    {
      packCRCsDefined.Add(false);
      packCRCs.Add(0);
    }
  }
  return S_OK;
}

HRESULT CInArchive::ReadUnPackInfo(
    const CObjectVector<CByteBuffer> *dataVector,
    CObjectVector<CFolderItemInfo> &folders)
{
  RETURN_IF_NOT_S_OK(WaitAttribute(NID::kFolder));
  UINT64 numFolders;
  RETURN_IF_NOT_S_OK(ReadNumber(numFolders));

  {
    CStreamSwitch streamSwitch;
    RETURN_IF_NOT_S_OK(streamSwitch.Set(this, dataVector));
    folders.Clear();
    folders.Reserve(numFolders);
    for(UINT64 i = 0; i < numFolders; i++)
    {
      folders.Add(CFolderItemInfo());
      RETURN_IF_NOT_S_OK(GetNextFolderItem(folders.Back()));
    }
  }

  RETURN_IF_NOT_S_OK(WaitAttribute(NID::kCodersUnPackSize));

  for(UINT64 i = 0; i < numFolders; i++)
  {
    CFolderItemInfo &folder = folders[i];
    int numOutStreams = folder.GetNumOutStreams();
    folder.UnPackSizes.Reserve(numOutStreams);
    for(int j = 0; j < numOutStreams; j++)
    {
      UINT64 unPackSize;
      RETURN_IF_NOT_S_OK(ReadNumber(unPackSize));
      folder.UnPackSizes.Add(unPackSize);
    }
  }

  while(true)
  {
    BYTE type;
    RETURN_IF_NOT_S_OK(SafeReadByte2(type));
    if (type == NID::kEnd)
      return S_OK;
    if (type == NID::kCRC)
    {
      CRecordVector<bool> crcsDefined;
      CRecordVector<UINT32> crcs;
      RETURN_IF_NOT_S_OK(ReadHashDigests(numFolders, crcsDefined, crcs)); 
      for(i = 0; i < numFolders; i++)
      {
        CFolderItemInfo &folder = folders[i];
        folder.UnPackCRCDefined = crcsDefined[i];
        folder.UnPackCRC = crcs[i];
      }
      continue;
    }
    RETURN_IF_NOT_S_OK(SkeepData());
  }
}

HRESULT CInArchive::ReadSubStreamsInfo(
    const CObjectVector<CFolderItemInfo> &folders,
    CRecordVector<UINT64> &numUnPackStreamsInFolders,
    CRecordVector<UINT64> &unPackSizes,
    CRecordVector<bool> &digestsDefined, 
    CRecordVector<UINT32> &digests)
{
  numUnPackStreamsInFolders.Clear();
  numUnPackStreamsInFolders.Reserve(folders.Size());
  BYTE type;
  while(true)
  {
    RETURN_IF_NOT_S_OK(SafeReadByte2(type));
    if (type == NID::kNumUnPackStream)
    {
      for(int i = 0; i < folders.Size(); i++)
      {
        UINT64 value;
        RETURN_IF_NOT_S_OK(ReadNumber(value));
        numUnPackStreamsInFolders.Add(value);
      }
      continue;
    }
    if (type == NID::kCRC || type == NID::kSize)
      break;
    if (type == NID::kEnd)
      break;
    RETURN_IF_NOT_S_OK(SkeepData());
  }

  if (numUnPackStreamsInFolders.IsEmpty())
    for(int i = 0; i < folders.Size(); i++)
      numUnPackStreamsInFolders.Add(1);

  for(int i = 0; i < numUnPackStreamsInFolders.Size(); i++)
  {
    UINT64 sum = 0;
    for (UINT64 j = 1; j < numUnPackStreamsInFolders[i]; j++)
    {
      UINT64 size;
      if (type == NID::kSize)
      {
        RETURN_IF_NOT_S_OK(ReadNumber(size));
        unPackSizes.Add(size);
        sum += size;
      }
    }
    unPackSizes.Add(folders[i].GetUnPackSize() - sum);
  }
  if (type == NID::kSize)
  {
    RETURN_IF_NOT_S_OK(SafeReadByte2(type));
  }

  int numDigests = 0;
  int numDigestsTotal = 0;
  for(i = 0; i < folders.Size(); i++)
  {
    int numSubstreams = numUnPackStreamsInFolders[i];
    if (numSubstreams != 1 || !folders[i].UnPackCRCDefined)
      numDigests += numSubstreams;
    numDigestsTotal += numSubstreams;
  }

  while(true)
  {
    if (type == NID::kCRC)
    {
      CRecordVector<bool> digestsDefined2; 
      CRecordVector<UINT32> digests2;
      RETURN_IF_NOT_S_OK(ReadHashDigests(numDigests, digestsDefined2, digests2));
      int digestIndex = 0;
      for (i = 0; i < folders.Size(); i++)
      {
        int numSubstreams = numUnPackStreamsInFolders[i];
        const CFolderItemInfo &folder = folders[i];
        if (numSubstreams == 1 && folder.UnPackCRCDefined)
        {
          digestsDefined.Add(true);
          digests.Add(folder.UnPackCRC);
        }
        else
          for (int j = 0; j < numSubstreams; j++, digestIndex++)
          {
            digestsDefined.Add(digestsDefined2[digestIndex]);
            digests.Add(digests2[digestIndex]);
          }
      }
    }
    else if (type == NID::kEnd)
    {
      if (digestsDefined.IsEmpty())
      {
        digestsDefined.Clear();
        digests.Clear();
        for (int i = 0; i < numDigestsTotal; i++)
        {
          digestsDefined.Add(false);
          digests.Add(0);
        }
      }
      return S_OK;
    }
    else
    {
      RETURN_IF_NOT_S_OK(SkeepData());
    }
    RETURN_IF_NOT_S_OK(SafeReadByte2(type));
  }
}


HRESULT CInArchive::ReadStreamsInfo(
    const CObjectVector<CByteBuffer> *dataVector,
    UINT64 &dataOffset,
    CRecordVector<UINT64> &packSizes,
    CRecordVector<bool> &packCRCsDefined,
    CRecordVector<UINT32> &packCRCs,
    CObjectVector<CFolderItemInfo> &folders,
    CRecordVector<UINT64> &numUnPackStreamsInFolders,
    CRecordVector<UINT64> &unPackSizes,
    CRecordVector<bool> &digestsDefined, 
    CRecordVector<UINT32> &digests)
{
  while(true)
  {
    BYTE type;
    RETURN_IF_NOT_S_OK(SafeReadByte2(type));
    switch(type)
    {
      case NID::kEnd:
        return S_OK;
      case NID::kPackInfo:
      {
        RETURN_IF_NOT_S_OK(ReadPackInfo(dataOffset, packSizes,
            packCRCsDefined, packCRCs));
        break;
      }
      case NID::kUnPackInfo:
      {
        RETURN_IF_NOT_S_OK(ReadUnPackInfo(dataVector, folders));
        break;
      }
      case NID::kSubStreamsInfo:
      {
        RETURN_IF_NOT_S_OK(ReadSubStreamsInfo(folders, numUnPackStreamsInFolders,
          unPackSizes, digestsDefined, digests));
        break;
      }
    }
  }
}


HRESULT CInArchive::ReadFileNames(CObjectVector<CFileItemInfo> &files)
{
  // UINT32 pos = 0;
  for(int i = 0; i < files.Size(); i++)
  {
    UString &name = files[i].Name;
    name.Empty();
    while (true)
    {
      // if (pos >= size)
      //   return S_FALSE;
      wchar_t c;
      RETURN_IF_NOT_S_OK(SafeReadWideCharLE(c));
      // pos++;
      if (c == '\0')
        break;
      name += c;
    }
  }
  // if (pos != size)
  //   return S_FALSE;
  return S_OK;
}

HRESULT CInArchive::CheckIntegrity()
{
  return S_OK;
}


HRESULT CInArchive::ReadBoolVector(UINT32 numItems, CBoolVector &vector)
{
  vector.Clear();
  vector.Reserve(numItems);
  BYTE b;
  BYTE mask = 0;
  for(UINT32 i = 0; i < numItems; i++)
  {
    if (mask == 0)
    {
      RETURN_IF_NOT_S_OK(SafeReadBytes2(&b, 1));
      mask = 0x80;
    }
    vector.Add((b & mask) != 0);
    mask >>= 1;
  }
  return S_OK;
}

HRESULT CInArchive::ReadBoolVector2(UINT32 numItems, CBoolVector &vector)
{
  BYTE allAreDefinedFlag;
  RETURN_IF_NOT_S_OK(SafeReadByte2(allAreDefinedFlag));
  if (allAreDefinedFlag == 0)
    return ReadBoolVector(numItems, vector);
  vector.Clear();
  vector.Reserve(numItems);
  for (int j = 0; j < numItems; j++)
    vector.Add(true);
  return S_OK;
}

HRESULT CInArchive::ReadTime(const CObjectVector<CByteBuffer> &dataVector,
    CObjectVector<CFileItemInfo> &files, BYTE type)
{
  CBoolVector boolVector;
  RETURN_IF_NOT_S_OK(ReadBoolVector2(files.Size(), boolVector))

  CStreamSwitch streamSwitch;
  RETURN_IF_NOT_S_OK(streamSwitch.Set(this, &dataVector));

  for(int i = 0; i < files.Size(); i++)
  {
    CFileItemInfo &itemInfo = files[i];
    CArchiveFileTime fileTime;
    bool defined = boolVector[i];
    if (defined)
    {
      RETURN_IF_NOT_S_OK(SafeReadBytes2(&fileTime, sizeof(fileTime)));
    }
    switch(type)
    {
      case NID::kCreationTime:
        itemInfo.IsCreationTimeDefined= defined;
        if (defined)
          itemInfo.CreationTime = fileTime;
        break;
      case NID::kLastWriteTime:
        itemInfo.IsLastWriteTimeDefined = defined;
        if (defined)
          itemInfo.LastWriteTime = fileTime;
        break;
      case NID::kLastAccessTime:
        itemInfo.IsLastAccessTimeDefined = defined;
        if (defined)
          itemInfo.LastAccessTime = fileTime;
        break;
    }
  }
  return S_OK;
}

HRESULT CInArchive::ReadHeader(CArchiveDatabaseEx &database)
{
  database.Clear();

  BYTE type;
  RETURN_IF_NOT_S_OK(SafeReadByte2(type));

  if (type == NID::kArchiveProperties)
  {
    RETURN_IF_NOT_S_OK(ReadArhiveProperties(database.ArchiveInfo));
    RETURN_IF_NOT_S_OK(SafeReadByte2(type));
  }
 
  CObjectVector<CByteBuffer> dataVector;
  
  if (type == NID::kAdditionalStreamsInfo)
  {
    CRecordVector<UINT64> packSizes;
    CRecordVector<bool> packCRCsDefined;
    CRecordVector<UINT32> packCRCs;
    CObjectVector<CFolderItemInfo> folders;

    CRecordVector<UINT64> numUnPackStreamsInFolders;
    CRecordVector<UINT64> unPackSizes;
    CRecordVector<bool> digestsDefined;
    CRecordVector<UINT32> digests;

    RETURN_IF_NOT_S_OK(ReadStreamsInfo(NULL, 
        database.ArchiveInfo.DataStartPosition2,
        packSizes, 
        packCRCsDefined, 
        packCRCs, 
        folders,
        numUnPackStreamsInFolders,
        unPackSizes,
        digestsDefined, 
        digests));

    database.ArchiveInfo.DataStartPosition2 += database.ArchiveInfo.StartPositionAfterHeader;

    UINT32 packIndex = 0;
    CDecoder decoder;
    UINT64 dataStartPos = database.ArchiveInfo.DataStartPosition2;
    for(int i = 0; i < folders.Size(); i++)
    {
      const CFolderItemInfo &folder = folders[i];
      dataVector.Add(CByteBuffer());
      CByteBuffer &data = dataVector.Back();
      UINT64 unPackSize = folder.GetUnPackSize();
      data.SetCapacity(unPackSize);
      
      CComObjectNoLock<CSequentialOutStreamImp2> *outStreamSpec = 
        new  CComObjectNoLock<CSequentialOutStreamImp2>;
      CComPtr<ISequentialOutStream> outStream = outStreamSpec;
      outStreamSpec->Init(data, unPackSize);
      
      RETURN_IF_NOT_S_OK(decoder.Decode(_stream, dataStartPos, 
          &packSizes[packIndex], folder, outStream, NULL));
      
      if (folder.UnPackCRCDefined)
        if (!CCRC::VerifyDigest(folder.UnPackCRC, data, unPackSize))
          throw CInArchiveException(CInArchiveException::kIncorrectHeader);
      for (int j = 0; j < folder.PackStreams.Size(); j++)
        dataStartPos += packSizes[packIndex++];
    }
    RETURN_IF_NOT_S_OK(SafeReadByte2(type));
  }

  CRecordVector<UINT64> unPackSizes;
  CRecordVector<bool> digestsDefined;
  CRecordVector<UINT32> digests;
  
  if (type == NID::kMainStreamsInfo)
  {
    RETURN_IF_NOT_S_OK(ReadStreamsInfo(&dataVector,
        database.ArchiveInfo.DataStartPosition,
        database.PackSizes, 
        database.PackCRCsDefined, 
        database.PackCRCs, 
        database.Folders,
        database.NumUnPackStreamsVector,
        unPackSizes,
        digestsDefined,
        digests));
    database.ArchiveInfo.DataStartPosition += database.ArchiveInfo.StartPositionAfterHeader;
    RETURN_IF_NOT_S_OK(SafeReadByte2(type));
  }
  else
  {
    for(int i = 0; i < database.Folders.Size(); i++)
    {
      database.NumUnPackStreamsVector.Add(1);
      CFolderItemInfo &folder = database.Folders[i];
      unPackSizes.Add(folder.GetUnPackSize());
      digestsDefined.Add(folder.UnPackCRCDefined);
      digests.Add(folder.UnPackCRC);
    }
  }

  UINT64 numUnPackStreamsTotal = 0;

  database.Files.Clear();

  if (type == NID::kEnd)
    return S_OK;
  if (type != NID::kFilesInfo)
    throw CInArchiveException(CInArchiveException::kIncorrectHeader);
  
  UINT64 numFiles;
  RETURN_IF_NOT_S_OK(ReadNumber(numFiles));
  database.Files.Reserve(numFiles);
  for(UINT64 i = 0; i < numFiles; i++)
    database.Files.Add(CFileItemInfo());

  // int sizePrev = -1;
  // int posPrev = 0;

  database.ArchiveInfo.FileInfoPopIDs.Add(NID::kSize);
  if (!database.PackSizes.IsEmpty())
    database.ArchiveInfo.FileInfoPopIDs.Add(NID::kPackInfo);
  if (numFiles > 0)
  {
    if (!digests.IsEmpty())
    { 
      database.ArchiveInfo.FileInfoPopIDs.Add(NID::kCRC);
    }
  }

  CBoolVector emptyStreamVector;
  emptyStreamVector.Reserve(numFiles);
  for(i = 0; i < numFiles; i++)
    emptyStreamVector.Add(false);
  CBoolVector emptyFileVector;
  CBoolVector antiFileVector;
  UINT32 numEmptyStreams = 0;

  while(true)
  {
    /*
    if (sizePrev >= 0)
      if (sizePrev != _inByteBack->GetProcessedSize() - posPrev)
        throw 2;
    */
    BYTE type;
    RETURN_IF_NOT_S_OK(SafeReadByte2(type));
    if (type == NID::kEnd)
      break;
    UINT64 size;
    RETURN_IF_NOT_S_OK(ReadNumber(size));
    
    // sizePrev = size;
    // posPrev = _inByteBack->GetProcessedSize();

    database.ArchiveInfo.FileInfoPopIDs.Add(type);
    switch(type)
    {
      case NID::kName:
      {
        CStreamSwitch streamSwitch;
        RETURN_IF_NOT_S_OK(streamSwitch.Set(this, &dataVector));
        RETURN_IF_NOT_S_OK(ReadFileNames(database.Files))
        break;
      }
      case NID::kWinAttributes:
      {
        CBoolVector boolVector;
        RETURN_IF_NOT_S_OK(ReadBoolVector2(database.Files.Size(), boolVector))
        CStreamSwitch streamSwitch;
        RETURN_IF_NOT_S_OK(streamSwitch.Set(this, &dataVector));
        for(i = 0; i < numFiles; i++)
        {
          CFileItemInfo &itemInfo = database.Files[i];
          if (itemInfo.AreAttributesDefined = boolVector[i])
          {
            RETURN_IF_NOT_S_OK(SafeReadBytes2(&itemInfo.Attributes, 
                sizeof(itemInfo.Attributes)));
          }
        }
        break;
      }
      case NID::kEmptyStream:
      {
        RETURN_IF_NOT_S_OK(ReadBoolVector(numFiles, emptyStreamVector))
        for (int i = 0; i < emptyStreamVector.Size(); i++)
          if (emptyStreamVector[i])
            numEmptyStreams++;
        emptyFileVector.Reserve(numEmptyStreams);
        antiFileVector.Reserve(numEmptyStreams);
        for (i = 0; i < numEmptyStreams; i++)
        {
          emptyFileVector.Add(false);
          antiFileVector.Add(false);
        }
        break;
      }
      case NID::kEmptyFile:
      {
        RETURN_IF_NOT_S_OK(ReadBoolVector(numEmptyStreams, emptyFileVector))
        break;
      }
      case NID::kAnti:
      {
        RETURN_IF_NOT_S_OK(ReadBoolVector(numEmptyStreams, antiFileVector))
        break;
      }
      case NID::kCreationTime:
      case NID::kLastWriteTime:
      case NID::kLastAccessTime:
      {
        CBoolVector boolVector;
        RETURN_IF_NOT_S_OK(ReadTime(dataVector, database.Files, type))
        break;
      }
      default:
      {
        database.ArchiveInfo.FileInfoPopIDs.DeleteBack();
        RETURN_IF_NOT_S_OK(SkeepData(size));
      }
    }
  }

  UINT32 emptyFileIndex = 0;
  UINT32 sizeIndex = 0;
  for(i = 0; i < numFiles; i++)
  {
    CFileItemInfo &itemInfo = database.Files[i];
    if(emptyStreamVector[i])
    {
      itemInfo.IsDirectory = !emptyFileVector[emptyFileIndex];
      itemInfo.IsAnti = antiFileVector[emptyFileIndex];
      emptyFileIndex++;
      itemInfo.UnPackSize = 0;
      itemInfo.FileCRCIsDefined = false;
    }
    else
    {
      itemInfo.IsDirectory = false;
      itemInfo.UnPackSize = unPackSizes[sizeIndex];
      itemInfo.FileCRC = digests[sizeIndex];
      itemInfo.FileCRCIsDefined = digestsDefined[sizeIndex];
      sizeIndex++;
    }
  }

  return S_OK;
}


void CArchiveDatabaseEx::FillFolderStartPackStream()
{
  FolderStartPackStreamIndex.Clear();
  FolderStartPackStreamIndex.Reserve(Folders.Size());
  UINT64 startPos = 0;
  for(UINT64 i = 0; i < Folders.Size(); i++)
  {
    FolderStartPackStreamIndex.Add(startPos);
    startPos += Folders[i].PackStreams.Size();
  }
}

void CArchiveDatabaseEx::FillStartPos()
{
  PackStreamStartPositions.Clear();
  PackStreamStartPositions.Reserve(PackSizes.Size());
  UINT64 startPos = 0;
  for(UINT64 i = 0; i < PackSizes.Size(); i++)
  {
    PackStreamStartPositions.Add(startPos);
    startPos += PackSizes[i];
  }
}

void CArchiveDatabaseEx::FillFolderStartFileIndex()
{
  FolderStartFileIndex.Clear();
  FolderStartFileIndex.Reserve(Folders.Size());
  FileIndexToFolderIndexMap.Clear();
  FileIndexToFolderIndexMap.Reserve(Files.Size());
  
  int folderIndex = 0;
  int indexInFolder = 0;
  for (int i = 0; i < Files.Size(); i++)
  {
    const CFileItemInfo &file = Files[i];
    bool emptyStream = (file.IsDirectory || file.UnPackSize == 0);
    if (emptyStream && indexInFolder == 0)
    {
      FileIndexToFolderIndexMap.Add(-1);
      continue;
    }
    if (indexInFolder == 0)
    {
      if (folderIndex >= Folders.Size())
        throw CInArchiveException(CInArchiveException::kIncorrectHeader);
      FolderStartFileIndex.Add(i);
    }
    FileIndexToFolderIndexMap.Add(folderIndex);
    if (emptyStream)
      continue;
    indexInFolder++;
    if (indexInFolder >= NumUnPackStreamsVector[folderIndex])
    {
      folderIndex++;
      indexInFolder = 0;
    }
  }
}

HRESULT CInArchive::ReadDatabase(CArchiveDatabaseEx &database)
{
  database.ArchiveInfo.StartPosition = _arhiveBeginStreamPosition;
  RETURN_IF_NOT_S_OK(SafeReadBytes(&database.ArchiveInfo.Version, 
      sizeof(database.ArchiveInfo.Version)));
  if (database.ArchiveInfo.Version.Major != kMajorVersion)
    throw  CInArchiveException(CInArchiveException::kUnsupportedVersion);

  UINT32 crcFromArchive;
  RETURN_IF_NOT_S_OK(SafeReadBytes(&crcFromArchive, sizeof(crcFromArchive)));
  CStartHeader startHeader;
  RETURN_IF_NOT_S_OK(SafeReadBytes(&startHeader, sizeof(startHeader)));
  if (!CCRC::VerifyDigest(crcFromArchive, &startHeader, sizeof(startHeader)))
    throw CInArchiveException(CInArchiveException::kIncorrectHeader);

  database.ArchiveInfo.StartPositionAfterHeader = _position;

  RETURN_IF_NOT_S_OK(_stream->Seek(startHeader.NextHeaderOffset, STREAM_SEEK_CUR, &_position));

  CByteBuffer buffer2;
  buffer2.SetCapacity(startHeader.NextHeaderSize);
  RETURN_IF_NOT_S_OK(SafeReadBytes(buffer2, startHeader.NextHeaderSize));
  if (!CCRC::VerifyDigest(startHeader.NextHeaderCRC, buffer2, startHeader.NextHeaderSize))
    throw CInArchiveException(CInArchiveException::kIncorrectHeader);
  
  {
    CStreamSwitch streamSwitch;
    streamSwitch.Set(this, buffer2, startHeader.NextHeaderSize);

    BYTE type;
    RETURN_IF_NOT_S_OK(SafeReadByte2(type));

    if (type != NID::kHeader)
      throw CInArchiveException(CInArchiveException::kIncorrectHeader);

    RETURN_IF_NOT_S_OK(ReadHeader(database));
  }
  return S_OK;
}

}}
