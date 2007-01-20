// ZipHandler.cpp

#include "StdAfx.h"

#include "ZipHandler.h"

#include "Common/Defs.h"
#include "Common/CRC.h"
#include "Common/StringConvert.h"
#include "Common/ComTry.h"
#include "Common/IntToString.h"

#include "Windows/Time.h"
#include "Windows/PropVariant.h"

#include "../../IPassword.h"

#include "../../Common/ProgressUtils.h"
#include "../../Common/StreamObjects.h"

#include "../../Compress/Copy/CopyCoder.h"

#include "../Common/ItemNameUtils.h"
#include "../Common/OutStreamWithCRC.h"
#include "../Common/FilterCoder.h"
#include "../7z/7zMethods.h"

#include "../../Compress/Shrink/ShrinkDecoder.h"
#include "../../Compress/Implode/ImplodeDecoder.h"

#ifdef COMPRESS_DEFLATE
#include "../../Compress/Deflate/DeflateDecoder.h"
#else
// {23170F69-40C1-278B-0401-080000000000}
DEFINE_GUID(CLSID_CCompressDeflateDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00);
#endif

#ifdef COMPRESS_DEFLATE64
#include "../../Compress/Deflate/DeflateDecoder.h"
#else
// {23170F69-40C1-278B-0401-090000000000}
DEFINE_GUID(CLSID_CCompressDeflate64Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00);
#endif

/*
#ifdef COMPRESS_IMPLODE
#else
// {23170F69-40C1-278B-0401-060000000000}
DEFINE_GUID(CLSID_CCompressImplodeDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00);
#endif
*/

#ifdef COMPRESS_BZIP2
#include "../../Compress/BZip2/BZip2Decoder.h"
#else
// {23170F69-40C1-278B-0402-020000000000}
DEFINE_GUID(CLSID_CCompressBZip2Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00);
#endif

#include "../../Crypto/Zip/ZipCipher.h"
#include "../../Crypto/WzAES/WzAES.h"

#ifndef EXCLUDE_COM
#include "../Common/CoderLoader.h"
#endif

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NZip {

const wchar_t *kHostOS[] = 
{
  L"FAT",
  L"AMIGA",
  L"VMS",
  L"Unix",
  L"VM_CMS",
  L"Atari",  // what if it's a minix filesystem? [cjh]
  L"HPFS",  // filesystem used by OS/2 (and NT 3.x)
  L"Mac",
  L"Z_System",
  L"CPM",
  L"TOPS20", // pkzip 2.50 NTFS 
  L"NTFS", // filesystem used by Windows NT 
  L"QDOS ", // SMS/QDOS
  L"Acorn", // Archimedes Acorn RISC OS
  L"VFAT", // filesystem used by Windows 95, NT
  L"MVS",
  L"BeOS", // hybrid POSIX/database filesystem
                        // BeBOX or PowerMac 
  L"Tandem",
  L"THEOS"
};


static const int kNumHostOSes = sizeof(kHostOS) / sizeof(kHostOS[0]);

static const wchar_t *kUnknownOS = L"Unknown";


/*
enum // PropID
{
  kpidUnPackVersion, 
};
*/

STATPROPSTG kProperties[] = 
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

  // { L"UnPack Version", kpidUnPackVersion, VT_UI1},
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

CHandler::CHandler():
  m_ArchiveIsOpen(false)
{
  InitMethodProperties();
}

STDMETHODIMP CHandler::GetArchiveProperty(PROPID /* propID */, PROPVARIANT *value)
{
  value->vt = VT_EMPTY;
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfProperties(UInt32 *numProperties)
{
  *numProperties = sizeof(kProperties) / sizeof(kProperties[0]);
  return S_OK;
}

STDMETHODIMP CHandler::GetPropertyInfo(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  if(index >= sizeof(kProperties) / sizeof(kProperties[0]))
    return E_INVALIDARG;
  const STATPROPSTG &srcItem = kProperties[index];
  *propID = srcItem.propid;
  *varType = srcItem.vt;
  *name = 0;
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfArchiveProperties(UInt32 *numProperties)
{
  *numProperties = 0;
  return S_OK;
}

STDMETHODIMP CHandler::GetArchivePropertyInfo(UInt32 /* index */,     
      BSTR * /* name */, PROPID * /* propID */, VARTYPE * /* varType */)
{
  return E_NOTIMPL;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = m_Items.Size();
  return S_OK;
}

static void MyStrNCpy(char *dest, const char *src, int size)
{
  for (int i = 0; i < size; i++)
  {
    char c = src[i];
    dest[i] = c;
    if (c == 0)
      break;
  }
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID aPropID,  PROPVARIANT *aValue)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
  const CItemEx &item = m_Items[index];
  switch(aPropID)
  {
    case kpidPath:
      propVariant = NItemName::GetOSName2(
          MultiByteToUnicodeString(item.Name, item.GetCodePage()));
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
    case kpidComment:
    {
      int size = (int)item.Comment.GetCapacity();
      if (size > 0)
      {
        AString s;
        char *p = s.GetBuffer(size + 1);
        MyStrNCpy(p, (const char *)(const Byte *)item.Comment, size);
        p[size] = '\0';
        s.ReleaseBuffer();
        propVariant = MultiByteToUnicodeString(s, item.GetCodePage());
      }
      break;
    }
    case kpidCRC:
      if (item.IsThereCrc())
        propVariant = item.FileCRC;
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
      propVariant = method;
      break;
    }
    case kpidHostOS:
      propVariant = (item.MadeByVersion.HostOS < kNumHostOSes) ?
        (kHostOS[item.MadeByVersion.HostOS]) : kUnknownOS;
      break;
  }
  propVariant.Detach(aValue);
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
  CFilterCoder *filterStreamSpec;
  CMyComPtr<ISequentialInStream> filterStream;
  CMyComPtr<ICryptoGetTextPassword> getTextPassword;
  #ifndef EXCLUDE_COM
  CCoderLibraries libraries;
  #endif
  CObjectVector<CMethodItem> methodItems;

public:
  CZipDecoder(): _zipCryptoDecoderSpec(0), _aesDecoderSpec(0), filterStreamSpec(0) {}

  static void Init()
  {
    #ifndef EXCLUDE_COM
    N7z::LoadMethodMap();
    #endif
  }

  HRESULT Decode(CInArchive &archive, const CItemEx &item, 
    ISequentialOutStream *realOutStream, 
    IArchiveExtractCallback *extractCallback, 
    ICompressProgressInfo *compressProgress,
    UInt32 numThreads, Int32 &res);
};

HRESULT CZipDecoder::Decode(CInArchive &archive, const CItemEx &item, 
    ISequentialOutStream *realOutStream, 
    IArchiveExtractCallback *extractCallback,
    ICompressProgressInfo *compressProgress,
    UInt32 numThreads, Int32 &res)
{
  res = NArchive::NExtract::NOperationResult::kDataError;
  CInStreamReleaser inStreamReleaser;

  bool needCRC = true;
  bool aesMode = false;
  UInt16 methodId = item.CompressionMethod;
  if (item.IsEncrypted())
    if (methodId == NFileHeader::NCompressionMethod::kWzAES)
    {
      CWzAesExtraField aesField;
      if (item.CentralExtra.GetWzAesField(aesField))
      {
        aesMode = true;
        needCRC = aesField.NeedCrc();
      }
    }
    
  COutStreamWithCRC *outStreamSpec = new COutStreamWithCRC;;
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
      if (aesMode)
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
      #ifdef EXCLUDE_COM
      switch(methodId)
      {
       case NFileHeader::NCompressionMethod::kDeflated:
          mi.Coder = new NCompress::NDeflate::NDecoder::CCOMCoder;
          break;
       case NFileHeader::NCompressionMethod::kDeflated64:
         mi.Coder = new NCompress::NDeflate::NDecoder::CCOMCoder64;
         break;
       case NFileHeader::NCompressionMethod::kBZip2:
         mi.Coder = new NCompress::NBZip2::CDecoder;
         break;
       default:
         res = NArchive::NExtract::NOperationResult::kUnSupportedMethod;
         return S_OK;
      }
      #else
      N7z::CMethodID methodID = { { 0x04, 0x01 } , 3 };
      if (methodId > 0xFF)
      {
        res = NArchive::NExtract::NOperationResult::kUnSupportedMethod;
        return S_OK;
      }
      methodID.ID[2] = (Byte)methodId;
      if (methodId == NFileHeader::NCompressionMethod::kStored)
      {
        methodID.ID[0] = 0;
        methodID.IDSize = 1;
      }
      else if (methodId == NFileHeader::NCompressionMethod::kBZip2)
      {
        methodID.ID[1] = 0x02;
        methodID.ID[2] = 0x02;
      }
      
      N7z::CMethodInfo methodInfo;
      if (!N7z::GetMethodInfo(methodID, methodInfo))
      {
        res = NArchive::NExtract::NOperationResult::kUnSupportedMethod;
        return S_OK;
      }
      RINOK(libraries.CreateCoder(methodInfo.FilePath, methodInfo.Decoder, &mi.Coder));
      #endif
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
  extractCallback->SetTotal(totalUnPacked);

  UInt64 currentTotalUnPacked = 0, currentTotalPacked = 0;
  UInt64 currentItemUnPacked, currentItemPacked;
  
  CLocalProgress *localProgressSpec = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = localProgressSpec;
  CLocalCompressProgressInfo *localCompressProgressSpec = new CLocalCompressProgressInfo;
  CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;

  CZipDecoder::Init();

  for (i = 0; i < numItems; i++, currentTotalUnPacked += currentItemUnPacked,
      currentTotalPacked += currentItemPacked)
  {
    currentItemUnPacked = 0;
    currentItemPacked = 0;

    RINOK(extractCallback->SetCompleted(&currentTotalUnPacked));
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

    localProgressSpec->Init(extractCallback, false);
    localCompressProgressSpec->Init(progress,  &currentTotalPacked, &currentTotalUnPacked);
    
    Int32 res;
    RINOK(myDecoder.Decode(m_Archive, item, realOutStream, extractCallback, 
        compressProgress, _numThreads, res));
    realOutStream.Release();
    
    RINOK(extractCallback->SetOperationResult(res))
  }
  return S_OK;
  COM_TRY_END
}

}}
