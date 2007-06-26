// 7zOut.cpp

#include "StdAfx.h"

#include "../../../Common/AutoPtr.h"
#include "../../Common/StreamObjects.h"

#include "7zOut.h"

extern "C" 
{ 
#include "../../../../C/7zCrc.h"
}

static HRESULT WriteBytes(ISequentialOutStream *stream, const void *data, size_t size)
{
  while (size > 0)
  {
    UInt32 curSize = (UInt32)MyMin(size, (size_t)0xFFFFFFFF);
    UInt32 processedSize;
    RINOK(stream->Write(data, curSize, &processedSize));
    if(processedSize == 0)
      return E_FAIL;
    data = (const void *)((const Byte *)data + processedSize);
    size -= processedSize;
  }
  return S_OK;
}

namespace NArchive {
namespace N7z {

HRESULT COutArchive::WriteDirect(const void *data, UInt32 size)
{
  return ::WriteBytes(SeqStream, data, size);
}

UInt32 CrcUpdateUInt32(UInt32 crc, UInt32 value)
{
  for (int i = 0; i < 4; i++, value >>= 8)
    crc = CRC_UPDATE_BYTE(crc, (Byte)value);
  return crc;
}

UInt32 CrcUpdateUInt64(UInt32 crc, UInt64 value)
{
  for (int i = 0; i < 8; i++, value >>= 8)
    crc = CRC_UPDATE_BYTE(crc, (Byte)value);
  return crc;
}

HRESULT COutArchive::WriteDirectUInt32(UInt32 value)
{
  for (int i = 0; i < 4; i++)
  {
    RINOK(WriteDirectByte((Byte)value));
    value >>= 8;
  }
  return S_OK;
}

HRESULT COutArchive::WriteDirectUInt64(UInt64 value)
{
  for (int i = 0; i < 8; i++)
  {
    RINOK(WriteDirectByte((Byte)value));
    value >>= 8;
  }
  return S_OK;
}

HRESULT COutArchive::WriteSignature()
{
  RINOK(WriteDirect(kSignature, kSignatureSize));
  RINOK(WriteDirectByte(kMajorVersion));
  return WriteDirectByte(2);
}

#ifdef _7Z_VOL
HRESULT COutArchive::WriteFinishSignature()
{
  RINOK(WriteDirect(kFinishSignature, kSignatureSize));
  CArchiveVersion av;
  av.Major = kMajorVersion;
  av.Minor = 2;
  RINOK(WriteDirectByte(av.Major));
  return WriteDirectByte(av.Minor);
}
#endif

HRESULT COutArchive::WriteStartHeader(const CStartHeader &h)
{
  UInt32 crc = CRC_INIT_VAL;
  crc = CrcUpdateUInt64(crc, h.NextHeaderOffset);
  crc = CrcUpdateUInt64(crc, h.NextHeaderSize);
  crc = CrcUpdateUInt32(crc, h.NextHeaderCRC);
  RINOK(WriteDirectUInt32(CRC_GET_DIGEST(crc)));
  RINOK(WriteDirectUInt64(h.NextHeaderOffset));
  RINOK(WriteDirectUInt64(h.NextHeaderSize));
  return WriteDirectUInt32(h.NextHeaderCRC);
}

#ifdef _7Z_VOL
HRESULT COutArchive::WriteFinishHeader(const CFinishHeader &h)
{
  CCRC crc;
  crc.UpdateUInt64(h.NextHeaderOffset);
  crc.UpdateUInt64(h.NextHeaderSize);
  crc.UpdateUInt32(h.NextHeaderCRC);
  crc.UpdateUInt64(h.ArchiveStartOffset);
  crc.UpdateUInt64(h.AdditionalStartBlockSize);
  RINOK(WriteDirectUInt32(crc.GetDigest()));
  RINOK(WriteDirectUInt64(h.NextHeaderOffset));
  RINOK(WriteDirectUInt64(h.NextHeaderSize));
  RINOK(WriteDirectUInt32(h.NextHeaderCRC));
  RINOK(WriteDirectUInt64(h.ArchiveStartOffset));
  return WriteDirectUInt64(h.AdditionalStartBlockSize);
}
#endif

HRESULT COutArchive::Create(ISequentialOutStream *stream, bool endMarker)
{
  Close();
  #ifdef _7Z_VOL
  // endMarker = false;
  _endMarker = endMarker;
  #endif
  SeqStream = stream;
  if (!endMarker)
  {
    SeqStream.QueryInterface(IID_IOutStream, &Stream);
    if (!Stream)
    {
      return E_NOTIMPL;
      // endMarker = true;
    }
  }
  #ifdef _7Z_VOL
  if (endMarker)
  {
    /*
    CStartHeader sh;
    sh.NextHeaderOffset = (UInt32)(Int32)-1;
    sh.NextHeaderSize = (UInt32)(Int32)-1;
    sh.NextHeaderCRC = 0;
    WriteStartHeader(sh);
    */
  }
  else
  #endif
  {
    if (!Stream)
      return E_FAIL;
    RINOK(WriteSignature());
    RINOK(Stream->Seek(0, STREAM_SEEK_CUR, &_prefixHeaderPos));
  }
  return S_OK;
}

void COutArchive::Close()
{
  SeqStream.Release();
  Stream.Release();
}

HRESULT COutArchive::SkeepPrefixArchiveHeader()
{
  #ifdef _7Z_VOL
  if (_endMarker)
    return S_OK;
  #endif
  return Stream->Seek(24, STREAM_SEEK_CUR, NULL);
}

HRESULT COutArchive::WriteBytes(const void *data, size_t size)
{
  if (_mainMode)
  {
    if (_dynamicMode)
      _dynamicBuffer.Write(data, size);
    else
      _outByte.WriteBytes(data, size);
    _crc = CrcUpdate(_crc, data, size);
  }
  else
  {
    if (_countMode)
      _countSize += size;
    else
      RINOK(_outByte2.Write(data, size));
  }
  return S_OK;
}

HRESULT COutArchive::WriteBytes(const CByteBuffer &data)
{
  return WriteBytes(data, data.GetCapacity());
}

HRESULT COutArchive::WriteByte(Byte b)
{
  return WriteBytes(&b, 1);
}

HRESULT COutArchive::WriteUInt32(UInt32 value)
{
  for (int i = 0; i < 4; i++)
  {
    RINOK(WriteByte((Byte)value));
    value >>= 8;
  }
  return S_OK;
}

HRESULT COutArchive::WriteNumber(UInt64 value)
{
  Byte firstByte = 0;
  Byte mask = 0x80;
  int i;
  for (i = 0; i < 8; i++)
  {
    if (value < ((UInt64(1) << ( 7  * (i + 1)))))
    {
      firstByte |= Byte(value >> (8 * i));
      break;
    }
    firstByte |= mask;
    mask >>= 1;
  }
  RINOK(WriteByte(firstByte));
  for (;i > 0; i--)
  {
    RINOK(WriteByte((Byte)value));
    value >>= 8;
  }
  return S_OK;
}

#ifdef _7Z_VOL
static UInt32 GetBigNumberSize(UInt64 value)
{
  int i;
  for (i = 0; i < 8; i++)
    if (value < ((UInt64(1) << ( 7  * (i + 1)))))
      break;
  return 1 + i;
}

UInt32 COutArchive::GetVolHeadersSize(UInt64 dataSize, int nameLength, bool props)
{
  UInt32 result = GetBigNumberSize(dataSize) * 2 + 41;
  if (nameLength != 0)
  {
    nameLength = (nameLength + 1) * 2;
    result += nameLength + GetBigNumberSize(nameLength) + 2;
  }
  if (props)
  {
    result += 20;
  }
  if (result >= 128)
    result++;
  result += kSignatureSize + 2 + kFinishHeaderSize;
  return result;
}

UInt64 COutArchive::GetVolPureSize(UInt64 volSize, int nameLength, bool props)
{
  UInt32 headersSizeBase = COutArchive::GetVolHeadersSize(1, nameLength, props);
  int testSize;
  if (volSize > headersSizeBase)
    testSize = volSize - headersSizeBase;
  else
    testSize = 1;
  UInt32 headersSize = COutArchive::GetVolHeadersSize(testSize, nameLength, props);
  UInt64 pureSize = 1;
  if (volSize > headersSize)
    pureSize = volSize - headersSize;
  return pureSize;
}
#endif

HRESULT COutArchive::WriteFolder(const CFolder &folder)
{
  RINOK(WriteNumber(folder.Coders.Size()));
  int i;
  for (i = 0; i < folder.Coders.Size(); i++)
  {
    const CCoderInfo &coder = folder.Coders[i];
    {
      size_t propertiesSize = coder.Properties.GetCapacity();
      
      UInt64 id = coder.MethodID; 
      int idSize;
      for (idSize = 1; idSize < sizeof(id); idSize++)
        if ((id >> (8 * idSize)) == 0)
          break;
      BYTE longID[15];
      for (int t = idSize - 1; t >= 0 ; t--, id >>= 8)
        longID[t] = (Byte)(id & 0xFF);
      Byte b;
      b = (Byte)(idSize & 0xF);
      bool isComplex = !coder.IsSimpleCoder();
      b |= (isComplex ? 0x10 : 0);
      b |= ((propertiesSize != 0) ? 0x20 : 0 );
      RINOK(WriteByte(b));
      RINOK(WriteBytes(longID, idSize));
      if (isComplex)
      {
        RINOK(WriteNumber(coder.NumInStreams));
        RINOK(WriteNumber(coder.NumOutStreams));
      }
      if (propertiesSize == 0)
        continue;
      RINOK(WriteNumber(propertiesSize));
      RINOK(WriteBytes(coder.Properties, propertiesSize));
    }
  }
  for (i = 0; i < folder.BindPairs.Size(); i++)
  {
    const CBindPair &bindPair = folder.BindPairs[i];
    RINOK(WriteNumber(bindPair.InIndex));
    RINOK(WriteNumber(bindPair.OutIndex));
  }
  if (folder.PackStreams.Size() > 1)
    for (i = 0; i < folder.PackStreams.Size(); i++)
    {
      RINOK(WriteNumber(folder.PackStreams[i]));
    }
  return S_OK;
}

HRESULT COutArchive::WriteBoolVector(const CBoolVector &boolVector)
{
  Byte b = 0;
  Byte mask = 0x80;
  for(int i = 0; i < boolVector.Size(); i++)
  {
    if (boolVector[i])
      b |= mask;
    mask >>= 1;
    if (mask == 0)
    {
      RINOK(WriteByte(b));
      mask = 0x80;
      b = 0;
    }
  }
  if (mask != 0x80)
  {
    RINOK(WriteByte(b));
  }
  return S_OK;
}


HRESULT COutArchive::WriteHashDigests(
    const CRecordVector<bool> &digestsDefined,
    const CRecordVector<UInt32> &digests)
{
  int numDefined = 0;
  int i;
  for(i = 0; i < digestsDefined.Size(); i++)
    if (digestsDefined[i])
      numDefined++;
  if (numDefined == 0)
    return S_OK;

  RINOK(WriteByte(NID::kCRC));
  if (numDefined == digestsDefined.Size())
  {
    RINOK(WriteByte(1));
  }
  else
  {
    RINOK(WriteByte(0));
    RINOK(WriteBoolVector(digestsDefined));
  }
  for(i = 0; i < digests.Size(); i++)
  {
    if(digestsDefined[i])
      RINOK(WriteUInt32(digests[i]));
  }
  return S_OK;
}

HRESULT COutArchive::WritePackInfo(
    UInt64 dataOffset,
    const CRecordVector<UInt64> &packSizes,
    const CRecordVector<bool> &packCRCsDefined,
    const CRecordVector<UInt32> &packCRCs)
{
  if (packSizes.IsEmpty())
    return S_OK;
  RINOK(WriteByte(NID::kPackInfo));
  RINOK(WriteNumber(dataOffset));
  RINOK(WriteNumber(packSizes.Size()));
  RINOK(WriteByte(NID::kSize));
  for(int i = 0; i < packSizes.Size(); i++)
    RINOK(WriteNumber(packSizes[i]));

  RINOK(WriteHashDigests(packCRCsDefined, packCRCs));
  
  return WriteByte(NID::kEnd);
}

HRESULT COutArchive::WriteUnPackInfo(const CObjectVector<CFolder> &folders)
{
  if (folders.IsEmpty())
    return S_OK;

  RINOK(WriteByte(NID::kUnPackInfo));

  RINOK(WriteByte(NID::kFolder));
  RINOK(WriteNumber(folders.Size()));
  {
    RINOK(WriteByte(0));
    for(int i = 0; i < folders.Size(); i++)
      RINOK(WriteFolder(folders[i]));
  }
  
  RINOK(WriteByte(NID::kCodersUnPackSize));
  int i;
  for(i = 0; i < folders.Size(); i++)
  {
    const CFolder &folder = folders[i];
    for (int j = 0; j < folder.UnPackSizes.Size(); j++)
      RINOK(WriteNumber(folder.UnPackSizes[j]));
  }

  CRecordVector<bool> unPackCRCsDefined;
  CRecordVector<UInt32> unPackCRCs;
  for(i = 0; i < folders.Size(); i++)
  {
    const CFolder &folder = folders[i];
    unPackCRCsDefined.Add(folder.UnPackCRCDefined);
    unPackCRCs.Add(folder.UnPackCRC);
  }
  RINOK(WriteHashDigests(unPackCRCsDefined, unPackCRCs));

  return WriteByte(NID::kEnd);
}

HRESULT COutArchive::WriteSubStreamsInfo(
    const CObjectVector<CFolder> &folders,
    const CRecordVector<CNum> &numUnPackStreamsInFolders,
    const CRecordVector<UInt64> &unPackSizes,
    const CRecordVector<bool> &digestsDefined,
    const CRecordVector<UInt32> &digests)
{
  RINOK(WriteByte(NID::kSubStreamsInfo));

  int i;
  for(i = 0; i < numUnPackStreamsInFolders.Size(); i++)
  {
    if (numUnPackStreamsInFolders[i] != 1)
    {
      RINOK(WriteByte(NID::kNumUnPackStream));
      for(i = 0; i < numUnPackStreamsInFolders.Size(); i++)
        RINOK(WriteNumber(numUnPackStreamsInFolders[i]));
      break;
    }
  }
 

  bool needFlag = true;
  CNum index = 0;
  for(i = 0; i < numUnPackStreamsInFolders.Size(); i++)
    for (CNum j = 0; j < numUnPackStreamsInFolders[i]; j++)
    {
      if (j + 1 != numUnPackStreamsInFolders[i])
      {
        if (needFlag)
          RINOK(WriteByte(NID::kSize));
        needFlag = false;
        RINOK(WriteNumber(unPackSizes[index]));
      }
      index++;
    }

  CRecordVector<bool> digestsDefined2;
  CRecordVector<UInt32> digests2;

  int digestIndex = 0;
  for (i = 0; i < folders.Size(); i++)
  {
    int numSubStreams = (int)numUnPackStreamsInFolders[i];
    if (numSubStreams == 1 && folders[i].UnPackCRCDefined)
      digestIndex++;
    else
      for (int j = 0; j < numSubStreams; j++, digestIndex++)
      {
        digestsDefined2.Add(digestsDefined[digestIndex]);
        digests2.Add(digests[digestIndex]);
      }
  }
  RINOK(WriteHashDigests(digestsDefined2, digests2));
  return WriteByte(NID::kEnd);
}

HRESULT COutArchive::WriteTime(
    const CObjectVector<CFileItem> &files, Byte type)
{
  /////////////////////////////////////////////////
  // CreationTime
  CBoolVector boolVector;
  boolVector.Reserve(files.Size());
  bool thereAreDefined = false;
  bool allDefined = true;
  int i;
  for(i = 0; i < files.Size(); i++)
  {
    const CFileItem &item = files[i];
    bool defined;
    switch(type)
    {
      case NID::kCreationTime:
        defined = item.IsCreationTimeDefined;
        break;
      case NID::kLastWriteTime:
        defined = item.IsLastWriteTimeDefined;
        break;
      case NID::kLastAccessTime:
        defined = item.IsLastAccessTimeDefined;
        break;
      default:
        throw 1;
    }
    boolVector.Add(defined);
    thereAreDefined = (thereAreDefined || defined);
    allDefined = (allDefined && defined);
  }
  if (!thereAreDefined)
    return S_OK;
  RINOK(WriteByte(type));
  size_t dataSize = 1 + 1;
    dataSize += files.Size() * 8;
  if (allDefined)
  {
    RINOK(WriteNumber(dataSize));
    WriteByte(1);
  }
  else
  {
    RINOK(WriteNumber(1 + (boolVector.Size() + 7) / 8 + dataSize));
    WriteByte(0);
    RINOK(WriteBoolVector(boolVector));
  }
  RINOK(WriteByte(0));
  for(i = 0; i < files.Size(); i++)
  {
    if (boolVector[i])
    {
      const CFileItem &item = files[i];
      CArchiveFileTime timeValue;
      timeValue.dwLowDateTime = 0;
      timeValue.dwHighDateTime = 0;
      switch(type)
      {
        case NID::kCreationTime:
          timeValue = item.CreationTime;
          break;
        case NID::kLastWriteTime:
          timeValue = item.LastWriteTime;
          break;
        case NID::kLastAccessTime:
          timeValue = item.LastAccessTime;
          break;
      }
      RINOK(WriteUInt32(timeValue.dwLowDateTime));
      RINOK(WriteUInt32(timeValue.dwHighDateTime));
    }
  }
  return S_OK;
}

HRESULT COutArchive::EncodeStream(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CEncoder &encoder, const Byte *data, size_t dataSize,
    CRecordVector<UInt64> &packSizes, CObjectVector<CFolder> &folders)
{
  CSequentialInStreamImp *streamSpec = new CSequentialInStreamImp;
  CMyComPtr<ISequentialInStream> stream = streamSpec;
  streamSpec->Init(data, dataSize);
  CFolder folderItem;
  folderItem.UnPackCRCDefined = true;
  folderItem.UnPackCRC = CrcCalc(data, dataSize);
  UInt64 dataSize64 = dataSize;
  RINOK(encoder.Encode(
      EXTERNAL_CODECS_LOC_VARS
      stream, NULL, &dataSize64, folderItem, SeqStream, packSizes, NULL))
  folders.Add(folderItem);
  return S_OK;
}

HRESULT COutArchive::EncodeStream(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CEncoder &encoder, const CByteBuffer &data, 
    CRecordVector<UInt64> &packSizes, CObjectVector<CFolder> &folders)
{
  return EncodeStream(
      EXTERNAL_CODECS_LOC_VARS
      encoder, data, data.GetCapacity(), packSizes, folders);
}

static void WriteUInt32ToBuffer(Byte *data, UInt32 value)
{
  for (int i = 0; i < 4; i++)
  {
    *data++ = (Byte)value;
    value >>= 8;
  }
}

static void WriteUInt64ToBuffer(Byte *data, UInt64 value)
{
  for (int i = 0; i < 8; i++)
  {
    *data++ = (Byte)value;
    value >>= 8;
  }
}


HRESULT COutArchive::WriteHeader(
    const CArchiveDatabase &database,
    const CHeaderOptions &headerOptions,
    UInt64 &headerOffset)
{
  int i;
  
  /////////////////////////////////
  // Names

  CNum numDefinedNames = 0;
  size_t namesDataSize = 0;
  for(i = 0; i < database.Files.Size(); i++)
  {
    const UString &name = database.Files[i].Name;
    if (!name.IsEmpty())
      numDefinedNames++;
    namesDataSize += (name.Length() + 1) * 2;
  }

  CByteBuffer namesData;
  if (numDefinedNames > 0)
  {
    namesData.SetCapacity((size_t)namesDataSize);
    size_t pos = 0;
    for(int i = 0; i < database.Files.Size(); i++)
    {
      const UString &name = database.Files[i].Name;
      for (int t = 0; t < name.Length(); t++)
      {
        wchar_t c = name[t];
        namesData[pos++] = Byte(c);
        namesData[pos++] = Byte(c >> 8);
      }
      namesData[pos++] = 0;
      namesData[pos++] = 0;
    }
  }

  /////////////////////////////////
  // Write Attributes
  CBoolVector attributesBoolVector;
  attributesBoolVector.Reserve(database.Files.Size());
  int numDefinedAttributes = 0;
  for(i = 0; i < database.Files.Size(); i++)
  {
    bool defined = database.Files[i].AreAttributesDefined;
    attributesBoolVector.Add(defined);
    if (defined)
      numDefinedAttributes++;
  }

  CByteBuffer attributesData;
  if (numDefinedAttributes > 0)
  {
    attributesData.SetCapacity(numDefinedAttributes * 4);
    size_t pos = 0;
    for(i = 0; i < database.Files.Size(); i++)
    {
      const CFileItem &file = database.Files[i];
      if (file.AreAttributesDefined)
      {
        WriteUInt32ToBuffer(attributesData + pos, file.Attributes);
        pos += 4;
      }
    }
  }

  /////////////////////////////////
  // Write StartPos
  CBoolVector startsBoolVector;
  startsBoolVector.Reserve(database.Files.Size());
  int numDefinedStarts = 0;
  for(i = 0; i < database.Files.Size(); i++)
  {
    bool defined = database.Files[i].IsStartPosDefined;
    startsBoolVector.Add(defined);
    if (defined)
      numDefinedStarts++;
  }

  CByteBuffer startsData;
  if (numDefinedStarts > 0)
  {
    startsData.SetCapacity(numDefinedStarts * 8);
    size_t pos = 0;
    for(i = 0; i < database.Files.Size(); i++)
    {
      const CFileItem &file = database.Files[i];
      if (file.IsStartPosDefined)
      {
        WriteUInt64ToBuffer(startsData + pos, file.StartPos);
        pos += 8;
      }
    }
  }
  
  /////////////////////////////////
  // Write Last Write Time
  // /*
  CNum numDefinedLastWriteTimes = 0;
  for(i = 0; i < database.Files.Size(); i++)
    if (database.Files[i].IsLastWriteTimeDefined)
      numDefinedLastWriteTimes++;

  if (numDefinedLastWriteTimes > 0)
  {
    CByteBuffer lastWriteTimeData;
    lastWriteTimeData.SetCapacity(numDefinedLastWriteTimes * 8);
    size_t pos = 0;
    for(i = 0; i < database.Files.Size(); i++)
    {
      const CFileItem &file = database.Files[i];
      if (file.IsLastWriteTimeDefined)
      {
        WriteUInt32ToBuffer(lastWriteTimeData + pos, file.LastWriteTime.dwLowDateTime);
        pos += 4;
        WriteUInt32ToBuffer(lastWriteTimeData + pos, file.LastWriteTime.dwHighDateTime);
        pos += 4;
      }
    }
  }
  // */
  

  UInt64 packedSize = 0;
  for(i = 0; i < database.PackSizes.Size(); i++)
    packedSize += database.PackSizes[i];

  headerOffset = packedSize;

  _mainMode = true;

  _outByte.SetStream(SeqStream);
  _outByte.Init();
  _crc = CRC_INIT_VAL;


  RINOK(WriteByte(NID::kHeader));

  // Archive Properties

  if (database.Folders.Size() > 0)
  {
    RINOK(WriteByte(NID::kMainStreamsInfo));
    RINOK(WritePackInfo(0, database.PackSizes, 
        database.PackCRCsDefined,
        database.PackCRCs));

    RINOK(WriteUnPackInfo(database.Folders));

    CRecordVector<UInt64> unPackSizes;
    CRecordVector<bool> digestsDefined;
    CRecordVector<UInt32> digests;
    for (i = 0; i < database.Files.Size(); i++)
    {
      const CFileItem &file = database.Files[i];
      if (!file.HasStream)
        continue;
      unPackSizes.Add(file.UnPackSize);
      digestsDefined.Add(file.IsFileCRCDefined);
      digests.Add(file.FileCRC);
    }

    RINOK(WriteSubStreamsInfo(
        database.Folders,
        database.NumUnPackStreamsVector,
        unPackSizes,
        digestsDefined,
        digests));
    RINOK(WriteByte(NID::kEnd));
  }

  if (database.Files.IsEmpty())
  {
    RINOK(WriteByte(NID::kEnd));
    return _outByte.Flush();
  }

  RINOK(WriteByte(NID::kFilesInfo));
  RINOK(WriteNumber(database.Files.Size()));

  CBoolVector emptyStreamVector;
  emptyStreamVector.Reserve(database.Files.Size());
  int numEmptyStreams = 0;
  for(i = 0; i < database.Files.Size(); i++)
    if (database.Files[i].HasStream)
      emptyStreamVector.Add(false);
    else
    {
      emptyStreamVector.Add(true);
      numEmptyStreams++;
    }
  if (numEmptyStreams > 0)
  {
    RINOK(WriteByte(NID::kEmptyStream));
    RINOK(WriteNumber((emptyStreamVector.Size() + 7) / 8));
    RINOK(WriteBoolVector(emptyStreamVector));

    CBoolVector emptyFileVector, antiVector;
    emptyFileVector.Reserve(numEmptyStreams);
    antiVector.Reserve(numEmptyStreams);
    CNum numEmptyFiles = 0, numAntiItems = 0;
    for(i = 0; i < database.Files.Size(); i++)
    {
      const CFileItem &file = database.Files[i];
      if (!file.HasStream)
      {
        emptyFileVector.Add(!file.IsDirectory);
        if (!file.IsDirectory)
          numEmptyFiles++;
        antiVector.Add(file.IsAnti);
        if (file.IsAnti)
          numAntiItems++;
      }
    }

    if (numEmptyFiles > 0)
    {
      RINOK(WriteByte(NID::kEmptyFile));
      RINOK(WriteNumber((emptyFileVector.Size() + 7) / 8));
      RINOK(WriteBoolVector(emptyFileVector));
    }

    if (numAntiItems > 0)
    {
      RINOK(WriteByte(NID::kAnti));
      RINOK(WriteNumber((antiVector.Size() + 7) / 8));
      RINOK(WriteBoolVector(antiVector));
    }
  }

  if (numDefinedNames > 0)
  {
    /////////////////////////////////////////////////
    RINOK(WriteByte(NID::kName));
    {
      RINOK(WriteNumber(1 + namesData.GetCapacity()));
      RINOK(WriteByte(0));
      RINOK(WriteBytes(namesData));
    }

  }

  if (headerOptions.WriteCreated)
  {
    RINOK(WriteTime(database.Files, NID::kCreationTime));
  }
  if (headerOptions.WriteModified)
  {
    RINOK(WriteTime(database.Files, NID::kLastWriteTime));
  }
  if (headerOptions.WriteAccessed)
  {
    RINOK(WriteTime(database.Files, NID::kLastAccessTime));
  }

  if (numDefinedAttributes > 0)
  {
    RINOK(WriteByte(NID::kWinAttributes));
    size_t size = 2;
    if (numDefinedAttributes != database.Files.Size())
      size += (attributesBoolVector.Size() + 7) / 8 + 1;
      size += attributesData.GetCapacity();

    RINOK(WriteNumber(size));
    if (numDefinedAttributes == database.Files.Size())
    {
      RINOK(WriteByte(1));
    }
    else
    {
      RINOK(WriteByte(0));
      RINOK(WriteBoolVector(attributesBoolVector));
    }

    {
      RINOK(WriteByte(0));
      RINOK(WriteBytes(attributesData));
    }
  }

  if (numDefinedStarts > 0)
  {
    RINOK(WriteByte(NID::kStartPos));
    size_t size = 2;
    if (numDefinedStarts != database.Files.Size())
      size += (startsBoolVector.Size() + 7) / 8 + 1;
      size += startsData.GetCapacity();

    RINOK(WriteNumber(size));
    if (numDefinedStarts == database.Files.Size())
    {
      RINOK(WriteByte(1));
    }
    else
    {
      RINOK(WriteByte(0));
      RINOK(WriteBoolVector(startsBoolVector));
    }

    {
      RINOK(WriteByte(0));
      RINOK(WriteBytes(startsData));
    }
  }

  RINOK(WriteByte(NID::kEnd)); // for files
  RINOK(WriteByte(NID::kEnd)); // for headers

  return _outByte.Flush();
}

HRESULT COutArchive::WriteDatabase(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const CArchiveDatabase &database,
    const CCompressionMethodMode *options, 
    const CHeaderOptions &headerOptions)
{
  UInt64 headerOffset;
  UInt32 headerCRC;
  UInt64 headerSize;
  if (database.IsEmpty())
  {
    headerSize = 0;
    headerOffset = 0;
    headerCRC = CrcCalc(0, 0);
  }
  else
  {
    _dynamicBuffer.Init();
    _dynamicMode = false;

    if (options != 0)
      if (options->IsEmpty())
        options = 0;
    if (options != 0)
      if (options->PasswordIsDefined || headerOptions.CompressMainHeader)
        _dynamicMode = true;
    RINOK(WriteHeader(database, headerOptions, headerOffset));

    if (_dynamicMode)
    {
      CCompressionMethodMode encryptOptions;
      encryptOptions.PasswordIsDefined = options->PasswordIsDefined;
      encryptOptions.Password = options->Password;
      CEncoder encoder(headerOptions.CompressMainHeader ? *options : encryptOptions);
      CRecordVector<UInt64> packSizes;
      CObjectVector<CFolder> folders;
      RINOK(EncodeStream(
          EXTERNAL_CODECS_LOC_VARS
          encoder, _dynamicBuffer, 
          _dynamicBuffer.GetSize(), packSizes, folders));
      _dynamicMode = false;
      _mainMode = true;
      
      _outByte.SetStream(SeqStream);
      _outByte.Init();
      _crc = CRC_INIT_VAL;
      
      if (folders.Size() == 0)
        throw 1;

      RINOK(WriteID(NID::kEncodedHeader));
      RINOK(WritePackInfo(headerOffset, packSizes, 
        CRecordVector<bool>(), CRecordVector<UInt32>()));
      RINOK(WriteUnPackInfo(folders));
      RINOK(WriteByte(NID::kEnd));
      for (int i = 0; i < packSizes.Size(); i++)
        headerOffset += packSizes[i];
      RINOK(_outByte.Flush());
    }
    headerCRC = CRC_GET_DIGEST(_crc);
    headerSize = _outByte.GetProcessedSize();
  }
  #ifdef _7Z_VOL
  if (_endMarker)
  {
    CFinishHeader h;
    h.NextHeaderSize = headerSize;
    h.NextHeaderCRC = headerCRC;
    h.NextHeaderOffset = 
        UInt64(0) - (headerSize + 
        4 + kFinishHeaderSize);
    h.ArchiveStartOffset = h.NextHeaderOffset - headerOffset;
    h.AdditionalStartBlockSize = 0;
    RINOK(WriteFinishHeader(h));
    return WriteFinishSignature();
  }
  else
  #endif
  {
    CStartHeader h;
    h.NextHeaderSize = headerSize;
    h.NextHeaderCRC = headerCRC;
    h.NextHeaderOffset = headerOffset;
    RINOK(Stream->Seek(_prefixHeaderPos, STREAM_SEEK_SET, NULL));
    return WriteStartHeader(h);
  }
}

}}
