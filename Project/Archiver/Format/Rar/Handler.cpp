// Rar/Handler.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"

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

#include "../../../Compress/Interface/CompressInterface.h"
#include "../../../Crypto/Cipher/Common/CipherInterface.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NRar {

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
  { NULL, kpidCreationTime, VT_FILETIME},
  { NULL, kpidLastAccessTime, VT_FILETIME},
  { NULL, kpidAttributes, VT_UI4},


  { NULL, kpidEncrypted, VT_BOOL},
  { NULL, kpidSolid, VT_BOOL},
  { NULL, kpidCommented, VT_BOOL},
  { NULL, kpidSplitBefore, VT_BOOL},
  { NULL, kpidSplitAfter, VT_BOOL},
  { NULL, kpidCRC, VT_UI4},
  { NULL, kpidHostOS, VT_BSTR},
  { NULL, kpidMethod, VT_BSTR},
  // { NULL, kpidDictionarySize, VT_UI4},
  { L"UnPack Version", kpidUnPackVersion, VT_UI1}
};

UINT64 CHandler::GetPackSize(int refIndex) const
{
  const CRefItem &refItem = _refItems[refIndex];
  UINT64 totalPackSize = 0;
  for (int i = 0; i < refItem.NumItems; i++)
    totalPackSize += _items[refItem.ItemIndex + i].PackSize;
  return totalPackSize;
}

STDMETHODIMP CHandler::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  COM_TRY_BEGIN
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
  COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UINT32 *numItems)
{
  *numItems = _refItems.Size();
  return S_OK;
}

static bool RarTimeToFileTime(const CRarTime &rarTime, FILETIME &result)
{
  if (!DosTimeToFileTime(rarTime.DosTime, result))
    return false;
  UINT64 &value = *(UINT64 *)&result;
  value += (int)rarTime.LowSecond * 10000000;
  UINT64 subTime = ((UINT64)rarTime.SubTime[2] << 16) + 
    ((UINT64)rarTime.SubTime[1] << 8) +
    ((UINT64)rarTime.SubTime[0]);
  // value += (subTime * 10000000) >> 24;
  value += subTime;
  return true;
}

STDMETHODIMP CHandler::GetProperty(UINT32 index, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
  const CRefItem &refItem = _refItems[index];
  const CItemInfoEx &item = _items[refItem.ItemIndex];
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
    {
      propVariant = GetPackSize(index);
      break;
    }
    case kpidLastWriteTime:
    {
      FILETIME localFileTime, utcFileTime;
      if (RarTimeToFileTime(item.LastWriteTime, localFileTime))
      {
        if (!LocalFileTimeToFileTime(&localFileTime, &utcFileTime))
          utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
      }
      else
        utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
      propVariant = utcFileTime;
      break;
    }
    case kpidCreationTime:
    {
      if (item.IsCreationTimeDefined)
      {
        FILETIME localFileTime, utcFileTime;
        if (RarTimeToFileTime(item.CreationTime, localFileTime))
        {
          if (!LocalFileTimeToFileTime(&localFileTime, &utcFileTime))
            utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
        }
        else
          utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
        propVariant = utcFileTime;
      }
      break;
    }
    case kpidLastAccessTime:
    {
      if (item.IsLastAccessTimeDefined)
      {
        FILETIME localFileTime, utcFileTime;
        if (RarTimeToFileTime(item.LastAccessTime, localFileTime))
        {
          if (!LocalFileTimeToFileTime(&localFileTime, &utcFileTime))
            utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
        }
        else
          utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
        propVariant = utcFileTime;
      }
      break;
    }
    case kpidAttributes:
      propVariant = item.GetWinAttributes();
      break;
    case kpidEncrypted:
      propVariant = item.IsEncrypted();
      break;
    case kpidSolid:
      propVariant = IsSolid(index);
      break;
    case kpidCommented:
      propVariant = item.IsCommented();
      break;
    case kpidSplitBefore:
      propVariant = item.IsSplitBefore();
      break;
    case kpidSplitAfter:
      propVariant = _items[refItem.ItemIndex + refItem.NumItems - 1].IsSplitAfter();
      break;
    /*
    case kpidDictionarySize:
      if (!item.IsDirectory())
        propVariant = UINT32(0x10000 << item.GetDictSize());
      break;
    */
    case kpidCRC:
    {
      const CItemInfoEx &lastItem = 
          _items[refItem.ItemIndex + refItem.NumItems - 1];
      if (lastItem.IsSplitAfter())
        propVariant = item.FileCRC;
      else
        propVariant = lastItem.FileCRC;
      break;
    }
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

class CVolumeName
{
  bool _first;
  bool _newStyle;
  UString _unchangedPart;
  UString _changedPart;    
  UString _afterPart;    
public:
  CVolumeName(): _newStyle(true) {};

  bool InitName(const UString &name, bool newStyle)
  {
    _first = true;
    _newStyle = newStyle;
    int dotPos = name.ReverseFind('.');
    UString basePart = name;
    if (dotPos >= 0)
    {
      UString ext = name.Mid(dotPos + 1);
      if (ext.CompareNoCase(L"RAR")==0 || 
        ext.CompareNoCase(L"EXE") == 0)
      {
        _afterPart = L".rar";
        basePart = name.Left(dotPos);
      }
    }

    if (!_newStyle)
    {
      _afterPart.Empty();
      _unchangedPart = basePart + UString(L".");
      _changedPart = L"r00";
      return true;;
    }

    int numLetters = 1;
    bool splitStyle = false;
    if (basePart.Right(numLetters) == L"1")
    {
      while (numLetters < basePart.Length())
      {
        if (basePart[basePart.Length() - numLetters - 1] != '0')
          break;
        numLetters++;
      }
    }
    else 
      return false;
    _unchangedPart = basePart.Left(basePart.Length() - numLetters);
    _changedPart = basePart.Right(numLetters);
    return true;
  }

  UString GetNextName()
  {
    UString newName; 
    if (_newStyle || !_first)
    {
      int i;
      int numLetters = _changedPart.Length();
      for (i = numLetters - 1; i >= 0; i--)
      {
        wchar_t c = _changedPart[i];
        if (c == L'9')
        {
          c = L'0';
          newName = c + newName;
          if (i == 0)
            newName = UString(L'1') + newName;
          continue;
        }
        c++;
        newName = UString(c) + newName;
        i--;
        for (; i >= 0; i--)
          newName = _changedPart[i] + newName;
        break;
      }
      _changedPart = newName;
    }
    _first = false;
    return _unchangedPart + _changedPart + _afterPart;
  }
};

STDMETHODIMP CHandler::Open(IInStream *stream, 
    const UINT64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  Close();
  try
  {
    CComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
    CComPtr<IArchiveOpenCallback> openArchiveCallbackWrap = openArchiveCallback;
    openArchiveCallbackWrap.QueryInterface(&openVolumeCallback);
    
    CVolumeName seqName;

    if (openArchiveCallback != NULL)
    {
      RINOK(openArchiveCallback->SetTotal(NULL, NULL));
      UINT64 numFiles = _items.Size();
      RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
    }

    while(true)
    {
      CComPtr<IInStream> inStream;
      if (!_archives.IsEmpty())
      {
        if (!openVolumeCallback)
          break;
        
        if(_archives.Size() == 1)
        {
          if (!_archiveInfo.IsVolume())
            break;
          UString baseName;
          {
            NCOM::CPropVariant propVariant;
            RINOK(openVolumeCallback->GetProperty(kpidName, &propVariant));
            if (propVariant.vt != VT_BSTR)
              break;
            baseName = propVariant.bstrVal;
          }
          seqName.InitName(baseName, _archiveInfo.HaveNewVolumeName());
        }

        UString fullName = seqName.GetNextName();
        HRESULT result = openVolumeCallback->GetStream(fullName, &inStream);
        if (result == S_FALSE)
          break;
        if (result != S_OK)
          return result;
        if (!stream)
          break;
      }
      else
        inStream = stream;
      
      NArchive::NRar::CInArchive archive;
      if(!archive.Open(inStream, maxCheckStartPosition))
        return S_FALSE;

      if (_archives.IsEmpty())
      {
        archive.GetArchiveInfo(_archiveInfo);
      }

      
      CItemInfoEx itemInfo;
      while(archive.GetNextItem(itemInfo))
      {
        if (itemInfo.IgnoreItem())
          continue;

        bool needAdd = true;
        if (itemInfo.IsSplitBefore())
        {
          if (!_refItems.IsEmpty())
          {
            CRefItem &refItem = _refItems.Back();
            refItem.NumItems++;
            needAdd = false;
          }
        }
        if (needAdd)
        {
          CRefItem refItem;
          refItem.ItemIndex = _items.Size();
          refItem.NumItems = 1;
          refItem.VolumeIndex = _archives.Size();
          _refItems.Add(refItem);
        }
        _items.Add(itemInfo);
        if (openArchiveCallback != NULL)
        {
          UINT64 numFiles = _items.Size();
          RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
        }
      }
      _archives.Add(archive);
    }
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
  COM_TRY_BEGIN
  _refItems.Clear();
  _items.Clear();
  _archives.Clear();
  return S_OK;
  COM_TRY_END
}


STDMETHODIMP CHandler::Extract(const UINT32* indices, UINT32 numItems,
    INT32 _aTestMode, IArchiveExtractCallback *_anExtractCallback)
{
  COM_TRY_BEGIN
  CComPtr<ICryptoGetTextPassword> getTextPassword;
  bool testMode = (_aTestMode != 0);
  CComPtr<IArchiveExtractCallback> extractCallback = _anExtractCallback;
  UINT64 censoredTotalUnPacked = 0, 
        // censoredTotalPacked = 0,
        importantTotalUnPacked = 0; 
        // importantTotalPacked = 0;
  if(numItems == 0)
    return S_OK;
  int lastIndex = 0;
  CRecordVector<int> importantIndexes;
  CRecordVector<bool> extractStatuses;

  for(UINT32 t = 0; t < numItems; t++)
  {
    int index = indices[t];
    const CRefItem &refItem = _refItems[index];
    const CItemInfoEx &itemInfo = _items[refItem.ItemIndex];
    censoredTotalUnPacked += itemInfo.UnPackSize;
    // censoredTotalPacked += itemInfo.PackSize;
    for(int j = lastIndex; j <= index; j++)
      // if(!_items[_refItems[j].ItemIndex].IsSolid())
      if(!IsSolid(j))
        lastIndex = j;
    for(j = lastIndex; j <= index; j++)
    {
      const CRefItem &refItem = _refItems[j];
      const CItemInfoEx &itemInfo = _items[refItem.ItemIndex];

      // const CItemInfoEx &itemInfo = _items[j];

      importantTotalUnPacked += itemInfo.UnPackSize;
      // importantTotalPacked += itemInfo.PackSize;
      importantIndexes.Add(j);
      extractStatuses.Add(j == index);
    }
    lastIndex = index + 1;
  }

  extractCallback->SetTotal(importantTotalUnPacked);
  UINT64 currentImportantTotalUnPacked = 0;
  UINT64 currentImportantTotalPacked = 0;
  UINT64 currentUnPackSize, currentPackSize;

  CComPtr<ICompressCoder> decoder15;
  CComPtr<ICompressCoder> decoder20;
  CComPtr<ICompressCoder> decoder29;

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

  CComObjectNoLock<CFolderInStream> *folderInStreamSpec = NULL;
  CComPtr<ISequentialInStream> folderInStream;

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

    const CRefItem &refItem = _refItems[index];
    const CItemInfoEx &itemInfo = _items[refItem.ItemIndex];

    currentUnPackSize = itemInfo.UnPackSize;

    currentPackSize = GetPackSize(index);

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
      // const CRefItem &nextRefItem = _refItems[importantIndexes[i + 1]];
      // const CItemInfoEx &nextItemInfo = _items[nextRefItem.ItemIndex];
      // mustBeProcessedAnywhere = nextItemInfo.IsSolid();
      mustBeProcessedAnywhere = IsSolid(importantIndexes[i + 1]);
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
    
    UINT64 packedPos = currentImportantTotalPacked;
    UINT64 unpackedPos = currentImportantTotalUnPacked;

    /*
    for (int partIndex = 0; partIndex < 1; partIndex++)
    {
    CComPtr<ISequentialInStream> inStream;

    // itemInfo redefinition
    const CItemInfoEx &itemInfo = _items[refItem.ItemIndex + partIndex];

    NArchive::NRar::CInArchive &archive = _archives[refItem.VolumeIndex + partIndex];

    inStream.Attach(archive.CreateLimitedStream(itemInfo.GetDataPosition(),
      itemInfo.PackSize));
    */
    if (!folderInStream)
    {
      folderInStreamSpec = new CComObjectNoLock<CFolderInStream>;
      folderInStream = folderInStreamSpec;
    }

    folderInStreamSpec->Init(&_archives, &_items, refItem);

    
    CComObjectNoLock<CLocalProgress> *localProgressSpec = new  CComObjectNoLock<CLocalProgress>;
    CComPtr<ICompressProgressInfo> progress = localProgressSpec;
    localProgressSpec->Init(extractCallback, false);
    
    CComObjectNoLock<CLocalCompressProgressInfo> *localCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
    CComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
    localCompressProgressSpec->Init(progress, 
      &packedPos,
      &unpackedPos);

    UINT64 packSize = currentPackSize;

    // packedPos += itemInfo.PackSize;
    // unpackedPos += 0;
    
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
            mixerCoderSpec->SetCoderInfo(0, &packSize, &itemInfo.UnPackSize);
            mixerCoderSpec->SetCoderInfo(1, &itemInfo.UnPackSize, &itemInfo.UnPackSize);
            mixerCoderSpec->SetProgressCoderIndex(1);
            
            RINOK(mixerCoder->Code(folderInStream, outStream,
              NULL, NULL, compressProgress));
          }
        }
        else
        {
          RINOK(copyCoder->Code(folderInStream, outStream,
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
        CComPtr<ICompressCoder> decoder;
        if (itemInfo.UnPackVersion < 20)
        {
          if(!decoder15)
          {
            RINOK(decoder15.CoCreateInstance(CLSID_CCompressRar15Decoder));
          }
          decoder = decoder15;
        }
        else if (itemInfo.UnPackVersion < 29)
        {
          if(!decoder20)
          {
            RINOK(decoder20.CoCreateInstance(CLSID_CCompressRar20Decoder));
          }
          decoder = decoder20;
        }
        else
        {
          if(!decoder29)
          {
            RINOK(decoder29.CoCreateInstance(CLSID_CCompressRar29Decoder));
          }
          decoder = decoder29;
        }

        CComPtr<ICompressSetDecoderProperties> compressSetDecoderProperties;
        RINOK(decoder.QueryInterface(&compressSetDecoderProperties));
        
        BYTE isSolid = (
          // itemInfo.IsSolid() 
          IsSolid(index)
          || 
          itemInfo.IsSplitBefore())
            ? 1: 0;

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
          mixerCoderSpec->SetCoderInfo(1, &packSize, 
              &itemInfo.UnPackSize);
          mixerCoderSpec->SetProgressCoderIndex(1);
          result = mixerCoder->Code(folderInStream, outStream, 
              NULL, NULL, compressProgress);
        }
        else
        {
          result = decoder->Code(folderInStream, outStream,
              &packSize, &itemInfo.UnPackSize, compressProgress);
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

    /*
    if (refItem.NumItems == 1 && 
        !itemInfo.IsSplitBefore() && !itemInfo.IsSplitAfter())
    */
    {
      const CItemInfoEx &lastItem = _items[refItem.ItemIndex + refItem.NumItems - 1];
      bool crcOK = outStreamSpec->GetCRC() == lastItem.FileCRC;
      outStream.Release();
      RINOK(extractCallback->SetOperationResult(crcOK ? NArchive::NExtract::NOperationResult::kOK:
          NArchive::NExtract::NOperationResult::kCRCError));
    }
    /*
    else
    {
      bool crcOK = true;
      for (int partIndex = 0; partIndex < refItem.NumItems; partIndex++)
      {
        const CItemInfoEx &itemInfo = _items[refItem.ItemIndex + partIndex];
        if (itemInfo.FileCRC != folderInStreamSpec->CRCs[partIndex])
        {
          crcOK = false;
          break;
        }
      }
      RINOK(extractCallback->SetOperationResult(crcOK ? NArchive::NExtract::NOperationResult::kOK:
          NArchive::NExtract::NOperationResult::kCRCError));
    }
    */
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::ExtractAllItems(INT32 testMode,
      IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  CRecordVector<UINT32> indices;
  indices.Reserve(_refItems.Size());
  for(int i = 0; i < _refItems.Size(); i++)
    indices.Add(i);
  return Extract(&indices.Front(), _refItems.Size(), testMode, extractCallback);
  COM_TRY_END
}

}}
