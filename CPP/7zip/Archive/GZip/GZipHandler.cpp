// GZipHandler.cpp

#include "StdAfx.h"

#include "GZipHandler.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../../ICoder.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/CreateCoder.h"
#include "../Common/OutStreamWithCRC.h"

using namespace NWindows;

namespace NArchive {
namespace NGZip {

static const CMethodId kMethodId_Deflate = 0x040108;

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

STATPROPSTG kProps[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},

  { NULL, kpidLastWriteTime, VT_FILETIME},
  // { NULL, kpidMethod, VT_UI1},
  { NULL, kpidHostOS, VT_BSTR},
    
  { NULL, kpidCRC, VT_UI4}
  // { L"Extra", kpidExtraIsPresent, VT_BOOL}
  // { L"Extra flags", kpidExtraFlags, VT_UI1},
  // { L"Is Text", kpidIsText, VT_BOOL},
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = 1;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 /* index */, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidPath:
      if (m_Item.NameIsPresent())
        prop = MultiByteToUnicodeString(m_Item.Name, CP_ACP);
      break;
    case kpidLastWriteTime:
    {
      FILETIME utcTime;
      if (m_Item.Time != 0)
      {
        NTime::UnixTimeToFileTime((UInt32)m_Item.Time, utcTime);
        prop = utcTime;
      }
      else
      {
        // utcTime.dwLowDateTime = utcTime.dwHighDateTime = 0;
        // prop = utcTime;
      }
      break;
    }
    case kpidSize:
      prop = UInt64(m_Item.UnPackSize32);
      break;
    case kpidPackedSize:
      prop = m_PackSize;
      break;
    case kpidCommented:
      prop = m_Item.CommentIsPresent();
      break;
    case kpidHostOS:
      prop = (m_Item.HostOS < kNumHostOSes) ?
          kHostOS[m_Item.HostOS] : kUnknownOS;
      break;
    case kpidMethod:
      prop = m_Item.CompressionMethod;
      break;
    case kpidCRC:
        prop = m_Item.FileCRC;
      break;
    /*
    case kpidExtraFlags:
      prop = m_Item.ExtraFlags;
      break;
    case kpidIsText:
      prop = m_Item.IsText();
      break;
    case kpidExtraIsPresent:
      prop = m_Item.ExtraFieldIsPresent();
      break;
    */
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Open(IInStream *inStream, 
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */)
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

  UInt64 currentTotalPacked = 0;
  
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
  outStreamSpec->SetStream(realOutStream);
  outStreamSpec->Init();
  realOutStream.Release();

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, true);

  CMyComPtr<ICompressCoder> deflateDecoder;
  bool firstItem = true;
  RINOK(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL));
  Int32 opRes;
  for (;;)
  {
    lps->InSize = currentTotalPacked;
    lps->OutSize = outStreamSpec->GetSize();

    CInArchive archive;
    CItem item;
    HRESULT result = archive.ReadHeader(m_Stream, item);
    if (result != S_OK)
    {
      if (firstItem)
        return E_FAIL;
      opRes = NArchive::NExtract::NOperationResult::kOK;
      break;
    }
    firstItem = false;

    UInt64 dataStartPos;
    RINOK(m_Stream->Seek(0, STREAM_SEEK_CUR, &dataStartPos));

    outStreamSpec->InitCRC();

    if (item.CompressionMethod != NFileHeader::NCompressionMethod::kDeflate)
    {
      opRes = NArchive::NExtract::NOperationResult::kUnSupportedMethod;
      break;
    }

    if (!deflateDecoder)
    {
      RINOK(CreateCoder(
          EXTERNAL_CODECS_VARS
          kMethodId_Deflate, deflateDecoder, false));
      if (!deflateDecoder)
      {
        opRes = NArchive::NExtract::NOperationResult::kUnSupportedMethod;
        break;
      }
    }
    result = deflateDecoder->Code(m_Stream, outStream, NULL, NULL, progress);
    if (result != S_OK)
    {
      if (result != S_FALSE)
        return result;
      opRes = NArchive::NExtract::NOperationResult::kDataError;
      break;
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
      opRes = NArchive::NExtract::NOperationResult::kCRCError;
      break;
    }
  }
  outStream.Release();
  return extractCallback->SetOperationResult(opRes);
  COM_TRY_END
}

IMPL_ISetCompressCodecsInfo

}}
