// Handler.cpp

#include "StdAfx.h"

#include "Handler.h"
#include "Interface/StreamObjects.h"

#include "Windows/PropVariant.h"
#include "Windows/Defs.h"
#include "Windows/COMTry.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"

#include "Interface/ProgressUtils.h"

#include "../../../Compress/Interface/CompressInterface.h"
#include "../Common/DummyOutStream.h"

#ifdef COMPRESS_BZIP2
#include "../../../Compress/BWT/BZip2/Decoder.h"
#else
// {23170F69-40C1-278B-0402-020000000000}
DEFINE_GUID(CLSID_CCompressBZip2Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00);
#endif

using namespace NWindows;

namespace NArchive {
namespace NBZip2 {

static const kNumItemInArchive = 1;


STATPROPSTG kProperties[] = 
{
  { NULL, kaipidPath, VT_BSTR},
  { NULL, kaipidIsFolder, VT_BOOL},
  // { NULL, kaipidLastWriteTime, VT_FILETIME},
  // { NULL, kaipidSize, VT_UI8},
  { NULL, kaipidPackedSize, VT_UI8},
};

static const kNumProperties = sizeof(kProperties) / sizeof(kProperties[0]);

class CEnumIDList: 
  public IEnumIDList,
  public CComObjectRoot
{
  int m_Index;
  CItemInfoEx m_Item;
public:

  BEGIN_COM_MAP(CEnumIDList)
  COM_INTERFACE_ENTRY(IEnumIDList)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CEnumIDList)

DECLARE_NO_REGISTRY()
  
  CEnumIDList(): m_Index(0) {};
  void Init(const CItemInfoEx &anItem);

  STDMETHODIMP Next(ULONG, LPITEMIDLIST *, ULONG *);
  STDMETHODIMP Skip(ULONG );
  STDMETHODIMP Reset();
  STDMETHODIMP Clone(IEnumIDList **);
};

void CEnumIDList::Init(const CItemInfoEx &anItem)
{
  m_Item = anItem;
}

STDMETHODIMP CEnumIDList::Reset()
{
  m_Index = 0;
  return S_OK;
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
  COM_TRY_BEGIN
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
  COM_TRY_END
}

STDMETHODIMP CEnumArchiveItemProperty::Skip(ULONG aNumSkip)
  {  return E_NOTIMPL; }

STDMETHODIMP CEnumArchiveItemProperty::Clone(IEnumSTATPROPSTG **anEnum)
  {  return E_NOTIMPL; }

STDMETHODIMP CHandler::EnumProperties(IEnumSTATPROPSTG **anEnumProperty)
{
  COM_TRY_BEGIN
  CComObjectNoLock<CEnumArchiveItemProperty> *anEnumObject = 
      new CComObjectNoLock<CEnumArchiveItemProperty>;
  if (anEnumObject == NULL)
    return E_OUTOFMEMORY;
  CComPtr<IEnumSTATPROPSTG> anEnum(anEnumObject);
  return anEnum->QueryInterface(IID_IEnumSTATPROPSTG, (LPVOID*)anEnumProperty);
  COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UINT32 *aNumItems)
{
  *aNumItems = kNumItemInArchive;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(
    UINT32 anIndex, 
    PROPID aPropID,  
    PROPVARIANT *aValue)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant aPropVariant;
  if (anIndex != 0)
    return E_INVALIDARG;
  switch(aPropID)
  {
    case kaipidIsFolder:
      aPropVariant = false;
      break;
    case kaipidPackedSize:
      aPropVariant = m_Item.PackSize;
      break;
  }
  aPropVariant.Detach(aValue);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Open(IInStream *aStream, 
    const UINT64 *aMaxCheckStartPosition,
    IOpenArchive2CallBack *anOpenArchiveCallBack)
{
  COM_TRY_BEGIN
  try
  {
    RETURN_IF_NOT_S_OK(aStream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition));
    const kSignatureSize = 3;
    BYTE aBuffer[kSignatureSize];
    UINT32 aProcessedSize;
    RETURN_IF_NOT_S_OK(aStream->Read(aBuffer, kSignatureSize, &aProcessedSize));
    if (aProcessedSize != kSignatureSize)
      return S_FALSE;
    if (aBuffer[0] != 'B' || aBuffer[1] != 'Z' || aBuffer[2] != 'h')
      return S_FALSE;

    UINT64 anEndPosition;
    RETURN_IF_NOT_S_OK(aStream->Seek(0, STREAM_SEEK_END, &anEndPosition));
    m_Item.PackSize = anEndPosition - m_StreamStartPosition;
    
    m_Stream = aStream;
  }
  catch(...)
  {
    return S_FALSE;
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  m_Stream.Release();
  return S_OK;
}


STDMETHODIMP CHandler::Extract(const UINT32* anIndexes, UINT32 aNumItems,
    INT32 _aTestMode, IExtractCallback200 *anExtractCallBack)
{
  COM_TRY_BEGIN
  if (aNumItems == 0)
    return S_OK;
  if (aNumItems != kNumItemInArchive)
    return E_INVALIDARG;
  if (anIndexes[0] != 0)
    return E_INVALIDARG;

  bool aTestMode = (_aTestMode != 0);

  anExtractCallBack->SetTotal(m_Item.PackSize);

  UINT64 aCurrentTotalPacked = 0;
  
  RETURN_IF_NOT_S_OK(anExtractCallBack->SetCompleted(&aCurrentTotalPacked));
  
  CComPtr<ISequentialOutStream> aRealOutStream;
  INT32 anAskMode;
  anAskMode = aTestMode ? NArchiveHandler::NExtract::NAskMode::kTest :
  NArchiveHandler::NExtract::NAskMode::kExtract;
  
  RETURN_IF_NOT_S_OK(anExtractCallBack->Extract(0, &aRealOutStream, anAskMode));
  
  
  if(!aTestMode && !aRealOutStream)
    return S_OK;

  anExtractCallBack->PrepareOperation(anAskMode);

  CComObjectNoLock<CDummyOutStream> *anOutStreamSpec = 
    new CComObjectNoLock<CDummyOutStream>;
  CComPtr<ISequentialOutStream> anOutStream(anOutStreamSpec);
  anOutStreamSpec->Init(aRealOutStream);
  
  aRealOutStream.Release();

  CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
  CComPtr<ICompressProgressInfo> aProgress = aLocalProgressSpec;
  aLocalProgressSpec->Init(anExtractCallBack, true);
  
  RETURN_IF_NOT_S_OK(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL));

  CComPtr<ICompressCoder> aDecoder;
  #ifdef COMPRESS_BZIP2
  aDecoder = new CComObjectNoLock<NCompress::NBZip2::NDecoder::CCoder>;
  #else
  RETURN_IF_NOT_S_OK(aDecoder.CoCreateInstance(CLSID_CCompressBZip2Decoder));
  #endif

  HRESULT aResult = aDecoder->Code(m_Stream, anOutStream, NULL, NULL, aProgress);
  anOutStream.Release();
  if (aResult == S_FALSE)
    RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
        NArchiveHandler::NExtract::NOperationResult::kDataError))
  else if (aResult == S_OK)
    RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
      NArchiveHandler::NExtract::NOperationResult::kOK))
  else
    return aResult;
 
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::ExtractAllItems(INT32 aTestMode, IExtractCallback200 *anExtractCallBack)
{
  UINT32 anIndex = 0;
  return Extract(&anIndex, 1, aTestMode, anExtractCallBack);
}

}}
