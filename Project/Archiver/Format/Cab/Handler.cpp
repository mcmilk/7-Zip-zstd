// Cab/Handler.cpp

#include "stdafx.h"

#include "Common/StringConvert.h"
#include "Common/Defs.h"
#include "Common/UTFConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"
#include "Windows/COMTry.h"

#include "Archive/Cab/StoreDecoder.h"
#include "Archive/Cab/LZXDecoder.h"
#include "Archive/Cab/MSZIPDecoder.h"

#include "Handler.h"

#include "Interface/ProgressUtils.h"
#include "Interface/EnumStatProp.h"


using namespace NWindows;
using namespace NTime;
using namespace std;
using namespace NArchive;
using namespace NCab;

STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidLastWriteTime, VT_FILETIME},
  { NULL, kpidAttributes, VT_UI4},

  { NULL, kpidMethod, VT_BSTR},
  // { NULL, kpidDictionarySize, VT_UI4},

  { L"Block", kpidBlock, VT_UI4}
};

static const kNumProperties = sizeof(kProperties) / sizeof(kProperties[0]);

const wchar_t *kMethods[] = 
{
  L"None",
  L"MSZip",
  L"Quantum",
  L"LZX"
};

const kNumMethods = sizeof(kMethods) / sizeof(kMethods[0]);
const wchar_t *kUnknownMethod = L"Unknown";

STDMETHODIMP CCabHandler::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  COM_TRY_BEGIN
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
  COM_TRY_END
}

STDMETHODIMP CCabHandler::GetProperty(UINT32 anIndex, PROPID aPropID,  PROPVARIANT *aValue)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
  const NArchive::NCab::CFileInfo &fileInfo = m_Files[anIndex];
  switch(aPropID)
  {
    case kpidPath:
      if (fileInfo.IsNameUTF())
      {
        UString aUnicodeName;
        if (!ConvertUTF8ToUnicode(fileInfo.Name, aUnicodeName))
          propVariant = L"";
        else
          propVariant = aUnicodeName;
      }
      else
        propVariant = MultiByteToUnicodeString(fileInfo.Name, CP_ACP);
      break;
    case kpidIsFolder:
      propVariant = false;
      break;
    case kpidSize:
      propVariant = fileInfo.UnPackSize;
      break;
    case kpidLastWriteTime:
    {
      FILETIME aLocalFileTime, anUTCFileTime;
      if (DosTimeToFileTime(fileInfo.Time, aLocalFileTime))
      {
        if (!LocalFileTimeToFileTime(&aLocalFileTime, &anUTCFileTime))
          anUTCFileTime.dwHighDateTime = anUTCFileTime.dwLowDateTime = 0;
      }
      else
        anUTCFileTime.dwHighDateTime = anUTCFileTime.dwLowDateTime = 0;
      propVariant = anUTCFileTime;
      break;
    }
    case kpidAttributes:
      propVariant = fileInfo.GetWinAttributes();
      break;

    case kpidMethod:
    {
      UINT16 aRealFolderIndex = NHeader::NFolderIndex::GetRealFolderIndex(
          m_Folders.Size(), fileInfo.FolderIndex);
      const NHeader::CFolder &aFolder = m_Folders[aRealFolderIndex];
      UString method;
      if (aFolder.CompressionTypeMajor < kNumMethods)
        method = kMethods[aFolder.CompressionTypeMajor];
      else
        method = kUnknownMethod;
      if (aFolder.CompressionTypeMajor == NHeader::NCompressionMethodMajor::kLZX)
      {
        method += L":";
        wchar_t temp[32];
        _itow (aFolder.CompressionTypeMinor, temp, 10);
        method += temp;
      }
      propVariant = method;
      // propVariant = aFolder.CompressionTypeMajor;
      break;
    }
    /*
    case kpidDictionarySize:
    {
      UINT16 aRealFolderIndex = NHeader::NFolderIndex::GetRealFolderIndex(
          m_Folders.Size(), fileInfo.FolderIndex);
      const NHeader::CFolder &aFolder = m_Folders[aRealFolderIndex];
      if (aFolder.CompressionTypeMajor == NHeader::NCompressionMethodMajor::kLZX)
        propVariant = UINT32(UINT32(1) << aFolder.CompressionTypeMinor);
      else
        propVariant = UINT32(0);
      break;
    }
    */
    case kpidBlock:
      propVariant = UINT32(fileInfo.FolderIndex);
      break;
  }
  propVariant.Detach(aValue);
  return S_OK;
  COM_TRY_END
}

class CPropgressImp: public CProgressVirt
{
  CComPtr<IArchiveOpenCallback> m_OpenArchiveCallback;
public:
  STDMETHOD(SetTotal)(const UINT64 *aNumFiles);
  STDMETHOD(SetCompleted)(const UINT64 *aNumFiles);
  void Init(IArchiveOpenCallback *openArchiveCallback)
    { m_OpenArchiveCallback = openArchiveCallback; }
};

STDMETHODIMP CPropgressImp::SetTotal(const UINT64 *aNumFiles)
{
  if (m_OpenArchiveCallback)
    return m_OpenArchiveCallback->SetCompleted(aNumFiles, NULL);
  return S_OK;
}

STDMETHODIMP CPropgressImp::SetCompleted(const UINT64 *aNumFiles)
{
  if (m_OpenArchiveCallback)
    return m_OpenArchiveCallback->SetCompleted(aNumFiles, NULL);
  return S_OK;
}

STDMETHODIMP CCabHandler::Open(IInStream *aStream, 
    const UINT64 *aMaxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  m_Stream.Release();
  // try
  {
    CInArchive anArchive;
    m_Files.Clear();
    CPropgressImp aPropgressImp;
    aPropgressImp.Init(openArchiveCallback);
    RINOK(anArchive.Open(aStream, aMaxCheckStartPosition, 
        m_ArchiveInfo, m_Folders, m_Files, &aPropgressImp));
    m_Stream = aStream;
  }
  /*
  catch(...)
  {
    return S_FALSE;
  }
  */
  COM_TRY_END
  return S_OK;
}

STDMETHODIMP CCabHandler::Close()
{
  m_Stream.Release();
  return S_OK;
}

//////////////////////////////////////
// CCabHandler::DecompressItems


class CCabFolderOutStream: 
  public ISequentialOutStream,
  public CComObjectRoot
{
public:
BEGIN_COM_MAP(CCabFolderOutStream)
  COM_INTERFACE_ENTRY(ISequentialOutStream)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CCabFolderOutStream)
DECLARE_NO_REGISTRY()

  STDMETHOD(Write)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
  STDMETHOD(WritePart)(const void *aData, UINT32 aSize, UINT32 *aProcessedSize);
private:
  const CObjectVector<NHeader::CFolder> *m_Folders;
  const CObjectVector<CFileInfo> *m_Files;
  const CRecordVector<int> *m_FileIndexes;
  const CRecordVector<bool> *m_ExtractStatuses;
  int m_StartIndex;
  int m_CurrentIndex;
  int m_NumFiles;
  UINT64 m_CurrentDataPos;
  CComPtr<IArchiveExtractCallback> m_ExtractCallback;
  bool m_TestMode;

  bool m_FileIsOpen;
  CComPtr<ISequentialOutStream> aRealOutStream;
  UINT64 m_FilePos;

  HRESULT OpenFile(int anIndexIndex, ISequentialOutStream **aRealOutStream);
  HRESULT WriteEmptyFiles();
  UINT64 m_StartImportantTotalUnPacked;
public:
  void Init(
      const CObjectVector<NHeader::CFolder> *aFolders,
      const CObjectVector<CFileInfo> *aFiles, 
      const CRecordVector<int> *aFileIndexes, 
      const CRecordVector<bool> *anExtractStatuses, 
      int aStartIndex, 
      int aNumFiles, 
      IArchiveExtractCallback *anExtractCallback,
      UINT64 aStartImportantTotalUnPacked,
      bool aTestMode);
  STDMETHOD(FlushCorrupted)();
};

void CCabFolderOutStream::Init(
    const CObjectVector<NHeader::CFolder> *aFolders,
    const CObjectVector<CFileInfo> *aFiles, 
    const CRecordVector<int> *aFileIndexes,
    const CRecordVector<bool> *anExtractStatuses, 
    int aStartIndex, 
    int aNumFiles,
    IArchiveExtractCallback *anExtractCallback,
    UINT64 aStartImportantTotalUnPacked,
    bool aTestMode)
{
  m_Folders = aFolders;
  m_Files = aFiles;
  m_FileIndexes = aFileIndexes;
  m_ExtractStatuses = anExtractStatuses;
  m_StartIndex = aStartIndex;
  m_NumFiles = aNumFiles;
  m_ExtractCallback = anExtractCallback;
  m_StartImportantTotalUnPacked = aStartImportantTotalUnPacked;
  m_TestMode = aTestMode;

  m_CurrentIndex = 0;
  m_FileIsOpen = false;
}

HRESULT CCabFolderOutStream::OpenFile(int anIndexIndex, ISequentialOutStream **aRealOutStream)
{
  // RINOK(m_ExtractCallback->SetCompleted(&m_StartImportantTotalUnPacked));
  
  int aFullIndex = m_StartIndex + anIndexIndex;

  INT32 anAskMode;
  if((*m_ExtractStatuses)[aFullIndex])
    anAskMode = m_TestMode ? 
        NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
  else
    anAskMode = NArchive::NExtract::NAskMode::kSkip;
  
  int anIndex = (*m_FileIndexes)[aFullIndex];
  const CFileInfo &fileInfo = (*m_Files)[anIndex];
  UINT16 aRealFolderIndex = NHeader::NFolderIndex::GetRealFolderIndex(
      m_Folders->Size(), fileInfo.FolderIndex);

  RINOK(m_ExtractCallback->GetStream(anIndex, aRealOutStream, anAskMode));
  
  UINT64 aCurrentUnPackSize = fileInfo.UnPackSize;
  
  bool aMustBeProcessedAnywhere = (anIndexIndex < m_NumFiles - 1);
    
  if (aRealOutStream || aMustBeProcessedAnywhere)
  {
    if (!aRealOutStream && !m_TestMode)
      anAskMode = NArchive::NExtract::NAskMode::kSkip;
    RINOK(m_ExtractCallback->PrepareOperation(anAskMode));
    return S_OK;
  }
  else
    return S_FALSE;
}


HRESULT CCabFolderOutStream::WriteEmptyFiles()
{
  for(;m_CurrentIndex < m_NumFiles; m_CurrentIndex++)
  {
    int anIndex = (*m_FileIndexes)[m_StartIndex + m_CurrentIndex];
    const CFileInfo &fileInfo = (*m_Files)[anIndex];
    if (fileInfo.UnPackSize != 0)
      return S_OK;
    aRealOutStream.Release();
    HRESULT aResult = OpenFile(m_CurrentIndex, &aRealOutStream);
    aRealOutStream.Release();
    if (aResult == S_FALSE)
    {
    }
    else if (aResult == S_OK)
    {
      RINOK(m_ExtractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
    }
    else
      return aResult;
  }
  return S_OK;
}

STDMETHODIMP CCabFolderOutStream::Write(const void *aData, 
    UINT32 aSize, UINT32 *aProcessedSize)
{
  UINT32 aProcessedSizeReal = 0;
  while(m_CurrentIndex < m_NumFiles)
  {
    if (m_FileIsOpen)
    {
      int anIndex = (*m_FileIndexes)[m_StartIndex + m_CurrentIndex];
      const CFileInfo &fileInfo = (*m_Files)[anIndex];
      UINT64 aFileSize = fileInfo.UnPackSize;
      
      UINT32 aNumBytesToWrite = (UINT32)MyMin(aFileSize - m_FilePos, 
          UINT64(aSize - aProcessedSizeReal));
      
      UINT32 aProcessedSizeLocal;
      if (!aRealOutStream)
      {
        aProcessedSizeLocal = aNumBytesToWrite;
      }
      else
      {
        RINOK(aRealOutStream->Write((const BYTE *)aData + aProcessedSizeReal, aNumBytesToWrite, &aProcessedSizeLocal));
      }
      m_FilePos += aProcessedSizeLocal;
      aProcessedSizeReal += aProcessedSizeLocal;
      if (m_FilePos == fileInfo.UnPackSize)
      {
        aRealOutStream.Release();
        RINOK(m_ExtractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
        m_FileIsOpen = false;
        m_CurrentIndex++;
      }
      if (aProcessedSizeReal == aSize)
      {
        RINOK(WriteEmptyFiles());
        if (aProcessedSize != NULL)
          *aProcessedSize = aProcessedSizeReal;
        return S_OK;
      }
    }
    else
    {
      HRESULT aResult = OpenFile(m_CurrentIndex, &aRealOutStream);
      if (aResult != S_FALSE && aResult != S_OK)
        return aResult;
      m_FileIsOpen = true;
      m_FilePos = 0;
    }
  }
  if (aProcessedSize != NULL)
    *aProcessedSize = aSize;
  return S_OK;
}

STDMETHODIMP CCabFolderOutStream::FlushCorrupted()
{
  // UINT32 aProcessedSizeReal = 0;
  while(m_CurrentIndex < m_NumFiles)
  {
    if (m_FileIsOpen)
    {
      int anIndex = (*m_FileIndexes)[m_StartIndex + m_CurrentIndex];
      const CFileInfo &fileInfo = (*m_Files)[anIndex];
      UINT64 aFileSize = fileInfo.UnPackSize;
      
      aRealOutStream.Release();
      RINOK(m_ExtractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kCRCError));
      m_FileIsOpen = false;
      m_CurrentIndex++;
    }
    else
    {
      HRESULT aResult = OpenFile(m_CurrentIndex, &aRealOutStream);
      if (aResult != S_FALSE && aResult != S_OK)
        return aResult;
      m_FileIsOpen = true;
    }
  }
  return S_OK;
}

STDMETHODIMP CCabFolderOutStream::WritePart(const void *aData, 
    UINT32 aSize, UINT32 *aProcessedSize)
{
  return Write(aData, aSize, aProcessedSize);
}


STDMETHODIMP CCabHandler::Extract(const UINT32* anIndexes, UINT32 aNumItems,
    INT32 _aTestMode, IArchiveExtractCallback *_anExtractCallback)
{
  COM_TRY_BEGIN
  bool aTestMode = (_aTestMode != 0);
  CComPtr<IArchiveExtractCallback> anExtractCallback = _anExtractCallback;
  UINT64 aCensoredTotalUnPacked = 0, anImportantTotalUnPacked = 0;
  if(aNumItems == 0)
    return S_OK;
  int aLastIndex = 0;
  CRecordVector<int> aFolderIndexes;
  CRecordVector<int> anImportantIndexes;
  CRecordVector<bool> anExtractStatuses;

  for(int i = 0; i < aNumItems; i++)
  {
    int anIndex = anIndexes[i];
    const CFileInfo &fileInfo = m_Files[anIndex];
    aCensoredTotalUnPacked += fileInfo.UnPackSize;

    int aFolderIndex = fileInfo.FolderIndex;
    if (aFolderIndexes.IsEmpty())
      aFolderIndexes.Add(aFolderIndex);
    else
    {
      if (aFolderIndex != aFolderIndexes.Back())
        aFolderIndexes.Add(aFolderIndex);
    }

    for(int j = anIndex - 1; j >= aLastIndex; j--)
      if(m_Files[j].FolderIndex != aFolderIndex)
        break;
    for(j++; j <= anIndex; j++)
    {
      const CFileInfo &fileInfo = m_Files[j];
      anImportantTotalUnPacked += fileInfo.UnPackSize;
      anImportantIndexes.Add(j);
      anExtractStatuses.Add(j == anIndex);
    }
    aLastIndex = anIndex + 1;
  }

  anExtractCallback->SetTotal(anImportantTotalUnPacked);
  UINT64 aCurrentImportantTotalUnPacked = 0;
  UINT64 aCurrentImportantTotalPacked = 0;

  CComObjectNoLock<CStoreDecoder> *aStoreDecoderSpec = NULL;
  CComPtr<ICompressCoder> aStoreDecoder;

  CComObjectNoLock<NMSZip::CDecoder> *aMSZipDecoderSpec = NULL;
  CComPtr<ICompressCoder> aMSZipDecoder;

  CComObjectNoLock<NLZX::CDecoder> *aLZXDecoderSpec = NULL;
  CComPtr<ICompressCoder> aLZXDecoder;


  int aCurImportantIndexIndex = 0;
  UINT64 aTotalFolderUnPacked;
  for(i = 0; i < aFolderIndexes.Size(); i++, aCurrentImportantTotalUnPacked += aTotalFolderUnPacked)
  {
    int aFolderIndex = aFolderIndexes[i];
    UINT16 aRealFolderIndex = NHeader::NFolderIndex::GetRealFolderIndex(
        m_Folders.Size(), aFolderIndex);

    RINOK(anExtractCallback->SetCompleted(&aCurrentImportantTotalUnPacked));
    aTotalFolderUnPacked = 0;
    for (int j = aCurImportantIndexIndex; j < anImportantIndexes.Size(); j++)
    {
      const CFileInfo &fileInfo = m_Files[anImportantIndexes[j]];
      if (fileInfo.FolderIndex != aFolderIndex)
        break;
      aTotalFolderUnPacked += fileInfo.UnPackSize;
    }
    
    CComObjectNoLock<CCabFolderOutStream> *aCabFolderOutStream = 
      new CComObjectNoLock<CCabFolderOutStream>;
    CComPtr<ISequentialOutStream> anOutStream(aCabFolderOutStream);

    aCabFolderOutStream->Init(&m_Folders, &m_Files, &anImportantIndexes, 
        &anExtractStatuses, aCurImportantIndexIndex, j - aCurImportantIndexIndex, 
        anExtractCallback, aCurrentImportantTotalUnPacked, aTestMode);

    aCurImportantIndexIndex = j;
  
    const NHeader::CFolder &aFolder = m_Folders[aRealFolderIndex];
    UINT64 aPos = aFolder.DataStart; // test it (+ archiveStart)
    RINOK(m_Stream->Seek(aPos, STREAM_SEEK_SET, NULL));

    CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
    CComPtr<ICompressProgressInfo> aProgress = aLocalProgressSpec;
    aLocalProgressSpec->Init(anExtractCallback, false);
   
    CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
    CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;
    aLocalCompressProgressSpec->Init(aProgress, 
        NULL,
        &aCurrentImportantTotalUnPacked);

    BYTE aReservedSize = m_ArchiveInfo.ReserveBlockPresent() ? 
      m_ArchiveInfo.PerDataSizes.PerDatablockAreaSize : 0;

    switch(aFolder.CompressionTypeMajor)
    {
      case NHeader::NCompressionMethodMajor::kNone:
      {
        if(aStoreDecoderSpec == NULL)
        {
          aStoreDecoderSpec = new CComObjectNoLock<CStoreDecoder>;
          aStoreDecoder = aStoreDecoderSpec;
        }
        try
        {
          aStoreDecoderSpec->SetParams(aReservedSize, aFolder.NumDataBlocks);
          RINOK(aStoreDecoder->Code(m_Stream, anOutStream,
              NULL, &aTotalFolderUnPacked, aCompressProgress));
        }
        catch(...)
        {
          RINOK(aCabFolderOutStream->FlushCorrupted());
          continue;
        }
        break;
      }
      case NHeader::NCompressionMethodMajor::kMSZip:
      {
        if(aLZXDecoderSpec == NULL)
        {
          aMSZipDecoderSpec = new CComObjectNoLock<NMSZip::CDecoder>;
          aMSZipDecoder = aMSZipDecoderSpec;
        }
        try
        {
          aMSZipDecoderSpec->SetParams(aReservedSize, aFolder.NumDataBlocks);
          RINOK(aMSZipDecoder->Code(m_Stream, anOutStream,
            NULL, &aTotalFolderUnPacked, aCompressProgress));
        }
        catch(...)
        {
          RINOK(aCabFolderOutStream->FlushCorrupted());
          continue;
        }
        break;
      }
      case NHeader::NCompressionMethodMajor::kLZX:
      {
        if(aLZXDecoderSpec == NULL)
        {
          aLZXDecoderSpec = new CComObjectNoLock<NLZX::CDecoder>;
          aLZXDecoder = aLZXDecoderSpec;
        }
        try
        {
          aLZXDecoderSpec->SetParams(aReservedSize, aFolder.NumDataBlocks, 
              aFolder.CompressionTypeMinor);
          RINOK(aLZXDecoder->Code(m_Stream, anOutStream,
            NULL, &aTotalFolderUnPacked, aCompressProgress));
        }
        catch(...)
        {
          RINOK(aCabFolderOutStream->FlushCorrupted());
          continue;
        }
        break;
      }
    default:
      return E_FAIL;
    }
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CCabHandler::GetNumberOfItems(UINT32 *aNumItems)
{
  COM_TRY_BEGIN
  *aNumItems = m_Files.Size();
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CCabHandler::ExtractAllItems(INT32 aTestMode,
      IArchiveExtractCallback *anExtractCallback)
{
  COM_TRY_BEGIN
  CRecordVector<UINT32> anIndexes;
  anIndexes.Reserve(m_Files.Size());
  for(int i = 0; i < m_Files.Size(); i++)
    anIndexes.Add(i);
  return Extract(&anIndexes.Front(), m_Files.Size(), aTestMode, anExtractCallback);
  COM_TRY_END
}
