// 7zIn.cpp

#include "StdAfx.h"

#include "7zIn.h"
#include "7zMethods.h"
#include "7zDecode.h"
#include "../../Common/StreamObjects.h"
#include "../../../Common/CRC.h"

namespace NArchive {
namespace N7z {

class CStreamSwitch
{
  CInArchive *_archive;
  bool _needRemove;
public:
  CStreamSwitch(): _needRemove(false) {}
  ~CStreamSwitch() { Remove(); }
  void Remove();
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
  RINOK(archive->SafeReadByte2(external));
  if (external != 0)
  {
    UINT64 dataIndex;
    RINOK(archive->ReadNumber(dataIndex));
    Set(archive, (*dataVector)[(UINT32)dataIndex]);
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
  RINOK(ReadBytes(data, size, &realProcessedSize));
  if (realProcessedSize != size)
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
  return S_OK;
}

HRESULT CInArchive::ReadNumber(UINT64 &value)
{
  BYTE firstByte;
  RINOK(SafeReadByte2(firstByte));
  BYTE mask = 0x80;
  for (int i = 0; i < 8; i++)
  {
    if ((firstByte & mask) == 0)
    {
      value = 0;
      RINOK(SafeReadBytes2(&value, i));
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
  RINOK(stream->Seek(_arhiveBeginStreamPosition, STREAM_SEEK_SET, NULL));

  BYTE signature[kSignatureSize];
  UINT32 processedSize; 
  RINOK(ReadBytes(stream, signature, kSignatureSize, &processedSize));
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
    RINOK(ReadBytes(stream, buffer + numBytesPrev, numReadBytes, &processedSize));
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
  RINOK(stream->Seek(0, STREAM_SEEK_CUR, &_arhiveBeginStreamPosition))
  _position = _arhiveBeginStreamPosition;
  RINOK(FindAndReadSignature(stream, searchHeaderSizeLimit));
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
    RINOK(SafeReadByte2(temp));
  }
  return S_OK;
}

HRESULT CInArchive::SkeepData()
{
  UINT64 size;
  RINOK(ReadNumber(size));
  return SkeepData(size);
}

HRESULT CInArchive::ReadArchiveProperties(CInArchiveInfo &archiveInfo)
{
  while(true)
  {
    UINT64 type;
    RINOK(ReadID(type));
    if (type == NID::kEnd)
      break;
    SkeepData();
  }
  return S_OK;
}

HRESULT CInArchive::GetNextFolderItem(CFolder &folder)
{
  UINT64 numCoders;
  RINOK(ReadNumber(numCoders));

  folder.Coders.Clear();
  folder.Coders.Reserve((int)numCoders);
  UINT32 numInStreams = 0;
  UINT32 numOutStreams = 0;
  UINT32 i;
  for (i = 0; i < numCoders; i++)
  {
    folder.Coders.Add(CCoderInfo());
    CCoderInfo &coderInfo = folder.Coders.Back();

    while (true)
    {
      coderInfo.AltCoders.Add(CAltCoderInfo());
      CAltCoderInfo &altCoderInfo = coderInfo.AltCoders.Back();
      BYTE mainByte;
      RINOK(SafeReadByte2(mainByte));
      altCoderInfo.MethodID.IDSize = mainByte & 0xF;
      bool isComplex = (mainByte & 0x10) != 0;
      bool tereAreProperties = (mainByte & 0x20) != 0;
      RINOK(SafeReadBytes2(&altCoderInfo.MethodID.ID[0], 
        altCoderInfo.MethodID.IDSize));
      if (isComplex)
      {
        RINOK(ReadNumber(coderInfo.NumInStreams));
        RINOK(ReadNumber(coderInfo.NumOutStreams));
      }
      else
      {
        coderInfo.NumInStreams = 1;
        coderInfo.NumOutStreams = 1;
      }
      UINT64 propertiesSize = 0;
      if (tereAreProperties)
      {
        RINOK(ReadNumber(propertiesSize));
      }
      altCoderInfo.Properties.SetCapacity((UINT32)propertiesSize);
      RINOK(SafeReadBytes2((BYTE *)altCoderInfo.Properties, 
          (UINT32)propertiesSize));

      // coderInfo.AltCoders.Add(coderInfo.AltCoders.Back());
      if ((mainByte & 0x80) == 0)
        break;
    }
    numInStreams += (UINT32)coderInfo.NumInStreams;
    numOutStreams += (UINT32)coderInfo.NumOutStreams;
  }

  UINT64 numBindPairs;
  // RINOK(ReadNumber(numBindPairs));
  numBindPairs = numOutStreams - 1;
  folder.BindPairs.Clear();
  folder.BindPairs.Reserve((UINT32)numBindPairs);
  for (i = 0; i < numBindPairs; i++)
  {
    CBindPair bindPair;
    RINOK(ReadNumber(bindPair.InIndex));
    RINOK(ReadNumber(bindPair.OutIndex)); 
    folder.BindPairs.Add(bindPair);
  }

  UINT32 numPackedStreams = numInStreams - (UINT32)numBindPairs;
  folder.PackStreams.Reserve(numPackedStreams);
  if (numPackedStreams == 1)
  {
    for (UINT32 j = 0; j < numInStreams; j++)
      if (folder.FindBindPairForInStream(j) < 0)
      {
        CPackStreamInfo packStreamInfo;
        packStreamInfo.Index = j;
        folder.PackStreams.Add(packStreamInfo);
        break;
      }
  }
  else
    for(i = 0; i < numPackedStreams; i++)
    {
      CPackStreamInfo packStreamInfo;
      RINOK(ReadNumber(packStreamInfo.Index));
      folder.PackStreams.Add(packStreamInfo);
    }

  return S_OK;
}

HRESULT CInArchive::WaitAttribute(UINT64 attribute)
{
  while(true)
  {
    UINT64 type;
    RINOK(ReadID(type));
    if (type == attribute)
      return S_OK;
    if (type == NID::kEnd)
      return S_FALSE;
    RINOK(SkeepData());
  }
}

HRESULT CInArchive::ReadHashDigests(int numItems,
    CRecordVector<bool> &digestsDefined, 
    CRecordVector<UINT32> &digests)
{
  RINOK(ReadBoolVector2(numItems, digestsDefined));
  digests.Clear();
  digests.Reserve(numItems);
  for(int i = 0; i < numItems; i++)
  {
    UINT32 crc;
    if (digestsDefined[i])
      RINOK(SafeReadBytes2(&crc, sizeof(crc)));
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
  RINOK(ReadNumber(dataOffset));
  UINT64 numPackStreams;
  RINOK(ReadNumber(numPackStreams));

  RINOK(WaitAttribute(NID::kSize));
  packSizes.Clear();
  packSizes.Reserve((UINT32)numPackStreams);
  for(UINT64 i = 0; i < numPackStreams; i++)
  {
    UINT64 size;
    RINOK(ReadNumber(size));
    packSizes.Add(size);
  }

  UINT64 type;
  while(true)
  {
    RINOK(ReadID(type));
    if (type == NID::kEnd)
      break;
    if (type == NID::kCRC)
    {
      RINOK(ReadHashDigests(
          (UINT32)numPackStreams, packCRCsDefined, packCRCs)); 
      continue;
    }
    RINOK(SkeepData());
  }
  if (packCRCsDefined.IsEmpty())
  {
    packCRCsDefined.Reserve((UINT32)numPackStreams);
    packCRCsDefined.Clear();
    packCRCs.Reserve((UINT32)numPackStreams);
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
    CObjectVector<CFolder> &folders)
{
  RINOK(WaitAttribute(NID::kFolder));
  UINT64 numFolders;
  RINOK(ReadNumber(numFolders));

  {
    CStreamSwitch streamSwitch;
    RINOK(streamSwitch.Set(this, dataVector));
    folders.Clear();
    folders.Reserve((UINT32)numFolders);
    for(UINT64 i = 0; i < numFolders; i++)
    {
      folders.Add(CFolder());
      RINOK(GetNextFolderItem(folders.Back()));
    }
  }

  RINOK(WaitAttribute(NID::kCodersUnPackSize));

  for(UINT32 i = 0; i < numFolders; i++)
  {
    CFolder &folder = folders[i];
    UINT32 numOutStreams = (UINT32)folder.GetNumOutStreams();
    folder.UnPackSizes.Reserve(numOutStreams);
    for(UINT32 j = 0; j < numOutStreams; j++)
    {
      UINT64 unPackSize;
      RINOK(ReadNumber(unPackSize));
      folder.UnPackSizes.Add(unPackSize);
    }
  }

  while(true)
  {
    UINT64 type;
    RINOK(ReadID(type));
    if (type == NID::kEnd)
      return S_OK;
    if (type == NID::kCRC)
    {
      CRecordVector<bool> crcsDefined;
      CRecordVector<UINT32> crcs;
      RINOK(ReadHashDigests((UINT32)numFolders, crcsDefined, crcs)); 
      for(i = 0; i < numFolders; i++)
      {
        CFolder &folder = folders[i];
        folder.UnPackCRCDefined = crcsDefined[i];
        folder.UnPackCRC = crcs[i];
      }
      continue;
    }
    RINOK(SkeepData());
  }
}

HRESULT CInArchive::ReadSubStreamsInfo(
    const CObjectVector<CFolder> &folders,
    CRecordVector<UINT64> &numUnPackStreamsInFolders,
    CRecordVector<UINT64> &unPackSizes,
    CRecordVector<bool> &digestsDefined, 
    CRecordVector<UINT32> &digests)
{
  numUnPackStreamsInFolders.Clear();
  numUnPackStreamsInFolders.Reserve(folders.Size());
  UINT64 type;
  while(true)
  {
    RINOK(ReadID(type));
    if (type == NID::kNumUnPackStream)
    {
      for(int i = 0; i < folders.Size(); i++)
      {
        UINT64 value;
        RINOK(ReadNumber(value));
        numUnPackStreamsInFolders.Add(value);
      }
      continue;
    }
    if (type == NID::kCRC || type == NID::kSize)
      break;
    if (type == NID::kEnd)
      break;
    RINOK(SkeepData());
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
        RINOK(ReadNumber(size));
        unPackSizes.Add(size);
        sum += size;
      }
    }
    unPackSizes.Add(folders[i].GetUnPackSize() - sum);
  }
  if (type == NID::kSize)
  {
    RINOK(ReadID(type));
  }

  int numDigests = 0;
  int numDigestsTotal = 0;
  for(i = 0; i < folders.Size(); i++)
  {
    UINT32 numSubstreams = (UINT32)numUnPackStreamsInFolders[i];
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
      RINOK(ReadHashDigests(numDigests, digestsDefined2, digests2));
      int digestIndex = 0;
      for (i = 0; i < folders.Size(); i++)
      {
        int numSubstreams = (UINT32)numUnPackStreamsInFolders[i];
        const CFolder &folder = folders[i];
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
      RINOK(SkeepData());
    }
    RINOK(ReadID(type));
  }
}


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
    CRecordVector<UINT32> &digests)
{
  while(true)
  {
    UINT64 type;
    RINOK(ReadID(type));
    switch(type)
    {
      case NID::kEnd:
        return S_OK;
      case NID::kPackInfo:
      {
        RINOK(ReadPackInfo(dataOffset, packSizes,
            packCRCsDefined, packCRCs));
        break;
      }
      case NID::kUnPackInfo:
      {
        RINOK(ReadUnPackInfo(dataVector, folders));
        break;
      }
      case NID::kSubStreamsInfo:
      {
        RINOK(ReadSubStreamsInfo(folders, numUnPackStreamsInFolders,
          unPackSizes, digestsDefined, digests));
        break;
      }
    }
  }
}


HRESULT CInArchive::ReadFileNames(CObjectVector<CFileItem> &files)
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
      RINOK(SafeReadWideCharLE(c));
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
      RINOK(SafeReadBytes2(&b, 1));
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
  RINOK(SafeReadByte2(allAreDefinedFlag));
  if (allAreDefinedFlag == 0)
    return ReadBoolVector(numItems, vector);
  vector.Clear();
  vector.Reserve(numItems);
  for (UINT32 j = 0; j < numItems; j++)
    vector.Add(true);
  return S_OK;
}

HRESULT CInArchive::ReadTime(const CObjectVector<CByteBuffer> &dataVector,
    CObjectVector<CFileItem> &files, UINT64 type)
{
  CBoolVector boolVector;
  RINOK(ReadBoolVector2(files.Size(), boolVector))

  CStreamSwitch streamSwitch;
  RINOK(streamSwitch.Set(this, &dataVector));

  for(int i = 0; i < files.Size(); i++)
  {
    CFileItem &file = files[i];
    CArchiveFileTime fileTime;
    bool defined = boolVector[i];
    if (defined)
    {
      RINOK(SafeReadBytes2(&fileTime, sizeof(fileTime)));
    }
    switch(type)
    {
      case NID::kCreationTime:
        file.IsCreationTimeDefined= defined;
        if (defined)
          file.CreationTime = fileTime;
        break;
      case NID::kLastWriteTime:
        file.IsLastWriteTimeDefined = defined;
        if (defined)
          file.LastWriteTime = fileTime;
        break;
      case NID::kLastAccessTime:
        file.IsLastAccessTimeDefined = defined;
        if (defined)
          file.LastAccessTime = fileTime;
        break;
    }
  }
  return S_OK;
}

HRESULT CInArchive::ReadAndDecodePackedStreams(UINT64 baseOffset, 
    UINT64 &dataOffset, CObjectVector<CByteBuffer> &dataVector
    #ifndef _NO_CRYPTO
    , ICryptoGetTextPassword *getTextPassword
    #endif
    )
{
  CRecordVector<UINT64> packSizes;
  CRecordVector<bool> packCRCsDefined;
  CRecordVector<UINT32> packCRCs;
  CObjectVector<CFolder> folders;
  
  CRecordVector<UINT64> numUnPackStreamsInFolders;
  CRecordVector<UINT64> unPackSizes;
  CRecordVector<bool> digestsDefined;
  CRecordVector<UINT32> digests;
  
  RINOK(ReadStreamsInfo(NULL, 
    dataOffset,
    packSizes, 
    packCRCsDefined, 
    packCRCs, 
    folders,
    numUnPackStreamsInFolders,
    unPackSizes,
    digestsDefined, 
    digests));
  
  // database.ArchiveInfo.DataStartPosition2 += database.ArchiveInfo.StartPositionAfterHeader;
  
  UINT32 packIndex = 0;
  CDecoder decoder;
  UINT64 dataStartPos = baseOffset + dataOffset;
  for(int i = 0; i < folders.Size(); i++)
  {
    const CFolder &folder = folders[i];
    dataVector.Add(CByteBuffer());
    CByteBuffer &data = dataVector.Back();
    UINT64 unPackSize = folder.GetUnPackSize();
    data.SetCapacity((UINT32)unPackSize);
    
    CSequentialOutStreamImp2 *outStreamSpec = new CSequentialOutStreamImp2;
    CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
    outStreamSpec->Init(data, (UINT32)unPackSize);
    
    HRESULT result = decoder.Decode(_stream, dataStartPos, 
      &packSizes[packIndex], folder, outStream, NULL
      #ifndef _NO_CRYPTO
      , getTextPassword
      #endif
      );
    RINOK(result);
    
    if (folder.UnPackCRCDefined)
      if (!CCRC::VerifyDigest(folder.UnPackCRC, data, (UINT32)unPackSize))
        throw CInArchiveException(CInArchiveException::kIncorrectHeader);
      for (int j = 0; j < folder.PackStreams.Size(); j++)
        dataStartPos += packSizes[packIndex++];
  }
  return S_OK;
}

HRESULT CInArchive::ReadHeader(CArchiveDatabaseEx &database
    #ifndef _NO_CRYPTO
    , ICryptoGetTextPassword *getTextPassword
    #endif
    )
{
  // database.Clear();

  UINT64 type;
  RINOK(ReadID(type));

  if (type == NID::kArchiveProperties)
  {
    RINOK(ReadArchiveProperties(database.ArchiveInfo));
    RINOK(ReadID(type));
  }
 
  CObjectVector<CByteBuffer> dataVector;
  
  if (type == NID::kAdditionalStreamsInfo)
  {
    /*
    CRecordVector<UINT64> packSizes;
    CRecordVector<bool> packCRCsDefined;
    CRecordVector<UINT32> packCRCs;
    CObjectVector<CFolder> folders;

    CRecordVector<UINT64> numUnPackStreamsInFolders;
    CRecordVector<UINT64> unPackSizes;
    CRecordVector<bool> digestsDefined;
    CRecordVector<UINT32> digests;

    RINOK(ReadStreamsInfo(NULL, 
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
      const CFolder &folder = folders[i];
      dataVector.Add(CByteBuffer());
      CByteBuffer &data = dataVector.Back();
      UINT64 unPackSize = folder.GetUnPackSize();
      data.SetCapacity(unPackSize);
      
      CComObjectNoLock<CSequentialOutStreamImp2> *outStreamSpec = 
        new  CComObjectNoLock<CSequentialOutStreamImp2>;
      CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
      outStreamSpec->Init(data, unPackSize);
      
      RINOK(decoder.Decode(_stream, dataStartPos, 
          &packSizes[packIndex], folder, outStream, NULL, getTextPassword));
      
      if (folder.UnPackCRCDefined)
        if (!CCRC::VerifyDigest(folder.UnPackCRC, data, unPackSize))
          throw CInArchiveException(CInArchiveException::kIncorrectHeader);
      for (int j = 0; j < folder.PackStreams.Size(); j++)
        dataStartPos += packSizes[packIndex++];
    }
    */
    HRESULT result = ReadAndDecodePackedStreams(
        database.ArchiveInfo.StartPositionAfterHeader, 
        database.ArchiveInfo.DataStartPosition2,
        dataVector
        #ifndef _NO_CRYPTO
        , getTextPassword
        #endif
        );
    RINOK(result);

    database.ArchiveInfo.DataStartPosition2 += database.ArchiveInfo.StartPositionAfterHeader;
    RINOK(ReadID(type));
  }

  CRecordVector<UINT64> unPackSizes;
  CRecordVector<bool> digestsDefined;
  CRecordVector<UINT32> digests;
  
  if (type == NID::kMainStreamsInfo)
  {
    RINOK(ReadStreamsInfo(&dataVector,
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
    RINOK(ReadID(type));
  }
  else
  {
    for(int i = 0; i < database.Folders.Size(); i++)
    {
      database.NumUnPackStreamsVector.Add(1);
      CFolder &folder = database.Folders[i];
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
  RINOK(ReadNumber(numFiles));
  database.Files.Reserve((size_t)numFiles);
  for(UINT64 i = 0; i < numFiles; i++)
    database.Files.Add(CFileItem());

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
  emptyStreamVector.Reserve((size_t)numFiles);
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
    UINT64 type;
    RINOK(ReadID(type));
    if (type == NID::kEnd)
      break;
    UINT64 size;
    RINOK(ReadNumber(size));
    
    // sizePrev = size;
    // posPrev = _inByteBack->GetProcessedSize();

    database.ArchiveInfo.FileInfoPopIDs.Add(type);
    switch(type)
    {
      case NID::kName:
      {
        CStreamSwitch streamSwitch;
        RINOK(streamSwitch.Set(this, &dataVector));
        RINOK(ReadFileNames(database.Files))
        break;
      }
      case NID::kWinAttributes:
      {
        CBoolVector boolVector;
        RINOK(ReadBoolVector2(database.Files.Size(), boolVector))
        CStreamSwitch streamSwitch;
        RINOK(streamSwitch.Set(this, &dataVector));
        for(i = 0; i < numFiles; i++)
        {
          CFileItem &file = database.Files[(UINT32)i];
          if (file.AreAttributesDefined = boolVector[(UINT32)i])
          {
            RINOK(SafeReadBytes2(&file.Attributes, 
                sizeof(file.Attributes)));
          }
        }
        break;
      }
      case NID::kEmptyStream:
      {
        RINOK(ReadBoolVector((UINT32)numFiles, emptyStreamVector))
        UINT32 i;
        for (i = 0; i < (UINT32)emptyStreamVector.Size(); i++)
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
        RINOK(ReadBoolVector(numEmptyStreams, emptyFileVector))
        break;
      }
      case NID::kAnti:
      {
        RINOK(ReadBoolVector(numEmptyStreams, antiFileVector))
        break;
      }
      case NID::kCreationTime:
      case NID::kLastWriteTime:
      case NID::kLastAccessTime:
      {
        RINOK(ReadTime(dataVector, database.Files, type))
        break;
      }
      default:
      {
        database.ArchiveInfo.FileInfoPopIDs.DeleteBack();
        RINOK(SkeepData(size));
      }
    }
  }

  UINT32 emptyFileIndex = 0;
  UINT32 sizeIndex = 0;
  for(i = 0; i < numFiles; i++)
  {
    CFileItem &file = database.Files[(UINT32)i];
    file.HasStream = !emptyStreamVector[(UINT32)i];
    if(file.HasStream)
    {
      file.IsDirectory = false;
      file.IsAnti = false;
      file.UnPackSize = unPackSizes[sizeIndex];
      file.FileCRC = digests[sizeIndex];
      file.FileCRCIsDefined = digestsDefined[sizeIndex];
      sizeIndex++;
    }
    else
    {
      file.IsDirectory = !emptyFileVector[emptyFileIndex];
      file.IsAnti = antiFileVector[emptyFileIndex];
      emptyFileIndex++;
      file.UnPackSize = 0;
      file.FileCRCIsDefined = false;
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
    FolderStartPackStreamIndex.Add((UINT32)startPos);
    startPos += Folders[(UINT32)i].PackStreams.Size();
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
    startPos += PackSizes[(UINT32)i];
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
    const CFileItem &file = Files[i];
    bool emptyStream = !file.HasStream;
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

HRESULT CInArchive::ReadDatabase(CArchiveDatabaseEx &database
    #ifndef _NO_CRYPTO
    , ICryptoGetTextPassword *getTextPassword
    #endif
    )
{
  database.Clear();
  database.ArchiveInfo.StartPosition = _arhiveBeginStreamPosition;
  RINOK(SafeReadBytes(&database.ArchiveInfo.Version, 
      sizeof(database.ArchiveInfo.Version)));
  if (database.ArchiveInfo.Version.Major != kMajorVersion)
    throw  CInArchiveException(CInArchiveException::kUnsupportedVersion);

  UINT32 crcFromArchive;
  RINOK(SafeReadBytes(&crcFromArchive, sizeof(crcFromArchive)));
  CStartHeader startHeader;
  RINOK(SafeReadBytes(&startHeader, sizeof(startHeader)));
  if (!CCRC::VerifyDigest(crcFromArchive, &startHeader, sizeof(startHeader)))
    throw CInArchiveException(CInArchiveException::kIncorrectHeader);

  database.ArchiveInfo.StartPositionAfterHeader = _position;

  if (startHeader.NextHeaderSize == 0)
    return S_OK;

  RINOK(_stream->Seek(startHeader.NextHeaderOffset, STREAM_SEEK_CUR, &_position));

  CByteBuffer buffer2;
  buffer2.SetCapacity((size_t)startHeader.NextHeaderSize);
  RINOK(SafeReadBytes(buffer2, (UINT32)startHeader.NextHeaderSize));
  if (!CCRC::VerifyDigest(startHeader.NextHeaderCRC, buffer2, (UINT32)startHeader.NextHeaderSize))
    throw CInArchiveException(CInArchiveException::kIncorrectHeader);
  
  CStreamSwitch streamSwitch;
  streamSwitch.Set(this, buffer2);
  
  CObjectVector<CByteBuffer> dataVector;
  
  while (true)
  {
    UINT64 type;
    RINOK(ReadID(type));
    if (type == NID::kHeader)
      break;
    if (type != NID::kEncodedHeader)
      throw CInArchiveException(CInArchiveException::kIncorrectHeader);
    HRESULT result = ReadAndDecodePackedStreams(
        database.ArchiveInfo.StartPositionAfterHeader, 
        database.ArchiveInfo.DataStartPosition2,
        dataVector
        #ifndef _NO_CRYPTO
        , getTextPassword
        #endif
        );
    RINOK(result);
    if (dataVector.Size() == 0)
      return S_OK;
    if (dataVector.Size() > 1)
      throw CInArchiveException(CInArchiveException::kIncorrectHeader);
    streamSwitch.Remove();
    streamSwitch.Set(this, dataVector.Front());
  }

  return ReadHeader(database
    #ifndef _NO_CRYPTO
    , getTextPassword
    #endif
    );
}

}}
