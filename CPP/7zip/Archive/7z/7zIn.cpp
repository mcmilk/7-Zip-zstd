// 7zIn.cpp

#include "StdAfx.h"

#include "7zIn.h"
#include "7zDecode.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/StreamUtils.h"
extern "C" 
{ 
#include "../../../../C/7zCrc.h"
}

// define FORMAT_7Z_RECOVERY if you want to recover multivolume archives with empty StartHeader 
#ifndef _SFX
#define FORMAT_7Z_RECOVERY
#endif

namespace NArchive {
namespace N7z {

class CInArchiveException {};

static void ThrowException() { throw CInArchiveException(); }
static inline void ThrowEndOfData()   { ThrowException(); }
static inline void ThrowUnsupported() { ThrowException(); }
static inline void ThrowIncorrect()   { ThrowException(); }
static inline void ThrowUnsupportedVersion() { ThrowException(); }

/*
class CInArchiveException
{
public:
  enum CCauseType
  {
    kUnsupportedVersion = 0,
    kUnsupported,
    kIncorrect, 
    kEndOfData,
  } Cause;
  CInArchiveException(CCauseType cause): Cause(cause) {};
};

static void ThrowException(CInArchiveException::CCauseType c) { throw CInArchiveException(c); }
static void ThrowEndOfData()   { ThrowException(CInArchiveException::kEndOfData); }
static void ThrowUnsupported() { ThrowException(CInArchiveException::kUnsupported); }
static void ThrowIncorrect()   { ThrowException(CInArchiveException::kIncorrect); }
static void ThrowUnsupportedVersion() { ThrowException(CInArchiveException::kUnsupportedVersion); }
*/

class CStreamSwitch
{
  CInArchive *_archive;
  bool _needRemove;
public:
  CStreamSwitch(): _needRemove(false) {}
  ~CStreamSwitch() { Remove(); }
  void Remove();
  void Set(CInArchive *archive, const Byte *data, size_t size);
  void Set(CInArchive *archive, const CByteBuffer &byteBuffer);
  void Set(CInArchive *archive, const CObjectVector<CByteBuffer> *dataVector);
};

void CStreamSwitch::Remove()
{
  if (_needRemove)
  {
    _archive->DeleteByteStream();
    _needRemove = false;
  }
}

void CStreamSwitch::Set(CInArchive *archive, const Byte *data, size_t size)
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

void CStreamSwitch::Set(CInArchive *archive, const CObjectVector<CByteBuffer> *dataVector)
{
  Remove();
  Byte external = archive->ReadByte();
  if (external != 0)
  {
    int dataIndex = (int)archive->ReadNum();
    if (dataIndex < 0 || dataIndex >= dataVector->Size())
      ThrowIncorrect();
    Set(archive, (*dataVector)[dataIndex]);
  }
}

#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(__i386__) || defined(__x86_64__)
#define SZ_LITTLE_ENDIAN_UNALIGN
#endif

#ifdef SZ_LITTLE_ENDIAN_UNALIGN
static inline UInt16 GetUInt16FromMem(const Byte *p) { return *(const UInt16 *)p; }
static inline UInt32 GetUInt32FromMem(const Byte *p) { return *(const UInt32 *)p; }
static inline UInt64 GetUInt64FromMem(const Byte *p) { return *(const UInt64 *)p; }
#else
static inline UInt16 GetUInt16FromMem(const Byte *p) { return p[0] | ((UInt16)p[1] << 8); }
static inline UInt32 GetUInt32FromMem(const Byte *p) { return p[0] | ((UInt32)p[1] << 8) | ((UInt32)p[2] << 16) | ((UInt32)p[3] << 24); }
static inline UInt64 GetUInt64FromMem(const Byte *p) { return GetUInt32FromMem(p) | ((UInt64)GetUInt32FromMem(p + 4) << 32); }
#endif

Byte CInByte2::ReadByte()
{
  if (_pos >= _size)
    ThrowEndOfData();
  return _buffer[_pos++];
}

void CInByte2::ReadBytes(Byte *data, size_t size)
{
  if (size > _size - _pos)
    ThrowEndOfData();
  for (size_t i = 0; i < size; i++)
    data[i] = _buffer[_pos++];
}

void CInByte2::SkeepData(UInt64 size)
{
  if (size > _size - _pos)
    ThrowEndOfData();
}

void CInByte2::SkeepData()
{
  SkeepData(ReadNumber());
}

UInt64 CInByte2::ReadNumber()
{
  if (_pos >= _size)
    ThrowEndOfData();
  Byte firstByte = _buffer[_pos++];
  Byte mask = 0x80;
  UInt64 value = 0;
  for (int i = 0; i < 8; i++)
  {
    if ((firstByte & mask) == 0)
    {
      UInt64 highPart = firstByte & (mask - 1);
      value += (highPart << (i * 8));
      return value;
    }
    if (_pos >= _size)
      ThrowEndOfData();
    value |= ((UInt64)_buffer[_pos++] << (8 * i));
    mask >>= 1;
  }
  return value;
}

CNum CInByte2::ReadNum()
{ 
  UInt64 value = ReadNumber(); 
  if (value > kNumMax)
    ThrowUnsupported();
  return (CNum)value;
}

UInt32 CInByte2::ReadUInt32()
{
  if (_pos + 4 > _size)
    ThrowEndOfData();
  UInt32 res = GetUInt32FromMem(_buffer + _pos);
  _pos += 4;
  return res;
}

UInt64 CInByte2::ReadUInt64()
{
  if (_pos + 8 > _size)
    ThrowEndOfData();
  UInt64 res = GetUInt64FromMem(_buffer + _pos);
  _pos += 8;
  return res;
}

void CInByte2::ReadString(UString &s)
{
  const Byte *buf = _buffer + _pos;
  size_t rem = (_size - _pos) / 2 * 2;
  {
    size_t i;
    for (i = 0; i < rem; i += 2)
      if (buf[i] == 0 && buf[i + 1] == 0)
        break;
    if (i == rem)
      ThrowEndOfData();
    rem = i;
  }
  int len = (int)(rem / 2);
  if (len < 0 || (size_t)len * 2 != rem)
    ThrowUnsupported();
  wchar_t *p = s.GetBuffer(len);
  int i;
  for (i = 0; i < len; i++, buf += 2) 
    p[i] = (wchar_t)GetUInt16FromMem(buf);
  p[i] = 0;
  s.ReleaseBuffer(len);
  _pos += rem + 2;
}

static inline bool TestSignatureCandidate(const Byte *p)
{
  for (int i = 0; i < kSignatureSize; i++)
    if (p[i] != kSignature[i])
      return false;
  return (p[0x1A] == 0 && p[0x1B] == 0);
}

HRESULT CInArchive::FindAndReadSignature(IInStream *stream, const UInt64 *searchHeaderSizeLimit)
{
  UInt32 processedSize; 
  RINOK(ReadStream(stream, _header, kHeaderSize, &processedSize));
  if (processedSize != kHeaderSize)
    return S_FALSE;
  if (TestSignatureCandidate(_header))
    return S_OK;

  CByteBuffer byteBuffer;
  const UInt32 kBufferSize = (1 << 16);
  byteBuffer.SetCapacity(kBufferSize);
  Byte *buffer = byteBuffer;
  UInt32 numPrevBytes = kHeaderSize - 1;
  memcpy(buffer, _header + 1, numPrevBytes);
  UInt64 curTestPos = _arhiveBeginStreamPosition + 1;
  for (;;)
  {
    if (searchHeaderSizeLimit != NULL)
      if (curTestPos - _arhiveBeginStreamPosition > *searchHeaderSizeLimit)
        break;
    UInt32 numReadBytes = kBufferSize - numPrevBytes;
    RINOK(stream->Read(buffer + numPrevBytes, numReadBytes, &processedSize));
    UInt32 numBytesInBuffer = numPrevBytes + processedSize;
    if (numBytesInBuffer < kHeaderSize)
      break;
    UInt32 numTests = numBytesInBuffer - kHeaderSize + 1;
    for(UInt32 pos = 0; pos < numTests; pos++, curTestPos++)
    { 
      if (TestSignatureCandidate(buffer + pos))
      {
        memcpy(_header, buffer + pos, kHeaderSize);
        _arhiveBeginStreamPosition = curTestPos;
        return stream->Seek(curTestPos + kHeaderSize, STREAM_SEEK_SET, NULL);
      }
    }
    numPrevBytes = numBytesInBuffer - numTests;
    memmove(buffer, buffer + numTests, numPrevBytes);
  }
  return S_FALSE;
}

// S_FALSE means that file is not archive
HRESULT CInArchive::Open(IInStream *stream, const UInt64 *searchHeaderSizeLimit)
{
  Close();
  RINOK(stream->Seek(0, STREAM_SEEK_CUR, &_arhiveBeginStreamPosition))
  RINOK(FindAndReadSignature(stream, searchHeaderSizeLimit));
  _stream = stream;
  return S_OK;
}
  
void CInArchive::Close()
{
  _stream.Release();
}

void CInArchive::ReadArchiveProperties(CInArchiveInfo & /* archiveInfo */)
{
  for (;;)
  {
    if (ReadID() == NID::kEnd)
      break;
    SkeepData();
  }
}

void CInArchive::GetNextFolderItem(CFolder &folder)
{
  CNum numCoders = ReadNum();

  folder.Coders.Clear();
  folder.Coders.Reserve((int)numCoders);
  CNum numInStreams = 0;
  CNum numOutStreams = 0;
  CNum i;
  for (i = 0; i < numCoders; i++)
  {
    folder.Coders.Add(CCoderInfo());
    CCoderInfo &coder = folder.Coders.Back();

    {
      Byte mainByte = ReadByte();
      int idSize = (mainByte & 0xF);
      Byte longID[15];
      ReadBytes(longID, idSize);
      if (idSize > 8)
        ThrowUnsupported();
      UInt64 id = 0;
      for (int j = 0; j < idSize; j++)
        id |= (UInt64)longID[idSize - 1 - j] << (8 * j);
      coder.MethodID = id;

      if ((mainByte & 0x10) != 0)
      {
        coder.NumInStreams = ReadNum();
        coder.NumOutStreams = ReadNum();
      }
      else
      {
        coder.NumInStreams = 1;
        coder.NumOutStreams = 1;
      }
      if ((mainByte & 0x20) != 0)
      {
        CNum propertiesSize = ReadNum();
        coder.Properties.SetCapacity((size_t)propertiesSize);
        ReadBytes((Byte *)coder.Properties, (size_t)propertiesSize);
      }
      if ((mainByte & 0x80) != 0)
        ThrowUnsupported();
    }
    numInStreams += coder.NumInStreams;
    numOutStreams += coder.NumOutStreams;
  }

  CNum numBindPairs;
  numBindPairs = numOutStreams - 1;
  folder.BindPairs.Clear();
  folder.BindPairs.Reserve(numBindPairs);
  for (i = 0; i < numBindPairs; i++)
  {
    CBindPair bindPair;
    bindPair.InIndex = ReadNum();
    bindPair.OutIndex = ReadNum(); 
    folder.BindPairs.Add(bindPair);
  }

  CNum numPackedStreams = numInStreams - numBindPairs;
  folder.PackStreams.Reserve(numPackedStreams);
  if (numPackedStreams == 1)
  {
    for (CNum j = 0; j < numInStreams; j++)
      if (folder.FindBindPairForInStream(j) < 0)
      {
        folder.PackStreams.Add(j);
        break;
      }
  }
  else
    for(i = 0; i < numPackedStreams; i++)
      folder.PackStreams.Add(ReadNum());
}

void CInArchive::WaitAttribute(UInt64 attribute)
{
  for (;;)
  {
    UInt64 type = ReadID();
    if (type == attribute)
      return;
    if (type == NID::kEnd)
      ThrowIncorrect();
    SkeepData();
  }
}

void CInArchive::ReadHashDigests(int numItems,
    CRecordVector<bool> &digestsDefined, 
    CRecordVector<UInt32> &digests)
{
  ReadBoolVector2(numItems, digestsDefined);
  digests.Clear();
  digests.Reserve(numItems);
  for(int i = 0; i < numItems; i++)
  {
    UInt32 crc = 0;
    if (digestsDefined[i])
      crc = ReadUInt32();
    digests.Add(crc);
  }
}

void CInArchive::ReadPackInfo(
    UInt64 &dataOffset,
    CRecordVector<UInt64> &packSizes,
    CRecordVector<bool> &packCRCsDefined,
    CRecordVector<UInt32> &packCRCs)
{
  dataOffset = ReadNumber();
  CNum numPackStreams = ReadNum();

  WaitAttribute(NID::kSize);
  packSizes.Clear();
  packSizes.Reserve(numPackStreams);
  for (CNum i = 0; i < numPackStreams; i++)
    packSizes.Add(ReadNumber());

  UInt64 type;
  for (;;)
  {
    type = ReadID();
    if (type == NID::kEnd)
      break;
    if (type == NID::kCRC)
    {
      ReadHashDigests(numPackStreams, packCRCsDefined, packCRCs); 
      continue;
    }
    SkeepData();
  }
  if (packCRCsDefined.IsEmpty())
  {
    packCRCsDefined.Reserve(numPackStreams);
    packCRCsDefined.Clear();
    packCRCs.Reserve(numPackStreams);
    packCRCs.Clear();
    for(CNum i = 0; i < numPackStreams; i++)
    {
      packCRCsDefined.Add(false);
      packCRCs.Add(0);
    }
  }
}

void CInArchive::ReadUnPackInfo(
    const CObjectVector<CByteBuffer> *dataVector,
    CObjectVector<CFolder> &folders)
{
  WaitAttribute(NID::kFolder);
  CNum numFolders = ReadNum();

  {
    CStreamSwitch streamSwitch;
    streamSwitch.Set(this, dataVector);
    folders.Clear();
    folders.Reserve(numFolders);
    for(CNum i = 0; i < numFolders; i++)
    {
      folders.Add(CFolder());
      GetNextFolderItem(folders.Back());
    }
  }

  WaitAttribute(NID::kCodersUnPackSize);

  CNum i;
  for (i = 0; i < numFolders; i++)
  {
    CFolder &folder = folders[i];
    CNum numOutStreams = folder.GetNumOutStreams();
    folder.UnPackSizes.Reserve(numOutStreams);
    for (CNum j = 0; j < numOutStreams; j++)
      folder.UnPackSizes.Add(ReadNumber());
  }

  for (;;)
  {
    UInt64 type = ReadID();
    if (type == NID::kEnd)
      return;
    if (type == NID::kCRC)
    {
      CRecordVector<bool> crcsDefined;
      CRecordVector<UInt32> crcs;
      ReadHashDigests(numFolders, crcsDefined, crcs); 
      for(i = 0; i < numFolders; i++)
      {
        CFolder &folder = folders[i];
        folder.UnPackCRCDefined = crcsDefined[i];
        folder.UnPackCRC = crcs[i];
      }
      continue;
    }
    SkeepData();
  }
}

void CInArchive::ReadSubStreamsInfo(
    const CObjectVector<CFolder> &folders,
    CRecordVector<CNum> &numUnPackStreamsInFolders,
    CRecordVector<UInt64> &unPackSizes,
    CRecordVector<bool> &digestsDefined, 
    CRecordVector<UInt32> &digests)
{
  numUnPackStreamsInFolders.Clear();
  numUnPackStreamsInFolders.Reserve(folders.Size());
  UInt64 type;
  for (;;)
  {
    type = ReadID();
    if (type == NID::kNumUnPackStream)
    {
      for(int i = 0; i < folders.Size(); i++)
        numUnPackStreamsInFolders.Add(ReadNum());
      continue;
    }
    if (type == NID::kCRC || type == NID::kSize)
      break;
    if (type == NID::kEnd)
      break;
    SkeepData();
  }

  if (numUnPackStreamsInFolders.IsEmpty())
    for(int i = 0; i < folders.Size(); i++)
      numUnPackStreamsInFolders.Add(1);

  int i;
  for(i = 0; i < numUnPackStreamsInFolders.Size(); i++)
  {
    // v3.13 incorrectly worked with empty folders
    // v4.07: we check that folder is empty
    CNum numSubstreams = numUnPackStreamsInFolders[i];
    if (numSubstreams == 0)
      continue;
    UInt64 sum = 0;
    for (CNum j = 1; j < numSubstreams; j++)
      if (type == NID::kSize)
      {
        UInt64 size = ReadNumber();
        unPackSizes.Add(size);
        sum += size;
      }
    unPackSizes.Add(folders[i].GetUnPackSize() - sum);
  }
  if (type == NID::kSize)
    type = ReadID();

  int numDigests = 0;
  int numDigestsTotal = 0;
  for(i = 0; i < folders.Size(); i++)
  {
    CNum numSubstreams = numUnPackStreamsInFolders[i];
    if (numSubstreams != 1 || !folders[i].UnPackCRCDefined)
      numDigests += numSubstreams;
    numDigestsTotal += numSubstreams;
  }

  for (;;)
  {
    if (type == NID::kCRC)
    {
      CRecordVector<bool> digestsDefined2; 
      CRecordVector<UInt32> digests2;
      ReadHashDigests(numDigests, digestsDefined2, digests2);
      int digestIndex = 0;
      for (i = 0; i < folders.Size(); i++)
      {
        CNum numSubstreams = numUnPackStreamsInFolders[i];
        const CFolder &folder = folders[i];
        if (numSubstreams == 1 && folder.UnPackCRCDefined)
        {
          digestsDefined.Add(true);
          digests.Add(folder.UnPackCRC);
        }
        else
          for (CNum j = 0; j < numSubstreams; j++, digestIndex++)
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
      return;
    }
    else
      SkeepData();
    type = ReadID();
  }
}

void CInArchive::ReadStreamsInfo(
    const CObjectVector<CByteBuffer> *dataVector,
    UInt64 &dataOffset,
    CRecordVector<UInt64> &packSizes,
    CRecordVector<bool> &packCRCsDefined,
    CRecordVector<UInt32> &packCRCs,
    CObjectVector<CFolder> &folders,
    CRecordVector<CNum> &numUnPackStreamsInFolders,
    CRecordVector<UInt64> &unPackSizes,
    CRecordVector<bool> &digestsDefined, 
    CRecordVector<UInt32> &digests)
{
  for (;;)
  {
    UInt64 type = ReadID();
    if (type > ((UInt32)1 << 30))
      ThrowIncorrect();
    switch((UInt32)type)
    {
      case NID::kEnd:
        return;
      case NID::kPackInfo:
      {
        ReadPackInfo(dataOffset, packSizes, packCRCsDefined, packCRCs);
        break;
      }
      case NID::kUnPackInfo:
      {
        ReadUnPackInfo(dataVector, folders);
        break;
      }
      case NID::kSubStreamsInfo:
      {
        ReadSubStreamsInfo(folders, numUnPackStreamsInFolders,
            unPackSizes, digestsDefined, digests);
        break;
      }
      default:
        ThrowIncorrect();
    }
  }
}

void CInArchive::ReadBoolVector(int numItems, CBoolVector &v)
{
  v.Clear();
  v.Reserve(numItems);
  Byte b = 0;
  Byte mask = 0;
  for(int i = 0; i < numItems; i++)
  {
    if (mask == 0)
    {
      b = ReadByte();
      mask = 0x80;
    }
    v.Add((b & mask) != 0);
    mask >>= 1;
  }
}

void CInArchive::ReadBoolVector2(int numItems, CBoolVector &v)
{
  Byte allAreDefined = ReadByte();
  if (allAreDefined == 0)
  {
    ReadBoolVector(numItems, v);
    return;
  }
  v.Clear();
  v.Reserve(numItems);
  for (int i = 0; i < numItems; i++)
    v.Add(true);
}

void CInArchive::ReadTime(const CObjectVector<CByteBuffer> &dataVector,
    CObjectVector<CFileItem> &files, UInt32 type)
{
  CBoolVector boolVector;
  ReadBoolVector2(files.Size(), boolVector);

  CStreamSwitch streamSwitch;
  streamSwitch.Set(this, &dataVector);

  for(int i = 0; i < files.Size(); i++)
  {
    CFileItem &file = files[i];
    CArchiveFileTime fileTime;
    fileTime.dwLowDateTime = 0;
    fileTime.dwHighDateTime = 0;
    bool defined = boolVector[i];
    if (defined)
    {
      fileTime.dwLowDateTime = ReadUInt32();
      fileTime.dwHighDateTime = ReadUInt32();
    }
    switch(type)
    {
      case NID::kCreationTime:
        file.IsCreationTimeDefined = defined;
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
}

HRESULT CInArchive::ReadAndDecodePackedStreams(
    DECL_EXTERNAL_CODECS_LOC_VARS
    UInt64 baseOffset, 
    UInt64 &dataOffset, CObjectVector<CByteBuffer> &dataVector
    #ifndef _NO_CRYPTO
    , ICryptoGetTextPassword *getTextPassword
    #endif
    )
{
  CRecordVector<UInt64> packSizes;
  CRecordVector<bool> packCRCsDefined;
  CRecordVector<UInt32> packCRCs;
  CObjectVector<CFolder> folders;
  
  CRecordVector<CNum> numUnPackStreamsInFolders;
  CRecordVector<UInt64> unPackSizes;
  CRecordVector<bool> digestsDefined;
  CRecordVector<UInt32> digests;
  
  ReadStreamsInfo(NULL, 
    dataOffset,
    packSizes, 
    packCRCsDefined, 
    packCRCs, 
    folders,
    numUnPackStreamsInFolders,
    unPackSizes,
    digestsDefined, 
    digests);
  
  // database.ArchiveInfo.DataStartPosition2 += database.ArchiveInfo.StartPositionAfterHeader;
  
  CNum packIndex = 0;
  CDecoder decoder(
    #ifdef _ST_MODE
    false
    #else
    true
    #endif
    );
  UInt64 dataStartPos = baseOffset + dataOffset;
  for(int i = 0; i < folders.Size(); i++)
  {
    const CFolder &folder = folders[i];
    dataVector.Add(CByteBuffer());
    CByteBuffer &data = dataVector.Back();
    UInt64 unPackSize64 = folder.GetUnPackSize();
    size_t unPackSize = (size_t)unPackSize64;
    if (unPackSize != unPackSize64)
      ThrowUnsupported();
    data.SetCapacity(unPackSize);
    
    CSequentialOutStreamImp2 *outStreamSpec = new CSequentialOutStreamImp2;
    CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
    outStreamSpec->Init(data, unPackSize);
    
    HRESULT result = decoder.Decode(
      EXTERNAL_CODECS_LOC_VARS
      _stream, dataStartPos, 
      &packSizes[packIndex], folder, outStream, NULL
      #ifndef _NO_CRYPTO
      , getTextPassword
      #endif
      #ifdef COMPRESS_MT
      , false, 1
      #endif
      );
    RINOK(result);
    
    if (folder.UnPackCRCDefined)
      if (CrcCalc(data, unPackSize) != folder.UnPackCRC)
        ThrowIncorrect();
      for (int j = 0; j < folder.PackStreams.Size(); j++)
        dataStartPos += packSizes[packIndex++];
  }
  return S_OK;
}

HRESULT CInArchive::ReadHeader(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CArchiveDatabaseEx &database
    #ifndef _NO_CRYPTO
    , ICryptoGetTextPassword *getTextPassword
    #endif
    )
{
  UInt64 type = ReadID();

  if (type == NID::kArchiveProperties)
  {
    ReadArchiveProperties(database.ArchiveInfo);
    type = ReadID();
  }
 
  CObjectVector<CByteBuffer> dataVector;
  
  if (type == NID::kAdditionalStreamsInfo)
  {
    HRESULT result = ReadAndDecodePackedStreams(
        EXTERNAL_CODECS_LOC_VARS
        database.ArchiveInfo.StartPositionAfterHeader, 
        database.ArchiveInfo.DataStartPosition2,
        dataVector
        #ifndef _NO_CRYPTO
        , getTextPassword
        #endif
        );
    RINOK(result);
    database.ArchiveInfo.DataStartPosition2 += database.ArchiveInfo.StartPositionAfterHeader;
    type = ReadID();
  }

  CRecordVector<UInt64> unPackSizes;
  CRecordVector<bool> digestsDefined;
  CRecordVector<UInt32> digests;
  
  if (type == NID::kMainStreamsInfo)
  {
    ReadStreamsInfo(&dataVector,
        database.ArchiveInfo.DataStartPosition,
        database.PackSizes, 
        database.PackCRCsDefined, 
        database.PackCRCs, 
        database.Folders,
        database.NumUnPackStreamsVector,
        unPackSizes,
        digestsDefined,
        digests);
    database.ArchiveInfo.DataStartPosition += database.ArchiveInfo.StartPositionAfterHeader;
    type = ReadID();
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

  database.Files.Clear();

  if (type == NID::kEnd)
    return S_OK;
  if (type != NID::kFilesInfo)
    ThrowIncorrect();
  
  CNum numFiles = ReadNum();
  database.Files.Reserve(numFiles);
  CNum i;
  for(i = 0; i < numFiles; i++)
    database.Files.Add(CFileItem());

  database.ArchiveInfo.FileInfoPopIDs.Add(NID::kSize);
  if (!database.PackSizes.IsEmpty())
    database.ArchiveInfo.FileInfoPopIDs.Add(NID::kPackInfo);
  if (numFiles > 0  && !digests.IsEmpty())
    database.ArchiveInfo.FileInfoPopIDs.Add(NID::kCRC);

  CBoolVector emptyStreamVector;
  emptyStreamVector.Reserve((int)numFiles);
  for(i = 0; i < numFiles; i++)
    emptyStreamVector.Add(false);
  CBoolVector emptyFileVector;
  CBoolVector antiFileVector;
  CNum numEmptyStreams = 0;

  for (;;)
  {
    UInt64 type = ReadID();
    if (type == NID::kEnd)
      break;
    UInt64 size = ReadNumber();
    bool isKnownType = true;
    if (type > ((UInt32)1 << 30))
      isKnownType = false;
    else switch((UInt32)type)
    {
      case NID::kName:
      {
        CStreamSwitch streamSwitch;
        streamSwitch.Set(this, &dataVector);
        for(int i = 0; i < database.Files.Size(); i++)
          _inByteBack->ReadString(database.Files[i].Name);
        break;
      }
      case NID::kWinAttributes:
      {
        CBoolVector boolVector;
        ReadBoolVector2(database.Files.Size(), boolVector);
        CStreamSwitch streamSwitch;
        streamSwitch.Set(this, &dataVector);
        for(i = 0; i < numFiles; i++)
        {
          CFileItem &file = database.Files[i];
          file.AreAttributesDefined = boolVector[i];
          if (file.AreAttributesDefined)
            file.Attributes = ReadUInt32();
        }
        break;
      }
      case NID::kStartPos:
      {
        CBoolVector boolVector;
        ReadBoolVector2(database.Files.Size(), boolVector);
        CStreamSwitch streamSwitch;
        streamSwitch.Set(this, &dataVector);
        for(i = 0; i < numFiles; i++)
        {
          CFileItem &file = database.Files[i];
          file.IsStartPosDefined = boolVector[i];
          if (file.IsStartPosDefined)
            file.StartPos = ReadUInt64();
        }
        break;
      }
      case NID::kEmptyStream:
      {
        ReadBoolVector(numFiles, emptyStreamVector);
        for (i = 0; i < (CNum)emptyStreamVector.Size(); i++)
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
        ReadBoolVector(numEmptyStreams, emptyFileVector);
        break;
      }
      case NID::kAnti:
      {
        ReadBoolVector(numEmptyStreams, antiFileVector);
        break;
      }
      case NID::kCreationTime:
      case NID::kLastWriteTime:
      case NID::kLastAccessTime:
      {
        ReadTime(dataVector, database.Files, (UInt32)type);
        break;
      }
      default:
        isKnownType = false;
    }
    if (isKnownType)
      database.ArchiveInfo.FileInfoPopIDs.Add(type);
    else
      SkeepData(size);
  }

  CNum emptyFileIndex = 0;
  CNum sizeIndex = 0;
  for(i = 0; i < numFiles; i++)
  {
    CFileItem &file = database.Files[i];
    file.HasStream = !emptyStreamVector[i];
    if(file.HasStream)
    {
      file.IsDirectory = false;
      file.IsAnti = false;
      file.UnPackSize = unPackSizes[sizeIndex];
      file.FileCRC = digests[sizeIndex];
      file.IsFileCRCDefined = digestsDefined[sizeIndex];
      sizeIndex++;
    }
    else
    {
      file.IsDirectory = !emptyFileVector[emptyFileIndex];
      file.IsAnti = antiFileVector[emptyFileIndex];
      emptyFileIndex++;
      file.UnPackSize = 0;
      file.IsFileCRCDefined = false;
    }
  }
  return S_OK;
}


void CArchiveDatabaseEx::FillFolderStartPackStream()
{
  FolderStartPackStreamIndex.Clear();
  FolderStartPackStreamIndex.Reserve(Folders.Size());
  CNum startPos = 0;
  for(int i = 0; i < Folders.Size(); i++)
  {
    FolderStartPackStreamIndex.Add(startPos);
    startPos += (CNum)Folders[i].PackStreams.Size();
  }
}

void CArchiveDatabaseEx::FillStartPos()
{
  PackStreamStartPositions.Clear();
  PackStreamStartPositions.Reserve(PackSizes.Size());
  UInt64 startPos = 0;
  for(int i = 0; i < PackSizes.Size(); i++)
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
  CNum indexInFolder = 0;
  for (int i = 0; i < Files.Size(); i++)
  {
    const CFileItem &file = Files[i];
    bool emptyStream = !file.HasStream;
    if (emptyStream && indexInFolder == 0)
    {
      FileIndexToFolderIndexMap.Add(kNumNoIndex);
      continue;
    }
    if (indexInFolder == 0)
    {
      // v3.13 incorrectly worked with empty folders
      // v4.07: Loop for skipping empty folders
      for (;;)
      {
        if (folderIndex >= Folders.Size())
          ThrowIncorrect();
        FolderStartFileIndex.Add(i); // check it
        if (NumUnPackStreamsVector[folderIndex] != 0)
          break;
        folderIndex++;
      }
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

HRESULT CInArchive::ReadDatabase2(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CArchiveDatabaseEx &database
    #ifndef _NO_CRYPTO
    , ICryptoGetTextPassword *getTextPassword
    #endif
    )
{
  database.Clear();
  database.ArchiveInfo.StartPosition = _arhiveBeginStreamPosition;

  database.ArchiveInfo.Version.Major = _header[6];
  database.ArchiveInfo.Version.Minor = _header[7];

  if (database.ArchiveInfo.Version.Major != kMajorVersion)
    ThrowUnsupportedVersion();

  UInt32 crcFromArchive = GetUInt32FromMem(_header + 8);
  UInt64 nextHeaderOffset = GetUInt64FromMem(_header + 0xC);
  UInt64 nextHeaderSize = GetUInt64FromMem(_header + 0x14);
  UInt32 nextHeaderCRC =  GetUInt32FromMem(_header + 0x1C);
  UInt32 crc = CrcCalc(_header + 0xC, 20);

  #ifdef FORMAT_7Z_RECOVERY
  if (crcFromArchive == 0 && nextHeaderOffset == 0 && nextHeaderSize == 0 && nextHeaderCRC == 0)
  {
    UInt64 cur, cur2;
    RINOK(_stream->Seek(0, STREAM_SEEK_CUR, &cur));
    const int kCheckSize = 500;
    Byte buf[kCheckSize];
    RINOK(_stream->Seek(0, STREAM_SEEK_END, &cur2));
    int checkSize = kCheckSize;
    if (cur2 - cur < kCheckSize)
      checkSize = (int)(cur2 - cur);
    RINOK(_stream->Seek(-checkSize, STREAM_SEEK_END, &cur2));
    
    UInt32 realProcessedSize;
    RINOK(_stream->Read(buf, (UInt32)kCheckSize, &realProcessedSize));

    int i;
    for (i = (int)realProcessedSize - 2; i >= 0; i--)
      if (buf[i] == 0x17 && buf[i + 1] == 0x6 || buf[i] == 0x01 && buf[i + 1] == 0x04)
        break;
    if (i < 0)
      return S_FALSE;
    nextHeaderSize = realProcessedSize - i;
    nextHeaderOffset = cur2 - cur + i;
    nextHeaderCRC = CrcCalc(buf + i, (size_t)nextHeaderSize);
    RINOK(_stream->Seek(cur, STREAM_SEEK_SET, NULL));
  }
  #endif

  #ifdef FORMAT_7Z_RECOVERY
  crcFromArchive = crc;
  #endif

  database.ArchiveInfo.StartPositionAfterHeader = _arhiveBeginStreamPosition + kHeaderSize;

  if (crc != crcFromArchive)
    ThrowIncorrect();

  if (nextHeaderSize == 0)
    return S_OK;

  if (nextHeaderSize > (UInt64)0xFFFFFFFF)
    return S_FALSE;

  RINOK(_stream->Seek(nextHeaderOffset, STREAM_SEEK_CUR, NULL));

  CByteBuffer buffer2;
  buffer2.SetCapacity((size_t)nextHeaderSize);

  UInt32 realProcessedSize;
  RINOK(_stream->Read(buffer2, (UInt32)nextHeaderSize, &realProcessedSize));
  if (realProcessedSize != (UInt32)nextHeaderSize)
    return S_FALSE;
  if (CrcCalc(buffer2, (UInt32)nextHeaderSize) != nextHeaderCRC)
    ThrowIncorrect();
  
  CStreamSwitch streamSwitch;
  streamSwitch.Set(this, buffer2);
  
  CObjectVector<CByteBuffer> dataVector;
  
  for (;;)
  {
    UInt64 type = ReadID();
    if (type == NID::kHeader)
      break;
    if (type != NID::kEncodedHeader)
      ThrowIncorrect();
    HRESULT result = ReadAndDecodePackedStreams(
        EXTERNAL_CODECS_LOC_VARS
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
      ThrowIncorrect();
    streamSwitch.Remove();
    streamSwitch.Set(this, dataVector.Front());
  }

  return ReadHeader(
    EXTERNAL_CODECS_LOC_VARS
    database
    #ifndef _NO_CRYPTO
    , getTextPassword
    #endif
    );
}

HRESULT CInArchive::ReadDatabase(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CArchiveDatabaseEx &database
    #ifndef _NO_CRYPTO
    , ICryptoGetTextPassword *getTextPassword
    #endif
    )
{
  try
  {
    return ReadDatabase2(
      EXTERNAL_CODECS_LOC_VARS database
      #ifndef _NO_CRYPTO
      , getTextPassword
      #endif
      );
  }
  catch(CInArchiveException &) { return S_FALSE; }
}

}}
