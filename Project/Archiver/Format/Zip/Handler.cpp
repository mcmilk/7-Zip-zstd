// Zip/Handler.cpp

#include "StdAfx.h"

#include "Handler.h"

#include "Common/Defs.h"
#include "Common/CRC.h"
#include "Common/StringConvert.h"

#include "Interface/ProgressUtils.h"
#include "Interface/StreamObjects.h"
#include "Interface/EnumStatProp.h"
#include "Interface/StreamObjects.h"

#include "Windows/Time.h"
#include "Windows/PropVariant.h"
#include "Windows/COMTry.h"

#include "Compression/CopyCoder.h"

#include "Archive/Common/ItemNameUtils.h"

#include "Interface/CryptoInterface.h"

#include "../Common/OutStreamWithCRC.h"
#include "../Common/CoderMixer.h"

#include "../../../Compress/Interface/CompressInterface.h"
#include "../../../Crypto/Cipher/Common/CipherInterface.h"

#ifdef COMPRESS_DEFLATE
#include "../../../Compress/LZ/Deflate/Decoder.h"
#else
// {23170F69-40C1-278B-0401-080000000000}
DEFINE_GUID(CLSID_CCompressDeflateDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00);
#endif

#ifdef COMPRESS_DEFLATE64
#include "../../../Compress/LZ/Deflate/Decoder.h"
#else
// {23170F69-40C1-278B-0401-090000000000}
DEFINE_GUID(CLSID_CCompressDeflate64Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00);
#endif

#ifdef COMPRESS_IMPLODE
#include "../../../Compress/LZ/Implode/Decoder.h"
#else
// {23170F69-40C1-278B-0401-060000000000}
DEFINE_GUID(CLSID_CCompressImplodeDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00);
#endif

#ifdef CRYPTO_ZIP
#include "../../../Crypto/Cipher/Zip/ZipCipher.h"
#else
// {23170F69-40C1-278B-06F1-0101000000000}
DEFINE_GUID(CLSID_CCryptoZipDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00);
#endif

using namespace std;

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


const kNumHostOSes = sizeof(kHostOS) / sizeof(kHostOS[0]);

const wchar_t *kUnknownOS = L"Unknown";


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
  { NULL, kpidCommented, VT_BOOL},
    
  { NULL, kpidCRC, VT_UI4},

  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidHostOS, VT_BSTR}

  // { L"UnPack Version", kpidUnPackVersion, VT_UI1},
};

const wchar_t *kMethods[] = 
{
  L"Store",
  L"Shrunk",
  L"Reduced1",
  L"Reduced2",
  L"Reduced2",
  L"Reduced3",
  L"Implode",
  L"Tokenizing",
  L"Deflate",
  L"Deflate64",
  L"PKImploding"
};

const kNumMethods = sizeof(kMethods) / sizeof(kMethods[0]);
const wchar_t *kUnknownMethod = L"Unknown";


CZipHandler::CZipHandler():
  m_ArchiveIsOpen(false)
{
  InitMethodProperties();
  m_Method.MethodSequence.Add(NFileHeader::NCompressionMethod::kDeflated);
  m_Method.MethodSequence.Add(NFileHeader::NCompressionMethod::kStored);
}

STDMETHODIMP CZipHandler::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  COM_TRY_BEGIN
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
  COM_TRY_END
}

STDMETHODIMP CZipHandler::GetNumberOfItems(UINT32 *numItems)
{
  *numItems = m_Items.Size();
  return S_OK;
}

STDMETHODIMP CZipHandler::GetProperty(UINT32 index, PROPID aPropID,  PROPVARIANT *aValue)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
  const NArchive::NZip::CItemInfoEx &item = m_Items[index];
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
    case kpidCommented:
      propVariant = item.IsCommented();
      break;
    case kpidCRC:
      propVariant = item.FileCRC;
      break;
    case kpidMethod:
    {
      UString method;
      if (item.CompressionMethod < kNumMethods)
        method = kMethods[item.CompressionMethod];
      else
        method = kUnknownMethod;
      propVariant = method;
      // propVariant = item.CompressionMethod;
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
  CComPtr<IArchiveOpenCallback> m_OpenArchiveCallback;
public:
  STDMETHOD(SetCompleted)(const UINT64 *numFiles);
  void Init(IArchiveOpenCallback *openArchiveCallback)
    { m_OpenArchiveCallback = openArchiveCallback; }
};

STDMETHODIMP CPropgressImp::SetCompleted(const UINT64 *numFiles)
{
  if (m_OpenArchiveCallback)
    return m_OpenArchiveCallback->SetCompleted(numFiles, NULL);
  return S_OK;
}

STDMETHODIMP CZipHandler::Open(IInStream *inStream, 
    const UINT64 *maxCheckStartPosition, IArchiveOpenCallback *openArchiveCallback)
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

STDMETHODIMP CZipHandler::Close()
{
  m_Archive.Close();
  m_ArchiveIsOpen = false;
  return S_OK;
}



//////////////////////////////////////
// CZipHandler::DecompressItems

STDMETHODIMP CZipHandler::Extract(const UINT32* indices, UINT32 numItems,
    INT32 _aTestMode, IArchiveExtractCallback *_anExtractCallback)
{
  COM_TRY_BEGIN
  CComPtr<ICryptoGetTextPassword> getTextPassword;
  bool testMode = (_aTestMode != 0);
  CComPtr<IArchiveExtractCallback> extractCallback = _anExtractCallback;
  UINT64 aTotalUnPacked = 0, aTotalPacked = 0;
  if(numItems == 0)
    return S_OK;
  int i;
  for(i = 0; i < numItems; i++)
  {
    const CItemInfoEx &itemInfo = m_Items[indices[i]];
    aTotalUnPacked += itemInfo.UnPackSize;
    aTotalPacked += itemInfo.PackSize;
  }
  extractCallback->SetTotal(aTotalUnPacked);

  UINT64 currentTotalUnPacked = 0, currentTotalPacked = 0;
  UINT64 currentItemUnPacked, currentItemPacked;
  
  CComPtr<ICompressCoder> deflateDecoder;
  CComPtr<ICompressCoder> deflate64Decoder;
  CComPtr<ICompressCoder> implodeDecoder;
  CComPtr<ICompressCoder> copyCoder;
  CComPtr<ICompressCoder> cryptoDecoder;
  CComObjectNoLock<CCoderMixer> *mixerCoderSpec;
  CComPtr<ICompressCoder> mixerCoder;

  UINT16 mixerCoderMethod;

  for(i = 0; i < numItems; i++, currentTotalUnPacked += currentItemUnPacked,
      currentTotalPacked += currentItemPacked)
  {
    currentItemUnPacked = 0;
    currentItemPacked = 0;

    RINOK(extractCallback->SetCompleted(&currentTotalUnPacked));
    CComPtr<ISequentialOutStream> realOutStream;
    INT32 askMode;
    askMode = testMode ? NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
    INT32 index = indices[i];
    const CItemInfoEx &itemInfo = m_Items[index];
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    if(itemInfo.IsDirectory() || itemInfo.IgnoreItem())
    {
      // if (!testMode)
      {
        RINOK(extractCallback->PrepareOperation(askMode));
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
      }
      continue;
    }

    if (!testMode && (!realOutStream)) 
      continue;

    RINOK(extractCallback->PrepareOperation(askMode));
    currentItemUnPacked = itemInfo.UnPackSize;
    currentItemPacked = itemInfo.PackSize;

    {
      CComObjectNoLock<COutStreamWithCRC> *outStreamSpec = 
        new CComObjectNoLock<COutStreamWithCRC>;
      CComPtr<ISequentialOutStream> outStream(outStreamSpec);
      outStreamSpec->Init(realOutStream);
      realOutStream.Release();
      
      CComPtr<ISequentialInStream> anInStream;
      anInStream.Attach(m_Archive.CreateLimitedStream(itemInfo.GetDataPosition(),
          itemInfo.PackSize));

      CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
      CComPtr<ICompressProgressInfo> aProgress = aLocalProgressSpec;
      aLocalProgressSpec->Init(extractCallback, false);


      CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
          new  CComObjectNoLock<CLocalCompressProgressInfo>;
      CComPtr<ICompressProgressInfo> compressProgress = aLocalCompressProgressSpec;
      aLocalCompressProgressSpec->Init(aProgress, 
          &currentTotalPacked,
          &currentTotalUnPacked);

      if (itemInfo.IsEncrypted())
      {
        if (!cryptoDecoder)
        {
          #ifdef CRYPTO_ZIP
          cryptoDecoder = new CComObjectNoLock<NCrypto::NZip::CDecoder>;
          #else
          RINOK(cryptoDecoder.CoCreateInstance(CLSID_CCryptoZipDecoder));
          #endif
        }
        CComPtr<ICryptoSetPassword> cryptoSetPassword;
        RINOK(cryptoDecoder.QueryInterface(&cryptoSetPassword));

        if (!getTextPassword)
          extractCallback.QueryInterface(&getTextPassword);

        if (getTextPassword)
        {
          CComBSTR password;
          RINOK(getTextPassword->CryptoGetTextPassword(&password));
          AString anOemPassword = UnicodeStringToMultiByte(
              (const wchar_t *)password, CP_OEMCP);
          RINOK(cryptoSetPassword->CryptoSetPassword(
              (const BYTE *)(const char *)anOemPassword, anOemPassword.Length()));
        }
        else
        {
          RINOK(cryptoSetPassword->CryptoSetPassword(0, 0));
        }
      }

      switch(itemInfo.CompressionMethod)
      {
        case NFileHeader::NCompressionMethod::kStored:
          {
            if(!copyCoder)
            {
              copyCoder = new CComObjectNoLock<NCompression::CCopyCoder>;
            }
            try
            {
              if (itemInfo.IsEncrypted())
              {
                if (!mixerCoder || mixerCoderMethod != itemInfo.CompressionMethod)
                {
                  mixerCoder.Release();
                  mixerCoderSpec = new CComObjectNoLock<CCoderMixer>;
                  mixerCoder = mixerCoderSpec;
                  mixerCoderSpec->AddCoder(cryptoDecoder);
                  mixerCoderSpec->AddCoder(copyCoder);
                  mixerCoderSpec->FinishAddingCoders();
                  mixerCoderMethod = itemInfo.CompressionMethod;
                }
                mixerCoderSpec->ReInit();
                mixerCoderSpec->SetCoderInfo(0, &currentItemPacked, 
                    &currentItemUnPacked);
                mixerCoderSpec->SetCoderInfo(1, NULL, NULL);
                mixerCoderSpec->SetProgressCoderIndex(1);
                RINOK(mixerCoder->Code(anInStream, outStream,
                  NULL, NULL, compressProgress));
              }
              else
              {
                RINOK(copyCoder->Code(anInStream, outStream,
                    NULL, NULL, compressProgress));
              }
            }
            catch(...)
            {
              outStream.Release();
              RINOK(extractCallback->SetOperationResult(
                  NArchive::NExtract::NOperationResult::kDataError));
              continue;
            }
            break;
          }
        case NFileHeader::NCompressionMethod::kImploded:
          {
            if(!implodeDecoder)
            {
              #ifdef COMPRESS_IMPLODE
              implodeDecoder = new CComObjectNoLock<NImplode::NDecoder::CCoder>;
              #else
              RINOK(implodeDecoder.CoCreateInstance(CLSID_CCompressImplodeDecoder));
              #endif
            }
            try
            {
              CComPtr<ICompressSetDecoderProperties> aCompressSetDecoderProperties;
              RINOK(implodeDecoder->QueryInterface(&aCompressSetDecoderProperties));

              BYTE aProperties[2] = 
              {
                itemInfo.IsImplodeBigDictionary() ? 1: 0,
                itemInfo.IsImplodeLiteralsOn() ? 1: 0
              };

              CComObjectNoLock<CSequentialInStreamImp> *anInStreamSpec = new 
                 CComObjectNoLock<CSequentialInStreamImp>;
              CComPtr<ISequentialInStream> anInStreamProperties(anInStreamSpec);
              anInStreamSpec->Init((const BYTE *)aProperties, 2);
              RINOK(aCompressSetDecoderProperties->SetDecoderProperties(anInStreamProperties));

              HRESULT result;
              if (itemInfo.IsEncrypted())
              {
                if (!mixerCoder || mixerCoderMethod != itemInfo.CompressionMethod)
                {
                  mixerCoder.Release();
                  mixerCoderSpec = new CComObjectNoLock<CCoderMixer>;
                  mixerCoder = mixerCoderSpec;
                  mixerCoderSpec->AddCoder(cryptoDecoder);
                  mixerCoderSpec->AddCoder(implodeDecoder);
                  mixerCoderSpec->FinishAddingCoders();
                  mixerCoderMethod = itemInfo.CompressionMethod;
                }
                mixerCoderSpec->ReInit();
                mixerCoderSpec->SetCoderInfo(0, &currentItemPacked, NULL);
                mixerCoderSpec->SetCoderInfo(1, NULL, &currentItemUnPacked);
                mixerCoderSpec->SetProgressCoderIndex(1);
                result = mixerCoder->Code(anInStream, outStream, 
                    NULL, NULL, compressProgress);
              }   
              else
              {
                result = implodeDecoder->Code(anInStream, outStream,
                    NULL, &currentItemUnPacked, compressProgress);
              }
              if (result == S_FALSE)
                throw "data error";
              if (result != S_OK)
                return result;
            }
            catch(...)
            {
              outStream.Release();
              RINOK(extractCallback->SetOperationResult(
                  NArchive::NExtract::NOperationResult::kDataError));
              continue;
            }
            break;
          }
        case NFileHeader::NCompressionMethod::kDeflated:
        case NFileHeader::NCompressionMethod::kDeflated64:
          {
            bool deflate64Mode = itemInfo.CompressionMethod == NFileHeader::NCompressionMethod::kDeflated64;
            if (deflate64Mode)
            {
              if(!deflate64Decoder)
              {
                #ifdef COMPRESS_DEFLATE64
                deflate64Decoder = new CComObjectNoLock<NDeflate::NDecoder::CCOMCoder64>;
                #else
                RINOK(deflate64Decoder.CoCreateInstance(CLSID_CCompressDeflate64Decoder));
                #endif
              }
            }
            else
            {
              if(!deflateDecoder)
              {
                #ifdef COMPRESS_DEFLATE
                deflateDecoder = new CComObjectNoLock<NDeflate::NDecoder::CCOMCoder>;
                #else
                RINOK(deflateDecoder.CoCreateInstance(CLSID_CCompressDeflateDecoder));
                #endif
              }
            }

            try
            {
              HRESULT result;
              if (itemInfo.IsEncrypted())
              {
                if (!mixerCoder || mixerCoderMethod != itemInfo.CompressionMethod)
                {
                  mixerCoder.Release();
                  mixerCoderSpec = new CComObjectNoLock<CCoderMixer>;
                  mixerCoder = mixerCoderSpec;
                  mixerCoderSpec->AddCoder(cryptoDecoder);
                  if (deflate64Mode)
                    mixerCoderSpec->AddCoder(deflate64Decoder);
                  else
                    mixerCoderSpec->AddCoder(deflateDecoder);
                  mixerCoderSpec->FinishAddingCoders();
                  mixerCoderMethod = itemInfo.CompressionMethod;
                }
                mixerCoderSpec->ReInit();
                mixerCoderSpec->SetCoderInfo(1, NULL, &currentItemUnPacked);
                mixerCoderSpec->SetProgressCoderIndex(1);
                result = mixerCoder->Code(anInStream, outStream, 
                    NULL, NULL, compressProgress);
              }   
              else
              {
                if (deflate64Mode)
                  result = deflate64Decoder->Code(anInStream, outStream,
                      NULL, &currentItemUnPacked, compressProgress);
                else
                  result = deflateDecoder->Code(anInStream, outStream,
                      NULL, &currentItemUnPacked, compressProgress);
              }
              if (result == S_FALSE)
                throw "data error";
              if (result != S_OK)
                return result;
            }
            catch(...)
            {
              outStream.Release();
              RINOK(extractCallback->SetOperationResult(
                  NArchive::NExtract::NOperationResult::kDataError));
              continue;
            }
            break;
          }
        default:
            RINOK(extractCallback->SetOperationResult(
                NArchive::NExtract::NOperationResult::kUnSupportedMethod));
            continue;
      }
      bool crcOK = outStreamSpec->GetCRC() == itemInfo.FileCRC;
      outStream.Release();
      RINOK(extractCallback->SetOperationResult(crcOK ? NArchive::NExtract::NOperationResult::kOK :
          NArchive::NExtract::NOperationResult::kCRCError))
    }
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CZipHandler::ExtractAllItems(INT32 testMode,
      IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  CRecordVector<UINT32> indices;
  for(int i = 0; i < m_Items.Size(); i++)
    indices.Add(i);
  return Extract(&indices.Front(), m_Items.Size(), testMode, extractCallback);
  COM_TRY_END
}

}}
