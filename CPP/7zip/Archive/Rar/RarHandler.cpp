// RarHandler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/PropVariantUtils.h"
#include "Windows/Time.h"

#include "../../IPassword.h"

#include "../../Common/CreateCoder.h"
#include "../../Common/FilterCoder.h"
#include "../../Common/MethodId.h"
#include "../../Common/ProgressUtils.h"

#include "../../Compress/CopyCoder.h"

#include "../../Crypto/Rar20Crypto.h"
#include "../../Crypto/RarAes.h"

#include "../Common/ItemNameUtils.h"
#include "../Common/OutStreamWithCRC.h"

#include "RarHandler.h"

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

static const CUInt32PCharPair k_Flags[] =
{
  { 0, "Volume" },
  { 1, "Comment" },
  { 2, "Lock" },
  { 3, "Solid" },
  { 4, "NewVolName" }, // pack_comment in old versuons
  { 5, "Authenticity" },
  { 6, "Recovery" },
  { 7, "BlockEncryption" },
  { 8, "FirstVolume" },
  { 9, "EncryptVer" }
};

static const STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsDir, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidATime, VT_FILETIME},
  { NULL, kpidAttrib, VT_UI4},

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

static const STATPROPSTG kArcProps[] =
{
  { NULL, kpidCharacts, VT_BSTR},
  { NULL, kpidSolid, VT_BOOL},
  { NULL, kpidNumBlocks, VT_UI4},
  // { NULL, kpidEncrypted, VT_BOOL},
  { NULL, kpidIsVolume, VT_BOOL},
  { NULL, kpidNumVolumes, VT_UI4},
  { NULL, kpidPhySize, VT_UI8}
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
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidSolid: prop = _archiveInfo.IsSolid(); break;
    case kpidCharacts: FLAGS_TO_PROP(k_Flags, _archiveInfo.Flags, prop); break;
    // case kpidEncrypted: prop = _archiveInfo.IsEncrypted(); break; // it's for encrypted names.
    case kpidIsVolume: prop = _archiveInfo.IsVolume(); break;
    case kpidNumVolumes: prop = (UInt32)_archives.Size(); break;
    case kpidOffset: if (_archiveInfo.StartPosition != 0) prop = _archiveInfo.StartPosition; break;
    // case kpidCommented: prop = _archiveInfo.IsCommented(); break;
    case kpidNumBlocks:
    {
      UInt32 numBlocks = 0;
      for (int i = 0; i < _refItems.Size(); i++)
        if (!IsSolid(i))
          numBlocks++;
      prop = (UInt32)numBlocks;
      break;
    }
    case kpidError: if (!_errorMessage.IsEmpty()) prop = _errorMessage; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
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
    case kpidIsDir: prop = item.IsDir(); break;
    case kpidSize: prop = item.Size; break;
    case kpidPackSize: prop = GetPackSize(index); break;
    case kpidMTime: RarTimeToProp(item.MTime, prop); break;
    case kpidCTime: if (item.CTimeDefined) RarTimeToProp(item.CTime, prop); break;
    case kpidATime: if (item.ATimeDefined) RarTimeToProp(item.ATime, prop); break;
    case kpidAttrib: prop = item.GetWinAttributes(); break;
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
        if (!item.IsDir())
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
      else if (!_newStyle)
      {
        if (ext.CompareNoCase(L"000") == 0 ||
            ext.CompareNoCase(L"001") == 0 ||
            ext.CompareNoCase(L"r00") == 0 ||
            ext.CompareNoCase(L"r01") == 0)
        {
          _afterPart.Empty();
          _first = false;
          _changedPart = ext;
          _unchangedPart = name.Left(dotPos + 1);
          return true;
        }
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

HRESULT CHandler::Open2(IInStream *stream,
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *openCallback)
{
  {
    CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
    CMyComPtr<ICryptoGetTextPassword> getTextPassword;
    CMyComPtr<IArchiveOpenCallback> openArchiveCallbackWrap = openCallback;
    
    CVolumeName seqName;

    UInt64 totalBytes = 0;
    UInt64 curBytes = 0;

    if (openCallback)
    {
      openArchiveCallbackWrap.QueryInterface(IID_IArchiveOpenVolumeCallback, &openVolumeCallback);
      openArchiveCallbackWrap.QueryInterface(IID_ICryptoGetTextPassword, &getTextPassword);
    }

    for (;;)
    {
      CMyComPtr<IInStream> inStream;
      if (!_archives.IsEmpty())
      {
        if (!openVolumeCallback)
          break;
        
        if (_archives.Size() == 1)
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

      UInt64 endPos = 0;
      RINOK(stream->Seek(0, STREAM_SEEK_END, &endPos));
      RINOK(stream->Seek(0, STREAM_SEEK_SET, NULL));
      if (openCallback)
      {
        totalBytes += endPos;
        RINOK(openCallback->SetTotal(NULL, &totalBytes));
      }
      
      NArchive::NRar::CInArchive archive;
      RINOK(archive.Open(inStream, maxCheckStartPosition));

      if (_archives.IsEmpty())
        archive.GetArchiveInfo(_archiveInfo);
     
      CItemEx item;
      for (;;)
      {
        if (archive.m_Position > endPos)
        {
          AddErrorMessage("Unexpected end of archive");
          break;
        }
        bool decryptionError;
        AString errorMessageLoc;
        HRESULT result = archive.GetNextItem(item, getTextPassword, decryptionError, errorMessageLoc);
        if (errorMessageLoc)
          AddErrorMessage(errorMessageLoc);
        if (result == S_FALSE)
        {
          if (decryptionError && _items.IsEmpty())
            return S_FALSE;
          break;
        }
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
        if (openCallback && _items.Size() % 100 == 0)
        {
          UInt64 numFiles = _items.Size();
          UInt64 numBytes = curBytes + item.Position;
          RINOK(openCallback->SetCompleted(&numFiles, &numBytes));
        }
      }
      curBytes += endPos;
      _archives.Add(archive);
    }
  }
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *openCallback)
{
  COM_TRY_BEGIN
  Close();
  try
  {
    HRESULT res = Open2(stream, maxCheckStartPosition, openCallback);
    if (res != S_OK)
      Close();
    return res;
  }
  catch(const CInArchiveException &) { Close(); return S_FALSE; }
  catch(...) { Close(); throw; }
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  COM_TRY_BEGIN
  _errorMessage.Empty();
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


STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  CMyComPtr<ICryptoGetTextPassword> getTextPassword;
  UInt64 censoredTotalUnPacked = 0,
        // censoredTotalPacked = 0,
        importantTotalUnPacked = 0;
        // importantTotalPacked = 0;
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = _refItems.Size();
  if (numItems == 0)
    return S_OK;
  int lastIndex = 0;
  CRecordVector<int> importantIndexes;
  CRecordVector<bool> extractStatuses;

  for (UInt32 t = 0; t < numItems; t++)
  {
    int index = allFilesMode ? t : indices[t];
    const CRefItem &refItem = _refItems[index];
    const CItemEx &item = _items[refItem.ItemIndex];
    censoredTotalUnPacked += item.Size;
    // censoredTotalPacked += item.PackSize;
    int j;
    for (j = lastIndex; j <= index; j++)
      // if (!_items[_refItems[j].ItemIndex].IsSolid())
      if (!IsSolid(j))
        lastIndex = j;
    for (j = lastIndex; j <= index; j++)
    {
      const CRefItem &refItem = _refItems[j];
      const CItemEx &item = _items[refItem.ItemIndex];

      // const CItemEx &item = _items[j];

      importantTotalUnPacked += item.Size;
      // importantTotalPacked += item.PackSize;
      importantIndexes.Add(j);
      extractStatuses.Add(j == index);
    }
    lastIndex = index + 1;
  }

  RINOK(extractCallback->SetTotal(importantTotalUnPacked));
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
  for (int i = 0; i < importantIndexes.Size(); i++,
      currentImportantTotalUnPacked += currentUnPackSize,
      currentImportantTotalPacked += currentPackSize)
  {
    lps->InSize = currentImportantTotalPacked;
    lps->OutSize = currentImportantTotalUnPacked;
    RINOK(lps->SetCur());
    CMyComPtr<ISequentialOutStream> realOutStream;

    Int32 askMode;
    if (extractStatuses[i])
      askMode = testMode ?
          NExtract::NAskMode::kTest :
          NExtract::NAskMode::kExtract;
    else
      askMode = NExtract::NAskMode::kSkip;

    UInt32 index = importantIndexes[i];

    const CRefItem &refItem = _refItems[index];
    const CItemEx &item = _items[refItem.ItemIndex];

    currentUnPackSize = item.Size;

    currentPackSize = GetPackSize(index);

    if (item.IgnoreItem())
      continue;

    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    if (!IsSolid(index))
      solidStart = true;
    if (item.IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      continue;
    }

    bool mustBeProcessedAnywhere = false;
    if (i < importantIndexes.Size() - 1)
    {
      // const CRefItem &nextRefItem = _refItems[importantIndexes[i + 1]];
      // const CItemEx &nextItemInfo = _items[nextRefItem.ItemIndex];
      // mustBeProcessedAnywhere = nextItemInfo.IsSolid();
      mustBeProcessedAnywhere = IsSolid(importantIndexes[i + 1]);
    }
    
    if (!mustBeProcessedAnywhere && !testMode && !realOutStream)
      continue;
    
    if (!realOutStream && !testMode)
      askMode = NExtract::NAskMode::kSkip;

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
        RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kUnSupportedMethod));
        continue;
      }
      RINOK(filterStreamSpec->Filter.QueryInterface(IID_ICryptoSetPassword,
          &cryptoSetPassword));

      if (!getTextPassword)
        extractCallback->QueryInterface(IID_ICryptoGetTextPassword, (void **)&getTextPassword);
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
          RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kUnSupportedMethod));
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
            RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kUnSupportedMethod));
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
        RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kUnSupportedMethod));
        continue;
    }
    HRESULT result = commonCoder->Code(inStream, outStream, &packSize, &item.Size, progress);
    if (item.IsEncrypted())
      filterStreamSpec->ReleaseInStream();
    if (result == S_FALSE)
    {
      outStream.Release();
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kDataError));
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
      RINOK(extractCallback->SetOperationResult(crcOK ?
          NExtract::NOperationResult::kOK:
          NExtract::NOperationResult::kCRCError));
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
      RINOK(extractCallback->SetOperationResult(crcOK ?
          NExtract::NOperationResult::kOK:
          NExtract::NOperationResult::kCRCError));
    }
    */
  }
  return S_OK;
  COM_TRY_END
}

IMPL_ISetCompressCodecsInfo

}}
