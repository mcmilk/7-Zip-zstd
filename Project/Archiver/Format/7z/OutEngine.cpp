// 7z/OutEngine.cpp

#include "StdAfx.h"

#include "OutEngine.h"

#include "Windows/Defs.h"
#include "Windows/PropVariant.h"

#include "RegistryInfo.h"

#include "../../../Compress/Interface/CompressInterface.h"
#include "Interface/StreamObjects.h"

static HRESULT WriteBytes(IOutStream *aStream, const void *aData, UINT32 aSize)
{
  UINT32 aProcessedSize;
  RETURN_IF_NOT_S_OK(aStream->Write(aData, aSize, &aProcessedSize));
  if(aProcessedSize != aSize)
    return E_FAIL;
  return S_OK;
}

extern const CLSID *g_MatchFinders[];

namespace NArchive {
namespace N7z {

HRESULT COutArchive::Create(IOutStream *aStream)
{
  Close();
  RETURN_IF_NOT_S_OK(::WriteBytes(aStream, kSignature, kSignatureSize));
  CArchiveVersion anArchiveVersion;
  anArchiveVersion.Major = kMajorVersion;
  anArchiveVersion.Minor = 1;
  RETURN_IF_NOT_S_OK(::WriteBytes(aStream, &anArchiveVersion, sizeof(anArchiveVersion)));
  RETURN_IF_NOT_S_OK(aStream->Seek(0, STREAM_SEEK_CUR, &m_PrefixHeaderPos));
  m_Stream = aStream;
  return S_OK;
}

void COutArchive::Close()
{
  m_Stream.Release();
}

HRESULT COutArchive::SkeepPrefixArchiveHeader()
{
  return m_Stream->Seek(sizeof(CStartHeader) + sizeof(UINT32), STREAM_SEEK_CUR, NULL);
}

HRESULT COutArchive::WriteBytes(const void *aData, UINT32 aSize)
{
  return ::WriteBytes(m_Stream, aData, aSize);
}


HRESULT COutArchive::WriteBytes2(const void *aData, UINT32 aSize)
{
  if (m_MainMode)
  {
    m_OutByte.WriteBytes(aData, aSize);
    m_CRC.Update(aData, aSize);
  }
  else
  {
    if (m_CountMode)
      m_CountSize += aSize;
    else
      RETURN_IF_NOT_S_OK(m_OutByte2.Write(aData, aSize));
  }
  return S_OK;
}

HRESULT COutArchive::WriteBytes2(const CByteBuffer &aData)
{
  return  WriteBytes2(aData, aData.GetCapacity());
}

HRESULT COutArchive::WriteByte2(BYTE aByte)
{
  return WriteBytes2(&aByte, 1);
}

HRESULT COutArchive::WriteNumber(UINT64 aValue)
{
  BYTE aFirstByte = 0;
  BYTE aMask = 0x80;
  int i;
  for (i = 0; i < 8; i++)
  {
    if (aValue < ((UINT64(1) << ( 7  * (i + 1)))))
    {
      aFirstByte |= BYTE(aValue >> (8 * i));
      break;
    }
    aFirstByte |= aMask;
    aMask >>= 1;
  }
  RETURN_IF_NOT_S_OK(WriteByte2(aFirstByte));
  return WriteBytes2(&aValue, i);
}

static UINT32 GetBigNumberSize(UINT64 aValue)
{
  int i;
  for (i = 0; i < 8; i++)
    if (aValue < ((UINT64(1) << ( 7  * (i + 1)))))
      break;
  return 1 + i;
}

HRESULT COutArchive::WriteFolderHeader(const CFolderItemInfo &anItemInfo)
{
  RETURN_IF_NOT_S_OK(WriteNumber(anItemInfo.CodersInfo.Size()));
  int i;
  for (i = 0; i < anItemInfo.CodersInfo.Size(); i++)
  {
    const CCoderInfo &aCoderInfo = anItemInfo.CodersInfo[i];
    UINT64 aPropertiesSize = aCoderInfo.Properties.GetCapacity();

    BYTE aByte;
    aByte = aCoderInfo.DecompressionMethod.IDSize & 0xF;
    bool anIsComplex = (aCoderInfo.NumInStreams != 1) ||  
        (aCoderInfo.NumOutStreams != 1);
    aByte |= (anIsComplex ? 0x10 : 0);
    aByte |= ((aPropertiesSize != 0) ? 0x20 : 0 );
    RETURN_IF_NOT_S_OK(WriteByte2(aByte));
    RETURN_IF_NOT_S_OK(WriteBytes2(&aCoderInfo.DecompressionMethod.ID[0], 
        aCoderInfo.DecompressionMethod.IDSize));
    if (anIsComplex)
    {
      RETURN_IF_NOT_S_OK(WriteNumber(aCoderInfo.NumInStreams));
      RETURN_IF_NOT_S_OK(WriteNumber(aCoderInfo.NumOutStreams));
    }
    if (aPropertiesSize == 0)
      continue;
    RETURN_IF_NOT_S_OK(WriteNumber(aPropertiesSize));
    RETURN_IF_NOT_S_OK(WriteBytes2(aCoderInfo.Properties, aPropertiesSize));
  }
  // RETURN_IF_NOT_S_OK(WriteNumber(anItemInfo.BindPairs.Size()));
  for (i = 0; i < anItemInfo.BindPairs.Size(); i++)
  {
    const CBindPair &aBindPair = anItemInfo.BindPairs[i];
    RETURN_IF_NOT_S_OK(WriteNumber(aBindPair.InIndex));
    RETURN_IF_NOT_S_OK(WriteNumber(aBindPair.OutIndex));
  }
  if (anItemInfo.PackStreams.Size() > 1)
    for (i = 0; i < anItemInfo.PackStreams.Size(); i++)
    {
      const CPackStreamInfo &aPackStreamInfo = anItemInfo.PackStreams[i];
      RETURN_IF_NOT_S_OK(WriteNumber(aPackStreamInfo.Index));
    }
  return S_OK;
}

HRESULT COutArchive::WriteBoolVector(const CBoolVector &aVector)
{
  BYTE aByte = 0;
  BYTE aMask = 0x80;
  for(UINT32 i = 0; i < aVector.Size(); i++)
  {
    if (aVector[i])
      aByte |= aMask;
    aMask >>= 1;
    if (aMask == 0)
    {
      RETURN_IF_NOT_S_OK(WriteBytes2(&aByte, 1));
      aMask = 0x80;
      aByte = 0;
    }
  }
  if (aMask != 0x80)
  {
    RETURN_IF_NOT_S_OK(WriteBytes2(&aByte, 1));
  }
  return S_OK;
}


HRESULT COutArchive::WriteHashDigests(
    const CRecordVector<bool> &digestsDefined,
    const CRecordVector<UINT32> &digests)
{
  int aNumDefined = 0;
  for(int i = 0; i < digestsDefined.Size(); i++)
    if (digestsDefined[i])
      aNumDefined++;
  if (aNumDefined == 0)
    return S_OK;

  RETURN_IF_NOT_S_OK(WriteByte2(NID::kCRC));
  if (aNumDefined == digestsDefined.Size())
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
    UINT64 aDataOffset,
    const CRecordVector<UINT64> &aPackSizes,
    const CRecordVector<bool> &aPackCRCsDefined,
    const CRecordVector<UINT32> &aPackCRCs)
{
  if (aPackSizes.IsEmpty())
    return S_OK;
  RETURN_IF_NOT_S_OK(WriteByte2(NID::kPackInfo));
  RETURN_IF_NOT_S_OK(WriteNumber(aDataOffset));
  RETURN_IF_NOT_S_OK(WriteNumber(aPackSizes.Size()));
  RETURN_IF_NOT_S_OK(WriteByte2(NID::kSize));
  for(UINT64 i = 0; i < aPackSizes.Size(); i++)
    RETURN_IF_NOT_S_OK(WriteNumber(aPackSizes[i]));

  RETURN_IF_NOT_S_OK(WriteHashDigests(aPackCRCsDefined, aPackCRCs));
  
  return WriteByte2(NID::kEnd);
}

HRESULT COutArchive::WriteUnPackInfo(
    bool externalFolders,
    UINT64 anExternalFoldersStreamIndex,
    const CObjectVector<CFolderItemInfo> &aFolders)
{
  if (aFolders.IsEmpty())
    return S_OK;

  RETURN_IF_NOT_S_OK(WriteByte2(NID::kUnPackInfo));

  RETURN_IF_NOT_S_OK(WriteByte2(NID::kFolder));
  RETURN_IF_NOT_S_OK(WriteNumber(aFolders.Size()));
  if (externalFolders)
  {
    RETURN_IF_NOT_S_OK(WriteByte2(1));
    RETURN_IF_NOT_S_OK(WriteNumber(anExternalFoldersStreamIndex));
  }
  else
  {
    RETURN_IF_NOT_S_OK(WriteByte2(0));
    for(int i = 0; i < aFolders.Size(); i++)
      RETURN_IF_NOT_S_OK(WriteFolderHeader(aFolders[i]));
  }
  
  RETURN_IF_NOT_S_OK(WriteByte2(NID::kCodersUnPackSize));
  for(int i = 0; i < aFolders.Size(); i++)
  {
    const CFolderItemInfo &aFolder = aFolders[i];
    for (int j = 0; j < aFolder.UnPackSizes.Size(); j++)
      RETURN_IF_NOT_S_OK(WriteNumber(aFolder.UnPackSizes[j]));
  }

  CRecordVector<bool> anUnPackCRCsDefined;
  CRecordVector<UINT32> anUnPackCRCs;
  for(i = 0; i < aFolders.Size(); i++)
  {
    const CFolderItemInfo &aFolder = aFolders[i];
    anUnPackCRCsDefined.Add(aFolder.UnPackCRCDefined);
    anUnPackCRCs.Add(aFolder.UnPackCRC);
  }
  RETURN_IF_NOT_S_OK(WriteHashDigests(anUnPackCRCsDefined, anUnPackCRCs));

  return WriteByte2(NID::kEnd);
}

HRESULT COutArchive::WriteSubStreamsInfo(
    const CObjectVector<CFolderItemInfo> &aFolders,
    const CRecordVector<UINT64> &aNumUnPackStreamsInFolders,
    const CRecordVector<UINT64> &unPackSizes,
    const CRecordVector<bool> &digestsDefined,
    const CRecordVector<UINT32> &digests)
{
  RETURN_IF_NOT_S_OK(WriteByte2(NID::kSubStreamsInfo));

  for(int i = 0; i < aNumUnPackStreamsInFolders.Size(); i++)
  {
    if (aNumUnPackStreamsInFolders[i] != 1)
    {
      RETURN_IF_NOT_S_OK(WriteByte2(NID::kNumUnPackStream));
      for(i = 0; i < aNumUnPackStreamsInFolders.Size(); i++)
        RETURN_IF_NOT_S_OK(WriteNumber(aNumUnPackStreamsInFolders[i]));
      break;
    }
  }
 

  UINT32 aNeedFlag = true;
  UINT32 anIndex = 0;
  for(i = 0; i < aNumUnPackStreamsInFolders.Size(); i++)
    for (UINT32 j = 0; j < aNumUnPackStreamsInFolders[i]; j++)
    {
      if (j + 1 != aNumUnPackStreamsInFolders[i])
      {
        if (aNeedFlag)
          RETURN_IF_NOT_S_OK(WriteByte2(NID::kSize));
        aNeedFlag = false;
        RETURN_IF_NOT_S_OK(WriteNumber(unPackSizes[anIndex]));
      }
      anIndex++;
    }

  CRecordVector<bool> aDigestsDefined2;
  CRecordVector<UINT32> aDigests2;

  int anDigestIndex = 0;
  for (i = 0; i < aFolders.Size(); i++)
  {
    int aNumSubstreams = aNumUnPackStreamsInFolders[i];
    if (aNumSubstreams == 1 && aFolders[i].UnPackCRCDefined)
      anDigestIndex++;
    else
      for (int j = 0; j < aNumSubstreams; j++, anDigestIndex++)
      {
        aDigestsDefined2.Add(digestsDefined[anDigestIndex]);
        aDigests2.Add(digests[anDigestIndex]);
      }
  }
  RETURN_IF_NOT_S_OK(WriteHashDigests(aDigestsDefined2, aDigests2));
  return WriteByte2(NID::kEnd);
}

HRESULT COutArchive::WriteTime(
    const CObjectVector<CFileItemInfo> &aFiles, BYTE aType,
    bool anIsExternal, int anExternalDataIndex)
{
  /////////////////////////////////////////////////
  // CreationTime
  CBoolVector aBoolVector;
  aBoolVector.Reserve(aFiles.Size());
  bool aThereAreDefined = false;
  bool anAllDefined = true;
  for(int i = 0; i < aFiles.Size(); i++)
  {
    const CFileItemInfo &anItem = aFiles[i];
    bool aDefined;
    switch(aType)
    {
      case NID::kCreationTime:
        aDefined = anItem.IsCreationTimeDefined;
        break;
      case NID::kLastWriteTime:
        aDefined = anItem.IsLastWriteTimeDefined;
        break;
      case NID::kLastAccessTime:
        aDefined = anItem.IsLastAccessTimeDefined;
        break;
      default:
        throw 1;
    }
    aBoolVector.Add(aDefined);
    aThereAreDefined = (aThereAreDefined || aDefined);
    anAllDefined = (anAllDefined && aDefined);
  }
  if (!aThereAreDefined)
    return S_OK;
  RETURN_IF_NOT_S_OK(WriteByte2(aType));
  UINT32 aDataSize = 1 + 1;
  if (anIsExternal)
    aDataSize += GetBigNumberSize(anExternalDataIndex);
  else
    aDataSize += aFiles.Size() * sizeof(CArchiveFileTime);
  if (anAllDefined)
  {
    RETURN_IF_NOT_S_OK(WriteNumber(aDataSize));
    WriteByte2(1);
  }
  else
  {
    RETURN_IF_NOT_S_OK(WriteNumber(1 + (aBoolVector.Size() + 7) / 8 + aDataSize));
    WriteByte2(0);
    RETURN_IF_NOT_S_OK(WriteBoolVector(aBoolVector));
  }
  if (anIsExternal)
  {
    RETURN_IF_NOT_S_OK(WriteByte2(1));
    RETURN_IF_NOT_S_OK(WriteNumber(anExternalDataIndex));
    return S_OK;
  }
  RETURN_IF_NOT_S_OK(WriteByte2(0));
  for(i = 0; i < aFiles.Size(); i++)
  {
    if (aBoolVector[i])
    {
      const CFileItemInfo &anItem = aFiles[i];
      CArchiveFileTime aTime;
      switch(aType)
      {
        case NID::kCreationTime:
          aTime = anItem.CreationTime;
          break;
        case NID::kLastWriteTime:
          aTime = anItem.LastWriteTime;
          break;
        case NID::kLastAccessTime:
          aTime = anItem.LastAccessTime;
          break;
      }
      RETURN_IF_NOT_S_OK(WriteBytes2(&aTime, sizeof(aTime)));
    }
  }
  return S_OK;
}

HRESULT COutArchive::EncodeStream(CEncoder &anEncoder, const CByteBuffer &aData, 
    CRecordVector<UINT64> &aPackSizes, CObjectVector<CFolderItemInfo> &aFolders)
{
  CComObjectNoLock<CSequentialInStreamImp> *aStreamSpec = 
    new CComObjectNoLock<CSequentialInStreamImp>;
  CComPtr<ISequentialInStream> anStream = aStreamSpec;
  aStreamSpec->Init(aData, aData.GetCapacity());
  CFolderItemInfo aFolderItem;
  aFolderItem.UnPackCRCDefined = true;
  aFolderItem.UnPackCRC = CCRC::CalculateDigest(aData, aData.GetCapacity());
  RETURN_IF_NOT_S_OK(anEncoder.Encode(anStream, NULL,
      aFolderItem, m_Stream,
      aPackSizes, NULL));
  aFolders.Add(aFolderItem);
  return S_OK;
}



HRESULT COutArchive::WriteHeader(const CArchiveDatabase &database,
    const CCompressionMethodMode *anOptions, UINT64 &aHeaderOffset)
{
  CObjectVector<CFolderItemInfo> aFolders;

  bool compressHeaders = (anOptions != NULL);
  std::auto_ptr<CEncoder> anEncoder;
  if (compressHeaders)
    anEncoder = std::auto_ptr<CEncoder>(new CEncoder(anOptions));

  CRecordVector<UINT64> aPackSizes;

  UINT64 aDataIndex = 0;

  //////////////////////////
  // Folders

  UINT64 anExternalFoldersStreamIndex;
  bool externalFolders = (compressHeaders && database.Folders.Size() > 8);
  if (externalFolders)
  {
    m_MainMode = false;
    m_CountMode = true;
    m_CountSize = 0;
    for(int i = 0; i < database.Folders.Size(); i++)
    {
      RETURN_IF_NOT_S_OK(WriteFolderHeader(database.Folders[i]));
    }
    
    m_CountMode = false;
    
    CByteBuffer aFoldersData;
    aFoldersData.SetCapacity(m_CountSize);
    m_OutByte2.Init(aFoldersData, aFoldersData.GetCapacity());
    
    for(i = 0; i < database.Folders.Size(); i++)
    {
      RETURN_IF_NOT_S_OK(WriteFolderHeader(database.Folders[i]));
    }
    
    {
      anExternalFoldersStreamIndex = aDataIndex++;
      RETURN_IF_NOT_S_OK(EncodeStream(*anEncoder, aFoldersData, aPackSizes, aFolders));
    }
  }


  /////////////////////////////////
  // Names

  CByteBuffer aNamesData;
  UINT64 anExternalNamesStreamIndex;
  bool anExternalNames = (compressHeaders && database.Files.Size() > 8);
  {
    UINT64 aNamesDataSize = 0;
    for(int i = 0; i < database.Files.Size(); i++)
      aNamesDataSize += (database.Files[i].Name.Length() + 1) * sizeof(wchar_t);
    aNamesData.SetCapacity(aNamesDataSize);
    UINT32 aPos = 0;
    for(i = 0; i < database.Files.Size(); i++)
    {
      const UString &aName = database.Files[i].Name;
      int aLength = aName.Length() * sizeof(wchar_t);
      memmove(aNamesData + aPos, aName, aLength);
      aPos += aLength;
      aNamesData[aPos++] = 0;
      aNamesData[aPos++] = 0;
    }

    if (anExternalNames)
    {
      anExternalNamesStreamIndex = aDataIndex++;
      RETURN_IF_NOT_S_OK(EncodeStream(*anEncoder, aNamesData, aPackSizes, aFolders));
    }
  }

  /////////////////////////////////
  // Write Attributes
  CBoolVector anAttributesBoolVector;
  anAttributesBoolVector.Reserve(database.Files.Size());
  UINT32 aNumDefinedAttributes = 0;
  for(int i = 0; i < database.Files.Size(); i++)
  {
    bool aDefined = database.Files[i].AreAttributesDefined;
    anAttributesBoolVector.Add(aDefined);
    if (aDefined)
      aNumDefinedAttributes++;
  }

  CByteBuffer anAttributesData;
  UINT64 anExternalAttributesStreamIndex;
  bool anExternalAttributes = (compressHeaders && aNumDefinedAttributes > 8);
  if (aNumDefinedAttributes > 0)
  {
    anAttributesData.SetCapacity(aNumDefinedAttributes * sizeof(UINT32));
    UINT32 aPos = 0;
    for(i = 0; i < database.Files.Size(); i++)
    {
      const CFileItemInfo &aFile = database.Files[i];
      if (aFile.AreAttributesDefined)
      {
        memmove(anAttributesData + aPos, &database.Files[i].Attributes, sizeof(UINT32));
        aPos += sizeof(UINT32);
      }
    }
    if (anExternalAttributes)
    {
      anExternalAttributesStreamIndex = aDataIndex++;
      RETURN_IF_NOT_S_OK(EncodeStream(*anEncoder, anAttributesData, aPackSizes, aFolders));
    }
  }
  
  /////////////////////////////////
  // Write Last Write Time
  UINT64 anExternalLastWriteTimeStreamIndex;
  bool anExternalLastWriteTime = false;
  // /*
  UINT32 aNumDefinedLastWriteTimes = 0;
  for(i = 0; i < database.Files.Size(); i++)
    if (database.Files[i].IsLastWriteTimeDefined)
      aNumDefinedLastWriteTimes++;

  anExternalLastWriteTime = (compressHeaders && aNumDefinedLastWriteTimes > 64);
  if (aNumDefinedLastWriteTimes > 0)
  {
    CByteBuffer aLastWriteTimeData;
    aLastWriteTimeData.SetCapacity(aNumDefinedLastWriteTimes * sizeof(CArchiveFileTime));
    UINT32 aPos = 0;
    for(i = 0; i < database.Files.Size(); i++)
    {
      const CFileItemInfo &aFile = database.Files[i];
      if (aFile.IsLastWriteTimeDefined)
      {
        memmove(aLastWriteTimeData + aPos, &database.Files[i].LastWriteTime, sizeof(CArchiveFileTime));
        aPos += sizeof(CArchiveFileTime);
      }
    }
    if (anExternalLastWriteTime)
    {
      anExternalLastWriteTimeStreamIndex = aDataIndex++;
      RETURN_IF_NOT_S_OK(EncodeStream(*anEncoder, aLastWriteTimeData, aPackSizes, aFolders));
    }
  }
  // */
  

  UINT64 aPackedSize = 0;
  for(i = 0; i < database.PackSizes.Size(); i++)
    aPackedSize += database.PackSizes[i];
  UINT64 aHeaderPackSize = 0;
  for (i = 0; i < aPackSizes.Size(); i++)
    aHeaderPackSize += aPackSizes[i];

  aHeaderOffset = aPackedSize + aHeaderPackSize;

  m_MainMode = true;

  m_OutByte.Init(m_Stream);
  m_CRC.Init();


  RETURN_IF_NOT_S_OK(WriteByte2(NID::kHeader));

  // Archive Properties

  if (aFolders.Size() > 0)
  {
    RETURN_IF_NOT_S_OK(WriteByte2(NID::kAdditionalStreamsInfo));
    RETURN_IF_NOT_S_OK(WritePackInfo(aPackedSize, aPackSizes, 
        CRecordVector<bool>(), CRecordVector<UINT32>()));
    RETURN_IF_NOT_S_OK(WriteUnPackInfo(false, 0, aFolders));
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
        externalFolders, anExternalFoldersStreamIndex, database.Folders));

    CRecordVector<UINT64> unPackSizes;
    CRecordVector<bool> digestsDefined;
    CRecordVector<UINT32> digests;
    for (i = 0; i < database.Files.Size(); i++)
    {
      const CFileItemInfo &aFile = database.Files[i];
      if (aFile.IsDirectory || aFile.UnPackSize == 0)
        continue;
      unPackSizes.Add(aFile.UnPackSize);
      digestsDefined.Add(aFile.FileCRCIsDefined);
      digests.Add(aFile.FileCRC);
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
    return m_OutByte.Flush();
  }

  RETURN_IF_NOT_S_OK(WriteByte2(NID::kFilesInfo));
  RETURN_IF_NOT_S_OK(WriteNumber(database.Files.Size()));

  CBoolVector anEmptyStreamVector;
  anEmptyStreamVector.Reserve(database.Files.Size());
  UINT64 aNumEmptyStreams = 0;
  for(i = 0; i < database.Files.Size(); i++)
  {
    const CFileItemInfo &aFile = database.Files[i];
    if (aFile.IsAnti || aFile.IsDirectory || aFile.UnPackSize == 0)
    {
      anEmptyStreamVector.Add(true);
      aNumEmptyStreams++;
    }
    else
      anEmptyStreamVector.Add(false);
  }
  if (aNumEmptyStreams > 0)
  {
    RETURN_IF_NOT_S_OK(WriteByte2(NID::kEmptyStream));
    RETURN_IF_NOT_S_OK(WriteNumber((anEmptyStreamVector.Size() + 7) / 8));
    RETURN_IF_NOT_S_OK(WriteBoolVector(anEmptyStreamVector));

    CBoolVector anEmptyFileVector, anAntiVector;
    anEmptyFileVector.Reserve(aNumEmptyStreams);
    anAntiVector.Reserve(aNumEmptyStreams);
    UINT64 aNumEmptyFiles = 0, aNumAntiItems = 0;
    for(i = 0; i < database.Files.Size(); i++)
    {
      const CFileItemInfo &aFile = database.Files[i];
      if (aFile.IsAnti || aFile.IsDirectory || aFile.UnPackSize == 0)
      {
        if (aFile.IsDirectory)
          anEmptyFileVector.Add(false);
        else
        {
          anEmptyFileVector.Add(true);
          aNumEmptyFiles++;
        }
        if (aFile.IsAnti)
        {
          anAntiVector.Add(true);
          aNumAntiItems++;
        }
        else
          anAntiVector.Add(false);
      }
    }

    if (aNumEmptyFiles > 0)
    {
      RETURN_IF_NOT_S_OK(WriteByte2(NID::kEmptyFile));
      RETURN_IF_NOT_S_OK(WriteNumber((anEmptyFileVector.Size() + 7) / 8));
      RETURN_IF_NOT_S_OK(WriteBoolVector(anEmptyFileVector));
    }

    if (aNumAntiItems > 0)
    {
      RETURN_IF_NOT_S_OK(WriteByte2(NID::kAnti));
      RETURN_IF_NOT_S_OK(WriteNumber((anAntiVector.Size() + 7) / 8));
      RETURN_IF_NOT_S_OK(WriteBoolVector(anAntiVector));
    }
  }

  {
    /////////////////////////////////////////////////
    RETURN_IF_NOT_S_OK(WriteByte2(NID::kName));
    if (anExternalNames)
    {
      RETURN_IF_NOT_S_OK(WriteNumber(1 + 
          GetBigNumberSize(anExternalNamesStreamIndex)));
      RETURN_IF_NOT_S_OK(WriteByte2(1));
      RETURN_IF_NOT_S_OK(WriteNumber(anExternalNamesStreamIndex));
    }
    else
    {
      RETURN_IF_NOT_S_OK(WriteNumber(1 + aNamesData.GetCapacity()));
      RETURN_IF_NOT_S_OK(WriteByte2(0));
      RETURN_IF_NOT_S_OK(WriteBytes2(aNamesData));
    }

  }

  RETURN_IF_NOT_S_OK(WriteTime(database.Files, NID::kCreationTime, false, 0));
  RETURN_IF_NOT_S_OK(WriteTime(database.Files, NID::kLastAccessTime, false, 0));
  RETURN_IF_NOT_S_OK(WriteTime(database.Files, NID::kLastWriteTime, 
      // false, 0));
      anExternalLastWriteTime, anExternalLastWriteTimeStreamIndex));

  if (aNumDefinedAttributes > 0)
  {
    /////////////////////////////////////////////////
    RETURN_IF_NOT_S_OK(WriteByte2(NID::kWinAttributes));
    UINT32 aSize = 2;
    if (aNumDefinedAttributes != database.Files.Size())
      aSize += (anAttributesBoolVector.Size() + 7) / 8 + 1;
    if (anExternalAttributes)
      aSize += GetBigNumberSize(anExternalAttributesStreamIndex);
    else
      aSize += anAttributesData.GetCapacity();

    RETURN_IF_NOT_S_OK(WriteNumber(aSize));
    if (aNumDefinedAttributes == database.Files.Size())
    {
      RETURN_IF_NOT_S_OK(WriteByte2(1));
    }
    else
    {
      RETURN_IF_NOT_S_OK(WriteByte2(0));
      RETURN_IF_NOT_S_OK(WriteBoolVector(anAttributesBoolVector));
    }

    if (anExternalAttributes)
    {
      RETURN_IF_NOT_S_OK(WriteByte2(1));
      RETURN_IF_NOT_S_OK(WriteNumber(anExternalAttributesStreamIndex));
    }
    else
    {
      RETURN_IF_NOT_S_OK(WriteByte2(0));
      RETURN_IF_NOT_S_OK(WriteBytes2(anAttributesData));
    }
  }

  RETURN_IF_NOT_S_OK(WriteByte2(NID::kEnd)); // for files
  RETURN_IF_NOT_S_OK(WriteByte2(NID::kEnd)); // for headers

  return m_OutByte.Flush();
}

HRESULT COutArchive::WriteDatabase(const CArchiveDatabase &database,
    const CCompressionMethodMode *anOptions)
{
  UINT64 aHeaderOffset;
  RETURN_IF_NOT_S_OK(WriteHeader(database, anOptions, aHeaderOffset));

  UINT32 aHeaderCRC = m_CRC.GetDigest();
  UINT64 aHeaderSize = m_OutByte.GetProcessedSize();

  RETURN_IF_NOT_S_OK(m_Stream->Seek(m_PrefixHeaderPos, STREAM_SEEK_SET, NULL));
  
  CStartHeader aStartHeader;
  aStartHeader.NextHeaderOffset = aHeaderOffset;
  aStartHeader.NextHeaderSize = aHeaderSize;
  aStartHeader.NextHeaderCRC = aHeaderCRC;

  UINT32 aCRC = CCRC::CalculateDigest(&aStartHeader, sizeof(aStartHeader));
  RETURN_IF_NOT_S_OK(WriteBytes(&aCRC, sizeof(aCRC)));
  return WriteBytes(&aStartHeader, sizeof(aStartHeader));
}

}}
