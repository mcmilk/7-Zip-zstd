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

  { NULL, kpidComment, VT_BOOL},
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

STDMETHODIMP CGZipHandler::GetNumberOfItems(UINT32 *aNumItems)
{
  COM_TRY_BEGIN
  *aNumItems = 1;
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
      FILETIME anUTCFileTime;
      if (m_Item.Time != 0)
        NTime::UnixTimeToFileTime(m_Item.Time, anUTCFileTime);
      else
        anUTCFileTime.dwLowDateTime = anUTCFileTime.dwHighDateTime = 0;
      propVariant = anUTCFileTime;
      break;
    }
    case kpidSize:
      propVariant = UINT64(m_Item.UnPackSize32);
      break;
    case kpidPackedSize:
      propVariant = m_Item.PackSize;
      break;
    case kpidComment:
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

STDMETHODIMP CGZipHandler::Open(IInStream *aStream, 
    const UINT64 *aMaxCheckStartPosition,
    IOpenArchive2CallBack *anOpenArchiveCallBack)
{
  COM_TRY_BEGIN
  try
  {
    CInArchive anArchive;
    RETURN_IF_NOT_S_OK(aStream->Seek(0, STREAM_SEEK_CUR, &m_StreamStartPosition));
    RETURN_IF_NOT_S_OK(anArchive.ReadHeader(aStream, m_Item));
    UINT64 aNewPosition;
    RETURN_IF_NOT_S_OK(aStream->Seek(-8, STREAM_SEEK_END, &aNewPosition));
    m_Item.PackSize = aNewPosition - anArchive.GetPosition();
    UINT32 aCRC, anUnpackSize32;
    if (anArchive.ReadPostInfo(aStream, aCRC, anUnpackSize32) != S_OK)
      return S_FALSE;
    m_Stream = aStream;
    m_Item.UnPackSize32 = anUnpackSize32;
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

//////////////////////////////////////
// CGZipHandler::DecompressItems

HRESULT SafeRead(IInStream *aStream, void *aData, UINT32 aSize)
{
  UINT32 aProcessedSizeReal;
  RETURN_IF_NOT_S_OK(aStream->Read(aData, aSize, &aProcessedSizeReal));
  if(aProcessedSizeReal != aSize)
    return E_FAIL;
  return S_OK;
}


STDMETHODIMP CGZipHandler::Extract(const UINT32* anIndexes, UINT32 aNumItems,
    INT32 _aTestMode, IExtractCallback200 *anExtractCallBack)
{
  COM_TRY_BEGIN
  if (aNumItems == 0)
    return S_OK;
  if (aNumItems != 1)
    return E_INVALIDARG;
  if (anIndexes[0] != 0)
    return E_INVALIDARG;

  bool aTestMode = (_aTestMode != 0);
  UINT64 aTotalUnPacked = 0, aTotalPacked = 0;

  aTotalUnPacked += m_Item.UnPackSize32;
  aTotalPacked += m_Item.PackSize;

  anExtractCallBack->SetTotal(aTotalUnPacked);

  UINT64 aCurrentTotalUnPacked = 0, aCurrentTotalPacked = 0;
  
  RETURN_IF_NOT_S_OK(anExtractCallBack->SetCompleted(&aCurrentTotalUnPacked));
  CComPtr<ISequentialOutStream> aRealOutStream;
  INT32 anAskMode;
  anAskMode = aTestMode ? NArchiveHandler::NExtract::NAskMode::kTest :
  NArchiveHandler::NExtract::NAskMode::kExtract;
  
  RETURN_IF_NOT_S_OK(anExtractCallBack->Extract(0, &aRealOutStream, anAskMode));
  
  
  if(!aTestMode && !aRealOutStream)
  {
    return S_OK;
  }

  anExtractCallBack->PrepareOperation(anAskMode);

  CComObjectNoLock<COutStreamWithCRC> *anOutStreamSpec = 
    new CComObjectNoLock<COutStreamWithCRC>;
  CComPtr<ISequentialOutStream> anOutStream(anOutStreamSpec);
  anOutStreamSpec->Init(aRealOutStream);
  aRealOutStream.Release();

  CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
  CComPtr<ICompressProgressInfo> aProgress = aLocalProgressSpec;
  aLocalProgressSpec->Init(anExtractCallBack, false);
  
  
  CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
    new  CComObjectNoLock<CLocalCompressProgressInfo>;
  CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;
  aLocalCompressProgressSpec->Init(aProgress, 
    &aCurrentTotalPacked,
    &aCurrentTotalUnPacked);

  CComPtr<ICompressCoder> aDeflateDecoder;
  bool aFirstItem = true;
  RETURN_IF_NOT_S_OK(m_Stream->Seek(m_StreamStartPosition, STREAM_SEEK_SET, NULL));
  while(true)
  {
    CInArchive anArchive;
    CItemInfoEx anItemInfo;
    HRESULT aResult = anArchive.ReadHeader(m_Stream, anItemInfo);
    if (aResult != S_OK)
    {
      if (aFirstItem)
        return E_FAIL;
      else
      {
        anOutStream.Release();
        RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kOK))
        return S_OK;
      }
    }
    aFirstItem = false;
    RETURN_IF_NOT_S_OK(m_Stream->Seek(anItemInfo.DataPosition, STREAM_SEEK_SET, NULL));

    anOutStreamSpec->InitCRC();

    switch(m_Item.CompressionMethod)
    {
      case NCompressionMethod::kDeflated:
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
          HRESULT aResult = aDeflateDecoder->Code(m_Stream, anOutStream, NULL, NULL, aCompressProgress);
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
          return S_OK;
        }
        break;
      }
    default:
      anOutStream.Release();
      RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(
        NArchiveHandler::NExtract::NOperationResult::kUnSupportedMethod));
      return S_OK;
    }
    CComPtr<IGetInStreamProcessedSize> aGetInStreamProcessedSize;
    aDeflateDecoder.QueryInterface(&aGetInStreamProcessedSize);
    UINT64 aPackSize;
    RETURN_IF_NOT_S_OK(aGetInStreamProcessedSize->GetInStreamProcessedSize(&aPackSize));
    RETURN_IF_NOT_S_OK(m_Stream->Seek(anItemInfo.DataPosition + aPackSize, STREAM_SEEK_SET, NULL));
    
    UINT32 aCRC, anUnpackSize32;
    if (anArchive.ReadPostInfo(m_Stream, aCRC, anUnpackSize32) != S_OK)
      return E_FAIL;

    if((anOutStreamSpec->GetCRC() != aCRC))
    {
      RETURN_IF_NOT_S_OK(anExtractCallBack->OperationResult(NArchiveHandler::NExtract::NOperationResult::kCRCError))
      return S_OK;
    }
  }
  COM_TRY_END
}

STDMETHODIMP CGZipHandler::ExtractAllItems(INT32 aTestMode,
      IExtractCallback200 *anExtractCallBack)
{
  UINT32 index = 0;
  return Extract(&index, 1, aTestMode, anExtractCallBack);
}

}}
