// Handler.cpp

#include "StdAfx.h"

#include "Handler.h"

#include "Common/Defs.h"
#include "Common/CRC.h"
#include "Common/StringConvert.h"

#include "Interface/ProgressUtils.h"
#include "Interface/EnumStatProp.h"
#include "Interface/StreamObjects.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"
#include "Windows/COMTry.h"


#include "../Common/OutStreamWithCRC.h"

#include "../../../Compress/Interface/CompressInterface.h"

#ifdef COMPRESS_DEFLATE
#include "../../../Compress/LZ/Deflate/Decoder.h"
#else
// {23170F69-40C1-278B-0401-080000000000}
DEFINE_GUID(CLSID_CCompressDeflateDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00);
#endif

using namespace NWindows;
using namespace NArchive;

namespace NArchive {
namespace NGZip {

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

enum // PropID
{
  kpidExtraIsPresent = kpidUserDefined,
  kpidExtraFlags,
  kpidIsText
};

STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidLastWriteTime, VT_FILETIME},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},

  { NULL, kpidCommented, VT_BOOL},
  // { NULL, kpidMethod, VT_UI1},
  { NULL, kpidHostOS, VT_BSTR}
    
  // { NULL, kpidCRC, VT_UI4},
  // { L"Extra", kpidExtraIsPresent, VT_BOOL}
  // { L"Extra flags", kpidExtraFlags, VT_UI1},
  // { L"Is Text", kpidIsText, VT_BOOL},
};

STDMETHODIMP CGZipHandler::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  COM_TRY_BEGIN
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
  COM_TRY_END
}

STDMETHODIMP CGZipHandler::GetNumberOfItems(UINT32 *numItems)
{
  COM_TRY_BEGIN
  *numItems = 1;
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CGZipHandler::GetProperty(UINT32 index, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
  switch(propID)
  {
    case kpidPath:
      if (m_Item.NameIsPresent())
        propVariant = MultiByteToUnicodeString(m_Item.Name, CP_ACP);
      break;
    case kpidIsFolder:
      propVariant = false;
      break;
    case kpidLastWriteTime:
    {
      FILETIME utcTime;
      if (m_Item.Time != 0)
        NTime::UnixTimeToFileTime(m_Item.Time, utcTime);
      else
        utcTime.dwLowDateTime = utcTime.dwHighDateTime = 0;
      propVariant = utcTime;
      break;
    }
    case kpidSize:
      propVariant = UINT64(m_Item.UnPackSize32);
      break;
    case kpidPackedSize:
      propVariant = m_Item.PackSize;
      break;
    case kpidCommented:
      propVariant = m_Item.CommentIsPresent();
      break;
    case kpidHostOS:
      propVariant = (m_Item.HostOS < kNumHostOSes) ?
          kHostOS[m_Item.HostOS] : kUnknownOS;
      break;
    case kpidMethod:
      propVariant = m_Item.CompressionMethod;
      break;
    case kpidExtraFlags:
      propVariant = m_Item.ExtraFlags;
      break;
    case kpidIsText:
      propVariant = m_Item.IsText();
      break;
    case kpidExtraIsPresent:
      propVariant = m_Item.ExtraFieldIsPresent();
      break;
  }
  propVariant.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CGZipHandler::Open(IInStream *inStream, 
    const UINT64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  try
  {
    CInArchive archive;
    RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition));
    RINOK(archive.ReadHeader(inStream, m_Item));
    UINT64 newPosition;
    RINOK(inStream->Seek(-8, STREAM_SEEK_END, &newPosition));
    m_Item.PackSize = newPosition - archive.GetPosition();
    UINT32 crc, unpackSize32;
    if (archive.ReadPostInfo(inStream, crc, unpackSize32) != S_OK)
      return S_FALSE;
    m_Stream = inStream;
    m_Item.UnPackSize32 = unpackSize32;
  }
  catch(...)
  {
    return S_FALSE;
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CGZipHandler::Close()
{
  m_Stream.Release();
  return S_OK;
}

STDMETHODIMP CGZipHandler::Extract(const UINT32* indices, UINT32 numItems,
    INT32 _aTestMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != 1)
    return E_INVALIDARG;
  if (indices[0] != 0)
    return E_INVALIDARG;

  bool testMode = (_aTestMode != 0);
  UINT64 totalUnPacked = 0, totalPacked = 0;

  totalUnPacked += m_Item.UnPackSize32;
  totalPacked += m_Item.PackSize;

  extractCallback->SetTotal(totalUnPacked);

  UINT64 currentTotalUnPacked = 0, currentTotalPacked = 0;
  
  RINOK(extractCallback->SetCompleted(&currentTotalUnPacked));
  CComPtr<ISequentialOutStream> realOutStream;
  INT32 askMode;
  askMode = testMode ? NArchive::NExtract::NAskMode::kTest :
  NArchive::NExtract::NAskMode::kExtract;
  
  RINOK(extractCallback->GetStream(0, &realOutStream, askMode));
  
  
  if(!testMode && !realOutStream)
  {
    return S_OK;
  }

  extractCallback->PrepareOperation(askMode);

  CComObjectNoLock<COutStreamWithCRC> *outStreamSpec = 
    new CComObjectNoLock<COutStreamWithCRC>;
  CComPtr<ISequentialOutStream> outStream(outStreamSpec);
  outStreamSpec->Init(realOutStream);
  realOutStream.Release();

  CComObjectNoLock<CLocalProgress> *localProgressSpec = new  CComObjectNoLock<CLocalProgress>;
  CComPtr<ICompressProgressInfo> progress = localProgressSpec;
  localProgressSpec->Init(extractCallback, false);
  
  
  CComObjectNoLock<CLocalCompressProgressInfo> *localCompressProgressSpec = 
    new  CComObjectNoLock<CLocalCompressProgressInfo>;
  CComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
  localCompressProgressSpec->Init(progress, 
    &currentTotalPacked,
    &currentTotalUnPacked);

  CComPtr<ICompressCoder> deflateDecoder;
  bool firstItem = true;
  RINOK(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL));
  while(true)
  {
    CInArchive archive;
    CItemInfoEx itemInfo;
    HRESULT result = archive.ReadHeader(m_Stream, itemInfo);
    if (result != S_OK)
    {
      if (firstItem)
        return E_FAIL;
      else
      {
        outStream.Release();
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK))
        return S_OK;
      }
    }
    firstItem = false;
    RINOK(m_Stream->Seek(itemInfo.DataPosition, STREAM_SEEK_SET, NULL));

    outStreamSpec->InitCRC();

    switch(m_Item.CompressionMethod)
    {
      case NCompressionMethod::kDeflated:
      {
        if(!deflateDecoder)
        {
          #ifdef COMPRESS_DEFLATE
          deflateDecoder = new CComObjectNoLock<NDeflate::NDecoder::CCOMCoder>;
          #else
          RINOK(deflateDecoder.CoCreateInstance(CLSID_CCompressDeflateDecoder));
          #endif
        }
        try
        {
          HRESULT result = deflateDecoder->Code(m_Stream, outStream, NULL, NULL, compressProgress);
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
          return S_OK;
        }
        break;
      }
    default:
      outStream.Release();
      RINOK(extractCallback->SetOperationResult(
        NArchive::NExtract::NOperationResult::kUnSupportedMethod));
      return S_OK;
    }
    CComPtr<IGetInStreamProcessedSize> getInStreamProcessedSize;
    deflateDecoder.QueryInterface(&getInStreamProcessedSize);
    UINT64 packSize;
    RINOK(getInStreamProcessedSize->GetInStreamProcessedSize(&packSize));
    RINOK(m_Stream->Seek(itemInfo.DataPosition + packSize, STREAM_SEEK_SET, NULL));
    
    UINT32 crc, unpackSize32;
    if (archive.ReadPostInfo(m_Stream, crc, unpackSize32) != S_OK)
      return E_FAIL;

    if((outStreamSpec->GetCRC() != crc))
    {
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kCRCError))
      return S_OK;
    }
  }
  COM_TRY_END
}

STDMETHODIMP CGZipHandler::ExtractAllItems(INT32 testMode,
    IArchiveExtractCallback *extractCallback)
{
  UINT32 index = 0;
  return Extract(&index, 1, testMode, extractCallback);
}

}}
