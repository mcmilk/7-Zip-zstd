// 7zOut.cpp

#include "StdAfx.h"

#include "7zOut.h"
#include "../../Common/StreamObjects.h"

static HRESULT WriteBytes(IOutStream *stream, const void *data, UINT32 size)
{
  UINT32 processedSize;
  RINOK(stream->Write(data, size, &processedSize));
  if(processedSize != size)
    return E_FAIL;
  return S_OK;
}

namespace NArchive {
namespace N7z {

HRESULT COutArchive::Create(IOutStream *stream)
{
  Close();
  RINOK(::WriteBytes(stream, kSignature, kSignatureSize));
  CArchiveVersion archiveVersion;
  archiveVersion.Major = kMajorVersion;
  archiveVersion.Minor = 2;
  RINOK(::WriteBytes(stream, &archiveVersion, sizeof(archiveVersion)));
  RINOK(stream->Seek(0, STREAM_SEEK_CUR, &_prefixHeaderPos));
  Stream = stream;
  return S_OK;
}

void COutArchive::Close()
{
  Stream.Release();
}

HRESULT COutArchive::SkeepPrefixArchiveHeader()
{
  return Stream->Seek(sizeof(CStartHeader) + sizeof(UINT32), STREAM_SEEK_CUR, NULL);
}

HRESULT COutArchive::WriteBytes(const void *data, UINT32 size)
{
  return ::WriteBytes(Stream, data, size);
}


HRESULT COutArchive::WriteBytes2(const void *data, UINT32 size)
{
  if (_mainMode)
  {
    if (_dynamicMode)
      _dynamicBuffer.Write(data, size);
    else
      _outByte.WriteBytes(data, size);
    _crc.Update(data, size);
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

HRESULT COutArchive::WriteBytes2(const CByteBuffer &data)
{
  return  WriteBytes2(data, data.GetCapacity());
}

HRESULT COutArchive::WriteByte2(BYTE b)
{
  return WriteBytes2(&b, 1);
}

HRESULT COutArchive::WriteNumber(UINT64 value)
{
  BYTE firstByte = 0;
  BYTE mask = 0x80;
  int i;
  for (i = 0; i < 8; i++)
  {
    if (value < ((UINT64(1) << ( 7  * (i + 1)))))
    {
      firstByte |= BYTE(value >> (8 * i));
      break;
    }
    firstByte |= mask;
    mask >>= 1;
  }
  RINOK(WriteByte2(firstByte));
  return WriteBytes2(&value, i);
}

static UINT32 GetBigNumberSize(UINT64 value)
{
  int i;
  for (i = 0; i < 8; i++)
    if (value < ((UINT64(1) << ( 7  * (i + 1)))))
      break;
  return 1 + i;
}

HRESULT COutArchive::WriteFolderHeader(const CFolder &itemInfo)
{
  RINOK(WriteNumber(itemInfo.Coders.Size()));
  int i;
  for (i = 0; i < itemInfo.Coders.Size(); i++)
  {
    const CCoderInfo &coderInfo = itemInfo.Coders[i];
    for (int j = 0; j < coderInfo.AltCoders.Size(); j++)
    {
      const CAltCoderInfo &altCoderInfo = coderInfo.AltCoders[j];
      UINT64 propertiesSize = altCoderInfo.Properties.GetCapacity();
      
      BYTE b;
      b = altCoderInfo.MethodID.IDSize & 0xF;
      bool isComplex = (coderInfo.NumInStreams != 1) ||  
        (coderInfo.NumOutStreams != 1);
      b |= (isComplex ? 0x10 : 0);
      b |= ((propertiesSize != 0) ? 0x20 : 0 );
      b |= ((j == coderInfo.AltCoders.Size() - 1) ? 0 : 0x80 );
      RINOK(WriteByte2(b));
      RINOK(WriteBytes2(&altCoderInfo.MethodID.ID[0], 
        altCoderInfo.MethodID.IDSize));
      if (isComplex)
      {
        RINOK(WriteNumber(coderInfo.NumInStreams));
        RINOK(WriteNumber(coderInfo.NumOutStreams));
      }
      if (propertiesSize == 0)
        continue;
      RINOK(WriteNumber(propertiesSize));
      RINOK(WriteBytes2(altCoderInfo.Properties, (UINT32)propertiesSize));
    }
  }
  // RINOK(WriteNumber(itemInfo.BindPairs.Size()));
  for (i = 0; i < itemInfo.BindPairs.Size(); i++)
  {
    const CBindPair &bindPair = itemInfo.BindPairs[i];
    RINOK(WriteNumber(bindPair.InIndex));
    RINOK(WriteNumber(bindPair.OutIndex));
  }
  if (itemInfo.PackStreams.Size() > 1)
    for (i = 0; i < itemInfo.PackStreams.Size(); i++)
    {
      const CPackStreamInfo &packStreamInfo = itemInfo.PackStreams[i];
      RINOK(WriteNumber(packStreamInfo.Index));
    }
  return S_OK;
}

HRESULT COutArchive::WriteBoolVector(const CBoolVector &boolVector)
{
  BYTE b = 0;
  BYTE mask = 0x80;
  for(int i = 0; i < boolVector.Size(); i++)
  {
    if (boolVector[i])
      b |= mask;
    mask >>= 1;
    if (mask == 0)
    {
      RINOK(WriteBytes2(&b, 1));
      mask = 0x80;
      b = 0;
    }
  }
  if (mask != 0x80)
  {
    RINOK(WriteBytes2(&b, 1));
  }
  return S_OK;
}


HRESULT COutArchive::WriteHashDigests(
    const CRecordVector<bool> &digestsDefined,
    const CRecordVector<UINT32> &digests)
{
  int numDefined = 0;
  for(int i = 0; i < digestsDefined.Size(); i++)
    if (digestsDefined[i])
      numDefined++;
  if (numDefined == 0)
    return S_OK;

  RINOK(WriteByte2(NID::kCRC));
  if (numDefined == digestsDefined.Size())
  {
    RINOK(WriteByte2(1));
  }
  else
  {
    RINOK(WriteByte2(0));
    RINOK(WriteBoolVector(digestsDefined));
  }
  for(i = 0; i < digests.Size(); i++)
  {
    if(digestsDefined[i])
      RINOK(WriteBytes2(&digests[i], sizeof(digests[i])));
  }
  return S_OK;
}

HRESULT COutArchive::WritePackInfo(
    UINT64 dataOffset,
    const CRecordVector<UINT64> &packSizes,
    const CRecordVector<bool> &packCRCsDefined,
    const CRecordVector<UINT32> &packCRCs)
{
  if (packSizes.IsEmpty())
    return S_OK;
  RINOK(WriteByte2(NID::kPackInfo));
  RINOK(WriteNumber(dataOffset));
  RINOK(WriteNumber(packSizes.Size()));
  RINOK(WriteByte2(NID::kSize));
  for(int i = 0; i < packSizes.Size(); i++)
    RINOK(WriteNumber(packSizes[i]));

  RINOK(WriteHashDigests(packCRCsDefined, packCRCs));
  
  return WriteByte2(NID::kEnd);
}

HRESULT COutArchive::WriteUnPackInfo(
    bool externalFolders,
    UINT64 externalFoldersStreamIndex,
    const CObjectVector<CFolder> &folders)
{
  if (folders.IsEmpty())
    return S_OK;

  RINOK(WriteByte2(NID::kUnPackInfo));

  RINOK(WriteByte2(NID::kFolder));
  RINOK(WriteNumber(folders.Size()));
  if (externalFolders)
  {
    RINOK(WriteByte2(1));
    RINOK(WriteNumber(externalFoldersStreamIndex));
  }
  else
  {
    RINOK(WriteByte2(0));
    for(int i = 0; i < folders.Size(); i++)
      RINOK(WriteFolderHeader(folders[i]));
  }
  
  RINOK(WriteByte2(NID::kCodersUnPackSize));
  for(int i = 0; i < folders.Size(); i++)
  {
    const CFolder &folder = folders[i];
    for (int j = 0; j < folder.UnPackSizes.Size(); j++)
      RINOK(WriteNumber(folder.UnPackSizes[j]));
  }

  CRecordVector<bool> unPackCRCsDefined;
  CRecordVector<UINT32> unPackCRCs;
  for(i = 0; i < folders.Size(); i++)
  {
    const CFolder &folder = folders[i];
    unPackCRCsDefined.Add(folder.UnPackCRCDefined);
    unPackCRCs.Add(folder.UnPackCRC);
  }
  RINOK(WriteHashDigests(unPackCRCsDefined, unPackCRCs));

  return WriteByte2(NID::kEnd);
}

HRESULT COutArchive::WriteSubStreamsInfo(
    const CObjectVector<CFolder> &folders,
    const CRecordVector<UINT64> &numUnPackStreamsInFolders,
    const CRecordVector<UINT64> &unPackSizes,
    const CRecordVector<bool> &digestsDefined,
    const CRecordVector<UINT32> &digests)
{
  RINOK(WriteByte2(NID::kSubStreamsInfo));

  for(int i = 0; i < numUnPackStreamsInFolders.Size(); i++)
  {
    if (numUnPackStreamsInFolders[i] != 1)
    {
      RINOK(WriteByte2(NID::kNumUnPackStream));
      for(i = 0; i < numUnPackStreamsInFolders.Size(); i++)
        RINOK(WriteNumber(numUnPackStreamsInFolders[i]));
      break;
    }
  }
 

  UINT32 needFlag = true;
  UINT32 index = 0;
  for(i = 0; i < numUnPackStreamsInFolders.Size(); i++)
    for (UINT32 j = 0; j < numUnPackStreamsInFolders[i]; j++)
    {
      if (j + 1 != numUnPackStreamsInFolders[i])
      {
        if (needFlag)
          RINOK(WriteByte2(NID::kSize));
        needFlag = false;
        RINOK(WriteNumber(unPackSizes[index]));
      }
      index++;
    }

  CRecordVector<bool> digestsDefined2;
  CRecordVector<UINT32> digests2;

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
  return WriteByte2(NID::kEnd);
}

HRESULT COutArchive::WriteTime(
    const CObjectVector<CFileItem> &files, BYTE type,
    bool isExternal, int externalDataIndex)
{
  /////////////////////////////////////////////////
  // CreationTime
  CBoolVector boolVector;
  boolVector.Reserve(files.Size());
  bool thereAreDefined = false;
  bool allDefined = true;
  for(int i = 0; i < files.Size(); i++)
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
  RINOK(WriteByte2(type));
  UINT32 dataSize = 1 + 1;
  if (isExternal)
    dataSize += GetBigNumberSize(externalDataIndex);
  else
    dataSize += files.Size() * sizeof(CArchiveFileTime);
  if (allDefined)
  {
    RINOK(WriteNumber(dataSize));
    WriteByte2(1);
  }
  else
  {
    RINOK(WriteNumber(1 + (boolVector.Size() + 7) / 8 + dataSize));
    WriteByte2(0);
    RINOK(WriteBoolVector(boolVector));
  }
  if (isExternal)
  {
    RINOK(WriteByte2(1));
    RINOK(WriteNumber(externalDataIndex));
    return S_OK;
  }
  RINOK(WriteByte2(0));
  for(i = 0; i < files.Size(); i++)
  {
    if (boolVector[i])
    {
      const CFileItem &item = files[i];
      CArchiveFileTime timeValue;
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
      RINOK(WriteBytes2(&timeValue, sizeof(timeValue)));
    }
  }
  return S_OK;
}

HRESULT COutArchive::EncodeStream(CEncoder &encoder, const BYTE *data, UINT32 dataSize,
    CRecordVector<UINT64> &packSizes, CObjectVector<CFolder> &folders)
{
  CSequentialInStreamImp *streamSpec = new CSequentialInStreamImp;
  CMyComPtr<ISequentialInStream> stream = streamSpec;
  streamSpec->Init(data, dataSize);
  CFolder folderItem;
  folderItem.UnPackCRCDefined = true;
  folderItem.UnPackCRC = CCRC::CalculateDigest(data, dataSize);
  RINOK(encoder.Encode(stream, NULL,
      folderItem, Stream,
      packSizes, NULL));
  folders.Add(folderItem);
  return S_OK;
}

HRESULT COutArchive::EncodeStream(CEncoder &encoder, const CByteBuffer &data, 
    CRecordVector<UINT64> &packSizes, CObjectVector<CFolder> &folders)
{
  return EncodeStream(encoder, data, data.GetCapacity(), packSizes, folders);
}



HRESULT COutArchive::WriteHeader(const CArchiveDatabase &database,
    const CCompressionMethodMode *options, UINT64 &headerOffset)
{
  CObjectVector<CFolder> folders;

  bool compressHeaders = (options != NULL);
  std::auto_ptr<CEncoder> encoder;
  if (compressHeaders)
    encoder = std::auto_ptr<CEncoder>(new CEncoder(*options));

  CRecordVector<UINT64> packSizes;

  UINT64 dataIndex = 0;

  //////////////////////////
  // Folders

  UINT64 externalFoldersStreamIndex;
  bool externalFolders = (compressHeaders && database.Folders.Size() > 8);
  if (externalFolders)
  {
    _mainMode = false;
    _countMode = true;
    _countSize = 0;
    for(int i = 0; i < database.Folders.Size(); i++)
    {
      RINOK(WriteFolderHeader(database.Folders[i]));
    }
    
    _countMode = false;
    
    CByteBuffer foldersData;
    foldersData.SetCapacity(_countSize);
    _outByte2.Init(foldersData, foldersData.GetCapacity());
    
    for(i = 0; i < database.Folders.Size(); i++)
    {
      RINOK(WriteFolderHeader(database.Folders[i]));
    }
    
    {
      externalFoldersStreamIndex = dataIndex++;
      RINOK(EncodeStream(*encoder, foldersData, packSizes, folders));
    }
  }


  /////////////////////////////////
  // Names

  CByteBuffer namesData;
  UINT64 externalNamesStreamIndex;
  bool externalNames = (compressHeaders && database.Files.Size() > 8);
  {
    UINT64 namesDataSize = 0;
    for(int i = 0; i < database.Files.Size(); i++)
      namesDataSize += (database.Files[i].Name.Length() + 1) * sizeof(wchar_t);
    namesData.SetCapacity(namesDataSize);
    UINT32 pos = 0;
    for(i = 0; i < database.Files.Size(); i++)
    {
      const UString &name = database.Files[i].Name;
      int length = name.Length() * sizeof(wchar_t);
      memmove(namesData + pos, name, length);
      pos += length;
      namesData[pos++] = 0;
      namesData[pos++] = 0;
    }

    if (externalNames)
    {
      externalNamesStreamIndex = dataIndex++;
      RINOK(EncodeStream(*encoder, namesData, packSizes, folders));
    }
  }

  /////////////////////////////////
  // Write Attributes
  CBoolVector attributesBoolVector;
  attributesBoolVector.Reserve(database.Files.Size());
  UINT32 numDefinedAttributes = 0;
  for(int i = 0; i < database.Files.Size(); i++)
  {
    bool defined = database.Files[i].AreAttributesDefined;
    attributesBoolVector.Add(defined);
    if (defined)
      numDefinedAttributes++;
  }

  CByteBuffer attributesData;
  UINT64 externalAttributesStreamIndex;
  bool externalAttributes = (compressHeaders && numDefinedAttributes > 8);
  if (numDefinedAttributes > 0)
  {
    attributesData.SetCapacity(numDefinedAttributes * sizeof(UINT32));
    UINT32 pos = 0;
    for(i = 0; i < database.Files.Size(); i++)
    {
      const CFileItem &file = database.Files[i];
      if (file.AreAttributesDefined)
      {
        memmove(attributesData + pos, &database.Files[i].Attributes, sizeof(UINT32));
        pos += sizeof(UINT32);
      }
    }
    if (externalAttributes)
    {
      externalAttributesStreamIndex = dataIndex++;
      RINOK(EncodeStream(*encoder, attributesData, packSizes, folders));
    }
  }
  
  /////////////////////////////////
  // Write Last Write Time
  UINT64 externalLastWriteTimeStreamIndex;
  bool externalLastWriteTime = false;
  // /*
  UINT32 numDefinedLastWriteTimes = 0;
  for(i = 0; i < database.Files.Size(); i++)
    if (database.Files[i].IsLastWriteTimeDefined)
      numDefinedLastWriteTimes++;

  externalLastWriteTime = (compressHeaders && numDefinedLastWriteTimes > 64);
  if (numDefinedLastWriteTimes > 0)
  {
    CByteBuffer lastWriteTimeData;
    lastWriteTimeData.SetCapacity(numDefinedLastWriteTimes * sizeof(CArchiveFileTime));
    UINT32 pos = 0;
    for(i = 0; i < database.Files.Size(); i++)
    {
      const CFileItem &file = database.Files[i];
      if (file.IsLastWriteTimeDefined)
      {
        memmove(lastWriteTimeData + pos, &database.Files[i].LastWriteTime, sizeof(CArchiveFileTime));
        pos += sizeof(CArchiveFileTime);
      }
    }
    if (externalLastWriteTime)
    {
      externalLastWriteTimeStreamIndex = dataIndex++;
      RINOK(EncodeStream(*encoder, lastWriteTimeData, packSizes, folders));
    }
  }
  // */
  

  UINT64 packedSize = 0;
  for(i = 0; i < database.PackSizes.Size(); i++)
    packedSize += database.PackSizes[i];
  UINT64 headerPackSize = 0;
  for (i = 0; i < packSizes.Size(); i++)
    headerPackSize += packSizes[i];

  headerOffset = packedSize + headerPackSize;

  _mainMode = true;

  _outByte.Init(Stream);
  _crc.Init();


  RINOK(WriteByte2(NID::kHeader));

  // Archive Properties

  if (folders.Size() > 0)
  {
    RINOK(WriteByte2(NID::kAdditionalStreamsInfo));
    RINOK(WritePackInfo(packedSize, packSizes, 
        CRecordVector<bool>(), CRecordVector<UINT32>()));
    RINOK(WriteUnPackInfo(false, 0, folders));
    RINOK(WriteByte2(NID::kEnd));
  }

  ////////////////////////////////////////////////////
 
  if (database.Folders.Size() > 0)
  {
    RINOK(WriteByte2(NID::kMainStreamsInfo));
    RINOK(WritePackInfo(0, database.PackSizes, 
        database.PackCRCsDefined,
        database.PackCRCs));

    RINOK(WriteUnPackInfo(
        externalFolders, externalFoldersStreamIndex, database.Folders));

    CRecordVector<UINT64> unPackSizes;
    CRecordVector<bool> digestsDefined;
    CRecordVector<UINT32> digests;
    for (i = 0; i < database.Files.Size(); i++)
    {
      const CFileItem &file = database.Files[i];
      if (!file.HasStream)
        continue;
      unPackSizes.Add(file.UnPackSize);
      digestsDefined.Add(file.FileCRCIsDefined);
      digests.Add(file.FileCRC);
    }

    RINOK(WriteSubStreamsInfo(
        database.Folders,
        database.NumUnPackStreamsVector,
        unPackSizes,
        digestsDefined,
        digests));
    RINOK(WriteByte2(NID::kEnd));
  }

  if (database.Files.IsEmpty())
  {
    RINOK(WriteByte2(NID::kEnd));
    return _outByte.Flush();
  }

  RINOK(WriteByte2(NID::kFilesInfo));
  RINOK(WriteNumber(database.Files.Size()));

  CBoolVector emptyStreamVector;
  emptyStreamVector.Reserve(database.Files.Size());
  UINT64 numEmptyStreams = 0;
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
    RINOK(WriteByte2(NID::kEmptyStream));
    RINOK(WriteNumber((emptyStreamVector.Size() + 7) / 8));
    RINOK(WriteBoolVector(emptyStreamVector));

    CBoolVector emptyFileVector, antiVector;
    emptyFileVector.Reserve(numEmptyStreams);
    antiVector.Reserve(numEmptyStreams);
    UINT64 numEmptyFiles = 0, numAntiItems = 0;
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
      RINOK(WriteByte2(NID::kEmptyFile));
      RINOK(WriteNumber((emptyFileVector.Size() + 7) / 8));
      RINOK(WriteBoolVector(emptyFileVector));
    }

    if (numAntiItems > 0)
    {
      RINOK(WriteByte2(NID::kAnti));
      RINOK(WriteNumber((antiVector.Size() + 7) / 8));
      RINOK(WriteBoolVector(antiVector));
    }
  }

  {
    /////////////////////////////////////////////////
    RINOK(WriteByte2(NID::kName));
    if (externalNames)
    {
      RINOK(WriteNumber(1 + 
          GetBigNumberSize(externalNamesStreamIndex)));
      RINOK(WriteByte2(1));
      RINOK(WriteNumber(externalNamesStreamIndex));
    }
    else
    {
      RINOK(WriteNumber(1 + namesData.GetCapacity()));
      RINOK(WriteByte2(0));
      RINOK(WriteBytes2(namesData));
    }

  }

  RINOK(WriteTime(database.Files, NID::kCreationTime, false, 0));
  RINOK(WriteTime(database.Files, NID::kLastAccessTime, false, 0));
  RINOK(WriteTime(database.Files, NID::kLastWriteTime, 
      // false, 0));
      externalLastWriteTime, externalLastWriteTimeStreamIndex));

  if (numDefinedAttributes > 0)
  {
    /////////////////////////////////////////////////
    RINOK(WriteByte2(NID::kWinAttributes));
    UINT32 size = 2;
    if (numDefinedAttributes != database.Files.Size())
      size += (attributesBoolVector.Size() + 7) / 8 + 1;
    if (externalAttributes)
      size += GetBigNumberSize(externalAttributesStreamIndex);
    else
      size += attributesData.GetCapacity();

    RINOK(WriteNumber(size));
    if (numDefinedAttributes == database.Files.Size())
    {
      RINOK(WriteByte2(1));
    }
    else
    {
      RINOK(WriteByte2(0));
      RINOK(WriteBoolVector(attributesBoolVector));
    }

    if (externalAttributes)
    {
      RINOK(WriteByte2(1));
      RINOK(WriteNumber(externalAttributesStreamIndex));
    }
    else
    {
      RINOK(WriteByte2(0));
      RINOK(WriteBytes2(attributesData));
    }
  }

  RINOK(WriteByte2(NID::kEnd)); // for files
  RINOK(WriteByte2(NID::kEnd)); // for headers

  return _outByte.Flush();
}

HRESULT COutArchive::WriteDatabase(const CArchiveDatabase &database,
    const CCompressionMethodMode *options, 
    bool useAdditionalStreams, bool compressMainHeader)
{
  UINT64 headerOffset;
  UINT32 headerCRC;
  UINT64 headerSize;
  if (database.IsEmpty())
  {
    headerSize = 0;
    headerOffset = 0;
    headerCRC = CCRC::CalculateDigest(0, 0);
  }
  else
  {
    _dynamicBuffer.Init();
    _dynamicMode = false;

    if (options != 0)
      if (options->IsEmpty())
        options = 0;
    const CCompressionMethodMode *additionalStreamsOptions = options;
    if (!useAdditionalStreams)
      additionalStreamsOptions = 0;
    /*
    if (database.Files.Size() < 2)
      compressMainHeader = false;
    */
    if (options != 0)
      if (options->PasswordIsDefined || compressMainHeader)
        _dynamicMode = true;
    RINOK(WriteHeader(database, additionalStreamsOptions, headerOffset));

    if (_dynamicMode)
    {
      CCompressionMethodMode encryptOptions;
      encryptOptions.PasswordIsDefined = options->PasswordIsDefined;
      encryptOptions.Password = options->Password;
      CEncoder encoder(compressMainHeader ? 
          *options : encryptOptions);
      CRecordVector<UINT64> packSizes;
      CObjectVector<CFolder> folders;
      RINOK(EncodeStream(encoder, _dynamicBuffer, 
          _dynamicBuffer.GetSize(), packSizes, folders));
      _dynamicMode = false;
      _mainMode = true;
      
      _outByte.Init(Stream);
      _crc.Init();
      
      if (folders.Size() == 0)
        throw 1;

      RINOK(WriteID(NID::kEncodedHeader));
      RINOK(WritePackInfo(headerOffset, packSizes, 
        CRecordVector<bool>(), CRecordVector<UINT32>()));
      RINOK(WriteUnPackInfo(false, 0, folders));
      RINOK(WriteByte2(NID::kEnd));
      for (int i = 0; i < packSizes.Size(); i++)
        headerOffset += packSizes[i];
      RINOK(_outByte.Flush());
    }
    headerCRC = _crc.GetDigest();
    headerSize = _outByte.GetProcessedSize();
  }
  RINOK(Stream->Seek(_prefixHeaderPos, STREAM_SEEK_SET, NULL));
  
  CStartHeader startHeader;
  startHeader.NextHeaderOffset = headerOffset;
  startHeader.NextHeaderSize = headerSize;
  startHeader.NextHeaderCRC = headerCRC;

  UINT32 crc = CCRC::CalculateDigest(&startHeader, sizeof(startHeader));
  RINOK(WriteBytes(&crc, sizeof(crc)));
  return WriteBytes(&startHeader, sizeof(startHeader));
}

}}
