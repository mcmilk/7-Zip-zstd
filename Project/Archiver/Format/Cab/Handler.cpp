// Cab/Handler.cpp

#include "stdafx.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"
#include "Windows/COMTry.h"

#include "Archive/Cab/StoreDecoder.h"
#include "Archive/Cab/LZXDecoder.h"
#include "Archive/Cab/MSZIPDecoder.h"

#include "Handler.h"

#include "Interface/ProgressUtils.h"

#include "Common/StringConvert.h"
#include "Common/Defs.h"

using namespace NWindows;
using namespace NTime;
using namespace std;
using namespace NArchive;
using namespace NCab;

enum // PropID
{
  kaipidMethod,
  kaipidFolderIndex,
};

STATPROPSTG kProperties[] = 
{
  { NULL, kaipidPath, VT_BSTR},
  { NULL, kaipidIsFolder, VT_BOOL},
  { NULL, kaipidSize, VT_UI8},
  { NULL, kaipidLastWriteTime, VT_FILETIME},
  { NULL, kaipidAttributes, VT_UI4},

  { L"Method", kaipidMethod, VT_UI1},
  { NULL, kaipidDictionarySize, VT_UI4},

  { L"Folder Index", kaipidFolderIndex, VT_UI2}
};

static const kNumProperties = sizeof(kProperties) / sizeof(kProperties[0]);


class CEnumIDList: 
  public IEnumIDList,
  public CComObjectRoot
{
  int m_Index;
  const CObjectVector<CFileInfo> *m_Files;
  const CObjectVector<NHeader::CFolder> *m_Folders;
public:

  BEGIN_COM_MAP(CEnumIDList)
  COM_INTERFACE_ENTRY(IEnumIDList)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CEnumIDList)

DECLARE_NO_REGISTRY()
  
  CEnumIDList(): m_Index(0) {};
  void Init(const CObjectVector<NHeader::CFolder> *aFolders, 
      const CObjectVector<CFileInfo> *aFiles);

  STDMETHODIMP Next(ULONG, LPITEMIDLIST *, ULONG *);
  STDMETHODIMP Skip(ULONG );
  STDMETHODIMP Reset();
  STDMETHODIMP Clone(IEnumIDList **);
};

////////////////////////////////////
// CEnumIDList

void CEnumIDList::Init(const CObjectVector<NHeader::CFolder> *aFolders, 
    const CObjectVector<CFileInfo> *aFiles)
{
  m_Files = aFiles;
  m_Folders = aFolders;
}



STDMETHODIMP CEnumIDList::Reset()
{
  m_Index = 0;
  return S_OK;
}

bool ConvertUTF8ToUnicode(const AString &anUTFString, UString &anResultString)
{
  anResultString.Empty();
  for(int i = 0; i < anUTFString.Length(); i++)
  {
    BYTE aChar = anUTFString[i];
    if (aChar < 0x80)
    {
      anResultString += aChar;
      continue;
    }
    if(aChar < 0xC0 || aChar >= 0xF0)
      return false;
    i++;
    if (i >= anUTFString.Length())
      return false;
    BYTE aChar2 = anUTFString[i];
    if (aChar2 < 0x80)
      return false;
    aChar2 -= 0x80;
    if (aChar2 >= 0x40)
      return false;
    if (aChar < 0xE0)
    {
      anResultString += wchar_t( ((wchar_t(aChar - 0xC0)) << 6) + aChar2);
      continue;
    }
    i++;
    if (i >= anUTFString.Length())
      return false;
    BYTE aChar3 = anUTFString[i];
    aChar3 -= 0x80;
    if (aChar3 >= 0x40)
      return false;
    anResultString += wchar_t(((wchar_t(aChar - 0xE0)) << 12) + 
      ((wchar_t(aChar2)) << 6) + aChar3);
  }
  return true; 
}

/////////////////////////////////////////////////
// CEnumArchiveItemProperty

class CEnumArchiveItemProperty:
  public IEnumSTATPROPSTG,
  public CComObjectRoot
{
public:
  int m_Index;

  BEGIN_COM_MAP(CEnumArchiveItemProperty)
    COM_INTERFACE_ENTRY(IEnumSTATPROPSTG)
  END_COM_MAP()
    
  DECLARE_NOT_AGGREGATABLE(CEnumArchiveItemProperty)
    
  DECLARE_NO_REGISTRY()
public:
  CEnumArchiveItemProperty(): m_Index(0) {};

  STDMETHOD(Next) (ULONG aNumItems, STATPROPSTG *anItems, ULONG *aNumFetched);
  STDMETHOD(Skip)  (ULONG aNumItems);
  STDMETHOD(Reset) ();
  STDMETHOD(Clone) (IEnumSTATPROPSTG **anEnum);
};

STDMETHODIMP CEnumArchiveItemProperty::Reset()
{
  m_Index = 0;
  return S_OK;
}

STDMETHODIMP CEnumArchiveItemProperty::Next(ULONG aNumItems, 
    STATPROPSTG *anItems, ULONG *aNumFetched)
{
  HRESULT aResult = S_OK;
  if(aNumItems > 1 && !aNumFetched)
    return E_INVALIDARG;

  for(DWORD anIndex = 0; anIndex < aNumItems; anIndex++, m_Index++)
  {
    if(m_Index >= kNumProperties)
    {
      aResult =  S_FALSE;
      break;
    }
    const STATPROPSTG &aSrcItem = kProperties[m_Index];
    STATPROPSTG &aDestItem = anItems[anIndex];
    aDestItem.propid = aSrcItem.propid;
    aDestItem.vt = aSrcItem.vt;
    if(aSrcItem.lpwstrName != NULL)
    {
      aDestItem.lpwstrName = (wchar_t *)CoTaskMemAlloc((wcslen(aSrcItem.lpwstrName) + 1) * sizeof(wchar_t));
      wcscpy(aDestItem.lpwstrName, aSrcItem.lpwstrName);
    }
    else
      aDestItem.lpwstrName = aSrcItem.lpwstrName;
  }
  if (aNumFetched)
    *aNumFetched = anIndex;
  return aResult;
}

STDMETHODIMP CEnumArchiveItemProperty::Skip(ULONG aNumSkip)
  {  return E_NOTIMPL; }

STDMETHODIMP CEnumArchiveItemProperty::Clone(IEnumSTATPROPSTG **anEnum)
  {  return E_NOTIMPL; }

STDMETHODIMP CCabHandler::EnumProperties(IEnumSTATPROPSTG **anEnumProperty)
{
  COM_TRY_BEGIN
  CComObjectNoLock<CEnumArchiveItemProperty> *anEnumObject = 
      new CComObjectNoLock<CEnumArchiveItemProperty>;
  if (anEnumObject == NULL)
    return E_OUTOFMEMORY;
  CComPtr<IEnumSTATPROPSTG> anEnum(anEnumObject);
  // ((CComObjectNoLock<CEnumIDList>*)(anEnumObject))->Init(this, m_IDList, aFlags); // TODO : Add any addl. params as needed
  return anEnum->QueryInterface(IID_IEnumSTATPROPSTG, (LPVOID*)anEnumProperty);
  COM_TRY_END
}

STDMETHODIMP CCabHandler::GetProperty(UINT32 anIndex, PROPID aPropID,  PROPVARIANT *aValue)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant aPropVariant;
  const NArchive::NCab::CFileInfo &aFileInfo = m_Files[anIndex];
  switch(aPropID)
  {
    case kaipidPath:
      if (aFileInfo.IsNameUTF())
      {
        UString aUnicodeName;
        if (!ConvertUTF8ToUnicode(aFileInfo.Name, aUnicodeName))
          aPropVariant = L"";
        else
          aPropVariant = aUnicodeName;
      }
      else
        aPropVariant = MultiByteToUnicodeString(aFileInfo.Name, CP_ACP);
      break;
    case kaipidIsFolder:
      aPropVariant = false;
      break;
    case kaipidSize:
      aPropVariant = aFileInfo.UnPackSize;
      break;
    case kaipidLastWriteTime:
    {
      FILETIME aLocalFileTime, anUTCFileTime;
      if (DosTimeToFileTime(aFileInfo.Time, aLocalFileTime))
      {
        if (!LocalFileTimeToFileTime(&aLocalFileTime, &anUTCFileTime))
          anUTCFileTime.dwHighDateTime = anUTCFileTime.dwLowDateTime = 0;
      }
      else
        anUTCFileTime.dwHighDateTime = anUTCFileTime.dwLowDateTime = 0;
      aPropVariant = anUTCFileTime;
      break;
    }
    case kaipidAttributes:
      aPropVariant = aFileInfo.GetWinAttributes();
      break;

    case kaipidMethod:
    {
      UINT16 aRealFolderIndex = NHeader::NFolderIndex::GetRealFolderIndex(
          m_Folders.Size(), aFileInfo.FolderIndex);
      const NHeader::CFolder &aFolder = m_Folders[aRealFolderIndex];
      aPropVariant = aFolder.CompressionTypeMajor;
      break;
    }
    case kaipidDictionarySize:
    {
      UINT16 aRealFolderIndex = NHeader::NFolderIndex::GetRealFolderIndex(
          m_Folders.Size(), aFileInfo.FolderIndex);
      const NHeader::CFolder &aFolder = m_Folders[aRealFolderIndex];
      if (aFolder.CompressionTypeMajor == NHeader::NCompressionMethodMajor::kLZX)
        aPropVariant = UINT32(UINT32(1) << aFolder.CompressionTypeMinor);
      else
        aPropVariant = UINT32(0);
      break;
    }
    case kaipidFolderIndex:
      aPropVariant = UINT32(aFileInfo.FolderIndex);
      break;
  }
  aPropVariant.Detach(aValue);
  return S_OK;
  COM_TRY_END
}

class CPropgressImp: public CProgressVirt
{
  CComPtr<IOpenArchive2CallBack> m_OpenArchiveCallBack;
public:
  STDMETHOD(SetTotal)(const UINT64 *aNumFiles);
  STDMETHOD(SetCompleted)(const UINT64 *aNumFiles);
  void Init(IOpenArchive2CallBack *anOpenArchiveCallBack)
    { m_OpenArchiveCallBack = anOpenArchiveCallBack; }
};

STDMETHODIMP CPropgressImp::SetTotal(const UINT64 *aNumFiles)
{
  if (m_OpenArchiveCallBack)
    return m_OpenArchiveCallBack->SetCompleted(aNumFiles, NULL);
  return S_OK;
}

STDMETHODIMP CPropgressImp::SetCompleted(const UINT64 *aNumFiles)
{
  if (m_OpenArchiveCallBack)
    return m_OpenArchiveCallBack->SetCompleted(aNumFiles, NULL);
  return S_OK;
}

STDMETHODIMP CCabHandler::Open(IInStream *aStream, 
    const UINT64 *aMaxCheckStartPosition,
    IOpenArchive2CallBack *anOpenArchiveCallBack)
{
  COM_TRY_BEGIN
  m_Stream.Release();
  // try
  {
    CInArchive anArchive;
    m_Files.Clear();
    CPropgressImp aPropgressImp;
    aPropgressImp.Init(anOpenArchiveCallBack);
    RETURN_IF_NOT_S_OK(anArchive.Open(aStream, aMaxCheckStartPosition, 
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
  CComPtr<IExtractCallback200> m_ExtractCallBack;
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
      IExtractCallback200 *anExtractCallBack,
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
    IExtractCallback200 *anExtractCallBack,
    UINT64 aStartImportantTotalUnPacked,
    bool aTestMode)
{
  m_Folders = aFolders;
  m_Files = aFiles;
  m_FileIndexes = aFileIndexes;
  m_ExtractStatuses = anExtractStatuses;
  m_StartIndex = aStartIndex;
  m_NumFiles = aNumFiles;
  m_ExtractCallBack = anExtractCallBack;
  m_StartImportantTotalUnPacked = aStartImportantTotalUnPacked;
  m_TestMode = aTestMode;

  m_CurrentIndex = 0;
  m_FileIsOpen = false;
}

HRESULT CCabFolderOutStream::OpenFile(int anIndexIndex, ISequentialOutStream **aRealOutStream)
{
  // RETURN_IF_NOT_S_OK(m_ExtractCallBack->SetCompleted(&m_StartImportantTotalUnPacked));
  
  int aFullIndex = m_StartIndex + anIndexIndex;

  INT32 anAskMode;
  if((*m_ExtractStatuses)[aFullIndex])
    anAskMode = m_TestMode ? 
        NArchiveHandler::NExtract::NAskMode::kTest :
        NArchiveHandler::NExtract::NAskMode::kExtract;
  else
    anAskMode = NArchiveHandler::NExtract::NAskMode::kSkip;
  
  int anIndex = (*m_FileIndexes)[aFullIndex];
  const CFileInfo &aFileInfo = (*m_Files)[anIndex];
  UINT16 aRealFolderIndex = NHeader::NFolderIndex::GetRealFolderIndex(
      m_Folders->Size(), aFileInfo.FolderIndex);

  RETURN_IF_NOT_S_OK(m_ExtractCallBack->Extract(anIndex, aRealOutStream, anAskMode));
  
  UINT64 aCurrentUnPackSize = aFileInfo.UnPackSize;
  
  bool aMustBeProcessedAnywhere = (anIndexIndex < m_NumFiles - 1);
    
  if (aRealOutStream || aMustBeProcessedAnywhere)
  {
    if (!aRealOutStream && !m_TestMode)
      anAskMode = NArchiveHandler::NExtract::NAskMode::kSkip;
    RETURN_IF_NOT_S_OK(m_ExtractCallBack->PrepareOperation(anAskMode));
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
    const CFileInfo &aFileInfo = (*m_Files)[anIndex];
    if (aFileInfo.UnPackSize != 0)
      return S_OK;
    aRealOutStream.Release();
    HRESULT aResult = OpenFile(m_CurrentIndex, &aRealOutStream);
    aRealOutStream.Release();
    if (aResult == S_FALSE)
    {
    }
    else if (aResult == S_OK)
    {
      RETURN_IF_NOT_S_OK(m_ExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK));
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
      const CFileInfo &aFileInfo = (*m_Files)[anIndex];
      UINT64 aFileSize = aFileInfo.UnPackSize;
      
      UINT32 aNumBytesToWrite = (UINT32)MyMin(aFileSize - m_FilePos, 
          UINT64(aSize - aProcessedSizeReal));
      
      UINT32 aProcessedSizeLocal;
      if (!aRealOutStream)
      {
        aProcessedSizeLocal = aNumBytesToWrite;
      }
      else
      {
        RETURN_IF_NOT_S_OK(aRealOutStream->Write((const BYTE *)aData + aProcessedSizeReal, aNumBytesToWrite, &aProcessedSizeLocal));
      }
      m_FilePos += aProcessedSizeLocal;
      aProcessedSizeReal += aProcessedSizeLocal;
      if (m_FilePos == aFileInfo.UnPackSize)
      {
        aRealOutStream.Release();
        RETURN_IF_NOT_S_OK(m_ExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK));
        m_FileIsOpen = false;
        m_CurrentIndex++;
      }
      if (aProcessedSizeReal == aSize)
      {
        RETURN_IF_NOT_S_OK(WriteEmptyFiles());
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
      const CFileInfo &aFileInfo = (*m_Files)[anIndex];
      UINT64 aFileSize = aFileInfo.UnPackSize;
      
      aRealOutStream.Release();
      RETURN_IF_NOT_S_OK(m_ExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kCRCError));
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
    INT32 _aTestMode, IExtractCallback200 *_anExtractCallBack)
{
  COM_TRY_BEGIN
  bool aTestMode = (_aTestMode != 0);
  CComPtr<IExtractCallback200> anExtractCallBack = _anExtractCallBack;
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
    const CFileInfo &aFileInfo = m_Files[anIndex];
    aCensoredTotalUnPacked += aFileInfo.UnPackSize;

    int aFolderIndex = aFileInfo.FolderIndex;
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
      const CFileInfo &aFileInfo = m_Files[j];
      anImportantTotalUnPacked += aFileInfo.UnPackSize;
      anImportantIndexes.Add(j);
      anExtractStatuses.Add(j == anIndex);
    }
    aLastIndex = anIndex + 1;
  }

  anExtractCallBack->SetTotal(anImportantTotalUnPacked);
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

    RETURN_IF_NOT_S_OK(anExtractCallBack->SetCompleted(&aCurrentImportantTotalUnPacked));
    aTotalFolderUnPacked = 0;
    for (int j = aCurImportantIndexIndex; j < anImportantIndexes.Size(); j++)
    {
      const CFileInfo &aFileInfo = m_Files[anImportantIndexes[j]];
      if (aFileInfo.FolderIndex != aFolderIndex)
        break;
      aTotalFolderUnPacked += aFileInfo.UnPackSize;
    }
    
    CComObjectNoLock<CCabFolderOutStream> *aCabFolderOutStream = 
      new CComObjectNoLock<CCabFolderOutStream>;
    CComPtr<ISequentialOutStream> anOutStream(aCabFolderOutStream);

    aCabFolderOutStream->Init(&m_Folders, &m_Files, &anImportantIndexes, 
        &anExtractStatuses, aCurImportantIndexIndex, j - aCurImportantIndexIndex, 
        anExtractCallBack, aCurrentImportantTotalUnPacked, aTestMode);

    aCurImportantIndexIndex = j;
  
    const NHeader::CFolder &aFolder = m_Folders[aRealFolderIndex];
    UINT64 aPos = aFolder.DataStart; // test it (+ archiveStart)
    RETURN_IF_NOT_S_OK(m_Stream->Seek(aPos, STREAM_SEEK_SET, NULL));

    CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
    CComPtr<ICompressProgressInfo> aProgress = aLocalProgressSpec;
    aLocalProgressSpec->Init(anExtractCallBack, false);
   
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
          RETURN_IF_NOT_S_OK(aStoreDecoder->Code(m_Stream, anOutStream,
              NULL, &aTotalFolderUnPacked, aCompressProgress));
        }
        catch(...)
        {
          RETURN_IF_NOT_S_OK(aCabFolderOutStream->FlushCorrupted());
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
          RETURN_IF_NOT_S_OK(aMSZipDecoder->Code(m_Stream, anOutStream,
            NULL, &aTotalFolderUnPacked, aCompressProgress));
        }
        catch(...)
        {
          RETURN_IF_NOT_S_OK(aCabFolderOutStream->FlushCorrupted());
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
          RETURN_IF_NOT_S_OK(aLZXDecoder->Code(m_Stream, anOutStream,
            NULL, &aTotalFolderUnPacked, aCompressProgress));
        }
        catch(...)
        {
          RETURN_IF_NOT_S_OK(aCabFolderOutStream->FlushCorrupted());
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
      IExtractCallback200 *anExtractCallBack)
{
  COM_TRY_BEGIN
  CRecordVector<UINT32> anIndexes;
  anIndexes.Reserve(m_Files.Size());
  for(int i = 0; i < m_Files.Size(); i++)
    anIndexes.Add(i);
  return Extract(&anIndexes.Front(), m_Files.Size(), aTestMode, anExtractCallBack);
  COM_TRY_END
}
