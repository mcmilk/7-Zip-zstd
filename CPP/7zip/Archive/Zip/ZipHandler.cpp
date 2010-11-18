// ZipHandler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/IntToString.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../../IPassword.h"

#include "../../Common/FilterCoder.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/StreamUtils.h"

#include "../../Compress/CopyCoder.h"
#include "../../Compress/LzmaDecoder.h"
#include "../../Compress/ImplodeDecoder.h"
#include "../../Compress/PpmdZip.h"
#include "../../Compress/ShrinkDecoder.h"

#include "../../Crypto/WzAes.h"
#include "../../Crypto/ZipCrypto.h"
#include "../../Crypto/ZipStrong.h"

#include "../Common/ItemNameUtils.h"
#include "../Common/OutStreamWithCRC.h"

#include "ZipHandler.h"

using namespace NWindows;

namespace NArchive {
namespace NZip {

static const CMethodId kMethodId_ZipBase = 0x040100;
static const CMethodId kMethodId_BZip2 = 0x040202;

static const char *kHostOS[] =
{
  "FAT",
  "AMIGA",
  "VMS",
  "Unix",
  "VM/CMS",
  "Atari",
  "HPFS",
  "Macintosh",
  "Z-System",
  "CP/M",
  "TOPS-20",
  "NTFS",
  "SMS/QDOS",
  "Acorn",
  "VFAT",
  "MVS",
  "BeOS",
  "Tandem",
  "OS/400",
  "OS/X"
};

static const char *kUnknownOS = "Unknown";

static const char *kMethods[] =
{
  "Store",
  "Shrink",
  "Reduced1",
  "Reduced2",
  "Reduced3",
  "Reduced4",
  "Implode",
  "Tokenizing",
  "Deflate",
  "Deflate64",
  "PKImploding"
};

static const char *kBZip2Method = "BZip2";
static const char *kLZMAMethod = "LZMA";
static const char *kJpegMethod = "Jpeg";
static const char *kWavPackMethod = "WavPack";
static const char *kPPMdMethod = "PPMd";
static const char *kAESMethod = "AES";
static const char *kZipCryptoMethod = "ZipCrypto";
static const char *kStrongCryptoMethod = "StrongCrypto";

static struct CStrongCryptoPair
{
  UInt16 Id;
  const char *Name;
} g_StrongCryptoPairs[] =
{
  { NStrongCryptoFlags::kDES, "DES" },
  { NStrongCryptoFlags::kRC2old, "RC2a" },
  { NStrongCryptoFlags::k3DES168, "3DES-168" },
  { NStrongCryptoFlags::k3DES112, "3DES-112" },
  { NStrongCryptoFlags::kAES128, "pkAES-128" },
  { NStrongCryptoFlags::kAES192, "pkAES-192" },
  { NStrongCryptoFlags::kAES256, "pkAES-256" },
  { NStrongCryptoFlags::kRC2, "RC2" },
  { NStrongCryptoFlags::kBlowfish, "Blowfish" },
  { NStrongCryptoFlags::kTwofish, "Twofish" },
  { NStrongCryptoFlags::kRC4, "RC4" }
};

static STATPROPSTG kProps[] =
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
  { NULL, kpidComment, VT_BSTR},
  { NULL, kpidCRC, VT_UI4},
  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidHostOS, VT_BSTR},
  { NULL, kpidUnpackVer, VT_UI4}
};

static STATPROPSTG kArcProps[] =
{
  { NULL, kpidBit64, VT_BOOL},
  { NULL, kpidComment, VT_BSTR},
  { NULL, kpidPhySize, VT_UI8},
  { NULL, kpidOffset, VT_UI8}
};

CHandler::CHandler()
{
  InitMethodProperties();
}

static AString BytesToString(const CByteBuffer &data)
{
  AString s;
  int size = (int)data.GetCapacity();
  if (size > 0)
  {
    char *p = s.GetBuffer(size + 1);
    memcpy(p, (const Byte *)data, size);
    p[size] = '\0';
    s.ReleaseBuffer();
  }
  return s;
}

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidBit64:  if (m_Archive.IsZip64) prop = m_Archive.IsZip64; break;
    case kpidComment:  prop = MultiByteToUnicodeString(BytesToString(m_Archive.ArcInfo.Comment), CP_ACP); break;
    case kpidPhySize:  prop = m_Archive.ArcInfo.GetPhySize(); break;
    case kpidOffset:  if (m_Archive.ArcInfo.StartPosition != 0) prop = m_Archive.ArcInfo.StartPosition; break;
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
    case kpidPath:  prop = NItemName::GetOSName2(item.GetUnicodeString(item.Name)); break;
    case kpidIsDir:  prop = item.IsDir(); break;
    case kpidSize:  prop = item.UnPackSize; break;
    case kpidPackSize:  prop = item.PackSize; break;
    case kpidTimeType:
    {
      FILETIME ft;
      UInt32 unixTime;
      if (item.CentralExtra.GetNtfsTime(NFileHeader::NNtfsExtra::kMTime, ft))
        prop = (UInt32)NFileTimeType::kWindows;
      else if (item.CentralExtra.GetUnixTime(NFileHeader::NUnixTime::kMTime, unixTime))
        prop = (UInt32)NFileTimeType::kUnix;
      else
        prop = (UInt32)NFileTimeType::kDOS;
      break;
    }
    case kpidCTime:
    {
      FILETIME ft;
      if (item.CentralExtra.GetNtfsTime(NFileHeader::NNtfsExtra::kCTime, ft))
        prop = ft;
      break;
    }
    case kpidATime:
    {
      FILETIME ft;
      if (item.CentralExtra.GetNtfsTime(NFileHeader::NNtfsExtra::kATime, ft))
        prop = ft;
      break;
    }
    case kpidMTime:
    {
      FILETIME utc;
      if (!item.CentralExtra.GetNtfsTime(NFileHeader::NNtfsExtra::kMTime, utc))
      {
        UInt32 unixTime;
        if (item.CentralExtra.GetUnixTime(NFileHeader::NUnixTime::kMTime, unixTime))
          NTime::UnixTimeToFileTime(unixTime, utc);
        else
        {
          FILETIME localFileTime;
          if (!NTime::DosTimeToFileTime(item.Time, localFileTime) ||
              !LocalFileTimeToFileTime(&localFileTime, &utc))
            utc.dwHighDateTime = utc.dwLowDateTime = 0;
        }
      }
      prop = utc;
      break;
    }
    case kpidAttrib:  prop = item.GetWinAttributes(); break;
    case kpidEncrypted:  prop = item.IsEncrypted(); break;
    case kpidComment:  prop = item.GetUnicodeString(BytesToString(item.Comment)); break;
    case kpidCRC:  if (item.IsThereCrc()) prop = item.FileCRC; break;
    case kpidMethod:
    {
      UInt16 methodId = item.CompressionMethod;
      AString method;
      if (item.IsEncrypted())
      {
        if (methodId == NFileHeader::NCompressionMethod::kWzAES)
        {
          method = kAESMethod;
          CWzAesExtraField aesField;
          if (item.CentralExtra.GetWzAesField(aesField))
          {
            method += '-';
            char s[32];
            ConvertUInt64ToString((aesField.Strength + 1) * 64 , s);
            method += s;
            method += ' ';
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
          method += ' ';
        }
      }
      if (methodId < sizeof(kMethods) / sizeof(kMethods[0]))
        method += kMethods[methodId];
      else switch (methodId)
      {
        case NFileHeader::NCompressionMethod::kLZMA:
          method += kLZMAMethod;
          if (item.IsLzmaEOS())
            method += ":EOS";
          break;
        case NFileHeader::NCompressionMethod::kBZip2: method += kBZip2Method; break;
        case NFileHeader::NCompressionMethod::kJpeg: method += kJpegMethod; break;
        case NFileHeader::NCompressionMethod::kWavPack: method += kWavPackMethod; break;
        case NFileHeader::NCompressionMethod::kPPMd: method += kPPMdMethod; break;
        default:
        {
          char s[32];
          ConvertUInt64ToString(methodId, s);
          method += s;
        }
      }
      prop = method;
      break;
    }
    case kpidHostOS:
      prop = (item.MadeByVersion.HostOS < sizeof(kHostOS) / sizeof(kHostOS[0])) ?
        (kHostOS[item.MadeByVersion.HostOS]) : kUnknownOS;
      break;
    case kpidUnpackVer:
      prop = (UInt32)item.ExtractVersion.Version;
      break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

class CProgressImp: public CProgressVirt
{
  CMyComPtr<IArchiveOpenCallback> _callback;
public:
  STDMETHOD(SetTotal)(UInt64 numFiles);
  STDMETHOD(SetCompleted)(UInt64 numFiles);
  CProgressImp(IArchiveOpenCallback *callback): _callback(callback) {}
};

STDMETHODIMP CProgressImp::SetTotal(UInt64 numFiles)
{
  if (_callback)
    return _callback->SetTotal(&numFiles, NULL);
  return S_OK;
}

STDMETHODIMP CProgressImp::SetCompleted(UInt64 numFiles)
{
  if (_callback)
    return _callback->SetCompleted(&numFiles, NULL);
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *inStream,
    const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *callback)
{
  COM_TRY_BEGIN
  try
  {
    Close();
    RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
    RINOK(m_Archive.Open(inStream, maxCheckStartPosition));
    CProgressImp progressImp(callback);
    return m_Archive.ReadHeaders(m_Items, &progressImp);
  }
  catch(const CInArchiveException &) { Close(); return S_FALSE; }
  catch(...) { Close(); throw; }
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  m_Items.Clear();
  m_Archive.Close();
  return S_OK;
}

//////////////////////////////////////
// CHandler::DecompressItems

class CLzmaDecoder:
  public ICompressCoder,
  public CMyUnknownImp
{
  NCompress::NLzma::CDecoder *DecoderSpec;
  CMyComPtr<ICompressCoder> Decoder;
public:
  CLzmaDecoder();
  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

  MY_UNKNOWN_IMP
};

CLzmaDecoder::CLzmaDecoder()
{
  DecoderSpec = new NCompress::NLzma::CDecoder;
  Decoder = DecoderSpec;
}

HRESULT CLzmaDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  Byte buf[9];
  RINOK(ReadStream_FALSE(inStream, buf, 9));
  if (buf[2] != 5 || buf[3] != 0)
    return E_NOTIMPL;
  RINOK(DecoderSpec->SetDecoderProperties2(buf + 4, 5));
  return Decoder->Code(inStream, outStream, NULL, outSize, progress);
}

struct CMethodItem
{
  UInt16 ZipMethod;
  CMyComPtr<ICompressCoder> Coder;
};

class CZipDecoder
{
  NCrypto::NZip::CDecoder *_zipCryptoDecoderSpec;
  NCrypto::NZipStrong::CDecoder *_pkAesDecoderSpec;
  NCrypto::NWzAes::CDecoder *_wzAesDecoderSpec;

  CMyComPtr<ICompressFilter> _zipCryptoDecoder;
  CMyComPtr<ICompressFilter> _pkAesDecoder;
  CMyComPtr<ICompressFilter> _wzAesDecoder;

  CFilterCoder *filterStreamSpec;
  CMyComPtr<ISequentialInStream> filterStream;
  CMyComPtr<ICryptoGetTextPassword> getTextPassword;
  CObjectVector<CMethodItem> methodItems;

public:
  CZipDecoder():
      _zipCryptoDecoderSpec(0),
      _pkAesDecoderSpec(0),
      _wzAesDecoderSpec(0),
      filterStreamSpec(0) {}

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
  res = NExtract::NOperationResult::kDataError;
  CInStreamReleaser inStreamReleaser;

  bool needCRC = true;
  bool wzAesMode = false;
  bool pkAesMode = false;
  UInt16 methodId = item.CompressionMethod;
  if (item.IsEncrypted())
  {
    if (item.IsStrongEncrypted())
    {
      CStrongCryptoField f;
      if (item.CentralExtra.GetStrongCryptoField(f))
      {
        pkAesMode = true;
      }
      if (!pkAesMode)
      {
        res = NExtract::NOperationResult::kUnSupportedMethod;
        return S_OK;
      }
    }
    if (methodId == NFileHeader::NCompressionMethod::kWzAES)
    {
      CWzAesExtraField aesField;
      if (item.CentralExtra.GetWzAesField(aesField))
      {
        wzAesMode = true;
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
    if (wzAesMode)
    {
      if (packSize < NCrypto::NWzAes::kMacSize)
        return S_OK;
      packSize -= NCrypto::NWzAes::kMacSize;
    }
    UInt64 dataPos = item.GetDataPosition();
    inStream.Attach(archive.CreateLimitedStream(dataPos, packSize));
    authenticationPos = dataPos + packSize;
  }
  
  CMyComPtr<ICompressFilter> cryptoFilter;
  if (item.IsEncrypted())
  {
    if (wzAesMode)
    {
      CWzAesExtraField aesField;
      if (!item.CentralExtra.GetWzAesField(aesField))
        return S_OK;
      methodId = aesField.Method;
      if (!_wzAesDecoder)
      {
        _wzAesDecoderSpec = new NCrypto::NWzAes::CDecoder;
        _wzAesDecoder = _wzAesDecoderSpec;
      }
      cryptoFilter = _wzAesDecoder;
      Byte properties = aesField.Strength;
      RINOK(_wzAesDecoderSpec->SetDecoderProperties2(&properties, 1));
    }
    else if (pkAesMode)
    {
      if (!_pkAesDecoder)
      {
        _pkAesDecoderSpec = new NCrypto::NZipStrong::CDecoder;
        _pkAesDecoder = _pkAesDecoderSpec;
      }
      cryptoFilter = _pkAesDecoder;
    }
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
      if (wzAesMode || pkAesMode)
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
            res = NExtract::NOperationResult::kDataError;
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
      HRESULT result = cryptoSetPassword->CryptoSetPassword(
        (const Byte *)(const char *)charPassword, charPassword.Length());
      if (result != S_OK)
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
    else if (methodId == NFileHeader::NCompressionMethod::kLZMA)
      mi.Coder = new CLzmaDecoder;
    else if (methodId == NFileHeader::NCompressionMethod::kPPMd)
      mi.Coder = new NCompress::NPpmdZip::CDecoder(true);
    else
    {
      CMethodId szMethodID;
      if (methodId == NFileHeader::NCompressionMethod::kBZip2)
        szMethodID = kMethodId_BZip2;
      else
      {
        if (methodId > 0xFF)
        {
          res = NExtract::NOperationResult::kUnSupportedMethod;
          return S_OK;
        }
        szMethodID = kMethodId_ZipBase + (Byte)methodId;
      }

      RINOK(CreateCoder(EXTERNAL_CODECS_LOC_VARS szMethodID, mi.Coder, false));

      if (mi.Coder == 0)
      {
        res = NExtract::NOperationResult::kUnSupportedMethod;
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
  
  #ifndef _7ZIP_ST
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
    HRESULT result = S_OK;
    CMyComPtr<ISequentialInStream> inStreamNew;
    if (item.IsEncrypted())
    {
      if (!filterStream)
      {
        filterStreamSpec = new CFilterCoder;
        filterStream = filterStreamSpec;
      }
      filterStreamSpec->Filter = cryptoFilter;
      if (wzAesMode)
      {
        result = _wzAesDecoderSpec->ReadHeader(inStream);
      }
      else if (pkAesMode)
      {
        result =_pkAesDecoderSpec->ReadHeader(inStream, item.FileCRC, item.UnPackSize);
        if (result == S_OK)
        {
          bool passwOK;
          result = _pkAesDecoderSpec->CheckPassword(passwOK);
          if (result == S_OK && !passwOK)
            result = S_FALSE;
        }
      }
      else
      {
        result = _zipCryptoDecoderSpec->ReadHeader(inStream);
      }

      if (result == S_OK)
      {
        RINOK(filterStreamSpec->SetInStream(inStream));
        inStreamReleaser.FilterCoder = filterStreamSpec;
        inStreamNew = filterStream;
        if (wzAesMode)
        {
          if (!_wzAesDecoderSpec->CheckPasswordVerifyCode())
            result = S_FALSE;
        }
      }
    }
    else
      inStreamNew = inStream;
    if (result == S_OK)
      result = coder->Code(inStreamNew, outStream, NULL, &item.UnPackSize, compressProgress);
    if (result == S_FALSE)
      return S_OK;
    if (result == E_NOTIMPL)
    {
      res = NExtract::NOperationResult::kUnSupportedMethod;
      return S_OK;
    }

    RINOK(result);
  }
  bool crcOK = true;
  bool authOk = true;
  if (needCRC)
    crcOK = (outStreamSpec->GetCRC() == item.FileCRC);
  if (wzAesMode)
  {
    inStream.Attach(archive.CreateLimitedStream(authenticationPos, NCrypto::NWzAes::kMacSize));
    if (_wzAesDecoderSpec->CheckMac(inStream, authOk) != S_OK)
      authOk = false;
  }
  
  res = ((crcOK && authOk) ?
    NExtract::NOperationResult::kOK :
    NExtract::NOperationResult::kCRCError);
  return S_OK;
}


STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  CZipDecoder myDecoder;
  UInt64 totalUnPacked = 0, totalPacked = 0;
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = m_Items.Size();
  if(numItems == 0)
    return S_OK;
  UInt32 i;
  for (i = 0; i < numItems; i++)
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
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];

    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    CItemEx item = m_Items[index];
    if (!item.FromLocal)
    {
      HRESULT res = m_Archive.ReadLocalItemAfterCdItem(item);
      if (res == S_FALSE)
      {
        if (item.IsDir() || realOutStream || testMode)
        {
          RINOK(extractCallback->PrepareOperation(askMode));
          realOutStream.Release();
          RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kUnSupportedMethod));
        }
        continue;
      }
      RINOK(res);
    }

    if (item.IsDir() || item.IgnoreItem())
    {
      // if (!testMode)
      {
        RINOK(extractCallback->PrepareOperation(askMode));
        realOutStream.Release();
        RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      }
      continue;
    }

    currentItemUnPacked = item.UnPackSize;
    currentItemPacked = item.PackSize;

    if (!testMode && !realOutStream)
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
