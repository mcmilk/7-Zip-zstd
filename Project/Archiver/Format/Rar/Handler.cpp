// Rar/Handler.cpp

#include "StdAfx.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"
#include "Windows/COMTry.h"

#include "Compression/CopyCoder.h"

#include "Handler.h"

#include "../Common/OutStreamWithCRC.h"
#include "../Common/CoderMixer.h"

#include "../../../Compress/Interface/CompressInterface.h"
#include "../../../Crypto/Cipher/Common/CipherInterface.h"
#include "../Common/FormatCryptoInterface.h"

#include "Interface/StreamObjects.h"

#include "Interface/ProgressUtils.h"

#include "Common/StringConvert.h"

using namespace NWindows;
using namespace NTime;

using namespace NArchive;
using namespace NRar;

const wchar_t *kHostOS[] =
{
  L"MS DOS",
  L"OS/2",
  L"Win32",
  L"Unix",
  L"Mac OS",
  L"BeOS"
};

const kNumHostOSes = sizeof(kHostOS) / sizeof(kHostOS[0]);

const wchar_t *kUnknownOS = L"Unknown";

enum // PropID
{
  kaipidUnPackVersion = kaipidUserDefined
};

STATPROPSTG kProperties[] = 
{
  { NULL, kaipidPath, VT_BSTR},
  { NULL, kaipidIsFolder, VT_BOOL},
  { NULL, kaipidSize, VT_UI8},
  { NULL, kaipidPackedSize, VT_UI8},
  { NULL, kaipidLastWriteTime, VT_FILETIME},
  { NULL, kaipidAttributes, VT_UI4},


  { NULL, kaipidEncrypted, VT_BOOL},
  { NULL, kaipidSolid, VT_BOOL},
  { NULL, kaipidComment, VT_BOOL},
  { NULL, kaipidSplitBefore, VT_BOOL},
  { NULL, kaipidSplitAfter, VT_BOOL},
    
  { NULL, kaipidDictionarySize, VT_UI4},
  { NULL, kaipidCRC, VT_UI4},

  { NULL, kaipidHostOS, VT_BSTR},
  { NULL, kaipidMethod, VT_UI1},
  { L"UnPack Version", kaipidUnPackVersion, VT_UI1}
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

STDMETHODIMP CRarHandler::EnumProperties(IEnumSTATPROPSTG **anEnumProperty)
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

STDMETHODIMP CRarHandler::GetNumberOfItems(UINT32 *aNumItems)
{
  *aNumItems = m_Items.Size();
  return S_OK;
}

STDMETHODIMP CRarHandler::GetProperty(UINT32 anIndex, PROPID aPropID,  PROPVARIANT *aValue)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant aPropVariant;
  const NArchive::NRar::CItemInfoEx &anItem = m_Items[anIndex];
  switch(aPropID)
  {
    case kaipidPath:
      aPropVariant = (const wchar_t *)MultiByteToUnicodeString(anItem.Name, CP_OEMCP);
      break;
    case kaipidIsFolder:
      aPropVariant = anItem.IsDirectory();
      break;
    case kaipidSize:
      aPropVariant = anItem.UnPackSize;
      break;
    case kaipidPackedSize:
      aPropVariant = anItem.PackSize;
      break;
    case kaipidLastWriteTime:
    {
      FILETIME aLocalFileTime, anUTCFileTime;
      if (DosTimeToFileTime(anItem.Time, aLocalFileTime))
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
      aPropVariant = anItem.GetWinAttributes();
      break;
    case kaipidEncrypted:
      aPropVariant = anItem.IsEncrypted();
      break;
    case kaipidSolid:
      aPropVariant = anItem.IsSolid();
      break;
    case kaipidComment:
      aPropVariant = anItem.IsCommented();
      break;
    case kaipidSplitBefore:
      aPropVariant = anItem.IsSplitBefore();
      break;
    case kaipidSplitAfter:
      aPropVariant = anItem.IsSplitAfter();
      break;
    case kaipidDictionarySize:
      aPropVariant = UINT32(0x10000 << anItem.GetDictSize());
      break;
    case kaipidCRC:
      aPropVariant = anItem.FileCRC;
      break;
    case kaipidUnPackVersion:
      aPropVariant = anItem.UnPackVersion;
      break;
    case kaipidMethod:
      aPropVariant = anItem.Method;
      break;
    case kaipidHostOS:
      aPropVariant = (anItem.HostOS < kNumHostOSes) ?
        (kHostOS[anItem.HostOS]) : kUnknownOS;
      break;
  }
  aPropVariant.Detach(aValue);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CRarHandler::Open(IInStream *aStream, 
    const UINT64 *aMaxCheckStartPosition,
    IOpenArchive2CallBack *anOpenArchiveCallBack)
{
  COM_TRY_BEGIN
  try
  {
    if(!m_Archive.Open(aStream, aMaxCheckStartPosition))
      return S_FALSE;
    m_Items.Clear();
    
    NRar::CItemInfoEx anItemInfo;
    if (anOpenArchiveCallBack != NULL)
    {
      RETURN_IF_NOT_S_OK(anOpenArchiveCallBack->SetTotal(NULL, NULL));
      UINT64 aNumFiles = m_Items.Size();
      RETURN_IF_NOT_S_OK(anOpenArchiveCallBack->SetCompleted(&aNumFiles, NULL));
    }
    while(m_Archive.GetNextItem(anItemInfo))
    {
      if (anItemInfo.IgnoreItem())
        continue;
      m_Items.Add(anItemInfo);
      if (anOpenArchiveCallBack != NULL)
      {
        UINT64 aNumFiles = m_Items.Size();
        RETURN_IF_NOT_S_OK(anOpenArchiveCallBack->SetCompleted(&aNumFiles, NULL));
      }
    }
  }
  catch(...)
  {
    return S_FALSE;
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CRarHandler::Close()
{
  COM_TRY_BEGIN
  m_Archive.Close();
  return S_OK;
  COM_TRY_END
}


STDMETHODIMP CRarHandler::Extract(const UINT32* anIndexes, UINT32 aNumItems,
    INT32 _aTestMode, IExtractCallback200 *_anExtractCallBack)
{
  COM_TRY_BEGIN
  CComPtr<ICryptoGetTextPassword> aGetTextPassword;
  bool aTestMode = (_aTestMode != 0);
  CComPtr<IExtractCallback200> anExtractCallBack = _anExtractCallBack;
  UINT64 aCensoredTotalUnPacked = 0, aCensoredTotalPacked = 0,
        anImportantTotalUnPacked = 0, anImportantTotalPacked = 0;
  if(aNumItems == 0)
    return S_OK;
  int aLastIndex = 0;
  CRecordVector<int> anImportantIndexes;
  CRecordVector<bool> anExtractStatuses;

  for(UINT32 t = 0; t < aNumItems; t++)
  {
    int anIndex = anIndexes[t];
    const CItemInfoEx &anItemInfo = m_Items[anIndex];
    aCensoredTotalUnPacked += anItemInfo.UnPackSize;
    aCensoredTotalPacked += anItemInfo.PackSize;
    for(int j = aLastIndex; j <= anIndex; j++)
      if(!m_Items[j].IsSolid())
        aLastIndex = j;
    for(j = aLastIndex; j <= anIndex; j++)
    {
      const CItemInfoEx &anItemInfo = m_Items[j];
      anImportantTotalUnPacked += anItemInfo.UnPackSize;
      anImportantTotalPacked += anItemInfo.PackSize;
      anImportantIndexes.Add(j);
      anExtractStatuses.Add(j == anIndex);
    }
    aLastIndex = anIndex + 1;
  }

  anExtractCallBack->SetTotal(anImportantTotalUnPacked);
  UINT64 aCurrentImportantTotalUnPacked = 0;
  UINT64 aCurrentImportantTotalPacked = 0;
  UINT64 aCurrentUnPackSize, aCurrentPackSize;

  CComPtr<ICompressCoder> aDecoder;

  CComObjectNoLock<NCompression::CCopyCoder> *aCopyCoderSpec = NULL;
  CComPtr<ICompressCoder> aCopyCoder;

  CComObjectNoLock<CCoderMixer> *aMixerCoderSpec;
  CComPtr<ICompressCoder> aMixerCoder;
  bool aMixerCoderStoreMethod;

  CComPtr<ICompressCoder> aRar20CryptoDecoder;

  for(int i = 0; i < anImportantIndexes.Size(); i++, 
      aCurrentImportantTotalUnPacked += aCurrentUnPackSize,
      aCurrentImportantTotalPacked += aCurrentPackSize)
  {
    RETURN_IF_NOT_S_OK(anExtractCallBack->SetCompleted(
        &aCurrentImportantTotalUnPacked));
    CComPtr<ISequentialOutStream> aRealOutStream;

    INT32 anAskMode;
    if(anExtractStatuses[i])
      anAskMode = aTestMode ? NArchiveHandler::NExtract::NAskMode::kTest :
          NArchiveHandler::NExtract::NAskMode::kExtract;
    else
      anAskMode = NArchiveHandler::NExtract::NAskMode::kSkip;

    UINT32 anIndex = anImportantIndexes[i];
    const CItemInfoEx &anItemInfo = m_Items[anIndex];

    aCurrentUnPackSize = anItemInfo.UnPackSize;
    aCurrentPackSize = anItemInfo.PackSize;

    if(anItemInfo.IgnoreItem())
      continue;

    RETURN_IF_NOT_S_OK(anExtractCallBack->Extract(anIndex, &aRealOutStream, anAskMode));

    if(anItemInfo.IsDirectory())
    {
      RETURN_IF_NOT_S_OK(anExtractCallBack->PrepareOperation(anAskMode));
      RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK));
      continue;
    }

    bool aMustBeProcessedAnywhere = false;
    if(i < anImportantIndexes.Size() - 1)
    {
      const CItemInfoEx &anItemInfoNext = m_Items[anImportantIndexes[i + 1]];
      aMustBeProcessedAnywhere = anItemInfoNext.IsSolid();
    }
    
    if (!aMustBeProcessedAnywhere && !aTestMode && !aRealOutStream)
      continue;
    
    if (!aRealOutStream && !aTestMode)
      anAskMode = NArchiveHandler::NExtract::NAskMode::kSkip;

    RETURN_IF_NOT_S_OK(anExtractCallBack->PrepareOperation(anAskMode));

    CComObjectNoLock<COutStreamWithCRC> *anOutStreamSpec = 
      new CComObjectNoLock<COutStreamWithCRC>;
    CComPtr<ISequentialOutStream> anOutStream(anOutStreamSpec);
    anOutStreamSpec->Init(aRealOutStream);
    aRealOutStream.Release();
    
    CComPtr<ISequentialInStream> anInStream;
    anInStream.Attach(m_Archive.CreateLimitedStream(anItemInfo.GetDataPosition(),
      anItemInfo.PackSize));
    
    
    CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
    CComPtr<ICompressProgressInfo> aProgress = aLocalProgressSpec;
    aLocalProgressSpec->Init(anExtractCallBack, false);
    
    
    CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
    CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;
    aLocalCompressProgressSpec->Init(aProgress, 
      &aCurrentImportantTotalPacked,
      &aCurrentImportantTotalUnPacked);
    
    if (anItemInfo.IsEncrypted())
    {
      if (!aRar20CryptoDecoder)
      {
        RETURN_IF_NOT_S_OK(aRar20CryptoDecoder.CoCreateInstance(CLSID_CCryptoRar20Decoder));
      }
      CComPtr<ICryptoSetPassword> aCryptoSetPassword;
      RETURN_IF_NOT_S_OK(aRar20CryptoDecoder.QueryInterface(&aCryptoSetPassword));
      if (!aGetTextPassword)
        anExtractCallBack.QueryInterface(&aGetTextPassword);
      if (aGetTextPassword)
      {
        CComBSTR aPassword;
        RETURN_IF_NOT_S_OK(aGetTextPassword->CryptoGetTextPassword(&aPassword));
        AString anOemPassword = UnicodeStringToMultiByte(
          (const wchar_t *)aPassword, CP_OEMCP);
        RETURN_IF_NOT_S_OK(aCryptoSetPassword->CryptoSetPassword(
          (const BYTE *)(const char *)anOemPassword, anOemPassword.Length()));
      }
      else
      {
        RETURN_IF_NOT_S_OK(aCryptoSetPassword->CryptoSetPassword(0, 0));
      }
    }
    switch(anItemInfo.Method)
    {
      case '0':
      {
        if(aCopyCoderSpec == NULL)
        {
          aCopyCoderSpec = new CComObjectNoLock<NCompression::CCopyCoder>;
          aCopyCoder = aCopyCoderSpec;
        }
        
        if (anItemInfo.IsEncrypted())
        {
          if (!aMixerCoder || !aMixerCoderStoreMethod)
          {
            aMixerCoder.Release();
            aMixerCoderSpec = new CComObjectNoLock<CCoderMixer>;
            aMixerCoder = aMixerCoderSpec;
            aMixerCoderSpec->AddCoder(aRar20CryptoDecoder);
            aMixerCoderSpec->AddCoder(aCopyCoder);
            aMixerCoderSpec->FinishAddingCoders();
            aMixerCoderStoreMethod = true;
          }
          aMixerCoderSpec->ReInit();
          aMixerCoderSpec->SetCoderInfo(0, NULL, &anItemInfo.UnPackSize);
          aMixerCoderSpec->SetCoderInfo(1, NULL, NULL);
          aMixerCoderSpec->SetProgressCoderIndex(1);

          RETURN_IF_NOT_S_OK(aMixerCoder->Code(anInStream, anOutStream,
              NULL, NULL, aCompressProgress));
        }
        else
        {
          RETURN_IF_NOT_S_OK(aCopyCoder->Code(anInStream, anOutStream,
              NULL, NULL, aCompressProgress));
        }
        break;
      }
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      {
        if(!aDecoder)
        {
          RETURN_IF_NOT_S_OK(aDecoder.CoCreateInstance(CLSID_CCompressRar20Decoder));
        }

        CComPtr<ICompressSetDecoderProperties> aCompressSetDecoderProperties;
        RETURN_IF_NOT_S_OK(aDecoder.QueryInterface(&aCompressSetDecoderProperties));
        
        BYTE anIsSolid = anItemInfo.IsSolid() ? 1: 0;
        CComObjectNoLock<CSequentialInStreamImp> *anInStreamSpec = new 
          CComObjectNoLock<CSequentialInStreamImp>;
        CComPtr<ISequentialInStream> anInStreamProperties(anInStreamSpec);
        anInStreamSpec->Init(&anIsSolid, 1);
        RETURN_IF_NOT_S_OK(aCompressSetDecoderProperties->SetDecoderProperties(anInStreamProperties));
          
        HRESULT aResult;
        if (anItemInfo.IsEncrypted())
        {
          if (!aMixerCoder || aMixerCoderStoreMethod)
          {
            aMixerCoder.Release();
            aMixerCoderSpec = new CComObjectNoLock<CCoderMixer>;
            aMixerCoder = aMixerCoderSpec;
            aMixerCoderSpec->AddCoder(aRar20CryptoDecoder);
            aMixerCoderSpec->AddCoder(aDecoder);
            aMixerCoderSpec->FinishAddingCoders();
            aMixerCoderStoreMethod = false;
          }
          aMixerCoderSpec->ReInit();
          aMixerCoderSpec->SetCoderInfo(1, &anItemInfo.PackSize, 
              &anItemInfo.UnPackSize);
          aMixerCoderSpec->SetProgressCoderIndex(1);
          aResult = aMixerCoder->Code(anInStream, anOutStream, 
              NULL, NULL, aCompressProgress);
        }
        else
        {
          aResult = aDecoder->Code(anInStream, anOutStream,
              &anItemInfo.PackSize, &anItemInfo.UnPackSize, aCompressProgress);
        }
        if (aResult == S_FALSE)
        {
          anOutStream.Release();
          RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kDataError));
          continue;
        }
        if (aResult != S_OK)
          return aResult;
        break;
      }
      default:
        anOutStream.Release();
        RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod));
        continue;
    }
    bool aCRC_Ok = anOutStreamSpec->GetCRC() == anItemInfo.FileCRC;
    anOutStream.Release();
    if(aCRC_Ok)
      RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK))
    else
      RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kCRCError))
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CRarHandler::ExtractAllItems(INT32 aTestMode,
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

