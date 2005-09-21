// GZipHandler.cpp

#include "StdAfx.h"

#include "GZipHandler.h"

#include "Common/Defs.h"
#include "Common/CRC.h"
#include "Common/StringConvert.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../../ICoder.h"
#include "../../Common/ProgressUtils.h"
#include "../Common/OutStreamWithCRC.h"

#ifdef COMPRESS_DEFLATE
#include "../../Compress/Deflate/DeflateDecoder.h"
#else
// {23170F69-40C1-278B-0401-080000000000}
DEFINE_GUID(CLSID_CCompressDeflateDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00);
#include "../Common/CoderLoader.h"
extern CSysString GetDeflateCodecPath();
#endif

using namespace NWindows;

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

static const int kNumHostOSes = sizeof(kHostOS) / sizeof(kHostOS[0]);

static const wchar_t *kUnknownOS = L"Unknown";

/*
enum // PropID
{
  kpidExtraIsPresent = kpidUserDefined,
  kpidExtraFlags,
  kpidIsText
};
*/

STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  // { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},

  { NULL, kpidLastWriteTime, VT_FILETIME},
  // { NULL, kpidCommented, VT_BOOL},
  // { NULL, kpidMethod, VT_UI1},
  { NULL, kpidHostOS, VT_BSTR},
    
  { NULL, kpidCRC, VT_UI4}
  // { L"Extra", kpidExtraIsPresent, VT_BOOL}
  // { L"Extra flags", kpidExtraFlags, VT_UI1},
  // { L"Is Text", kpidIsText, VT_BOOL},
};

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
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
  const STATPROPSTG &prop = kProperties[index];
  *propID = prop.propid;
  *varType = prop.vt;
  *name = 0;
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfArchiveProperties(UInt32 *numProperties)
{
  *numProperties = 0;
  return S_OK;
}

STDMETHODIMP CHandler::GetArchivePropertyInfo(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  return E_NOTIMPL;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  COM_TRY_BEGIN
  *numItems = 1;
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID,  PROPVARIANT *value)
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
      {
        NTime::UnixTimeToFileTime((UInt32)m_Item.Time, utcTime);
        propVariant = utcTime;
      }
      else
      {
        // utcTime.dwLowDateTime = utcTime.dwHighDateTime = 0;
        // propVariant = utcTime;
      }
      break;
    }
    case kpidSize:
      propVariant = UInt64(m_Item.UnPackSize32);
      break;
    case kpidPackedSize:
      propVariant = m_PackSize;
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
    case kpidCRC:
        propVariant = m_Item.FileCRC;
      break;
    /*
    case kpidExtraFlags:
      propVariant = m_Item.ExtraFlags;
      break;
    case kpidIsText:
      propVariant = m_Item.IsText();
      break;
    case kpidExtraIsPresent:
      propVariant = m_Item.ExtraFieldIsPresent();
      break;
    */
  }
  propVariant.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Open(IInStream *inStream, 
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  try
  {
    CInArchive archive;
    RINOK(inStream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition));
    RINOK(archive.ReadHeader(inStream, m_Item));
    m_DataOffset = archive.GetOffset(); 
    UInt64 newPosition;
    RINOK(inStream->Seek(-8, STREAM_SEEK_END, &newPosition));
    m_PackSize = newPosition - (m_StreamStartPosition + m_DataOffset);
    if (archive.ReadPostHeader(inStream, m_Item) != S_OK)
      return S_FALSE;
    m_Stream = inStream;
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
  m_Stream.Release();
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 _aTestMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == UInt32(-1));
  if (!allFilesMode)
  {
    if (numItems == 0)
      return S_OK;
    if (numItems != 1)
      return E_INVALIDARG;
    if (indices[0] != 0)
      return E_INVALIDARG;
  }

  bool testMode = (_aTestMode != 0);

  extractCallback->SetTotal(m_PackSize);

  UInt64 currentTotalUnPacked = 0, currentTotalPacked = 0;
  
  RINOK(extractCallback->SetCompleted(&currentTotalPacked));
  CMyComPtr<ISequentialOutStream> realOutStream;
  Int32 askMode;
  askMode = testMode ? NArchive::NExtract::NAskMode::kTest :
      NArchive::NExtract::NAskMode::kExtract;
  
  RINOK(extractCallback->GetStream(0, &realOutStream, askMode));
  
  if(!testMode && !realOutStream)
    return S_OK;

  extractCallback->PrepareOperation(askMode);

  COutStreamWithCRC *outStreamSpec = new COutStreamWithCRC;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
  outStreamSpec->Init(realOutStream);
  realOutStream.Release();

  CLocalProgress *localProgressSpec = new  CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = localProgressSpec;
  localProgressSpec->Init(extractCallback, true);
  
  CLocalCompressProgressInfo *localCompressProgressSpec = 
      new CLocalCompressProgressInfo;
  CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;

  #ifndef COMPRESS_DEFLATE
  CCoderLibrary lib;
  #endif
  CMyComPtr<ICompressCoder> deflateDecoder;
  bool firstItem = true;
  RINOK(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL));
  while(true)
  {
    localCompressProgressSpec->Init(progress, 
      &currentTotalPacked,
      &currentTotalUnPacked);

    CInArchive archive;
    CItem item;
    HRESULT result = archive.ReadHeader(m_Stream, item);
    if (result != S_OK)
    {
      if (firstItem)
        return E_FAIL;
      outStream.Release();
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK))
      return S_OK;
    }
    firstItem = false;

    UInt64 dataStartPos;
    RINOK(m_Stream->Seek(0, STREAM_SEEK_CUR, &dataStartPos));

    outStreamSpec->InitCRC();

    switch(m_Item.CompressionMethod)
    {
      case NFileHeader::NCompressionMethod::kDeflate:
      {
        if(!deflateDecoder)
        {
          #ifdef COMPRESS_DEFLATE
          deflateDecoder = new NCompress::NDeflate::NDecoder::CCOMCoder;
          #else
          RINOK(lib.LoadAndCreateCoder(GetDeflateCodecPath(), 
              CLSID_CCompressDeflateDecoder, &deflateDecoder));
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
    CMyComPtr<ICompressGetInStreamProcessedSize> getInStreamProcessedSize;
    RINOK(deflateDecoder.QueryInterface(IID_ICompressGetInStreamProcessedSize, 
        &getInStreamProcessedSize));
    UInt64 packSize;
    RINOK(getInStreamProcessedSize->GetInStreamProcessedSize(&packSize));
    UInt64 pos;
    RINOK(m_Stream->Seek(dataStartPos + packSize, STREAM_SEEK_SET, &pos));

    currentTotalPacked = pos - m_StreamStartPosition;
    
    CItem postItem;
    if (archive.ReadPostHeader(m_Stream, postItem) != S_OK)
      return E_FAIL;
    if((outStreamSpec->GetCRC() != postItem.FileCRC))
    {
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kCRCError))
      return S_OK;
    }
  }
  COM_TRY_END
}

}}
