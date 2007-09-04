// RarHandler.cpp

#include "StdAfx.h"

#include "RarHandler.h"

#include "Common/StringConvert.h"
#include "Common/ComTry.h"
#include "Common/IntToString.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../../IPassword.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/CreateCoder.h"
#include "../../Common/MethodId.h"
#include "../../Common/FilterCoder.h"
#include "../../Compress/Copy/CopyCoder.h"
#include "../../Crypto/Rar20/Rar20Cipher.h"
#include "../../Crypto/RarAES/RarAES.h"
#include "../Common/OutStreamWithCRC.h"
#include "../Common/ItemNameUtils.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NRar {

static const wchar_t *kHostOS[] =
{
  L"MS DOS",
  L"OS/2",
  L"Win32",
  L"Unix",
  L"Mac OS",
  L"BeOS"
};

static const int kNumHostOSes = sizeof(kHostOS) / sizeof(kHostOS[0]);

static const wchar_t *kUnknownOS = L"Unknown";

STATPROPSTG kProps[] = 
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
  { NULL, kpidUnpackVer, VT_UI1}
};

STATPROPSTG kArcProps[] = 
{
  { NULL, kpidSolid, VT_BOOL},
  { NULL, kpidNumBlocks, VT_UI4},
  { NULL, kpidEncrypted, VT_BOOL},
  { NULL, kpidIsVolume, VT_BOOL},
  { NULL, kpidNumVolumes, VT_UI4},
  // { NULL, kpidCommented, VT_BOOL}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

UInt64 CHandler::GetPackSize(int refIndex) const
{
  const CRefItem &refItem = _refItems[refIndex];
  UInt64 totalPackSize = 0;
  for (int i = 0; i < refItem.NumItems; i++)
    totalPackSize += _items[refItem.ItemIndex + i].PackSize;
  return totalPackSize;
}

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  // COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidSolid: prop = _archiveInfo.IsSolid(); break;
    case kpidEncrypted: prop = _archiveInfo.IsEncrypted(); break;
    case kpidIsVolume: prop = _archiveInfo.IsVolume(); break;
    case kpidNumBlocks:
    {
      UInt32 numBlocks = 0;
      for (int i = 0; i < _refItems.Size(); i++)
        if (!IsSolid(i))
          numBlocks++;
      prop = (UInt32)numBlocks; 
      break;
    }
    case kpidNumVolumes: prop = (UInt32)_archives.Size();

    // case kpidCommented: prop = _archiveInfo.IsCommented(); break;
  }
  prop.Detach(value);
  return S_OK;
  // COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _refItems.Size();
  return S_OK;
}

static bool RarTimeToFileTime(const CRarTime &rarTime, FILETIME &result)
{
  if (!DosTimeToFileTime(rarTime.DosTime, result))
    return false;
  UInt64 value =  (((UInt64)result.dwHighDateTime) << 32) + result.dwLowDateTime;
  value += (UInt64)rarTime.LowSecond * 10000000;
  value += ((UInt64)rarTime.SubTime[2] << 16) + 
    ((UInt64)rarTime.SubTime[1] << 8) +
    ((UInt64)rarTime.SubTime[0]);
  result.dwLowDateTime = (DWORD)value;
  result.dwHighDateTime = DWORD(value >> 32);
  return true;
}

static void RarTimeToProp(const CRarTime &rarTime, NWindows::NCOM::CPropVariant &prop)
{
  FILETIME localFileTime, utcFileTime;
  if (RarTimeToFileTime(rarTime, localFileTime))
  {
    if (!LocalFileTimeToFileTime(&localFileTime, &utcFileTime))
      utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
  }
  else
    utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
  prop = utcFileTime;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CRefItem &refItem = _refItems[index];
  const CItemEx &item = _items[refItem.ItemIndex];
  switch(propID)
  {
    case kpidPath:
    {
      UString u;
      if (item.HasUnicodeName() && !item.UnicodeName.IsEmpty())
        u = item.UnicodeName;
      else
        u = MultiByteToUnicodeString(item.Name, CP_OEMCP);
      prop = (const wchar_t *)NItemName::WinNameToOSName(u);
      break;
    }
    case kpidIsFolder: prop = item.IsDirectory(); break;
    case kpidSize: prop = item.UnPackSize; break;
    case kpidPackedSize: prop = GetPackSize(index); break;
    case kpidLastWriteTime: RarTimeToProp(item.LastWriteTime, prop);
    case kpidCreationTime: if (item.IsCreationTimeDefined) RarTimeToProp(item.CreationTime, prop); break;
    case kpidLastAccessTime: if (item.IsLastAccessTimeDefined) RarTimeToProp(item.LastAccessTime, prop); break;
    case kpidAttributes: prop = item.GetWinAttributes(); break;
    case kpidEncrypted: prop = item.IsEncrypted(); break;
    case kpidSolid: prop = IsSolid(index); break;
    case kpidCommented: prop = item.IsCommented(); break;
    case kpidSplitBefore: prop = item.IsSplitBefore(); break;
    case kpidSplitAfter: prop = _items[refItem.ItemIndex + refItem.NumItems - 1].IsSplitAfter(); break;
    case kpidCRC:
    {
      const CItemEx &lastItem = _items[refItem.ItemIndex + refItem.NumItems - 1];
      prop = ((lastItem.IsSplitAfter()) ? item.FileCRC : lastItem.FileCRC);
      break;
    }
    case kpidUnpackVer: prop = item.UnPackVersion; break;
    case kpidMethod:
    {
      UString method;
      if (item.Method >= Byte('0') && item.Method <= Byte('5'))
      {
        method = L"m";
        wchar_t temp[32];
        ConvertUInt64ToString(item.Method - Byte('0'), temp);
        method += temp;
        if (!item.IsDirectory())
        {
          method += L":";
          ConvertUInt64ToString(16 + item.GetDictSize(), temp);
          method += temp;
        }
      }
      else
      {
        wchar_t temp[32];
        ConvertUInt64ToString(item.Method, temp);
        method += temp;
      }
      prop = method;
      break;
    }
    case kpidHostOS: prop = (item.HostOS < kNumHostOSes) ? (kHostOS[item.HostOS]) : kUnknownOS; break;
  }
  prop.Detach(value);
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
      if (ext.CompareNoCase(L"rar") == 0)
      {
        _afterPart = name.Mid(dotPos);
        basePart = name.Left(dotPos);
      }
      else if (ext.CompareNoCase(L"exe") == 0)
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
      return true;
    }

    int numLetters = 1;
    if (basePart.Right(numLetters) == L"1" || basePart.Right(numLetters) == L"0")
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
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  Close();
  try
  {
    CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
    CMyComPtr<ICryptoGetTextPassword> getTextPassword;
    CMyComPtr<IArchiveOpenCallback> openArchiveCallbackWrap = openArchiveCallback;
    
    CVolumeName seqName;

    if (openArchiveCallback != NULL)
    {
      openArchiveCallbackWrap.QueryInterface(IID_IArchiveOpenVolumeCallback, &openVolumeCallback);
      RINOK(openArchiveCallback->SetTotal(NULL, NULL));
      UInt64 numFiles = _items.Size();
      RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
      openArchiveCallbackWrap.QueryInterface(IID_ICryptoGetTextPassword, &getTextPassword);
    }

    for (;;)
    {
      CMyComPtr<IInStream> inStream;
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
            NCOM::CPropVariant prop;
            RINOK(openVolumeCallback->GetProperty(kpidName, &prop));
            if (prop.vt != VT_BSTR)
              break;
            baseName = prop.bstrVal;
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
        archive.GetArchiveInfo(_archiveInfo);
     
      CItemEx item;
      for (;;)
      {
        HRESULT result = archive.GetNextItem(item, getTextPassword);
        if (result == S_FALSE)
          break;
        RINOK(result);
        if (item.IgnoreItem())
          continue;

        bool needAdd = true;
        if (item.IsSplitBefore())
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
        _items.Add(item);
        if (openArchiveCallback != NULL)
        {
          UInt64 numFiles = _items.Size();
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

struct CMethodItem
{
  Byte RarUnPackVersion;
  CMyComPtr<ICompressCoder> Coder;
};


STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 _aTestMode, IArchiveExtractCallback *_anExtractCallback)
{
  COM_TRY_BEGIN
  CMyComPtr<ICryptoGetTextPassword> getTextPassword;
  bool testMode = (_aTestMode != 0);
  CMyComPtr<IArchiveExtractCallback> extractCallback = _anExtractCallback;
  UInt64 censoredTotalUnPacked = 0, 
        // censoredTotalPacked = 0,
        importantTotalUnPacked = 0; 
        // importantTotalPacked = 0;
  bool allFilesMode = (numItems == UInt32(-1));
  if (allFilesMode)
    numItems = _refItems.Size();
  if(numItems == 0)
    return S_OK;
  int lastIndex = 0;
  CRecordVector<int> importantIndexes;
  CRecordVector<bool> extractStatuses;

  for(UInt32 t = 0; t < numItems; t++)
  {
    int index = allFilesMode ? t : indices[t];
    const CRefItem &refItem = _refItems[index];
    const CItemEx &item = _items[refItem.ItemIndex];
    censoredTotalUnPacked += item.UnPackSize;
    // censoredTotalPacked += item.PackSize;
    int j;
    for(j = lastIndex; j <= index; j++)
      // if(!_items[_refItems[j].ItemIndex].IsSolid())
      if(!IsSolid(j))
        lastIndex = j;
    for(j = lastIndex; j <= index; j++)
    {
      const CRefItem &refItem = _refItems[j];
      const CItemEx &item = _items[refItem.ItemIndex];

      // const CItemEx &item = _items[j];

      importantTotalUnPacked += item.UnPackSize;
      // importantTotalPacked += item.PackSize;
      importantIndexes.Add(j);
      extractStatuses.Add(j == index);
    }
    lastIndex = index + 1;
  }

  extractCallback->SetTotal(importantTotalUnPacked);
  UInt64 currentImportantTotalUnPacked = 0;
  UInt64 currentImportantTotalPacked = 0;
  UInt64 currentUnPackSize, currentPackSize;

  CObjectVector<CMethodItem> methodItems;

  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder;
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CFilterCoder *filterStreamSpec = new CFilterCoder;
  CMyComPtr<ISequentialInStream> filterStream = filterStreamSpec;

  NCrypto::NRar20::CDecoder *rar20CryptoDecoderSpec = NULL;
  CMyComPtr<ICompressFilter> rar20CryptoDecoder;
  NCrypto::NRar29::CDecoder *rar29CryptoDecoderSpec = NULL;
  CMyComPtr<ICompressFilter> rar29CryptoDecoder;

  CFolderInStream *folderInStreamSpec = NULL;
  CMyComPtr<ISequentialInStream> folderInStream;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  bool solidStart = true;
  for(int i = 0; i < importantIndexes.Size(); i++, 
      currentImportantTotalUnPacked += currentUnPackSize,
      currentImportantTotalPacked += currentPackSize)
  {
    lps->InSize = currentImportantTotalPacked;
    lps->OutSize = currentImportantTotalUnPacked;
    RINOK(lps->SetCur());
    CMyComPtr<ISequentialOutStream> realOutStream;

    Int32 askMode;
    if(extractStatuses[i])
      askMode = testMode ? 
          NArchive::NExtract::NAskMode::kTest :
          NArchive::NExtract::NAskMode::kExtract;
    else
      askMode = NArchive::NExtract::NAskMode::kSkip;

    UInt32 index = importantIndexes[i];

    const CRefItem &refItem = _refItems[index];
    const CItemEx &item = _items[refItem.ItemIndex];

    currentUnPackSize = item.UnPackSize;

    currentPackSize = GetPackSize(index);

    if(item.IgnoreItem())
      continue;

    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    if (!IsSolid(index))
      solidStart = true;
    if(item.IsDirectory())
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
      continue;
    }

    bool mustBeProcessedAnywhere = false;
    if(i < importantIndexes.Size() - 1)
    {
      // const CRefItem &nextRefItem = _refItems[importantIndexes[i + 1]];
      // const CItemEx &nextItemInfo = _items[nextRefItem.ItemIndex];
      // mustBeProcessedAnywhere = nextItemInfo.IsSolid();
      mustBeProcessedAnywhere = IsSolid(importantIndexes[i + 1]);
    }
    
    if (!mustBeProcessedAnywhere && !testMode && !realOutStream)
      continue;
    
    if (!realOutStream && !testMode)
      askMode = NArchive::NExtract::NAskMode::kSkip;

    RINOK(extractCallback->PrepareOperation(askMode));

    COutStreamWithCRC *outStreamSpec = new COutStreamWithCRC;
    CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
    outStreamSpec->SetStream(realOutStream);
    outStreamSpec->Init();
    realOutStream.Release();
    
    /*
    for (int partIndex = 0; partIndex < 1; partIndex++)
    {
    CMyComPtr<ISequentialInStream> inStream;

    // item redefinition
    const CItemEx &item = _items[refItem.ItemIndex + partIndex];

    NArchive::NRar::CInArchive &archive = _archives[refItem.VolumeIndex + partIndex];

    inStream.Attach(archive.CreateLimitedStream(item.GetDataPosition(),
      item.PackSize));
    */
    if (!folderInStream)
    {
      folderInStreamSpec = new CFolderInStream;
      folderInStream = folderInStreamSpec;
    }

    folderInStreamSpec->Init(&_archives, &_items, refItem);

    UInt64 packSize = currentPackSize;

    // packedPos += item.PackSize;
    // unpackedPos += 0;
    
    CMyComPtr<ISequentialInStream> inStream;
    if (item.IsEncrypted())
    {
      CMyComPtr<ICryptoSetPassword> cryptoSetPassword;
      if (item.UnPackVersion >= 29)
      {
        if (!rar29CryptoDecoder)
        {
          rar29CryptoDecoderSpec = new NCrypto::NRar29::CDecoder;
          rar29CryptoDecoder = rar29CryptoDecoderSpec;
          // RINOK(rar29CryptoDecoder.CoCreateInstance(CLSID_CCryptoRar29Decoder));
        }
        rar29CryptoDecoderSpec->SetRar350Mode(item.UnPackVersion < 36);
        CMyComPtr<ICompressSetDecoderProperties2> cryptoProperties;
        RINOK(rar29CryptoDecoder.QueryInterface(IID_ICompressSetDecoderProperties2, 
            &cryptoProperties));
        RINOK(cryptoProperties->SetDecoderProperties2(item.Salt, item.HasSalt() ? sizeof(item.Salt) : 0));
        filterStreamSpec->Filter = rar29CryptoDecoder;
      }
      else if (item.UnPackVersion >= 20)
      {
        if (!rar20CryptoDecoder)
        {
          rar20CryptoDecoderSpec = new NCrypto::NRar20::CDecoder;
          rar20CryptoDecoder = rar20CryptoDecoderSpec;
          // RINOK(rar20CryptoDecoder.CoCreateInstance(CLSID_CCryptoRar20Decoder));
        }
        filterStreamSpec->Filter = rar20CryptoDecoder;
      }
      else
      {
        outStream.Release();
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kUnSupportedMethod));
        continue;
      }
      RINOK(filterStreamSpec->Filter.QueryInterface(IID_ICryptoSetPassword, 
          &cryptoSetPassword));

      if (!getTextPassword)
        extractCallback.QueryInterface(IID_ICryptoGetTextPassword, 
          &getTextPassword);
      if (getTextPassword)
      {
        CMyComBSTR password;
        RINOK(getTextPassword->CryptoGetTextPassword(&password));
        if (item.UnPackVersion >= 29)
        {
          CByteBuffer buffer;
          UString unicodePassword(password);
          const UInt32 sizeInBytes = unicodePassword.Length() * 2;
          buffer.SetCapacity(sizeInBytes);
          for (int i = 0; i < unicodePassword.Length(); i++)
          {
            wchar_t c = unicodePassword[i];
            ((Byte *)buffer)[i * 2] = (Byte)c;
            ((Byte *)buffer)[i * 2 + 1] = (Byte)(c >> 8);
          }
          RINOK(cryptoSetPassword->CryptoSetPassword(
            (const Byte *)buffer, sizeInBytes));
        }
        else
        {
          AString oemPassword = UnicodeStringToMultiByte(
            (const wchar_t *)password, CP_OEMCP);
          RINOK(cryptoSetPassword->CryptoSetPassword(
            (const Byte *)(const char *)oemPassword, oemPassword.Length()));
        }
      }
      else
      {
        RINOK(cryptoSetPassword->CryptoSetPassword(0, 0));
      }
      filterStreamSpec->SetInStream(folderInStream);
      inStream = filterStream;
    }
    else
    {
      inStream = folderInStream;
    }
    CMyComPtr<ICompressCoder> commonCoder;
    switch(item.Method)
    {
      case '0':
      {
        commonCoder = copyCoder;
        break;
      }
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      {
        /*
        if (item.UnPackVersion >= 29)
        {
          outStream.Release();
          RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kUnSupportedMethod));
          continue;
        }
        */
        int m;
        for (m = 0; m < methodItems.Size(); m++)
          if (methodItems[m].RarUnPackVersion == item.UnPackVersion)
            break;
        if (m == methodItems.Size())
        {
          CMethodItem mi;
          mi.RarUnPackVersion = item.UnPackVersion;

          mi.Coder.Release();
          if (item.UnPackVersion <= 30)
          {
            UInt32 methodID = 0x040300;
            if (item.UnPackVersion < 20)
              methodID += 1;
            else if (item.UnPackVersion < 29)
              methodID += 2;
            else 
              methodID += 3;
            RINOK(CreateCoder(EXTERNAL_CODECS_VARS methodID, mi.Coder, false));
          }
         
          if (mi.Coder == 0)
          {
            outStream.Release();
            RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kUnSupportedMethod));
            continue;
          }

          m = methodItems.Add(mi);
        }
        CMyComPtr<ICompressCoder> decoder = methodItems[m].Coder;

        CMyComPtr<ICompressSetDecoderProperties2> compressSetDecoderProperties;
        RINOK(decoder.QueryInterface(IID_ICompressSetDecoderProperties2, 
            &compressSetDecoderProperties));
        
        Byte isSolid = (Byte)((IsSolid(index) || item.IsSplitBefore()) ? 1: 0);
        if (solidStart)
        {
          isSolid = false;
          solidStart = false;
        }


        RINOK(compressSetDecoderProperties->SetDecoderProperties2(&isSolid, 1));
          
        commonCoder = decoder;
        break;
      }
      default:
        outStream.Release();
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kUnSupportedMethod));
        continue;
    }
    HRESULT result = commonCoder->Code(inStream, outStream, &packSize, &item.UnPackSize, progress);
    if (item.IsEncrypted())
      filterStreamSpec->ReleaseInStream();
    if (result == S_FALSE)
    {
      outStream.Release();
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kDataError));
      continue;
    }
    if (result != S_OK)
      return result;

    /*
    if (refItem.NumItems == 1 && 
        !item.IsSplitBefore() && !item.IsSplitAfter())
    */
    {
      const CItemEx &lastItem = _items[refItem.ItemIndex + refItem.NumItems - 1];
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
        const CItemEx &item = _items[refItem.ItemIndex + partIndex];
        if (item.FileCRC != folderInStreamSpec->CRCs[partIndex])
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

IMPL_ISetCompressCodecsInfo

}}
