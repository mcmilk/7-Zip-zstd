// ZipHandler.cpp

#include "StdAfx.h"

#include "ZipHandler.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/ComTry.h"
#include "Common/IntToString.h"

#include "Windows/Time.h"
#include "Windows/PropVariant.h"

#include "../../IPassword.h"

#include "../../Common/ProgressUtils.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/CreateCoder.h"
#include "../../Common/FilterCoder.h"

#include "../../Compress/Copy/CopyCoder.h"

#include "../Common/ItemNameUtils.h"
#include "../Common/OutStreamWithCRC.h"

#include "../../Compress/Shrink/ShrinkDecoder.h"
#include "../../Compress/Implode/ImplodeDecoder.h"


#include "../../Crypto/Zip/ZipCipher.h"
#include "../../Crypto/WzAES/WzAES.h"

#ifdef ZIP_STRONG_SUPORT
#include "../../Crypto/ZipStrong/ZipStrong.h"
#endif

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NZip {

// static const CMethodId kMethodId_Store = 0;
static const CMethodId kMethodId_ZipBase = 0x040100;
static const CMethodId kMethodId_BZip2 = 0x040202;

const wchar_t *kHostOS[] = 
{
  L"FAT",
  L"AMIGA",
  L"VMS",
  L"Unix",
  L"VM/CMS",
  L"Atari",
  L"HPFS",
  L"Macintosh",
  L"Z-System",
  L"CP/M",
  L"TOPS-20",
  L"NTFS",
  L"SMS/QDOS",
  L"Acorn",
  L"VFAT",
  L"MVS",
  L"BeOS",
  L"Tandem",
  L"OS/400",
  L"OS/X"
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
  { NULL, kpidAttributes, VT_UI4},

  { NULL, kpidEncrypted, VT_BOOL},
  { NULL, kpidComment, VT_BSTR},
    
  { NULL, kpidCRC, VT_UI4},

  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidHostOS, VT_BSTR}

  // { NULL, kpidUnpackVer, VT_UI1},
};

const wchar_t *kMethods[] = 
{
  L"Store",
  L"Shrink",
  L"Reduced1",
  L"Reduced2",
  L"Reduced2",
  L"Reduced3",
  L"Implode",
  L"Tokenizing",
  L"Deflate",
  L"Deflate64",
  L"PKImploding",
  L"Unknown",
  L"BZip2"
};

const int kNumMethods = sizeof(kMethods) / sizeof(kMethods[0]);
// const wchar_t *kUnknownMethod = L"Unknown";
const wchar_t *kPPMdMethod = L"PPMd";
const wchar_t *kAESMethod = L"AES";
const wchar_t *kZipCryptoMethod = L"ZipCrypto";
const wchar_t *kStrongCryptoMethod = L"StrongCrypto";

struct CStrongCryptoPair
{
  UInt16 Id;
  const wchar_t *Name;
};

CStrongCryptoPair g_StrongCryptoPairs[] = 
{
  { NStrongCryptoFlags::kDES, L"DES" },
  { NStrongCryptoFlags::kRC2old, L"RC2a" },
  { NStrongCryptoFlags::k3DES168, L"3DES-168" },
  { NStrongCryptoFlags::k3DES112, L"3DES-112" },
  { NStrongCryptoFlags::kAES128, L"pkAES-128" },
  { NStrongCryptoFlags::kAES192, L"pkAES-192" },
  { NStrongCryptoFlags::kAES256, L"pkAES-256" },
  { NStrongCryptoFlags::kRC2, L"RC2" },
  { NStrongCryptoFlags::kBlowfish, L"Blowfish" },
  { NStrongCryptoFlags::kTwofish, L"Twofish" },
  { NStrongCryptoFlags::kRC4, L"RC4" }
};

STATPROPSTG kArcProps[] = 
{
  { NULL, kpidComment, VT_BSTR}
};

CHandler::CHandler():
  m_ArchiveIsOpen(false)
{
  InitMethodProperties();
}

static void StringToProp(const CByteBuffer &data, UINT codePage, NWindows::NCOM::CPropVariant &prop)
{
  int size = (int)data.GetCapacity();
  if (size <= 0)
    return;
  AString s;
  char *p = s.GetBuffer(size + 1);
  memcpy(p, (const Byte *)data, size);
  p[size] = '\0';
  s.ReleaseBuffer();
  prop = MultiByteToUnicodeString(s, codePage);
}

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidComment:
    {
      StringToProp(m_Archive.m_ArchiveInfo.Comment, CP_ACP, prop);
      break;
    }
  }
  prop.Detach(value);
  COM_TRY_END
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = m_Items.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CItemEx &item = m_Items[index];
  switch(propID)
  {
    case kpidPath:
      prop = NItemName::GetOSName2(
          MultiByteToUnicodeString(item.Name, item.GetCodePage()));
      break;
    case kpidIsFolder:
      prop = item.IsDirectory();
      break;
    case kpidSize:
      prop = item.UnPackSize;
      break;
    case kpidPackedSize:
      prop = item.PackSize;
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
      prop = utcFileTime;
      break;
    }
    case kpidAttributes:
      prop = item.GetWinAttributes();
      break;
    case kpidEncrypted:
      prop = item.IsEncrypted();
      break;
    case kpidComment:
    {
      StringToProp(item.Comment, item.GetCodePage(), prop);
      break;
    }
    case kpidCRC:
      if (item.IsThereCrc())
        prop = item.FileCRC;
      break;
    case kpidMethod:
    {
      UInt16 methodId = item.CompressionMethod;
      UString method;
      if (item.IsEncrypted())
      {
        if (methodId == NFileHeader::NCompressionMethod::kWzAES)
        {
          method = kAESMethod;
          CWzAesExtraField aesField;
          if (item.CentralExtra.GetWzAesField(aesField))
          {
            method += L"-";
            wchar_t s[32];
            ConvertUInt64ToString((aesField.Strength + 1) * 64 , s);
            method += s;
            method += L" ";
            methodId = aesField.Method;
          }
        }
        else
        {
          if (item.IsStrongEncrypted())
          {
            CStrongCryptoField f;
            bool finded = false;
            if (item.CentralExtra.GetStrongCryptoField(f))
            {
              for (int i = 0; i < sizeof(g_StrongCryptoPairs) / sizeof(g_StrongCryptoPairs[0]); i++)
              {
                const CStrongCryptoPair &pair = g_StrongCryptoPairs[i];
                if (f.AlgId == pair.Id)
                {
                  method += pair.Name;
                  finded = true;
                  break;
                }
              }
            }
            if (!finded)
              method += kStrongCryptoMethod;
          }
          else
            method += kZipCryptoMethod;
          method += L" ";
        }
      }
      if (methodId < kNumMethods)
        method += kMethods[methodId];
      else if (methodId == NFileHeader::NCompressionMethod::kWzPPMd)
        method += kPPMdMethod;
      else
      {
        wchar_t s[32];
        ConvertUInt64ToString(methodId, s);
        method += s;
      }
      prop = method;
      break;
    }
    case kpidHostOS:
      prop = (item.MadeByVersion.HostOS < kNumHostOSes) ?
        (kHostOS[item.MadeByVersion.HostOS]) : kUnknownOS;
      break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

class CPropgressImp: public CProgressVirt
{
  CMyComPtr<IArchiveOpenCallback> m_OpenArchiveCallback;
public:
  STDMETHOD(SetCompleted)(const UInt64 *numFiles);
  void Init(IArchiveOpenCallback *openArchiveCallback)
    { m_OpenArchiveCallback = openArchiveCallback; }
};

STDMETHODIMP CPropgressImp::SetCompleted(const UInt64 *numFiles)
{
  if (m_OpenArchiveCallback)
    return m_OpenArchiveCallback->SetCompleted(numFiles, NULL);
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *inStream, 
    const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  // try
  {
    if(!m_Archive.Open(inStream, maxCheckStartPosition))
      return S_FALSE;
    m_ArchiveIsOpen = true;
    m_Items.Clear();
    if (openArchiveCallback != NULL)
    {
      RINOK(openArchiveCallback->SetTotal(NULL, NULL));
    }
    CPropgressImp propgressImp;
    propgressImp.Init(openArchiveCallback);
    RINOK(m_Archive.ReadHeaders(m_Items, &propgressImp));
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

STDMETHODIMP CHandler::Close()
{
  m_Items.Clear();
  m_Archive.Close();
  m_ArchiveIsOpen = false;
  return S_OK;
}

//////////////////////////////////////
// CHandler::DecompressItems

struct CMethodItem
{
  UInt16 ZipMethod;
  CMyComPtr<ICompressCoder> Coder;
};

class CZipDecoder
{
  NCrypto::NZip::CDecoder *_zipCryptoDecoderSpec;
  NCrypto::NWzAES::CDecoder *_aesDecoderSpec;
  CMyComPtr<ICompressFilter> _zipCryptoDecoder;
  CMyComPtr<ICompressFilter> _aesDecoder;
  #ifdef ZIP_STRONG_SUPORT
  NCrypto::NZipStrong::CDecoder *_zsDecoderSpec;
  CMyComPtr<ICompressFilter> _zsDecoder;
  #endif
  CFilterCoder *filterStreamSpec;
  CMyComPtr<ISequentialInStream> filterStream;
  CMyComPtr<ICryptoGetTextPassword> getTextPassword;
  CObjectVector<CMethodItem> methodItems;

public:
  CZipDecoder(): _zipCryptoDecoderSpec(0), _aesDecoderSpec(0), filterStreamSpec(0) {}

  HRESULT Decode(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CInArchive &archive, const CItemEx &item, 
    ISequentialOutStream *realOutStream, 
    IArchiveExtractCallback *extractCallback, 
    ICompressProgressInfo *compressProgress,
    UInt32 numThreads, Int32 &res);
};

HRESULT CZipDecoder::Decode(
    DECL_EXTERNAL_CODECS_LOC_VARS
    CInArchive &archive, const CItemEx &item, 
    ISequentialOutStream *realOutStream, 
    IArchiveExtractCallback *extractCallback,
    ICompressProgressInfo *compressProgress,
    UInt32 numThreads, Int32 &res)
{
  res = NArchive::NExtract::NOperationResult::kDataError;
  CInStreamReleaser inStreamReleaser;

  bool needCRC = true;
  bool aesMode = false;
  #ifdef ZIP_STRONG_SUPORT
  bool pkAesMode = false;
  #endif
  UInt16 methodId = item.CompressionMethod;
  if (item.IsEncrypted())
  {
    if (item.IsStrongEncrypted())
    {
      #ifdef ZIP_STRONG_SUPORT
      CStrongCryptoField f;
      if (item.CentralExtra.GetStrongCryptoField(f))
      {
        pkAesMode = true;
      }
      if (!pkAesMode)
      #endif
      {
        res = NArchive::NExtract::NOperationResult::kUnSupportedMethod;
        return S_OK;
      }
    }
    if (methodId == NFileHeader::NCompressionMethod::kWzAES)
    {
      CWzAesExtraField aesField;
      if (item.CentralExtra.GetWzAesField(aesField))
      {
        aesMode = true;
        needCRC = aesField.NeedCrc();
      }
    }
  }
    
  COutStreamWithCRC *outStreamSpec = new COutStreamWithCRC;
  CMyComPtr<ISequentialOutStream> outStream = outStreamSpec;
  outStreamSpec->SetStream(realOutStream);
  outStreamSpec->Init(needCRC);
  
  UInt64 authenticationPos;
  
  CMyComPtr<ISequentialInStream> inStream;
  {
    UInt64 packSize = item.PackSize;
    if (aesMode)
    {
      if (packSize < NCrypto::NWzAES::kMacSize)
        return S_OK;
      packSize -= NCrypto::NWzAES::kMacSize;
    }
    UInt64 dataPos = item.GetDataPosition();
    inStream.Attach(archive.CreateLimitedStream(dataPos, packSize));
    authenticationPos = dataPos + packSize;
  }
  
  CMyComPtr<ICompressFilter> cryptoFilter;
  if (item.IsEncrypted())
  {
    if (aesMode)
    {
      CWzAesExtraField aesField;
      if (!item.CentralExtra.GetWzAesField(aesField))
        return S_OK;
      methodId = aesField.Method;
      if (!_aesDecoder)
      {
        _aesDecoderSpec = new NCrypto::NWzAES::CDecoder;
        _aesDecoder = _aesDecoderSpec;
      }
      cryptoFilter = _aesDecoder;
      Byte properties = aesField.Strength;
      RINOK(_aesDecoderSpec->SetDecoderProperties2(&properties, 1));
    }
    #ifdef ZIP_STRONG_SUPORT
    else if (pkAesMode)
    {
      if (!_zsDecoder)
      {
        _zsDecoderSpec = new NCrypto::NZipStrong::CDecoder;
        _zsDecoder = _zsDecoderSpec;
      }
      cryptoFilter = _zsDecoder;
    }
    #endif
    else
    {
      if (!_zipCryptoDecoder)
      {
        _zipCryptoDecoderSpec = new NCrypto::NZip::CDecoder;
        _zipCryptoDecoder = _zipCryptoDecoderSpec;
      }
      cryptoFilter = _zipCryptoDecoder;
    }
    CMyComPtr<ICryptoSetPassword> cryptoSetPassword;
    RINOK(cryptoFilter.QueryInterface(IID_ICryptoSetPassword, &cryptoSetPassword));
    
    if (!getTextPassword)
      extractCallback->QueryInterface(IID_ICryptoGetTextPassword, (void **)&getTextPassword);
    
    if (getTextPassword)
    {
      CMyComBSTR password;
      RINOK(getTextPassword->CryptoGetTextPassword(&password));
      AString charPassword;
      if (aesMode 
        #ifdef ZIP_STRONG_SUPORT
        || pkAesMode
        #endif
        )
      {
        charPassword = UnicodeStringToMultiByte((const wchar_t *)password, CP_ACP);
        /*
        for (int i = 0;; i++)
        {
          wchar_t c = password[i];
          if (c == 0)
            break;
          if (c >= 0x80)
          {
            res = NArchive::NExtract::NOperationResult::kDataError;
            return S_OK;
          }
          charPassword += (char)c;
        }
        */
      }
      else
      {
        // we use OEM. WinZip/Windows probably use ANSI for some files
        charPassword = UnicodeStringToMultiByte((const wchar_t *)password, CP_OEMCP);
      }
      HRESULT res = cryptoSetPassword->CryptoSetPassword(
        (const Byte *)(const char *)charPassword, charPassword.Length());
      if (res != S_OK)
        return S_OK;
    }
    else
    {
      RINOK(cryptoSetPassword->CryptoSetPassword(0, 0));
    }
  }
  
  int m;
  for (m = 0; m < methodItems.Size(); m++)
    if (methodItems[m].ZipMethod == methodId)
      break;

  if (m == methodItems.Size())
  {
    CMethodItem mi;
    mi.ZipMethod = methodId;
    if (methodId == NFileHeader::NCompressionMethod::kStored)
      mi.Coder = new NCompress::CCopyCoder;
    else if (methodId == NFileHeader::NCompressionMethod::kShrunk)
      mi.Coder = new NCompress::NShrink::CDecoder;
    else if (methodId == NFileHeader::NCompressionMethod::kImploded)
      mi.Coder = new NCompress::NImplode::NDecoder::CCoder;
    else
    {
      CMethodId szMethodID;
      if (methodId == NFileHeader::NCompressionMethod::kBZip2)
        szMethodID = kMethodId_BZip2;
      else
      {
        if (methodId > 0xFF)
        {
          res = NArchive::NExtract::NOperationResult::kUnSupportedMethod;
          return S_OK;
        }
        szMethodID = kMethodId_ZipBase + (Byte)methodId;
      }

      RINOK(CreateCoder(EXTERNAL_CODECS_LOC_VARS szMethodID, mi.Coder, false));

      if (mi.Coder == 0)
      {
        res = NArchive::NExtract::NOperationResult::kUnSupportedMethod;
        return S_OK;
      }
    }
    m = methodItems.Add(mi);
  }
  ICompressCoder *coder = methodItems[m].Coder;
  
  {
    CMyComPtr<ICompressSetDecoderProperties2> setDecoderProperties;
    coder->QueryInterface(IID_ICompressSetDecoderProperties2, (void **)&setDecoderProperties);
    if (setDecoderProperties)
    {
      Byte properties = (Byte)item.Flags;
      RINOK(setDecoderProperties->SetDecoderProperties2(&properties, 1));
    }
  }
  
  #ifdef COMPRESS_MT
  {
    CMyComPtr<ICompressSetCoderMt> setCoderMt;
    coder->QueryInterface(IID_ICompressSetCoderMt, (void **)&setCoderMt);
    if (setCoderMt)
    {
      RINOK(setCoderMt->SetNumberOfThreads(numThreads));
    }
  }
  #endif
  
  {
    HRESULT result;
    CMyComPtr<ISequentialInStream> inStreamNew;
    if (item.IsEncrypted())
    {
      if (!filterStream)
      {
        filterStreamSpec = new CFilterCoder;
        filterStream = filterStreamSpec;
      }
      filterStreamSpec->Filter = cryptoFilter;
      if (aesMode)
      {
        RINOK(_aesDecoderSpec->ReadHeader(inStream));
      }
      #ifdef ZIP_STRONG_SUPORT
      else if (pkAesMode)
      {
        RINOK(_zsDecoderSpec->ReadHeader(inStream));
      }
      #endif
      else
      {
        RINOK(_zipCryptoDecoderSpec->ReadHeader(inStream));
      }
      RINOK(filterStreamSpec->SetInStream(inStream));
      inStreamReleaser.FilterCoder = filterStreamSpec;
      inStreamNew = filterStream; 
      
      if (aesMode)
      {
        if (!_aesDecoderSpec->CheckPasswordVerifyCode())
          return S_OK;
      }
    }
    else
      inStreamNew = inStream; 
    result = coder->Code(inStreamNew, outStream, NULL, &item.UnPackSize, compressProgress);
    if (result == S_FALSE)
      return S_OK;
    RINOK(result);
  }
  bool crcOK = true;
  bool authOk = true;
  if (needCRC)
    crcOK = (outStreamSpec->GetCRC() == item.FileCRC);
  if (aesMode)
  {
    inStream.Attach(archive.CreateLimitedStream(authenticationPos, NCrypto::NWzAES::kMacSize));
    if (_aesDecoderSpec->CheckMac(inStream, authOk) != S_OK)
      authOk = false;
  }
  
  res = ((crcOK && authOk) ? 
    NArchive::NExtract::NOperationResult::kOK :
    NArchive::NExtract::NOperationResult::kCRCError);
  return S_OK;
}


STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 _aTestMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  CZipDecoder myDecoder;
  bool testMode = (_aTestMode != 0);
  UInt64 totalUnPacked = 0, totalPacked = 0;
  bool allFilesMode = (numItems == UInt32(-1));
  if (allFilesMode)
    numItems = m_Items.Size();
  if(numItems == 0)
    return S_OK;
  UInt32 i;
  for(i = 0; i < numItems; i++)
  {
    const CItemEx &item = m_Items[allFilesMode ? i : indices[i]];
    totalUnPacked += item.UnPackSize;
    totalPacked += item.PackSize;
  }
  RINOK(extractCallback->SetTotal(totalUnPacked));

  UInt64 currentTotalUnPacked = 0, currentTotalPacked = 0;
  UInt64 currentItemUnPacked, currentItemPacked;
  
  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  for (i = 0; i < numItems; i++, currentTotalUnPacked += currentItemUnPacked,
      currentTotalPacked += currentItemPacked)
  {
    currentItemUnPacked = 0;
    currentItemPacked = 0;

    lps->InSize = currentTotalPacked;
    lps->OutSize = currentTotalUnPacked;
    RINOK(lps->SetCur());

    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ? 
        NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];

    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    CItemEx item = m_Items[index];
    if (!item.FromLocal)
    {
      HRESULT res = m_Archive.ReadLocalItemAfterCdItem(item);
      if (res == S_FALSE)
      {
        if (item.IsDirectory() || realOutStream || testMode)
        {
          RINOK(extractCallback->PrepareOperation(askMode));
          realOutStream.Release();
          RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kUnSupportedMethod));
        }
        continue;
      }
      RINOK(res);
    }

    if (item.IsDirectory() || item.IgnoreItem())
    {
      // if (!testMode)
      {
        RINOK(extractCallback->PrepareOperation(askMode));
        realOutStream.Release();
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
      }
      continue;
    }

    currentItemUnPacked = item.UnPackSize;
    currentItemPacked = item.PackSize;

    if (!testMode && (!realOutStream)) 
      continue;

    RINOK(extractCallback->PrepareOperation(askMode));

    Int32 res;
    RINOK(myDecoder.Decode(
        EXTERNAL_CODECS_VARS
        m_Archive, item, realOutStream, extractCallback, 
        progress, _numThreads, res));
    realOutStream.Release();
    
    RINOK(extractCallback->SetOperationResult(res))
  }
  return S_OK;
  COM_TRY_END
}

IMPL_ISetCompressCodecsInfo

}}
