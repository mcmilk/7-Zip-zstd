// Rar/Handler.cpp

#include "StdAfx.h"

#include "Interface/StreamObjects.h"
#include "Interface/EnumStatProp.h"
#include "Interface/ProgressUtils.h"

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
  kpidUnPackVersion = kpidUserDefined
};

STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},
  { NULL, kpidLastWriteTime, VT_FILETIME},
  { NULL, kpidAttributes, VT_UI4},


  { NULL, kpidEncrypted, VT_BOOL},
  { NULL, kpidSolid, VT_BOOL},
  { NULL, kpidComment, VT_BOOL},
  { NULL, kpidSplitBefore, VT_BOOL},
  { NULL, kpidSplitAfter, VT_BOOL},
  { NULL, kpidCRC, VT_UI4},
  { NULL, kpidHostOS, VT_BSTR},
  { NULL, kpidMethod, VT_BSTR},
  // { NULL, kpidDictionarySize, VT_UI4},
  { L"UnPack Version", kpidUnPackVersion, VT_UI1}
};

STDMETHODIMP CRarHandler::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  COM_TRY_BEGIN
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
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
  NWindows::NCOM::CPropVariant propVariant;
  const NArchive::NRar::CItemInfoEx &item = m_Items[anIndex];
  switch(aPropID)
  {
    case kpidPath:
      propVariant = (const wchar_t *)MultiByteToUnicodeString(item.Name, CP_OEMCP);
      break;
    case kpidIsFolder:
      propVariant = item.IsDirectory();
      break;
    case kpidSize:
      propVariant = item.UnPackSize;
      break;
    case kpidPackedSize:
      propVariant = item.PackSize;
      break;
    case kpidLastWriteTime:
    {
      FILETIME aLocalFileTime, anUTCFileTime;
      if (DosTimeToFileTime(item.Time, aLocalFileTime))
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
      propVariant = item.GetWinAttributes();
      break;
    case kpidEncrypted:
      propVariant = item.IsEncrypted();
      break;
    case kpidSolid:
      propVariant = item.IsSolid();
      break;
    case kpidComment:
      propVariant = item.IsCommented();
      break;
    case kpidSplitBefore:
      propVariant = item.IsSplitBefore();
      break;
    case kpidSplitAfter:
      propVariant = item.IsSplitAfter();
      break;
    /*
    case kpidDictionarySize:
      if (!item.IsDirectory())
        propVariant = UINT32(0x10000 << item.GetDictSize());
      break;
    */
    case kpidCRC:
      propVariant = item.FileCRC;
      break;
    case kpidUnPackVersion:
      propVariant = item.UnPackVersion;
      break;
    case kpidMethod:
    {
      UString method;
      if (item.Method >= BYTE('0') && item.Method <= BYTE('5'))
      {
        method = L"m";
        wchar_t temp[32];
        _itow (item.Method - BYTE('0'), temp, 10);
        method += temp;
        if (!item.IsDirectory())
        {
          method += L":";
          _itow (16 + item.GetDictSize(), temp, 10);
          method += temp;
        }
      }
      else
      {
        wchar_t temp[32];
        _itow (item.Method, temp, 10);
        method += temp;
      }
      propVariant = method;

      // propVariant = item.Method;
      break;
    }
    case kpidHostOS:
      propVariant = (item.HostOS < kNumHostOSes) ?
        (kHostOS[item.HostOS]) : kUnknownOS;
      break;
  }
  propVariant.Detach(aValue);
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
          if (anItemInfo.UnPackVersion >= 29)
          {
            anOutStream.Release();
            RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod));
            continue;
          }
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
        /*
        if (anItemInfo.UnPackVersion >= 29)
        {
          anOutStream.Release();
          RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod));
          continue;
        }
        */

        if(!aDecoder)
        {
          if (anItemInfo.UnPackVersion < 29)
          {
            RETURN_IF_NOT_S_OK(aDecoder.CoCreateInstance(CLSID_CCompressRar20Decoder));
          }
          else
          {
            RETURN_IF_NOT_S_OK(aDecoder.CoCreateInstance(CLSID_CCompressRar29Decoder));
          }
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
          if (anItemInfo.UnPackVersion >= 29)
          {
            anOutStream.Release();
            RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod));
            continue;
          }
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

