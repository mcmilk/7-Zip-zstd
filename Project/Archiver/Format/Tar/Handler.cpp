// Tar/Handler.cpp

#include "StdAfx.h"

#include "Handler.h"
#include "Interface/StreamObjects.h"

#include "Windows/Time.h"
#include "Windows/PropVariant.h"
#include "Windows/COMTry.h"

#include "Compression/CopyCoder.h"
#include "Archive/Tar/ItemNameUtils.h"
#include "Archive/Tar/InEngine.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/NewHandler.h"

#include "Interface/ProgressUtils.h"
#include "Interface/LimitedStreams.h"

#include "../Common/DummyOutStream.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NTar {

enum // PropID
{
  kaipidUserName = kaipidUserDefined,
  kaipidGroupName, 
};

STATPROPSTG kProperties[] = 
{
  { NULL, kaipidPath, VT_BSTR},
  { NULL, kaipidIsFolder, VT_BOOL},
  { NULL, kaipidSize, VT_UI8},
  { NULL, kaipidPackedSize, VT_UI8},
  { NULL, kaipidLastWriteTime, VT_FILETIME},
  { L"User Name", kaipidUserName, VT_BSTR},
  { L"Group Name", kaipidGroupName, VT_BSTR},
};

static const kNumProperties = sizeof(kProperties) / sizeof(kProperties[0]);

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

STDMETHODIMP CTarHandler::EnumProperties(IEnumSTATPROPSTG **anEnumProperty)
{
  COM_TRY_BEGIN
  CComObjectNoLock<CEnumArchiveItemProperty> *anEnumObject = 
      new CComObjectNoLock<CEnumArchiveItemProperty>;
  // if (anEnumObject == NULL)
  //   return E_OUTOFMEMORY;
  CComPtr<IEnumSTATPROPSTG> anEnum(anEnumObject);
  // ((CComObjectNoLock<CTestEnumIDList>*)(anEnumObject))->Init(this, m_IDList, aFlags); // TODO : Add any addl. params as needed
  return anEnum->QueryInterface(IID_IEnumSTATPROPSTG, (LPVOID*)anEnumProperty);
  COM_TRY_END
}

STDMETHODIMP CTarHandler::Open(IInStream *aStream, 
    const UINT64 *aMaxCheckStartPosition,
    IOpenArchive2CallBack *anOpenArchiveCallBack)
{
  COM_TRY_BEGIN
  bool aMustBeClosed = true;
  // try
  {
    CInArchive anArchive;

    if(anArchive.Open(aStream) != S_OK)
      return S_FALSE;

    m_Items.Clear();

    if (anOpenArchiveCallBack != NULL)
    {
      RETURN_IF_NOT_S_OK(anOpenArchiveCallBack->SetTotal(NULL, NULL));
      UINT64 aNumFiles = m_Items.Size();
      RETURN_IF_NOT_S_OK(anOpenArchiveCallBack->SetCompleted(&aNumFiles, NULL));
    }

    while(true)
    {
      CItemInfoEx anItemInfo;
      bool aFilled;
      HRESULT aResult = anArchive.GetNextItem(aFilled, anItemInfo);
      if (aResult == S_FALSE)
        return S_FALSE;
      if (aResult != S_OK)
        return S_FALSE;
      if (!aFilled)
        break;
      m_Items.Add(anItemInfo);
      anArchive.SkeepDataRecords(anItemInfo.Size);
      if (anOpenArchiveCallBack != NULL)
      {
        UINT64 aNumFiles = m_Items.Size();
        RETURN_IF_NOT_S_OK(anOpenArchiveCallBack->SetCompleted(&aNumFiles, NULL));
      }
    }
    if (m_Items.Size() == 0)
      return S_FALSE;

    m_InStream = aStream;
  }
  /*
  catch(...)
  {
    return S_FALSE;
  }
  */
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CTarHandler::Close()
{
  m_InStream.Release();
  return S_OK;
}

STDMETHODIMP CTarHandler::GetNumberOfItems(UINT32 *aNumItems)
{
  *aNumItems = m_Items.Size();
  return S_OK;
}

STDMETHODIMP CTarHandler::GetProperty(
    UINT32 anIndex, 
    PROPID aPropID,  
    PROPVARIANT *aValue)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant aPropVariant;
  const NArchive::NTar::CItemInfoEx &anItem = m_Items[anIndex];

  switch(aPropID)
  {
    case kaipidPath:
      aPropVariant = (const wchar_t *)NItemName::GetOSName(
          MultiByteToUnicodeString(anItem.Name, CP_OEMCP));
      break;
    case kaipidIsFolder:
      aPropVariant = anItem.IsDirectory();
      break;
    case kaipidSize:
    case kaipidPackedSize:
      aPropVariant = anItem.Size;
      break;
    case kaipidLastWriteTime:
    {
      FILETIME anUTCFileTime;
      if (anItem.ModificationTime != 0)
        NTime::UnixTimeToFileTime(anItem.ModificationTime, anUTCFileTime);
      else
      {
        anUTCFileTime.dwLowDateTime = 0;
        anUTCFileTime.dwHighDateTime = 0;
      }
      aPropVariant = anUTCFileTime;
      break;
    }
    case kaipidUserName:
      aPropVariant = (const wchar_t *)
          MultiByteToUnicodeString(anItem.UserName, CP_OEMCP);
      break;
    case kaipidGroupName:
      aPropVariant = (const wchar_t *)
          MultiByteToUnicodeString(anItem.GroupName, CP_OEMCP);
      break;
  }
  aPropVariant.Detach(aValue);
  return S_OK;
  COM_TRY_END
}

//////////////////////////////////////
// CTarHandler::DecompressItems

STDMETHODIMP CTarHandler::Extract(const UINT32* anIndexes, UINT32 aNumItems,
    INT32 _aTestMode, IExtractCallback200 *_anExtractCallBack)
{
  COM_TRY_BEGIN
  bool aTestMode = (_aTestMode != 0);
  CComPtr<IExtractCallback200> anExtractCallBack = _anExtractCallBack;
  UINT64 aTotalSize = 0;
  if(aNumItems == 0)
    return S_OK;
  for(UINT32 i = 0; i < aNumItems; i++)
    aTotalSize += m_Items[anIndexes[i]].Size;
  anExtractCallBack->SetTotal(aTotalSize);

  UINT64 aCurrentTotalSize = 0;
  UINT64 aCurrentItemSize;
  
  CComObjectNoLock<NCompression::CCopyCoder> *aCopyCoderSpec = NULL;
  CComPtr<ICompressCoder> aCopyCoder;

  for(i = 0; i < aNumItems; i++, aCurrentTotalSize += aCurrentItemSize)
  {
    RETURN_IF_NOT_S_OK(anExtractCallBack->SetCompleted(&aCurrentTotalSize));
    CComPtr<ISequentialOutStream> aRealOutStream;
    INT32 anAskMode;
    anAskMode = aTestMode ? NArchiveHandler::NExtract::NAskMode::kTest :
        NArchiveHandler::NExtract::NAskMode::kExtract;
    INT32 anIndex = anIndexes[i];
    // NItemIDList::CHolder anItemIDList;
    const CItemInfoEx &anItemInfo = m_Items[anIndex];
    // SetItemIDListFromItemInfo(anItemInfo, anItemIDList);
    
    RETURN_IF_NOT_S_OK(anExtractCallBack->Extract(anIndex, &aRealOutStream, anAskMode));

    aCurrentItemSize = anItemInfo.Size;

    if(anItemInfo.IsDirectory())
    {
      RETURN_IF_NOT_S_OK(anExtractCallBack->PrepareOperation(anAskMode));
      RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK));
      continue;
    }
    if(!aTestMode && (!aRealOutStream))
    {
      continue;
    }
    RETURN_IF_NOT_S_OK(anExtractCallBack->PrepareOperation(anAskMode));
    {
      CComObjectNoLock<CDummyOutStream> *anOutStreamSpec = 
        new CComObjectNoLock<CDummyOutStream>;
      CComPtr<ISequentialOutStream> anOutStream(anOutStreamSpec);
      anOutStreamSpec->Init(aRealOutStream);

      aRealOutStream.Release();

      RETURN_IF_NOT_S_OK(m_InStream->Seek(anItemInfo.GetDataPosition(), STREAM_SEEK_SET, NULL));
      CComObjectNoLock<CLimitedSequentialInStream> *aStreamSpec = new 
          CComObjectNoLock<CLimitedSequentialInStream>;
      CComPtr<ISequentialInStream> anInStream(aStreamSpec);
      aStreamSpec->Init(m_InStream, anItemInfo.Size);

      CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
      CComPtr<ICompressProgressInfo> aProgress = aLocalProgressSpec;
      aLocalProgressSpec->Init(anExtractCallBack, false);


      CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
          new  CComObjectNoLock<CLocalCompressProgressInfo>;
      CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;
      aLocalCompressProgressSpec->Init(aProgress, 
          &aCurrentTotalSize, &aCurrentTotalSize);

      if(aCopyCoderSpec == NULL)
      {
        aCopyCoderSpec = new CComObjectNoLock<NCompression::CCopyCoder>;
        aCopyCoder = aCopyCoderSpec;
      }
      try
      {
        RETURN_IF_NOT_S_OK(aCopyCoder->Code(anInStream, anOutStream,
            NULL, NULL, aCompressProgress));
      }
      catch(...)
      {
        anOutStream.Release();
        RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kDataError));
        continue;
      }
      anOutStream.Release();
      RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK));
    }
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CTarHandler::ExtractAllItems(INT32 aTestMode,
      IExtractCallback200 *anExtractCallBack)
{
  COM_TRY_BEGIN
  CRecordVector<UINT32> anIndexes;
  anIndexes.Reserve(m_Items.Size());
  for(int i = 0; i < m_Items.Size(); i++)
    anIndexes.Add(i);
  return Extract(&anIndexes.Front(), m_Items.Size(), aTestMode,
      anExtractCallBack);
  COM_TRY_END
}

}}
