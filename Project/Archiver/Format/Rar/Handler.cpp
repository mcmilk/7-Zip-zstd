// Rar/Handler.cpp

#include "StdAfx.h"

#include "Interface/StreamObjects.h"
#include "Interface/EnumStatProp.h"
#include "Interface/ProgressUtils.h"
#include "Interface/CryptoInterface.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"
#include "Windows/COMTry.h"

#include "Compression/CopyCoder.h"

#include "Handler.h"

#include "../Common/OutStreamWithCRC.h"
#include "../Common/CoderMixer.h"
// #include "../Common/CoderMixer2.h"

#include "../../../Compress/Interface/CompressInterface.h"
#include "../../../Crypto/Cipher/Common/CipherInterface.h"


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

STDMETHODIMP CRarHandler::GetNumberOfItems(UINT32 *numItems)
{
  *numItems = _items.Size();
  return S_OK;
}

STDMETHODIMP CRarHandler::GetProperty(UINT32 index, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
  const NArchive::NRar::CItemInfoEx &item = _items[index];
  switch(propID)
  {
    case kpidPath:
      if (item.HasUnicodeName())
        propVariant = item.UnicodeName;
      else
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
      FILETIME localFileTime, utcFileTime;
      if (DosTimeToFileTime(item.Time, localFileTime))
      {
        if (!LocalFileTimeToFileTime(&localFileTime, &utcFileTime))
          utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
      }
      else
        utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
      propVariant = utcFileTime;
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
  propVariant.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CRarHandler::Open(IInStream *stream, 
    const UINT64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  try
  {
    if(!_archive.Open(stream, maxCheckStartPosition))
      return S_FALSE;
    _items.Clear();
    
    NRar::CItemInfoEx itemInfo;
    if (openArchiveCallback != NULL)
    {
      RINOK(openArchiveCallback->SetTotal(NULL, NULL));
      UINT64 numFiles = _items.Size();
      RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
    }
    while(_archive.GetNextItem(itemInfo))
    {
      if (itemInfo.IgnoreItem())
        continue;
      _items.Add(itemInfo);
      if (openArchiveCallback != NULL)
      {
        UINT64 numFiles = _items.Size();
        RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
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
  _archive.Close();
  return S_OK;
  COM_TRY_END
}


STDMETHODIMP CRarHandler::Extract(const UINT32* indices, UINT32 numItems,
    INT32 _aTestMode, IArchiveExtractCallback *_anExtractCallback)
{
  COM_TRY_BEGIN
  CComPtr<ICryptoGetTextPassword> getTextPassword;
  bool testMode = (_aTestMode != 0);
  CComPtr<IArchiveExtractCallback> extractCallback = _anExtractCallback;
  UINT64 censoredTotalUnPacked = 0, censoredTotalPacked = 0,
        importantTotalUnPacked = 0, importantTotalPacked = 0;
  if(numItems == 0)
    return S_OK;
  int lastIndex = 0;
  CRecordVector<int> importantIndexes;
  CRecordVector<bool> extractStatuses;

  for(UINT32 t = 0; t < numItems; t++)
  {
    int index = indices[t];
    const CItemInfoEx &itemInfo = _items[index];
    censoredTotalUnPacked += itemInfo.UnPackSize;
    censoredTotalPacked += itemInfo.PackSize;
    for(int j = lastIndex; j <= index; j++)
      if(!_items[j].IsSolid())
        lastIndex = j;
    for(j = lastIndex; j <= index; j++)
    {
      const CItemInfoEx &itemInfo = _items[j];
      importantTotalUnPacked += itemInfo.UnPackSize;
      importantTotalPacked += itemInfo.PackSize;
      importantIndexes.Add(j);
      extractStatuses.Add(j == index);
    }
    lastIndex = index + 1;
  }

  extractCallback->SetTotal(importantTotalUnPacked);
  UINT64 currentImportantTotalUnPacked = 0;
  UINT64 currentImportantTotalPacked = 0;
  UINT64 currentUnPackSize, currentPackSize;

  CComPtr<ICompressCoder> decoder;

  CComObjectNoLock<NCompression::CCopyCoder> *copyCoderSpec = NULL;
  CComPtr<ICompressCoder> copyCoder;

  CComObjectNoLock<CCoderMixer> *mixerCoderSpec;
  CComPtr<ICompressCoder> mixerCoder;

  // CComObjectNoLock<NCoderMixer2::CCoderMixer2> *mixerCoder2Spec;
  // CComPtr<ICompressCoder2> mixerCoder2;

  bool mixerCoderStoreMethod;

  int mixerCryptoVersion;

  CComPtr<ICompressCoder> rar20CryptoDecoder;
  
  CComPtr<ICompressCoder> rar29CryptoDecoder;

  for(int i = 0; i < importantIndexes.Size(); i++, 
      currentImportantTotalUnPacked += currentUnPackSize,
      currentImportantTotalPacked += currentPackSize)
  {
    RINOK(extractCallback->SetCompleted(
        &currentImportantTotalUnPacked));
    CComPtr<ISequentialOutStream> realOutStream;

    INT32 askMode;
    if(extractStatuses[i])
      askMode = testMode ? NArchive::NExtract::NAskMode::kTest :
          NArchive::NExtract::NAskMode::kExtract;
    else
      askMode = NArchive::NExtract::NAskMode::kSkip;

    UINT32 index = importantIndexes[i];
    const CItemInfoEx &itemInfo = _items[index];

    currentUnPackSize = itemInfo.UnPackSize;
    currentPackSize = itemInfo.PackSize;

    if(itemInfo.IgnoreItem())
      continue;

    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    if(itemInfo.IsDirectory())
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
      continue;
    }

    bool mustBeProcessedAnywhere = false;
    if(i < importantIndexes.Size() - 1)
    {
      const CItemInfoEx &nextItemInfo = _items[importantIndexes[i + 1]];
      mustBeProcessedAnywhere = nextItemInfo.IsSolid();
    }
    
    if (!mustBeProcessedAnywhere && !testMode && !realOutStream)
      continue;
    
    if (!realOutStream && !testMode)
      askMode = NArchive::NExtract::NAskMode::kSkip;

    RINOK(extractCallback->PrepareOperation(askMode));

    CComObjectNoLock<COutStreamWithCRC> *outStreamSpec = 
      new CComObjectNoLock<COutStreamWithCRC>;
    CComPtr<ISequentialOutStream> outStream(outStreamSpec);
    outStreamSpec->Init(realOutStream);
    realOutStream.Release();
    
    CComPtr<ISequentialInStream> inStream;
    inStream.Attach(_archive.CreateLimitedStream(itemInfo.GetDataPosition(),
      itemInfo.PackSize));
    
    
    CComObjectNoLock<CLocalProgress> *localProgressSpec = new  CComObjectNoLock<CLocalProgress>;
    CComPtr<ICompressProgressInfo> progress = localProgressSpec;
    localProgressSpec->Init(extractCallback, false);
    
    
    CComObjectNoLock<CLocalCompressProgressInfo> *localCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
    CComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
    localCompressProgressSpec->Init(progress, 
      &currentImportantTotalPacked,
      &currentImportantTotalUnPacked);
    
    if (itemInfo.IsEncrypted())
    {
      CComPtr<ICryptoSetPassword> cryptoSetPassword;
      if (itemInfo.UnPackVersion >= 29)
      {
        if (!rar29CryptoDecoder)
        {
          RINOK(rar29CryptoDecoder.CoCreateInstance(CLSID_CCryptoRar29Decoder));
        }
        CComPtr<ICompressSetDecoderProperties> cryptoProperties;
        RINOK(rar29CryptoDecoder.QueryInterface(&cryptoProperties));
        CComObjectNoLock<CSequentialInStreamImp> *inStreamSpec = new 
            CComObjectNoLock<CSequentialInStreamImp>;
        CComPtr<ISequentialInStream> inStreamProperties(inStreamSpec);
        inStreamSpec->Init(itemInfo.Salt, itemInfo.HasSalt() ? sizeof(itemInfo.Salt) : 0);
        RINOK(cryptoProperties->SetDecoderProperties(inStreamProperties));
        RINOK(rar29CryptoDecoder.QueryInterface(&cryptoSetPassword));
      }
      else if (itemInfo.UnPackVersion >= 20)
      {
        if (!rar20CryptoDecoder)
        {
          RINOK(rar20CryptoDecoder.CoCreateInstance(CLSID_CCryptoRar20Decoder));
        }
        RINOK(rar20CryptoDecoder.QueryInterface(&cryptoSetPassword));
      }
      else
      {
        outStream.Release();
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kUnSupportedMethod));
        continue;
      }

      if (!getTextPassword)
        extractCallback.QueryInterface(&getTextPassword);
      if (getTextPassword)
      {
        CComBSTR password;
        RINOK(getTextPassword->CryptoGetTextPassword(&password));
        if (itemInfo.UnPackVersion >= 29)
        {
          RINOK(cryptoSetPassword->CryptoSetPassword(
            (const BYTE *)(const wchar_t *)password, password.Length() * 
            sizeof(wchar_t)));
        }
        else
        {
          AString oemPassword = UnicodeStringToMultiByte(
            (const wchar_t *)password, CP_OEMCP);
          RINOK(cryptoSetPassword->CryptoSetPassword(
            (const BYTE *)(const char *)oemPassword, oemPassword.Length()));
        }
      }
      else
      {
        RINOK(cryptoSetPassword->CryptoSetPassword(0, 0));
      }
    }
    switch(itemInfo.Method)
    {
      case '0':
      {
        if(copyCoderSpec == NULL)
        {
          copyCoderSpec = new CComObjectNoLock<NCompression::CCopyCoder>;
          copyCoder = copyCoderSpec;
        }
        
        if (itemInfo.IsEncrypted())
        {
          {
            if (!mixerCoder || !mixerCoderStoreMethod || 
                itemInfo.UnPackVersion != mixerCryptoVersion)
            {
              mixerCoder.Release();
              mixerCoderSpec = new CComObjectNoLock<CCoderMixer>;
              mixerCoder = mixerCoderSpec;
              if (itemInfo.UnPackVersion >= 29)
                mixerCoderSpec->AddCoder(rar29CryptoDecoder);
              else
                mixerCoderSpec->AddCoder(rar20CryptoDecoder);
              mixerCoderSpec->AddCoder(copyCoder);
              mixerCoderSpec->FinishAddingCoders();
              mixerCoderStoreMethod = true;
              mixerCryptoVersion = itemInfo.UnPackVersion;
            }
            mixerCoderSpec->ReInit();
            mixerCoderSpec->SetCoderInfo(0, &itemInfo.PackSize, &itemInfo.UnPackSize);
            mixerCoderSpec->SetCoderInfo(1, &itemInfo.UnPackSize, &itemInfo.UnPackSize);
            mixerCoderSpec->SetProgressCoderIndex(1);
            
            RINOK(mixerCoder->Code(inStream, outStream,
              NULL, NULL, compressProgress));
          }
        }
        else
        {
          RINOK(copyCoder->Code(inStream, outStream,
              NULL, NULL, compressProgress));
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
        if (itemInfo.UnPackVersion >= 29)
        {
          outStream.Release();
          RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kUnSupportedMethod));
          continue;
        }
        */

        if(!decoder)
        {
          if (itemInfo.UnPackVersion < 29)
          {
            RINOK(decoder.CoCreateInstance(CLSID_CCompressRar20Decoder));
          }
          else
          {
            RINOK(decoder.CoCreateInstance(CLSID_CCompressRar29Decoder));
          }
        }

        CComPtr<ICompressSetDecoderProperties> compressSetDecoderProperties;
        RINOK(decoder.QueryInterface(&compressSetDecoderProperties));
        
        BYTE isSolid = itemInfo.IsSolid() ? 1: 0;
        CComObjectNoLock<CSequentialInStreamImp> *inStreamSpec = new 
          CComObjectNoLock<CSequentialInStreamImp>;
        CComPtr<ISequentialInStream> inStreamProperties(inStreamSpec);
        inStreamSpec->Init(&isSolid, 1);
        RINOK(compressSetDecoderProperties->SetDecoderProperties(inStreamProperties));
          
        HRESULT result;
        if (itemInfo.IsEncrypted())
        {
          if (!mixerCoder || mixerCoderStoreMethod)
          {
            mixerCoder.Release();
            mixerCoderSpec = new CComObjectNoLock<CCoderMixer>;
            mixerCoder = mixerCoderSpec;
            if (itemInfo.UnPackVersion >= 29)
              mixerCoderSpec->AddCoder(rar29CryptoDecoder);
            else
              mixerCoderSpec->AddCoder(rar20CryptoDecoder);
            mixerCoderSpec->AddCoder(decoder);
            mixerCoderSpec->FinishAddingCoders();
            mixerCoderStoreMethod = false;
          }
          mixerCoderSpec->ReInit();
          mixerCoderSpec->SetCoderInfo(1, &itemInfo.PackSize, 
              &itemInfo.UnPackSize);
          mixerCoderSpec->SetProgressCoderIndex(1);
          result = mixerCoder->Code(inStream, outStream, 
              NULL, NULL, compressProgress);
        }
        else
        {
          result = decoder->Code(inStream, outStream,
              &itemInfo.PackSize, &itemInfo.UnPackSize, compressProgress);
        }
        if (result == S_FALSE)
        {
          outStream.Release();
          RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kDataError));
          continue;
        }
        if (result != S_OK)
          return result;
        break;
      }
      default:
        outStream.Release();
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kUnSupportedMethod));
        continue;
    }
    bool crcOK = outStreamSpec->GetCRC() == itemInfo.FileCRC;
    outStream.Release();
    if(crcOK)
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK))
    else
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kCRCError))
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CRarHandler::ExtractAllItems(INT32 testMode,
      IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  CRecordVector<UINT32> indices;
  indices.Reserve(_items.Size());
  for(int i = 0; i < _items.Size(); i++)
    indices.Add(i);
  return Extract(&indices.Front(), _items.Size(), testMode, extractCallback);
  COM_TRY_END
}

