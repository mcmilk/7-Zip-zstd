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
  CInArchive *m_Archive;
  bool m_NeedRemove;
  void Remove();
public:
  CStreamSwitch(): m_NeedRemove(false) {}
  ~CStreamSwitch() { Remove(); }
  void Set(CInArchive *anArchive, const BYTE *aData, UINT32 aSize);
  void Set(CInArchive *anArchive, const CByteBuffer &aByteBuffer);
  HRESULT Set(CInArchive *anArchive, const CObjectVector<CByteBuffer> *aDataVector);
};

void CStreamSwitch::Remove()
{
  if (m_NeedRemove)
  {
    m_Archive->DeleteByteStream();
    m_NeedRemove = false;
  }
}

void CStreamSwitch::Set(CInArchive *anArchive, const BYTE *aData, UINT32 aSize)
{
  Remove();
  m_Archive = anArchive;
  m_Archive->AddByteStream(aData, aSize);
  m_NeedRemove = true;
}

void CStreamSwitch::Set(CInArchive *anArchive, const CByteBuffer &aByteBuffer)
{
  Set(anArchive, aByteBuffer, aByteBuffer.GetCapacity());
}

HRESULT CStreamSwitch::Set(CInArchive *anArchive, const CObjectVector<CByteBuffer> *aDataVector)
{
  Remove();
  BYTE anExternal;
  RETURN_IF_NOT_S_OK(anArchive->SafeReadByte2(anExternal));
  if (anExternal != 0)
  {
    UINT64 aDataIndex;
    RETURN_IF_NOT_S_OK(anArchive->ReadNumber(aDataIndex));
    Set(anArchive, (*aDataVector)[aDataIndex]);
  }
  return S_OK;
}

  
CInArchiveException::CInArchiveException(CCauseType aCause):
  Cause(aCause)
{}

HRESULT CInArchive::ReadBytes(IInStream *aStream, void *aData, UINT32 aSize, 
    UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal;
  HRESULT aResult = aStream->Read(aData, aSize, &aProcessedSizeReal);
  if(aProcessedSize != NULL)
    *aProcessedSize = aProcessedSizeReal;
  m_Position += aProcessedSizeReal;
  return aResult;
}

HRESULT CInArchive::ReadBytes(void *aData, UINT32 aSize, UINT32 *aProcessedSize)
{
  return ReadBytes(m_Stream, aData, aSize, aProcessedSize);
}

HRESULT CInArchive::SafeReadBytes(void *aData, UINT32 aSize)
{
  UINT32 aProcessedSizeReal;
  RETURN_IF_NOT_S_OK(ReadBytes(aData, aSize, &aProcessedSizeReal));
  if (aProcessedSizeReal != aSize)
    throw CInArchiveException(CInArchiveException::kUnexpectedEndOfArchive);
  return S_OK;
}

HRESULT CInArchive::ReadNumber(UINT64 &aValue)
{
  BYTE aFirstByte;
  RETURN_IF_NOT_S_OK(SafeReadByte2(aFirstByte));
  BYTE aMask = 0x80;
  for (int i = 0; i < 8; i++)
  {
    if ((aFirstByte & aMask) == 0)
    {
      aValue = 0;
      RETURN_IF_NOT_S_OK(SafeReadBytes2(&aValue, i));
      UINT64 aHighPart = aFirstByte & (aMask - 1);
      aValue += (aHighPart << (i * 8));
      return S_OK;
    }
    aMask >>= 1;
  }
  return SafeReadBytes2(&aValue, 8);
}

static inline bool TestSignatureCandidate(const void *aTestBytes)
{
  // return (memcmp(aTestBytes, kSignature, kSignatureSize) == 0);
  for (UINT32 i = 0; i < kSignatureSize; i++)
    if (((const BYTE *)aTestBytes)[i] != kSignature[i])
      return false;
  return true;
}

HRESULT CInArchive::FindAndReadSignature(IInStream *aStream, const UINT64 *aSearchHeaderSizeLimit)
{
  m_Position = m_ArhiveBeginStreamPosition;
  RETURN_IF_NOT_S_OK(aStream->Seek(m_ArhiveBeginStreamPosition, STREAM_SEEK_SET, NULL));

  BYTE aSignature[kSignatureSize];
  UINT32 aProcessedSize; 
  RETURN_IF_NOT_S_OK(ReadBytes(aStream, aSignature, kSignatureSize, &aProcessedSize));
  if(aProcessedSize != kSignatureSize)
    return S_FALSE;
  if (TestSignatureCandidate(aSignature))
    return S_OK;

  CByteBuffer aByteBuffer;
  static const UINT32 kSearchSignatureBufferSize = 0x10000;
  aByteBuffer.SetCapacity(kSearchSignatureBufferSize);
  BYTE *aBuffer = aByteBuffer;
  UINT32 aNumBytesPrev = kSignatureSize - 1;
  memmove(aBuffer, aSignature + 1, aNumBytesPrev);
  UINT64 aCurTestPos = m_ArhiveBeginStreamPosition + 1;
  while(true)
  {
    if (aSearchHeaderSizeLimit != NULL)
      if (aCurTestPos - m_ArhiveBeginStreamPosition > *aSearchHeaderSizeLimit)
        return false;
    UINT32 aNumReadBytes = kSearchSignatureBufferSize - aNumBytesPrev;
    RETURN_IF_NOT_S_OK(ReadBytes(aStream, aBuffer + aNumBytesPrev, aNumReadBytes, &aProcessedSize));
    UINT32 aNumBytesInBuffer = aNumBytesPrev + aProcessedSize;
    if (aNumBytesInBuffer < kSignatureSize)
      return S_FALSE;
    UINT32 aNumTests = aNumBytesInBuffer - kSignatureSize + 1;
    for(UINT32 aPos = 0; aPos < aNumTests; aPos++, aCurTestPos++)
    { 
      if (TestSignatureCandidate(aBuffer + aPos))
      {
        m_ArhiveBeginStreamPosition = aCurTestPos;
        m_Position = aCurTestPos + kSignatureSize;
        return aStream->Seek(m_Position, STREAM_SEEK_SET, NULL);
      }
    }
    aNumBytesPrev = aNumBytesInBuffer - aNumTests;
    memmove(aBuffer, aBuffer + aNumTests, aNumBytesPrev);
  }
}

// S_FALSE means that file is not archive
HRESULT CInArchive::Open(IInStream *aStream, const UINT64 *aSearchHeaderSizeLimit)
{
  Close();
  RETURN_IF_NOT_S_OK(aStream->Seek(0, STREAM_SEEK_CUR, &m_ArhiveBeginStreamPosition))
  m_Position = m_ArhiveBeginStreamPosition;
  RETURN_IF_NOT_S_OK(FindAndReadSignature(aStream, aSearchHeaderSizeLimit));
  m_Stream = aStream;
  return S_OK;
}
  
void CInArchive::Close()
{
  m_Stream.Release();
}

HRESULT CInArchive::SkeepData(UINT64 aSize)
{
  for (UINT64 i = 0; i < aSize; i++)
  {
    BYTE aTemp;
    RETURN_IF_NOT_S_OK(SafeReadByte2(aTemp));
  }
  return S_OK;
}

HRESULT CInArchive::SkeepData()
{
  UINT64 aSize;
  RETURN_IF_NOT_S_OK(ReadNumber(aSize));
  return SkeepData(aSize);
}

HRESULT CInArchive::ReadArhiveProperties(CInArchiveInfo &anArchiveInfo)
{
  while(true)
  {
    BYTE aType;
    RETURN_IF_NOT_S_OK(SafeReadByte2(aType));
    if (aType == NID::kEnd)
      break;
    SkeepData();
  }
  return S_OK;
}

HRESULT CInArchive::GetNextFolderItem(CFolderItemInfo &anItemInfo)
{
  UINT64 aNumCoders;
  RETURN_IF_NOT_S_OK(ReadNumber(aNumCoders));

  anItemInfo.CodersInfo.Clear();
  anItemInfo.CodersInfo.Reserve(aNumCoders);
  UINT32 aNumInStreams = 0;
  UINT32 aNumOutStreams = 0;
  for (int i = 0; i < aNumCoders; i++)
  {
    anItemInfo.CodersInfo.Add(CCoderInfo());
    CCoderInfo &aCoderInfo = anItemInfo.CodersInfo.Back();

    BYTE aByte;
    RETURN_IF_NOT_S_OK(SafeReadByte2(aByte));
    aCoderInfo.DecompressionMethod.IDSize = aByte & 0xF;
    bool anIsCompex = (aByte & 0x10) != 0;
    bool aThereAreProperties = (aByte & 0x20) != 0;
    RETURN_IF_NOT_S_OK(SafeReadBytes2(&aCoderInfo.DecompressionMethod.ID[0], 
        aCoderInfo.DecompressionMethod.IDSize));
    if (anIsCompex)
    {
      RETURN_IF_NOT_S_OK(ReadNumber(aCoderInfo.NumInStreams));
      RETURN_IF_NOT_S_OK(ReadNumber(aCoderInfo.NumOutStreams));
    }
    else
    {
      aCoderInfo.NumInStreams = 1;
      aCoderInfo.NumOutStreams = 1;
    }
    aNumInStreams += aCoderInfo.NumInStreams;
    aNumOutStreams += aCoderInfo.NumOutStreams;
    UINT64 aPropertiesSize = 0;
    if (aThereAreProperties)
    {
      RETURN_IF_NOT_S_OK(ReadNumber(aPropertiesSize));
    }
    aCoderInfo.Properties.SetCapacity(aPropertiesSize);
    RETURN_IF_NOT_S_OK(SafeReadBytes2((BYTE *)aCoderInfo.Properties, aPropertiesSize));
  }

  UINT64 aNumBindPairs;
  // RETURN_IF_NOT_S_OK(ReadNumber(aNumBindPairs));
  aNumBindPairs = aNumOutStreams - 1;
  anItemInfo.BindPairs.Clear();
  anItemInfo.BindPairs.Reserve(aNumBindPairs);
  for (i = 0; i < aNumBindPairs; i++)
  {
    CBindPair aBindPair;
    RETURN_IF_NOT_S_OK(ReadNumber(aBindPair.InIndex));
    RETURN_IF_NOT_S_OK(ReadNumber(aBindPair.OutIndex)); 
    anItemInfo.BindPairs.Add(aBindPair);
  }

  UINT32 aNumPackedStreams = aNumInStreams - aNumBindPairs;
  anItemInfo.PackStreams.Reserve(aNumPackedStreams);
  if (aNumPackedStreams == 1)
  {
    for (int j = 0; j < aNumInStreams; j++)
      if (anItemInfo.FindBindPairForInStream(j) < 0)
      {
        CPackStreamInfo aPackStreamInfo;
        aPackStreamInfo.Index = j;
        anItemInfo.PackStreams.Add(aPackStreamInfo);
        break;
      }
  }
  else
    for(i = 0; i < aNumPackedStreams; i++)
    {
      CPackStreamInfo aPackStreamInfo;
      RETURN_IF_NOT_S_OK(ReadNumber(aPackStreamInfo.Index));
      anItemInfo.PackStreams.Add(aPackStreamInfo);
    }

  return S_OK;
}

HRESULT CInArchive::WaitAttribute(BYTE anAttribute)
{
  while(true)
  {
    BYTE aType;
    RETURN_IF_NOT_S_OK(SafeReadByte2(aType));
    if (aType == anAttribute)
      return S_OK;
    if (aType == NID::kEnd)
      return S_FALSE;
    RETURN_IF_NOT_S_OK(SkeepData());
  }
}

HRESULT CInArchive::ReadHashDigests(int aNumItems,
    CRecordVector<bool> &aDigestsDefined, 
    CRecordVector<UINT32> &aDigests)
{
  RETURN_IF_NOT_S_OK(ReadBoolVector2(aNumItems, aDigestsDefined));
  aDigests.Clear();
  aDigests.Reserve(aNumItems);
  for(int i = 0; i < aNumItems; i++)
  {
    UINT32 aCRC;
    if (aDigestsDefined[i])
      RETURN_IF_NOT_S_OK(SafeReadBytes2(&aCRC, sizeof(aCRC)));
    aDigests.Add(aCRC);
  }
  return S_OK;
}

HRESULT CInArchive::ReadPackInfo(
    UINT64 &aDataOffset,
    CRecordVector<UINT64> &aPackSizes,
    CRecordVector<bool> &aPackCRCsDefined,
    CRecordVector<UINT32> &aPackCRCs)
{
  RETURN_IF_NOT_S_OK(ReadNumber(aDataOffset));
  UINT64 aNumPackStreams;
  RETURN_IF_NOT_S_OK(ReadNumber(aNumPackStreams));

  RETURN_IF_NOT_S_OK(WaitAttribute(NID::kSize));
  aPackSizes.Clear();
  aPackSizes.Reserve(aNumPackStreams);
  for(UINT64 i = 0; i < aNumPackStreams; i++)
  {
    UINT64 aSize;
    RETURN_IF_NOT_S_OK(ReadNumber(aSize));
    aPackSizes.Add(aSize);
  }

  BYTE aType;
  while(true)
  {
    RETURN_IF_NOT_S_OK(SafeReadByte2(aType));
    if (aType == NID::kEnd)
      break;
    if (aType == NID::kCRC)
    {
      RETURN_IF_NOT_S_OK(ReadHashDigests(
          aNumPackStreams, aPackCRCsDefined, aPackCRCs)); 
      continue;
    }
    RETURN_IF_NOT_S_OK(SkeepData());
  }
  if (aPackCRCsDefined.IsEmpty())
  {
    aPackCRCsDefined.Reserve(aNumPackStreams);
    aPackCRCsDefined.Clear();
    aPackCRCs.Reserve(aNumPackStreams);
    aPackCRCs.Clear();
    for(UINT64 i = 0; i < aNumPackStreams; i++)
    {
      aPackCRCsDefined.Add(false);
      aPackCRCs.Add(0);
    }
  }
  return S_OK;
}

HRESULT CInArchive::ReadUnPackInfo(
    const CObjectVector<CByteBuffer> *aDataVector,
    CObjectVector<CFolderItemInfo> &aFolders)
{
  RETURN_IF_NOT_S_OK(WaitAttribute(NID::kFolder));
  UINT64 aNumFolders;
  RETURN_IF_NOT_S_OK(ReadNumber(aNumFolders));

  {
    CStreamSwitch aStreamSwitch;
    RETURN_IF_NOT_S_OK(aStreamSwitch.Set(this, aDataVector));
    aFolders.Clear();
    aFolders.Reserve(aNumFolders);
    for(UINT64 i = 0; i < aNumFolders; i++)
    {
      aFolders.Add(CFolderItemInfo());
      RETURN_IF_NOT_S_OK(GetNextFolderItem(aFolders.Back()));
    }
  }

  RETURN_IF_NOT_S_OK(WaitAttribute(NID::kCodersUnPackSize));

  for(UINT64 i = 0; i < aNumFolders; i++)
  {
    CFolderItemInfo &aFolder = aFolders[i];
    int aNumOutStreams = aFolder.GetNumOutStreams();
    aFolder.UnPackSizes.Reserve(aNumOutStreams);
    for(int j = 0; j < aNumOutStreams; j++)
    {
      UINT64 anUnPackSize;
      RETURN_IF_NOT_S_OK(ReadNumber(anUnPackSize));
      aFolder.UnPackSizes.Add(anUnPackSize);
    }
  }

  while(true)
  {
    BYTE aType;
    RETURN_IF_NOT_S_OK(SafeReadByte2(aType));
    if (aType == NID::kEnd)
      return S_OK;
    if (aType == NID::kCRC)
    {
      CRecordVector<bool> aCRCsDefined;
      CRecordVector<UINT32> aCRCs;
      RETURN_IF_NOT_S_OK(ReadHashDigests(aNumFolders, aCRCsDefined, aCRCs)); 
      for(i = 0; i < aNumFolders; i++)
      {
        CFolderItemInfo &aFolder = aFolders[i];
        aFolder.UnPackCRCDefined = aCRCsDefined[i];
        aFolder.UnPackCRC = aCRCs[i];
      }
      continue;
    }
    RETURN_IF_NOT_S_OK(SkeepData());
  }
}

HRESULT CInArchive::ReadSubStreamsInfo(
    const CObjectVector<CFolderItemInfo> &aFolders,
    CRecordVector<UINT64> &aNumUnPackStreamsInFolders,
    CRecordVector<UINT64> &anUnPackSizes,
    CRecordVector<bool> &aDigestsDefined, 
    CRecordVector<UINT32> &aDigests)
{
  aNumUnPackStreamsInFolders.Clear();
  aNumUnPackStreamsInFolders.Reserve(aFolders.Size());
  BYTE aType;
  while(true)
  {
    RETURN_IF_NOT_S_OK(SafeReadByte2(aType));
    if (aType == NID::kNumUnPackStream)
    {
      for(int i = 0; i < aFolders.Size(); i++)
      {
        UINT64 aValue;
        RETURN_IF_NOT_S_OK(ReadNumber(aValue));
        aNumUnPackStreamsInFolders.Add(aValue);
      }
      continue;
    }
    if (aType == NID::kCRC || aType == NID::kSize)
      break;
    if (aType == NID::kEnd)
      break;
    RETURN_IF_NOT_S_OK(SkeepData());
  }

  if (aNumUnPackStreamsInFolders.IsEmpty())
    for(int i = 0; i < aFolders.Size(); i++)
      aNumUnPackStreamsInFolders.Add(1);

  for(int i = 0; i < aNumUnPackStreamsInFolders.Size(); i++)
  {
    UINT64 aSum = 0;
    for (UINT64 j = 1; j < aNumUnPackStreamsInFolders[i]; j++)
    {
      UINT64 aSize;
      if (aType == NID::kSize)
      {
        RETURN_IF_NOT_S_OK(ReadNumber(aSize));
        anUnPackSizes.Add(aSize);
        aSum += aSize;
      }
    }
    anUnPackSizes.Add(aFolders[i].GetUnPackSize() - aSum);
  }
  if (aType == NID::kSize)
  {
    RETURN_IF_NOT_S_OK(SafeReadByte2(aType));
  }

  int aNumDigests = 0;
  int aNumDigestsTotal = 0;
  for(i = 0; i < aFolders.Size(); i++)
  {
    int aNumSubstreams = aNumUnPackStreamsInFolders[i];
    if (aNumSubstreams != 1 || !aFolders[i].UnPackCRCDefined)
      aNumDigests += aNumSubstreams;
    aNumDigestsTotal += aNumSubstreams;
  }

  while(true)
  {
    if (aType == NID::kCRC)
    {
      CRecordVector<bool> aDigestsDefined2; 
      CRecordVector<UINT32> aDigests2;
      RETURN_IF_NOT_S_OK(ReadHashDigests(aNumDigests, aDigestsDefined2, aDigests2));
      int anDigestIndex = 0;
      for (i = 0; i < aFolders.Size(); i++)
      {
        int aNumSubstreams = aNumUnPackStreamsInFolders[i];
        const CFolderItemInfo &aFolder = aFolders[i];
        if (aNumSubstreams == 1 && aFolder.UnPackCRCDefined)
        {
          aDigestsDefined.Add(true);
          aDigests.Add(aFolder.UnPackCRC);
        }
        else
          for (int j = 0; j < aNumSubstreams; j++, anDigestIndex++)
          {
            aDigestsDefined.Add(aDigestsDefined2[anDigestIndex]);
            aDigests.Add(aDigests2[anDigestIndex]);
          }
      }
    }
    else if (aType == NID::kEnd)
    {
      if (aDigestsDefined.IsEmpty())
      {
        aDigestsDefined.Clear();
        aDigests.Clear();
        for (int i = 0; i < aNumDigestsTotal; i++)
        {
          aDigestsDefined.Add(false);
          aDigests.Add(0);
        }
      }
      return S_OK;
    }
    else
    {
      RETURN_IF_NOT_S_OK(SkeepData());
    }
    RETURN_IF_NOT_S_OK(SafeReadByte2(aType));
  }
}


HRESULT CInArchive::ReadStreamsInfo(
    const CObjectVector<CByteBuffer> *aDataVector,
    UINT64 &aDataOffset,
    CRecordVector<UINT64> &aPackSizes,
    CRecordVector<bool> &aPackCRCsDefined,
    CRecordVector<UINT32> &aPackCRCs,
    CObjectVector<CFolderItemInfo> &aFolders,
    CRecordVector<UINT64> &aNumUnPackStreamsInFolders,
    CRecordVector<UINT64> &anUnPackSizes,
    CRecordVector<bool> &aDigestsDefined, 
    CRecordVector<UINT32> &aDigests)
{
  while(true)
  {
    BYTE aType;
    RETURN_IF_NOT_S_OK(SafeReadByte2(aType));
    switch(aType)
    {
      case NID::kEnd:
        return S_OK;
      case NID::kPackInfo:
      {
        RETURN_IF_NOT_S_OK(ReadPackInfo(aDataOffset, aPackSizes,
            aPackCRCsDefined, aPackCRCs));
        break;
      }
      case NID::kUnPackInfo:
      {
        RETURN_IF_NOT_S_OK(ReadUnPackInfo(aDataVector, aFolders));
        break;
      }
      case NID::kSubStreamsInfo:
      {
        RETURN_IF_NOT_S_OK(ReadSubStreamsInfo(aFolders, aNumUnPackStreamsInFolders,
          anUnPackSizes, aDigestsDefined, aDigests));
        break;
      }
    }
  }
}


HRESULT CInArchive::ReadFileNames(CObjectVector<CFileItemInfo> &aFiles)
{
  // UINT32 aPos = 0;
  for(int i = 0; i < aFiles.Size(); i++)
  {
    UString &aName = aFiles[i].Name;
    aName.Empty();
    while (true)
    {
      // if (aPos >= aSize)
      //   return S_FALSE;
      wchar_t aChar;
      RETURN_IF_NOT_S_OK(SafeReadWideCharLE(aChar));
      // aPos++;
      if (aChar == '\0')
        break;
      aName += aChar;
    }
  }
  // if (aPos != aSize)
  //   return S_FALSE;
  return S_OK;
}

HRESULT CInArchive::CheckIntegrity()
{
  return S_OK;
}


HRESULT CInArchive::ReadBoolVector(UINT32 aNumItems, CBoolVector &aVector)
{
  aVector.Clear();
  aVector.Reserve(aNumItems);
  BYTE aByte;
  BYTE aMask = 0;
  for(UINT32 i = 0; i < aNumItems; i++)
  {
    if (aMask == 0)
    {
      RETURN_IF_NOT_S_OK(SafeReadBytes2(&aByte, 1));
      aMask = 0x80;
    }
    aVector.Add((aByte & aMask) != 0);
    aMask >>= 1;
  }
  return S_OK;
}

HRESULT CInArchive::ReadBoolVector2(UINT32 aNumItems, CBoolVector &aVector)
{
  BYTE anAllAreDefinedFlag;
  RETURN_IF_NOT_S_OK(SafeReadByte2(anAllAreDefinedFlag));
  if (anAllAreDefinedFlag == 0)
    return ReadBoolVector(aNumItems, aVector);
  aVector.Clear();
  aVector.Reserve(aNumItems);
  for (int j = 0; j < aNumItems; j++)
    aVector.Add(true);
  return S_OK;
}

HRESULT CInArchive::ReadTime(const CObjectVector<CByteBuffer> &aDataVector,
    CObjectVector<CFileItemInfo> &aFiles, BYTE aType)
{
  CBoolVector aBoolVector;
  RETURN_IF_NOT_S_OK(ReadBoolVector2(aFiles.Size(), aBoolVector))

  CStreamSwitch aStreamSwitch;
  RETURN_IF_NOT_S_OK(aStreamSwitch.Set(this, &aDataVector));

  for(int i = 0; i < aFiles.Size(); i++)
  {
    CFileItemInfo &anItemInfo = aFiles[i];
    CArchiveFileTime aTime;
    bool aDefined = aBoolVector[i];
    if (aDefined)
    {
      RETURN_IF_NOT_S_OK(SafeReadBytes2(&aTime, sizeof(aTime)));
    }
    switch(aType)
    {
      case NID::kCreationTime:
        anItemInfo.IsCreationTimeDefined= aDefined;
        if (aDefined)
          anItemInfo.CreationTime = aTime;
        break;
      case NID::kLastWriteTime:
        anItemInfo.IsLastWriteTimeDefined = aDefined;
        if (aDefined)
          anItemInfo.LastWriteTime = aTime;
        break;
      case NID::kLastAccessTime:
        anItemInfo.IsLastAccessTimeDefined = aDefined;
        if (aDefined)
          anItemInfo.LastAccessTime = aTime;
        break;
    }
  }
  return S_OK;
}

HRESULT CInArchive::ReadHeader(CArchiveDatabaseEx &aDatabase)
{
  aDatabase.Clear();

  BYTE aType;
  RETURN_IF_NOT_S_OK(SafeReadByte2(aType));

  if (aType == NID::kArchiveProperties)
  {
    RETURN_IF_NOT_S_OK(ReadArhiveProperties(aDatabase.m_ArchiveInfo));
    RETURN_IF_NOT_S_OK(SafeReadByte2(aType));
  }
 
  CObjectVector<CByteBuffer> aDataVector;
  
  if (aType == NID::kAdditionalStreamsInfo)
  {
    CRecordVector<UINT64> aPackSizes;
    CRecordVector<bool> aPackCRCsDefined;
    CRecordVector<UINT32> aPackCRCs;
    CObjectVector<CFolderItemInfo> aFolders;

    CRecordVector<UINT64> aNumUnPackStreamsInFolders;
    CRecordVector<UINT64> anUnPackSizes;
    CRecordVector<bool> aDigestsDefined;
    CRecordVector<UINT32> aDigests;

    RETURN_IF_NOT_S_OK(ReadStreamsInfo(NULL, 
        aDatabase.m_ArchiveInfo.DataStartPosition2,
        aPackSizes, 
        aPackCRCsDefined, 
        aPackCRCs, 
        aFolders,
        aNumUnPackStreamsInFolders,
        anUnPackSizes,
        aDigestsDefined, 
        aDigests));

    aDatabase.m_ArchiveInfo.DataStartPosition2 += aDatabase.m_ArchiveInfo.StartPositionAfterHeader;

    UINT32 aPackIndex = 0;
    CDecoder aDecoder;
    UINT64 aDataStartPos = aDatabase.m_ArchiveInfo.DataStartPosition2;
    for(int i = 0; i < aFolders.Size(); i++)
    {
      const CFolderItemInfo &aFolder = aFolders[i];
      aDataVector.Add(CByteBuffer());
      CByteBuffer &aData = aDataVector.Back();
      UINT64 anUnPackSize = aFolder.GetUnPackSize();
      aData.SetCapacity(anUnPackSize);
      
      CComObjectNoLock<CSequentialOutStreamImp2> *anOutStreamSpec = 
        new  CComObjectNoLock<CSequentialOutStreamImp2>;
      CComPtr<ISequentialOutStream> anOutStream = anOutStreamSpec;
      anOutStreamSpec->Init(aData, anUnPackSize);
      
      RETURN_IF_NOT_S_OK(aDecoder.Decode(m_Stream, aDataStartPos, 
          &aPackSizes[aPackIndex], aFolder, anOutStream, NULL));
      
      if (aFolder.UnPackCRCDefined)
        if (!CCRC::VerifyDigest(aFolder.UnPackCRC, aData, anUnPackSize))
          throw CInArchiveException(CInArchiveException::kIncorrectHeader);
      for (int j = 0; j < aFolder.PackStreams.Size(); j++)
        aDataStartPos += aPackSizes[aPackIndex++];
    }
    RETURN_IF_NOT_S_OK(SafeReadByte2(aType));
  }

  CRecordVector<UINT64> anUnPackSizes;
  CRecordVector<bool> aDigestsDefined;
  CRecordVector<UINT32> aDigests;
  
  if (aType == NID::kMainStreamsInfo)
  {
    RETURN_IF_NOT_S_OK(ReadStreamsInfo(&aDataVector,
        aDatabase.m_ArchiveInfo.DataStartPosition,
        aDatabase.m_PackSizes, 
        aDatabase.m_PackCRCsDefined, 
        aDatabase.m_PackCRCs, 
        aDatabase.m_Folders,
        aDatabase.m_NumUnPackStreamsVector,
        anUnPackSizes,
        aDigestsDefined,
        aDigests));
    aDatabase.m_ArchiveInfo.DataStartPosition += aDatabase.m_ArchiveInfo.StartPositionAfterHeader;
    RETURN_IF_NOT_S_OK(SafeReadByte2(aType));
  }
  else
  {
    for(int i = 0; i < aDatabase.m_Folders.Size(); i++)
    {
      aDatabase.m_NumUnPackStreamsVector.Add(1);
      CFolderItemInfo &aFolder = aDatabase.m_Folders[i];
      anUnPackSizes.Add(aFolder.GetUnPackSize());
      aDigestsDefined.Add(aFolder.UnPackCRCDefined);
      aDigests.Add(aFolder.UnPackCRC);
    }
  }

  UINT64 aNumUnPackStreamsTotal = 0;

  aDatabase.m_Files.Clear();

  if (aType == NID::kEnd)
    return S_OK;
  if (aType != NID::kFilesInfo)
    throw CInArchiveException(CInArchiveException::kIncorrectHeader);
  
  UINT64 aNumFiles;
  RETURN_IF_NOT_S_OK(ReadNumber(aNumFiles));
  aDatabase.m_Files.Reserve(aNumFiles);
  for(UINT64 i = 0; i < aNumFiles; i++)
    aDatabase.m_Files.Add(CFileItemInfo());

  // int aSizePrev = -1;
  // int aPosPrev = 0;

  aDatabase.m_ArchiveInfo.FileInfoPopIDs.Add(NID::kSize);
  if (!aDatabase.m_PackSizes.IsEmpty())
    aDatabase.m_ArchiveInfo.FileInfoPopIDs.Add(NID::kPackInfo);
  if (aNumFiles > 0)
  {
    if (!aDigests.IsEmpty())
    { 
      aDatabase.m_ArchiveInfo.FileInfoPopIDs.Add(NID::kCRC);
    }
  }

  CBoolVector anEmptyStreamVector;
  anEmptyStreamVector.Reserve(aNumFiles);
  for(i = 0; i < aNumFiles; i++)
    anEmptyStreamVector.Add(false);
  CBoolVector anEmptyFileVector;
  CBoolVector anAntiFileVector;
  UINT32 aNumEmptyStreams = 0;

  while(true)
  {
    /*
    if (aSizePrev >= 0)
      if (aSizePrev != m_InByteBack->GetProcessedSize() - aPosPrev)
        throw 2;
    */
    BYTE aType;
    RETURN_IF_NOT_S_OK(SafeReadByte2(aType));
    if (aType == NID::kEnd)
      break;
    UINT64 aSize;
    RETURN_IF_NOT_S_OK(ReadNumber(aSize));
    
    // aSizePrev = aSize;
    // aPosPrev = m_InByteBack->GetProcessedSize();

    aDatabase.m_ArchiveInfo.FileInfoPopIDs.Add(aType);
    switch(aType)
    {
      case NID::kName:
      {
        CStreamSwitch aStreamSwitch;
        RETURN_IF_NOT_S_OK(aStreamSwitch.Set(this, &aDataVector));
        RETURN_IF_NOT_S_OK(ReadFileNames(aDatabase.m_Files))
        break;
      }
      case NID::kWinAttributes:
      {
        CBoolVector aBoolVector;
        RETURN_IF_NOT_S_OK(ReadBoolVector2(aDatabase.m_Files.Size(), aBoolVector))
        CStreamSwitch aStreamSwitch;
        RETURN_IF_NOT_S_OK(aStreamSwitch.Set(this, &aDataVector));
        for(i = 0; i < aNumFiles; i++)
        {
          CFileItemInfo &anItemInfo = aDatabase.m_Files[i];
          if (anItemInfo.AreAttributesDefined = aBoolVector[i])
          {
            RETURN_IF_NOT_S_OK(SafeReadBytes2(&anItemInfo.Attributes, 
                sizeof(anItemInfo.Attributes)));
          }
        }
        break;
      }
      case NID::kEmptyStream:
      {
        RETURN_IF_NOT_S_OK(ReadBoolVector(aNumFiles, anEmptyStreamVector))
        for (int i = 0; i < anEmptyStreamVector.Size(); i++)
          if (anEmptyStreamVector[i])
            aNumEmptyStreams++;
        anEmptyFileVector.Reserve(aNumEmptyStreams);
        anAntiFileVector.Reserve(aNumEmptyStreams);
        for (i = 0; i < aNumEmptyStreams; i++)
        {
          anEmptyFileVector.Add(false);
          anAntiFileVector.Add(false);
        }
        break;
      }
      case NID::kEmptyFile:
      {
        RETURN_IF_NOT_S_OK(ReadBoolVector(aNumEmptyStreams, anEmptyFileVector))
        break;
      }
      case NID::kAnti:
      {
        RETURN_IF_NOT_S_OK(ReadBoolVector(aNumEmptyStreams, anAntiFileVector))
        break;
      }
      case NID::kCreationTime:
      case NID::kLastWriteTime:
      case NID::kLastAccessTime:
      {
        CBoolVector aBoolVector;
        RETURN_IF_NOT_S_OK(ReadTime(aDataVector, aDatabase.m_Files, aType))
        break;
      }
      default:
      {
        aDatabase.m_ArchiveInfo.FileInfoPopIDs.DeleteBack();
        RETURN_IF_NOT_S_OK(SkeepData(aSize));
      }
    }
  }

  UINT32 anEmptyFileIndex = 0;
  UINT32 aSizeIndex = 0;
  for(i = 0; i < aNumFiles; i++)
  {
    CFileItemInfo &anItemInfo = aDatabase.m_Files[i];
    if(anEmptyStreamVector[i])
    {
      anItemInfo.IsDirectory = !anEmptyFileVector[anEmptyFileIndex];
      anItemInfo.IsAnti = anAntiFileVector[anEmptyFileIndex];
      anEmptyFileIndex++;
      anItemInfo.UnPackSize = 0;
      anItemInfo.FileCRCIsDefined = false;
    }
    else
    {
      anItemInfo.IsDirectory = false;
      anItemInfo.UnPackSize = anUnPackSizes[aSizeIndex];
      anItemInfo.FileCRC = aDigests[aSizeIndex];
      anItemInfo.FileCRCIsDefined = aDigestsDefined[aSizeIndex];
      aSizeIndex++;
    }
  }

  return S_OK;
}


void CArchiveDatabaseEx::FillFolderStartPackStream()
{
  m_FolderStartPackStreamIndex.Clear();
  m_FolderStartPackStreamIndex.Reserve(m_Folders.Size());
  UINT64 aStartPos = 0;
  for(UINT64 i = 0; i < m_Folders.Size(); i++)
  {
    m_FolderStartPackStreamIndex.Add(aStartPos);
    aStartPos += m_Folders[i].PackStreams.Size();
  }
}

void CArchiveDatabaseEx::FillStartPos()
{
  m_PackStreamStartPositions.Clear();
  m_PackStreamStartPositions.Reserve(m_PackSizes.Size());
  UINT64 aStartPos = 0;
  for(UINT64 i = 0; i < m_PackSizes.Size(); i++)
  {
    m_PackStreamStartPositions.Add(aStartPos);
    aStartPos += m_PackSizes[i];
  }
}

void CArchiveDatabaseEx::FillFolderStartFileIndex()
{
  m_FolderStartFileIndex.Clear();
  m_FolderStartFileIndex.Reserve(m_Folders.Size());
  m_FileIndexToFolderIndexMap.Clear();
  m_FileIndexToFolderIndexMap.Reserve(m_Files.Size());
  
  int aFolderIndex = 0;
  int anIndexInFolder = 0;
  for (int i = 0; i < m_Files.Size(); i++)
  {
    const CFileItemInfo &aFile = m_Files[i];
    bool anEmptyStream = (aFile.IsDirectory || aFile.UnPackSize == 0);
    if (anEmptyStream && anIndexInFolder == 0)
    {
      m_FileIndexToFolderIndexMap.Add(-1);
      continue;
    }
    if (anIndexInFolder == 0)
    {
      if (aFolderIndex >= m_Folders.Size())
        throw CInArchiveException(CInArchiveException::kIncorrectHeader);
      m_FolderStartFileIndex.Add(i);
    }
    m_FileIndexToFolderIndexMap.Add(aFolderIndex);
    if (anEmptyStream)
      continue;
    anIndexInFolder++;
    if (anIndexInFolder >= m_NumUnPackStreamsVector[aFolderIndex])
    {
      aFolderIndex++;
      anIndexInFolder = 0;
    }
  }
}

HRESULT CInArchive::ReadDatabase(CArchiveDatabaseEx &aDatabase)
{
  aDatabase.m_ArchiveInfo.StartPosition = m_ArhiveBeginStreamPosition;
  RETURN_IF_NOT_S_OK(SafeReadBytes(&aDatabase.m_ArchiveInfo.Version, 
      sizeof(aDatabase.m_ArchiveInfo.Version)));
  if (aDatabase.m_ArchiveInfo.Version.Major != kMajorVersion)
    throw  CInArchiveException(CInArchiveException::kUnsupportedVersion);

  UINT32 aCRCFromArchive;
  RETURN_IF_NOT_S_OK(SafeReadBytes(&aCRCFromArchive, sizeof(aCRCFromArchive)));
  CStartHeader aStartHeader;
  RETURN_IF_NOT_S_OK(SafeReadBytes(&aStartHeader, sizeof(aStartHeader)));
  if (!CCRC::VerifyDigest(aCRCFromArchive, &aStartHeader, sizeof(aStartHeader)))
    throw CInArchiveException(CInArchiveException::kIncorrectHeader);

  aDatabase.m_ArchiveInfo.StartPositionAfterHeader = m_Position;

  RETURN_IF_NOT_S_OK(m_Stream->Seek(aStartHeader.NextHeaderOffset, STREAM_SEEK_CUR, &m_Position));

  CByteBuffer aBuffer2;
  aBuffer2.SetCapacity(aStartHeader.NextHeaderSize);
  RETURN_IF_NOT_S_OK(SafeReadBytes(aBuffer2, aStartHeader.NextHeaderSize));
  if (!CCRC::VerifyDigest(aStartHeader.NextHeaderCRC, aBuffer2, aStartHeader.NextHeaderSize))
    throw CInArchiveException(CInArchiveException::kIncorrectHeader);
  
  {
    CStreamSwitch aStreamSwitch;
    aStreamSwitch.Set(this, aBuffer2, aStartHeader.NextHeaderSize);

    BYTE aType;
    RETURN_IF_NOT_S_OK(SafeReadByte2(aType));

    if (aType != NID::kHeader)
      throw CInArchiveException(CInArchiveException::kIncorrectHeader);

    RETURN_IF_NOT_S_OK(ReadHeader(aDatabase));
  }
  return S_OK;
}

}}
