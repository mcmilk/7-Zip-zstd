// 7z/OutEngine.cpp

#include "StdAfx.h"

#include "OutEngine.h"

#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "RegistryInfo.h"

#include "../../../Compress/Interface/CompressInterface.h"
#include "Interface/StreamObjects.h"

static HRESULT WriteBytes(IOutStream *stream, const void *data, UINT32 size)
{
  UINT32 processedSize;
  RETURN_IF_NOT_S_OK(stream->Write(data, size, &processedSize));
  if(processedSize != size)
    return E_FAIL;
  return S_OK;
}

extern const CLSID *g_MatchFinders[];

namespace NArchive {
namespace N7z {

HRESULT COutArchive::Create(IOutStream *stream)
{
  Close();
  RETURN_IF_NOT_S_OK(::WriteBytes(stream, kSignature, kSignatureSize));
  CArchiveVersion archiveVersion;
  archiveVersion.Major = kMajorVersion;
  archiveVersion.Minor = 2;
  RETURN_IF_NOT_S_OK(::WriteBytes(stream, &archiveVersion, sizeof(archiveVersion)));
  RETURN_IF_NOT_S_OK(stream->Seek(0, STREAM_SEEK_CUR, &_prefixHeaderPos));
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
      RETURN_IF_NOT_S_OK(_outByte2.Write(data, size));
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
  RETURN_IF_NOT_S_OK(WriteByte2(firstByte));
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

HRESULT COutArchive::WriteFolderHeader(const CFolderItemInfo &itemInfo)
{
  RETURN_IF_NOT_S_OK(WriteNumber(itemInfo.CodersInfo.Size()));
  int i;
  for (i = 0; i < itemInfo.CodersInfo.Size(); i++)
  {
    const CCoderInfo &coderInfo = itemInfo.CodersInfo[i];
    for (int j = 0; j < coderInfo.AltCoders.Size(); j++)
    {
      const CAltCoderInfo &altCoderInfo = coderInfo.AltCoders[j];
      UINT64 propertiesSize = altCoderInfo.Properties.GetCapacity();
      
      BYTE b;
      b = altCoderInfo.DecompressionMethod.IDSize & 0xF;
      bool isComplex = (coderInfo.NumInStreams != 1) ||  
        (coderInfo.NumOutStreams != 1);
      b |= (isComplex ? 0x10 : 0);
      b |= ((propertiesSize != 0) ? 0x20 : 0 );
      b |= ((j == coderInfo.AltCoders.Size() - 1) ? 0 : 0x80 );
      RETURN_IF_NOT_S_OK(WriteByte2(b));
      RETURN_IF_NOT_S_OK(WriteBytes2(&altCoderInfo.DecompressionMethod.ID[0], 
        altCoderInfo.DecompressionMethod.IDSize));
      if (isComplex)
      {
        RETURN_IF_NOT_S_OK(WriteNumber(coderInfo.NumInStreams));
        RETURN_IF_NOT_S_OK(WriteNumber(coderInfo.NumOutStreams));
      }
      if (propertiesSize == 0)
        continue;
      RETURN_IF_NOT_S_OK(WriteNumber(propertiesSize));
      RETURN_IF_NOT_S_OK(WriteBytes2(altCoderInfo.Properties, propertiesSize));
    }
  }
  // RETURN_IF_NOT_S_OK(WriteNumber(itemInfo.BindPairs.Size()));
  for (i = 0; i < itemInfo.BindPairs.Size(); i++)
  {
    const CBindPair &bindPair = itemInfo.BindPairs[i];
    RETURN_IF_NOT_S_OK(WriteNumber(bindPair.InIndex));
    RETURN_IF_NOT_S_OK(WriteNumber(bindPair.OutIndex));
  }
  if (itemInfo.PackStreams.Size() > 1)
    for (i = 0; i < itemInfo.PackStreams.Size(); i++)
    {
      const CPackStreamInfo &packStreamInfo = itemInfo.PackStreams[i];
      RETURN_IF_NOT_S_OK(WriteNumber(packStreamInfo.Index));
    }
  return S_OK;
}

HRESULT COutArchive::WriteBoolVector(const CBoolVector &boolVector)
{
  BYTE b = 0;
  BYTE mask = 0x80;
  for(UINT32 i = 0; i < boolVector.Size(); i++)
  {
    if (boolVector[i])
      b |= mask;
    mask >>= 1;
    if (mask == 0)
    {
      RETURN_IF_NOT_S_OK(WriteBytes2(&b, 1));
      mask = 0x80;
      b = 0;
    }
  }
  if (mask != 0x80)
  {
    RETURN_IF_NOT_S_OK(WriteBytes2(&b, 1));
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

  RETURN_IF_NOT_S_OK(WriteByte2(NID::kCRC));
  if (numDefined == digestsDefined.Size())
  {
    RETURN_IF_NOT_S_OK(WriteByte2(1));
  }
  else
  {
    RETURN_IF_NOT_S_OK(WriteByte2(0));
    RETURN_IF_NOT_S_OK(WriteBoolVector(digestsDefined));
  }
  for(i = 0; i < digests.Size(); i++)
  {
    if(digestsDefined[i])
      RETURN_IF_NOT_S_OK(WriteBytes2(&digests[i], sizeof(digests[i])));
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
  RETURN_IF_NOT_S_OK(WriteByte2(NID::kPackInfo));
  RETURN_IF_NOT_S_OK(WriteNumber(dataOffset));
  RETURN_IF_NOT_S_OK(WriteNumber(packSizes.Size()));
  RETURN_IF_NOT_S_OK(WriteByte2(NID::kSize));
  for(UINT64 i = 0; i < packSizes.Size(); i++)
    RETURN_IF_NOT_S_OK(WriteNumber(packSizes[i]));

  RETURN_IF_NOT_S_OK(WriteHashDigests(packCRCsDefined, packCRCs));
  
  return WriteByte2(NID::kEnd);
}

HRESULT COutArchive::WriteUnPackInfo(
    bool externalFolders,
    UINT64 externalFoldersStreamIndex,
    const CObjectVector<CFolderItemInfo> &folders)
{
  if (folders.IsEmpty())
    return S_OK;

  RETURN_IF_NOT_S_OK(WriteByte2(NID::kUnPackInfo));

  RETURN_IF_NOT_S_OK(WriteByte2(NID::kFolder));
  RETURN_IF_NOT_S_OK(WriteNumber(folders.Size()));
  if (externalFolders)
  {
    RETURN_IF_NOT_S_OK(WriteByte2(1));
    RETURN_IF_NOT_S_OK(WriteNumber(externalFoldersStreamIndex));
  }
  else
  {
    RETURN_IF_NOT_S_OK(WriteByte2(0));
    for(int i = 0; i < folders.Size(); i++)
      RETURN_IF_NOT_S_OK(WriteFolderHeader(folders[i]));
  }
  
  RETURN_IF_NOT_S_OK(WriteByte2(NID::kCodersUnPackSize));
  for(int i = 0; i < folders.Size(); i++)
  {
    const CFolderItemInfo &folder = folders[i];
    for (int j = 0; j < folder.UnPackSizes.Size(); j++)
      RETURN_IF_NOT_S_OK(WriteNumber(folder.UnPackSizes[j]));
  }

  CRecordVector<bool> unPackCRCsDefined;
  CRecordVector<UINT32> unPackCRCs;
  for(i = 0; i < folders.Size(); i++)
  {
    const CFolderItemInfo &folder = folders[i];
    unPackCRCsDefined.Add(folder.UnPackCRCDefined);
    unPackCRCs.Add(folder.UnPackCRC);
  }
  RETURN_IF_NOT_S_OK(WriteHashDigests(unPackCRCsDefined, unPackCRCs));

  return WriteByte2(NID::kEnd);
}

HRESULT COutArchive::WriteSubStreamsInfo(
    const CObjectVector<CFolderItemInfo> &folders,
    const CRecordVector<UINT64> &numUnPackStreamsInFolders,
    const CRecordVector<UINT64> &unPackSizes,
    const CRecordVector<bool> &digestsDefined,
    const CRecordVector<UINT32> &digests)
{
  RETURN_IF_NOT_S_OK(WriteByte2(NID::kSubStreamsInfo));

  for(int i = 0; i < numUnPackStreamsInFolders.Size(); i++)
  {
    if (numUnPackStreamsInFolders[i] != 1)
    {
      RETURN_IF_NOT_S_OK(WriteByte2(NID::kNumUnPackStream));
      for(i = 0; i < numUnPackStreamsInFolders.Size(); i++)
        RETURN_IF_NOT_S_OK(WriteNumber(numUnPackStreamsInFolders[i]));
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
          RETURN_IF_NOT_S_OK(WriteByte2(NID::kSize));
        needFlag = false;
        RETURN_IF_NOT_S_OK(WriteNumber(unPackSizes[index]));
      }
      index++;
    }

  CRecordVector<bool> digestsDefined2;
  CRecordVector<UINT32> digests2;

  int digestIndex = 0;
  for (i = 0; i < folders.Size(); i++)
  {
    int numSubStreams = numUnPackStreamsInFolders[i];
    if (numSubStreams == 1 && folders[i].UnPackCRCDefined)
      digestIndex++;
    else
      for (int j = 0; j < numSubStreams; j++, digestIndex++)
      {
        digestsDefined2.Add(digestsDefined[digestIndex]);
        digests2.Add(digests[digestIndex]);
      }
  }
  RETURN_IF_NOT_S_OK(WriteHashDigests(digestsDefined2, digests2));
  return WriteByte2(NID::kEnd);
}

HRESULT COutArchive::WriteTime(
    const CObjectVector<CFileItemInfo> &files, BYTE type,
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
    const CFileItemInfo &item = files[i];
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
  RETURN_IF_NOT_S_OK(WriteByte2(type));
  UINT32 dataSize = 1 + 1;
  if (isExternal)
    dataSize += GetBigNumberSize(externalDataIndex);
  else
    dataSize += files.Size() * sizeof(CArchiveFileTime);
  if (allDefined)
  {
    RETURN_IF_NOT_S_OK(WriteNumber(dataSize));
    WriteByte2(1);
  }
  else
  {
    RETURN_IF_NOT_S_OK(WriteNumber(1 + (boolVector.Size() + 7) / 8 + dataSize));
    WriteByte2(0);
    RETURN_IF_NOT_S_OK(WriteBoolVector(boolVector));
  }
  if (isExternal)
  {
    RETURN_IF_NOT_S_OK(WriteByte2(1));
    RETURN_IF_NOT_S_OK(WriteNumber(externalDataIndex));
    return S_OK;
  }
  RETURN_IF_NOT_S_OK(WriteByte2(0));
  for(i = 0; i < files.Size(); i++)
  {
    if (boolVector[i])
    {
      const CFileItemInfo &item = files[i];
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
      RETURN_IF_NOT_S_OK(WriteBytes2(&timeValue, sizeof(timeValue)));
    }
  }
  return S_OK;
}

HRESULT COutArchive::EncodeStream(CEncoder &encoder, const BYTE *data, UINT32 dataSize,
    CRecordVector<UINT64> &packSizes, CObjectVector<CFolderItemInfo> &folders)
{
  CComObjectNoLock<CSequentialInStreamImp> *streamSpec = 
    new CComObjectNoLock<CSequentialInStreamImp>;
  CComPtr<ISequentialInStream> stream = streamSpec;
  streamSpec->Init(data, dataSize);
  CFolderItemInfo folderItem;
  folderItem.UnPackCRCDefined = true;
  folderItem.UnPackCRC = CCRC::CalculateDigest(data, dataSize);
  RETURN_IF_NOT_S_OK(encoder.Encode(stream, NULL,
      folderItem, Stream,
      packSizes, NULL));
  folders.Add(folderItem);
  return S_OK;
}

HRESULT COutArchive::EncodeStream(CEncoder &encoder, const CByteBuffer &data, 
    CRecordVector<UINT64> &packSizes, CObjectVector<CFolderItemInfo> &folders)
{
  return EncodeStream(encoder, data, data.GetCapacity(), packSizes, folders);
}



HRESULT COutArchive::WriteHeader(const CArchiveDatabase &database,
    const CCompressionMethodMode *options, UINT64 &headerOffset)
{
  CObjectVector<CFolderItemInfo> folders;

  bool compressHeaders = (options != NULL);
  std::auto_ptr<CEncoder> encoder;
  if (compressHeaders)
    encoder = std::auto_ptr<CEncoder>(new CEncoder(options));

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
      RETURN_IF_NOT_S_OK(WriteFolderHeader(database.Folders[i]));
    }
    
    _countMode = false;
    
    CByteBuffer foldersData;
    foldersData.SetCapacity(_countSize);
    _outByte2.Init(foldersData, foldersData.GetCapacity());
    
    for(i = 0; i < database.Folders.Size(); i++)
    {
      RETURN_IF_NOT_S_OK(WriteFolderHeader(database.Folders[i]));
    }
    
    {
      externalFoldersStreamIndex = dataIndex++;
      RETURN_IF_NOT_S_OK(EncodeStream(*encoder, foldersData, packSizes, folders));
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
      RETURN_IF_NOT_S_OK(EncodeStream(*encoder, namesData, packSizes, folders));
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
      const CFileItemInfo &fileInfo = database.Files[i];
      if (fileInfo.AreAttributesDefined)
      {
        memmove(attributesData + pos, &database.Files[i].Attributes, sizeof(UINT32));
        pos += sizeof(UINT32);
      }
    }
    if (externalAttributes)
    {
      externalAttributesStreamIndex = dataIndex++;
      RETURN_IF_NOT_S_OK(EncodeStream(*encoder, attributesData, packSizes, folders));
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
      const CFileItemInfo &fileInfo = database.Files[i];
      if (fileInfo.IsLastWriteTimeDefined)
      {
        memmove(lastWriteTimeData + pos, &database.Files[i].LastWriteTime, sizeof(CArchiveFileTime));
        pos += sizeof(CArchiveFileTime);
      }
    }
    if (externalLastWriteTime)
    {
      externalLastWriteTimeStreamIndex = dataIndex++;
      RETURN_IF_NOT_S_OK(EncodeStream(*encoder, lastWriteTimeData, packSizes, folders));
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


  RETURN_IF_NOT_S_OK(WriteByte2(NID::kHeader));

  // Archive Properties

  if (folders.Size() > 0)
  {
    RETURN_IF_NOT_S_OK(WriteByte2(NID::kAdditionalStreamsInfo));
    RETURN_IF_NOT_S_OK(WritePackInfo(packedSize, packSizes, 
        CRecordVector<bool>(), CRecordVector<UINT32>()));
    RETURN_IF_NOT_S_OK(WriteUnPackInfo(false, 0, folders));
    RETURN_IF_NOT_S_OK(WriteByte2(NID::kEnd));
  }

  ////////////////////////////////////////////////////
 
  if (database.Folders.Size() > 0)
  {
    RETURN_IF_NOT_S_OK(WriteByte2(NID::kMainStreamsInfo));
    RETURN_IF_NOT_S_OK(WritePackInfo(0, database.PackSizes, 
        database.PackCRCsDefined,
        database.PackCRCs));

    RETURN_IF_NOT_S_OK(WriteUnPackInfo(
        externalFolders, externalFoldersStreamIndex, database.Folders));

    CRecordVector<UINT64> unPackSizes;
    CRecordVector<bool> digestsDefined;
    CRecordVector<UINT32> digests;
    for (i = 0; i < database.Files.Size(); i++)
    {
      const CFileItemInfo &fileInfo = database.Files[i];
      if (fileInfo.IsDirectory || fileInfo.UnPackSize == 0)
        continue;
      unPackSizes.Add(fileInfo.UnPackSize);
      digestsDefined.Add(fileInfo.FileCRCIsDefined);
      digests.Add(fileInfo.FileCRC);
    }

    RETURN_IF_NOT_S_OK(WriteSubStreamsInfo(
        database.Folders,
        database.NumUnPackStreamsVector,
        unPackSizes,
        digestsDefined,
        digests));
    RETURN_IF_NOT_S_OK(WriteByte2(NID::kEnd));
  }

  if (database.Files.IsEmpty())
  {
    RETURN_IF_NOT_S_OK(WriteByte2(NID::kEnd));
    return _outByte.Flush();
  }

  RETURN_IF_NOT_S_OK(WriteByte2(NID::kFilesInfo));
  RETURN_IF_NOT_S_OK(WriteNumber(database.Files.Size()));

  CBoolVector emptyStreamVector;
  emptyStreamVector.Reserve(database.Files.Size());
  UINT64 numEmptyStreams = 0;
  for(i = 0; i < database.Files.Size(); i++)
  {
    const CFileItemInfo &fileInfo = database.Files[i];
    if (fileInfo.IsAnti || fileInfo.IsDirectory || fileInfo.UnPackSize == 0)
    {
      emptyStreamVector.Add(true);
      numEmptyStreams++;
    }
    else
      emptyStreamVector.Add(false);
  }
  if (numEmptyStreams > 0)
  {
    RETURN_IF_NOT_S_OK(WriteByte2(NID::kEmptyStream));
    RETURN_IF_NOT_S_OK(WriteNumber((emptyStreamVector.Size() + 7) / 8));
    RETURN_IF_NOT_S_OK(WriteBoolVector(emptyStreamVector));

    CBoolVector emptyFileVector, antiVector;
    emptyFileVector.Reserve(numEmptyStreams);
    antiVector.Reserve(numEmptyStreams);
    UINT64 numEmptyFiles = 0, numAntiItems = 0;
    for(i = 0; i < database.Files.Size(); i++)
    {
      const CFileItemInfo &fileInfo = database.Files[i];
      if (fileInfo.IsAnti || fileInfo.IsDirectory || fileInfo.UnPackSize == 0)
      {
        if (fileInfo.IsDirectory)
          emptyFileVector.Add(false);
        else
        {
          emptyFileVector.Add(true);
          numEmptyFiles++;
        }
        if (fileInfo.IsAnti)
        {
          antiVector.Add(true);
          numAntiItems++;
        }
        else
          antiVector.Add(false);
      }
    }

    if (numEmptyFiles > 0)
    {
      RETURN_IF_NOT_S_OK(WriteByte2(NID::kEmptyFile));
      RETURN_IF_NOT_S_OK(WriteNumber((emptyFileVector.Size() + 7) / 8));
      RETURN_IF_NOT_S_OK(WriteBoolVector(emptyFileVector));
    }

    if (numAntiItems > 0)
    {
      RETURN_IF_NOT_S_OK(WriteByte2(NID::kAnti));
      RETURN_IF_NOT_S_OK(WriteNumber((antiVector.Size() + 7) / 8));
      RETURN_IF_NOT_S_OK(WriteBoolVector(antiVector));
    }
  }

  {
    /////////////////////////////////////////////////
    RETURN_IF_NOT_S_OK(WriteByte2(NID::kName));
    if (externalNames)
    {
      RETURN_IF_NOT_S_OK(WriteNumber(1 + 
          GetBigNumberSize(externalNamesStreamIndex)));
      RETURN_IF_NOT_S_OK(WriteByte2(1));
      RETURN_IF_NOT_S_OK(WriteNumber(externalNamesStreamIndex));
    }
    else
    {
      RETURN_IF_NOT_S_OK(WriteNumber(1 + namesData.GetCapacity()));
      RETURN_IF_NOT_S_OK(WriteByte2(0));
      RETURN_IF_NOT_S_OK(WriteBytes2(namesData));
    }

  }

  RETURN_IF_NOT_S_OK(WriteTime(database.Files, NID::kCreationTime, false, 0));
  RETURN_IF_NOT_S_OK(WriteTime(database.Files, NID::kLastAccessTime, false, 0));
  RETURN_IF_NOT_S_OK(WriteTime(database.Files, NID::kLastWriteTime, 
      // false, 0));
      externalLastWriteTime, externalLastWriteTimeStreamIndex));

  if (numDefinedAttributes > 0)
  {
    /////////////////////////////////////////////////
    RETURN_IF_NOT_S_OK(WriteByte2(NID::kWinAttributes));
    UINT32 size = 2;
    if (numDefinedAttributes != database.Files.Size())
      size += (attributesBoolVector.Size() + 7) / 8 + 1;
    if (externalAttributes)
      size += GetBigNumberSize(externalAttributesStreamIndex);
    else
      size += attributesData.GetCapacity();

    RETURN_IF_NOT_S_OK(WriteNumber(size));
    if (numDefinedAttributes == database.Files.Size())
    {
      RETURN_IF_NOT_S_OK(WriteByte2(1));
    }
    else
    {
      RETURN_IF_NOT_S_OK(WriteByte2(0));
      RETURN_IF_NOT_S_OK(WriteBoolVector(attributesBoolVector));
    }

    if (externalAttributes)
    {
      RETURN_IF_NOT_S_OK(WriteByte2(1));
      RETURN_IF_NOT_S_OK(WriteNumber(externalAttributesStreamIndex));
    }
    else
    {
      RETURN_IF_NOT_S_OK(WriteByte2(0));
      RETURN_IF_NOT_S_OK(WriteBytes2(attributesData));
    }
  }

  RETURN_IF_NOT_S_OK(WriteByte2(NID::kEnd)); // for files
  RETURN_IF_NOT_S_OK(WriteByte2(NID::kEnd)); // for headers

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
    RETURN_IF_NOT_S_OK(WriteHeader(database, additionalStreamsOptions, headerOffset));

    if (_dynamicMode)
    {
      CCompressionMethodMode encryptOptions;
      encryptOptions.PasswordIsDefined = options->PasswordIsDefined;
      encryptOptions.Password = options->Password;
      CEncoder encoder(compressMainHeader ? 
          options : &encryptOptions);
      CRecordVector<UINT64> packSizes;
      CObjectVector<CFolderItemInfo> folders;
      RETURN_IF_NOT_S_OK(EncodeStream(encoder, _dynamicBuffer, 
          _dynamicBuffer.GetSize(), packSizes, folders));
      _dynamicMode = false;
      _mainMode = true;
      
      _outByte.Init(Stream);
      _crc.Init();
      
      if (folders.Size() == 0)
        throw 1;

      RETURN_IF_NOT_S_OK(WriteID(NID::kEncodedHeader));
      RETURN_IF_NOT_S_OK(WritePackInfo(headerOffset, packSizes, 
        CRecordVector<bool>(), CRecordVector<UINT32>()));
      RETURN_IF_NOT_S_OK(WriteUnPackInfo(false, 0, folders));
      RETURN_IF_NOT_S_OK(WriteByte2(NID::kEnd));
      for (int i = 0; i < packSizes.Size(); i++)
        headerOffset += packSizes[i];
      RETURN_IF_NOT_S_OK(_outByte.Flush());
    }
    headerCRC = _crc.GetDigest();
    headerSize = _outByte.GetProcessedSize();
  }
  RETURN_IF_NOT_S_OK(Stream->Seek(_prefixHeaderPos, STREAM_SEEK_SET, NULL));
  
  CStartHeader startHeader;
  startHeader.NextHeaderOffset = headerOffset;
  startHeader.NextHeaderSize = headerSize;
  startHeader.NextHeaderCRC = headerCRC;

  UINT32 crc = CCRC::CalculateDigest(&startHeader, sizeof(startHeader));
  RETURN_IF_NOT_S_OK(WriteBytes(&crc, sizeof(crc)));
  return WriteBytes(&startHeader, sizeof(startHeader));
}

}}
