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

#include "../Common/OutStreamWithCRC.h"
#include "../Common/CoderMixer.h"

#include "../../../Compress/Interface/CompressInterface.h"
#include "../../../Crypto/Cipher/Common/CipherInterface.h"
#include "../Common/FormatCryptoInterface.h"

#ifdef COMPRESS_DEFLATE
#include "../../../Compress/LZ/Deflate/Decoder.h"
#else
// {23170F69-40C1-278B-0401-080000000000}
DEFINE_GUID(CLSID_CCompressDeflateDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00);
#endif

#ifdef COMPRESS_IMPLODE
#include "../../../Compress/LZ/Implode/Decoder.h"
#else
// {23170F69-40C1-278B-0401-060000000000}
DEFINE_GUID(CLSID_CCompressImplodeDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00);
#endif

#ifdef CRYPTO_ZIP
#include "../../../Crypto/Cipher/Zip/Coder.h"
#else
// {23170F69-40C1-278A-1000-000250030000}
DEFINE_GUID(CLSID_CCryptoZipDecoder, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x02, 0x50, 0x03, 0x00, 0x00);
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
  { NULL, kpidComment, VT_BOOL},
    
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

STDMETHODIMP CZipHandler::GetNumberOfItems(UINT32 *aNumItems)
{
  *aNumItems = m_Items.Size();
  return S_OK;
}

STDMETHODIMP CZipHandler::GetProperty(UINT32 anIndex, PROPID aPropID,  PROPVARIANT *aValue)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
  const NArchive::NZip::CItemInfoEx &item = m_Items[anIndex];
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
  CComPtr<IOpenArchive2CallBack> m_OpenArchiveCallBack;
public:
  STDMETHOD(SetCompleted)(const UINT64 *aNumFiles);
  void Init(IOpenArchive2CallBack *anOpenArchiveCallBack)
    { m_OpenArchiveCallBack = anOpenArchiveCallBack; }
};

STDMETHODIMP CPropgressImp::SetCompleted(const UINT64 *aNumFiles)
{
  if (m_OpenArchiveCallBack)
    return m_OpenArchiveCallBack->SetCompleted(aNumFiles, NULL);
  return S_OK;
}

STDMETHODIMP CZipHandler::Open(IInStream *aStream, 
    const UINT64 *aMaxCheckStartPosition, IOpenArchive2CallBack *anOpenArchiveCallBack)
{
  COM_TRY_BEGIN
  // try
  {
    if(!m_Archive.Open(aStream, aMaxCheckStartPosition))
      return S_FALSE;
    m_ArchiveIsOpen = true;
    m_Items.Clear();
    if (anOpenArchiveCallBack != NULL)
    {
      RETURN_IF_NOT_S_OK(anOpenArchiveCallBack->SetTotal(NULL, NULL));
    }
    CPropgressImp aPropgressImp;
    aPropgressImp.Init(anOpenArchiveCallBack);
    RETURN_IF_NOT_S_OK(m_Archive.ReadHeaders(m_Items, &aPropgressImp));
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

STDMETHODIMP CZipHandler::Extract(const UINT32* anIndexes, UINT32 aNumItems,
    INT32 _aTestMode, IExtractCallback200 *_anExtractCallBack)
{
  COM_TRY_BEGIN
  CComPtr<ICryptoGetTextPassword> getTextPassword;
  bool aTestMode = (_aTestMode != 0);
  CComPtr<IExtractCallback200> anExtractCallBack = _anExtractCallBack;
  UINT64 aTotalUnPacked = 0, aTotalPacked = 0;
  if(aNumItems == 0)
    return S_OK;
  int i;
  for(i = 0; i < aNumItems; i++)
  {
    const CItemInfoEx &anItemInfo = m_Items[anIndexes[i]];
    aTotalUnPacked += anItemInfo.UnPackSize;
    aTotalPacked += anItemInfo.PackSize;
  }
  anExtractCallBack->SetTotal(aTotalUnPacked);

  UINT64 aCurrentTotalUnPacked = 0, aCurrentTotalPacked = 0;
  UINT64 aCurrentItemUnPacked, aCurrentItemPacked;
  
  CComObjectNoLock<NCompression::CCopyCoder> *aCopyCoderSpec = NULL;
  CComPtr<ICompressCoder> aDeflateDecoder;
  CComPtr<ICompressCoder> anImplodeDecoder;
  CComPtr<ICompressCoder> aCopyCoder;
  CComPtr<ICompressCoder> cryptoDecoder;
  CComObjectNoLock<CCoderMixer> *mixerCoderSpec;
  CComPtr<ICompressCoder> mixerCoder;

  UINT16 mixerCoderMethod;

  for(i = 0; i < aNumItems; i++, aCurrentTotalUnPacked += aCurrentItemUnPacked,
      aCurrentTotalPacked += aCurrentItemPacked)
  {
    aCurrentItemUnPacked = 0;
    aCurrentItemPacked = 0;

    RETURN_IF_NOT_S_OK(anExtractCallBack->SetCompleted(&aCurrentTotalUnPacked));
    CComPtr<ISequentialOutStream> aRealOutStream;
    INT32 anAskMode;
    anAskMode = aTestMode ? NArchiveHandler::NExtract::NAskMode::kTest :
        NArchiveHandler::NExtract::NAskMode::kExtract;
    INT32 anIndex = anIndexes[i];
    const CItemInfoEx &anItemInfo = m_Items[anIndex];
    RETURN_IF_NOT_S_OK(anExtractCallBack->Extract(anIndex, &aRealOutStream, anAskMode));

    if(anItemInfo.IsDirectory() || anItemInfo.IgnoreItem())
    {
      // if (!aTestMode)
      {
        RETURN_IF_NOT_S_OK(anExtractCallBack->PrepareOperation(anAskMode));
        RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK));
      }
      continue;
    }

    if (!aTestMode && (!aRealOutStream)) 
      continue;

    RETURN_IF_NOT_S_OK(anExtractCallBack->PrepareOperation(anAskMode));
    aCurrentItemUnPacked = anItemInfo.UnPackSize;
    aCurrentItemPacked = anItemInfo.PackSize;

    {
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
          &aCurrentTotalPacked,
          &aCurrentTotalUnPacked);

      if (anItemInfo.IsEncrypted())
      {
        if (!cryptoDecoder)
        {
          #ifdef CRYPTO_ZIP
          cryptoDecoder = new CComObjectNoLock<NCrypto::NZip::CDecoder>;
          #else
          RETURN_IF_NOT_S_OK(cryptoDecoder.CoCreateInstance(CLSID_CCryptoZipDecoder));
          #endif
        }
        CComPtr<ICryptoSetPassword> cryptoSetPassword;
        RETURN_IF_NOT_S_OK(cryptoDecoder.QueryInterface(&cryptoSetPassword));

        if (!getTextPassword)
          anExtractCallBack.QueryInterface(&getTextPassword);

        if (getTextPassword)
        {
          CComBSTR password;
          RETURN_IF_NOT_S_OK(getTextPassword->CryptoGetTextPassword(&password));
          AString anOemPassword = UnicodeStringToMultiByte(
              (const wchar_t *)password, CP_OEMCP);
          RETURN_IF_NOT_S_OK(cryptoSetPassword->CryptoSetPassword(
              (const BYTE *)(const char *)anOemPassword, anOemPassword.Length()));
        }
        else
        {
          RETURN_IF_NOT_S_OK(cryptoSetPassword->CryptoSetPassword(0, 0));
        }
      }

      switch(anItemInfo.CompressionMethod)
      {
        case NFileHeader::NCompressionMethod::kStored:
          {
            if(aCopyCoderSpec == NULL)
            {
              aCopyCoderSpec = new CComObjectNoLock<NCompression::CCopyCoder>;
              aCopyCoder = aCopyCoderSpec;
            }
            try
            {
              if (anItemInfo.IsEncrypted())
              {
                if (!mixerCoder || mixerCoderMethod != anItemInfo.CompressionMethod)
                {
                  mixerCoder.Release();
                  mixerCoderSpec = new CComObjectNoLock<CCoderMixer>;
                  mixerCoder = mixerCoderSpec;
                  mixerCoderSpec->AddCoder(cryptoDecoder);
                  mixerCoderSpec->AddCoder(aCopyCoder);
                  mixerCoderSpec->FinishAddingCoders();
                  mixerCoderMethod = anItemInfo.CompressionMethod;
                }
                mixerCoderSpec->ReInit();
                mixerCoderSpec->SetCoderInfo(0, NULL, &aCurrentItemUnPacked);
                mixerCoderSpec->SetCoderInfo(1, NULL, NULL);
                mixerCoderSpec->SetProgressCoderIndex(1);
                RETURN_IF_NOT_S_OK(mixerCoder->Code(anInStream, anOutStream,
                  NULL, NULL, aCompressProgress));
              }
              else
              {
                RETURN_IF_NOT_S_OK(aCopyCoder->Code(anInStream, anOutStream,
                    NULL, NULL, aCompressProgress));
              }
            }
            catch(...)
            {
              anOutStream.Release();
              RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
                  NArchiveHandler::NExtract::NOperationResult::kDataError));
              continue;
            }
            break;
          }
        case NFileHeader::NCompressionMethod::kImploded:
          {
            if(!anImplodeDecoder)
            {
              #ifdef COMPRESS_IMPLODE
              anImplodeDecoder = new CComObjectNoLock<NImplode::NDecoder::CCoder>;
              #else
              RETURN_IF_NOT_S_OK(anImplodeDecoder.CoCreateInstance(CLSID_CCompressImplodeDecoder));
              #endif
            }
            try
            {
              CComPtr<ICompressSetDecoderProperties> aCompressSetDecoderProperties;
              RETURN_IF_NOT_S_OK(anImplodeDecoder->QueryInterface(&aCompressSetDecoderProperties));

              BYTE aProperties[2] = 
              {
                anItemInfo.IsImplodeBigDictionary() ? 1: 0,
                anItemInfo.IsImplodeLiteralsOn() ? 1: 0
              };

              CComObjectNoLock<CSequentialInStreamImp> *anInStreamSpec = new 
                 CComObjectNoLock<CSequentialInStreamImp>;
              CComPtr<ISequentialInStream> anInStreamProperties(anInStreamSpec);
              anInStreamSpec->Init((const BYTE *)aProperties, 2);
              RETURN_IF_NOT_S_OK(aCompressSetDecoderProperties->SetDecoderProperties(anInStreamProperties));

              HRESULT aResult;
              if (anItemInfo.IsEncrypted())
              {
                if (!mixerCoder || mixerCoderMethod != anItemInfo.CompressionMethod)
                {
                  mixerCoder.Release();
                  mixerCoderSpec = new CComObjectNoLock<CCoderMixer>;
                  mixerCoder = mixerCoderSpec;
                  mixerCoderSpec->AddCoder(cryptoDecoder);
                  mixerCoderSpec->AddCoder(anImplodeDecoder);
                  mixerCoderSpec->FinishAddingCoders();
                  mixerCoderMethod = anItemInfo.CompressionMethod;
                }
                mixerCoderSpec->ReInit();
                mixerCoderSpec->SetCoderInfo(1, NULL, &aCurrentItemUnPacked);
                mixerCoderSpec->SetProgressCoderIndex(1);
                aResult = mixerCoder->Code(anInStream, anOutStream, 
                    NULL, NULL, aCompressProgress);
              }   
              else
              {
                aResult = anImplodeDecoder->Code(anInStream, anOutStream,
                    NULL, &aCurrentItemUnPacked, aCompressProgress);
              }
              if (aResult == S_FALSE)
                throw "data error";
              if (aResult != S_OK)
                return aResult;
            }
            catch(...)
            {
              anOutStream.Release();
              RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
                  NArchiveHandler::NExtract::NOperationResult::kDataError));
              continue;
            }
            break;
          }
        case NFileHeader::NCompressionMethod::kDeflated:
          {
            if(!aDeflateDecoder)
            {
              #ifdef COMPRESS_DEFLATE
              aDeflateDecoder = new CComObjectNoLock<NDeflate::NDecoder::CCoder>;
              #else
              RETURN_IF_NOT_S_OK(aDeflateDecoder.CoCreateInstance(CLSID_CCompressDeflateDecoder));
              #endif
            }
            try
            {
              HRESULT aResult;
              if (anItemInfo.IsEncrypted())
              {
                if (!mixerCoder || mixerCoderMethod != anItemInfo.CompressionMethod)
                {
                  mixerCoder.Release();
                  mixerCoderSpec = new CComObjectNoLock<CCoderMixer>;
                  mixerCoder = mixerCoderSpec;
                  mixerCoderSpec->AddCoder(cryptoDecoder);
                  mixerCoderSpec->AddCoder(aDeflateDecoder);
                  mixerCoderSpec->FinishAddingCoders();
                  mixerCoderMethod = anItemInfo.CompressionMethod;
                }
                mixerCoderSpec->ReInit();
                mixerCoderSpec->SetCoderInfo(1, NULL, &aCurrentItemUnPacked);
                mixerCoderSpec->SetProgressCoderIndex(1);
                aResult = mixerCoder->Code(anInStream, anOutStream, 
                    NULL, NULL, aCompressProgress);
              }   
              else
              {
                aResult = aDeflateDecoder->Code(anInStream, anOutStream,
                    NULL, &aCurrentItemUnPacked, aCompressProgress);
              }
              if (aResult == S_FALSE)
                throw "data error";
              if (aResult != S_OK)
                return aResult;
            }
            catch(...)
            {
              anOutStream.Release();
              RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
                  NArchiveHandler::NExtract::NOperationResult::kDataError));
              continue;
            }
            break;
          }
        default:
            RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
                NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod));
            continue;
      }
      bool aCRC_Ok = anOutStreamSpec->GetCRC() == anItemInfo.FileCRC;
      anOutStream.Release();
      if(aCRC_Ok)
        RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK))
      else
        RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kCRCError))
    }
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CZipHandler::ExtractAllItems(INT32 aTestMode,
      IExtractCallback200 *anExtractCallBack)
{
  COM_TRY_BEGIN
  CRecordVector<UINT32> anIndexes;
  for(int i = 0; i < m_Items.Size(); i++)
    anIndexes.Add(i);
  return Extract(&anIndexes.Front(), m_Items.Size(), aTestMode, anExtractCallBack);
  COM_TRY_END
}

}}
