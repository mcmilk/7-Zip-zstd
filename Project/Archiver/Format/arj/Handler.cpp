// arj/Handler.cpp

#include "StdAfx.h"

#include "Handler.h"
#include "Interface/StreamObjects.h"

#include "Common/Defs.h"
#include "Common/CRC.h"
#include "Common/StringConvert.h"

#include "Windows/Time.h"
#include "Windows/PropVariant.h"
#include "Windows/COMTry.h"

#include "Compression/CopyCoder.h"

#include "Archive/Common/ItemNameUtils.h"

#include "Interface/ProgressUtils.h"
#include "Interface/LimitedStreams.h"
#include "Interface/EnumStatProp.h"

#include "../Common/OutStreamWithCRC.h"

#include "../../../Compress/Interface/CompressInterface.h"
#include "../../../Compress/LZ/arj/Decoder1.h"
#include "../../../Compress/LZ/arj/Decoder2.h"

#include "../../../Crypto/Cipher/Common/CipherInterface.h"


using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NArj{

const wchar_t *kHostOS[] = 
{
  L"MSDOS",
  L"PRIMOS",
  L"Unix",
  L"AMIGA",
  L"Mac",
  L"OS/2",
  L"APPLE GS",
  L"Atari ST",
  L"Next",
  L"VAX VMS",
  L"WIN95"
};


const kNumHostOSes = sizeof(kHostOS) / sizeof(kHostOS[0]);

const wchar_t *kUnknownOS = L"Unknown";


/*
enum // PropID
{
  kpidHostOS = kpidUserDefined,
  kpidUnPackVersion, 
  kpidMethod, 
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
  // { NULL, kpidComment, VT_BOOL},
    
  { NULL, kpidCRC, VT_UI4},

  { NULL, kpidMethod, VT_UI1},
  { NULL, kpidHostOS, VT_BSTR}

  // { L"UnPack Version", kpidUnPackVersion, VT_UI1},
  // { L"Method", kpidMethod, VT_UI1},
  // { L"Host OS", kpidHostOS, VT_BSTR}
};


CHandler::CHandler()
{}

STDMETHODIMP CHandler::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  COM_TRY_BEGIN
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
  COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UINT32 *numItems)
{
  *numItems = _items.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UINT32 index, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
  const CItemInfoEx &item = _items[index];
  switch(propID)
  {
    case kpidPath:
      propVariant = 
      NItemName::GetOSName(MultiByteToUnicodeString(item.Name, CP_OEMCP));
      /*
                     NItemName::GetOSName2(
          MultiByteToUnicodeString(item.Name, item.GetCodePage()));
          */
      break;
    case kpidIsFolder:
      propVariant = item.IsDirectory();
      break;
    case kpidSize:
      propVariant = item.Size;
      break;
    case kpidPackedSize:
      propVariant = item.PackSize;
      break;
    case kpidLastWriteTime:
    {
      FILETIME aLocalFileTime, utcFileTime;
      if (DosTimeToFileTime(item.ModifiedTime, aLocalFileTime))
      {
        if (!LocalFileTimeToFileTime(&aLocalFileTime, &utcFileTime))
          utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
      }
      else
        utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
      propVariant = utcFileTime;
      break;
    }
    case kpidAttributes:
      propVariant = item.GetWinAttributes();
      break;
    case kpidEncrypted:
      propVariant = item.IsEncrypted();
      break;
    /*
    case kpidComment:
      propVariant = item.IsCommented();
      break;
    */
    case kpidCRC:
      propVariant = item.FileCRC;
      break;
    case kpidMethod:
      propVariant = item.Method;
      break;
    case kpidHostOS:
      propVariant = (item.HostOS < kNumHostOSes) ?
        (kHostOS[item.HostOS]) : kUnknownOS;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
  COM_TRY_END
}

class CPropgressImp: public CProgressVirt
{
  CComPtr<IArchiveOpenCallback> m_OpenArchiveCallBack;
public:
  STDMETHOD(SetCompleted)(const UINT64 *numFiles);
  void Init(IArchiveOpenCallback *openArchiveCallback)
    { m_OpenArchiveCallBack = openArchiveCallback; }
};

STDMETHODIMP CPropgressImp::SetCompleted(const UINT64 *numFiles)
{
  if (m_OpenArchiveCallBack)
    return m_OpenArchiveCallBack->SetCompleted(numFiles, NULL);
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *inStream, 
    const UINT64 *maxCheckStartPosition, IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  try
  {
    _items.Clear();
    CInArchive anArchive;
    if(!anArchive.Open(inStream, maxCheckStartPosition))
      return S_FALSE;
    if (openArchiveCallback != NULL)
    {
      RINOK(openArchiveCallback->SetTotal(NULL, NULL));
      UINT64 numFiles = _items.Size();
      RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
    }
    while(true)
    {
      CItemInfoEx itemInfo;
      bool aFilled;
      HRESULT result = anArchive.GetNextItem(aFilled, itemInfo);
      if (result == S_FALSE)
        return S_FALSE;
      if (result != S_OK)
        return S_FALSE;
      if (!aFilled)
        break;
      _items.Add(itemInfo);
      anArchive.IncreaseRealPosition(itemInfo.PackSize);
      if (openArchiveCallback != NULL)
      {
        UINT64 numFiles = _items.Size();
        RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
      }
    }
    _stream = inStream;
  }
  catch(...)
  {
    return S_FALSE;
  }
  COM_TRY_END
  return S_OK;
}

STDMETHODIMP CHandler::Close()
{
  _stream.Release();
  return S_OK;
}



//////////////////////////////////////
// CHandler::DecompressItems

STDMETHODIMP CHandler::Extract(const UINT32* indices, UINT32 numItems,
    INT32 testModeSpec, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool testMode = (testModeSpec != 0);
  UINT64 totalUnPacked = 0, totalPacked = 0;
  if(numItems == 0)
    return S_OK;
  for(int i = 0; i < numItems; i++)
  {
    const CItemInfoEx &itemInfo = _items[indices[i]];
    totalUnPacked += itemInfo.Size;
    totalPacked += itemInfo.PackSize;
  }
  extractCallback->SetTotal(totalUnPacked);

  UINT64 currentTotalUnPacked = 0, currentTotalPacked = 0;
  UINT64 currentItemUnPacked, currentItemPacked;
  
  CComObjectNoLock<NCompression::CCopyCoder> *copyCoderSpec = NULL;
  CComPtr<ICompressCoder> arj1Decoder;
  CComPtr<ICompressCoder> arj2Decoder;
  CComPtr<ICompressCoder> copyCoder;

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
    const CItemInfoEx &itemInfo = _items[index];
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    if(itemInfo.IsDirectory())
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
    currentItemUnPacked = itemInfo.Size;
    currentItemPacked = itemInfo.PackSize;

    {
      CComObjectNoLock<COutStreamWithCRC> *anOutStreamSpec = 
        new CComObjectNoLock<COutStreamWithCRC>;
      CComPtr<ISequentialOutStream> outStream(anOutStreamSpec);
      anOutStreamSpec->Init(realOutStream);
      realOutStream.Release();
      
      CComObjectNoLock<CLimitedSequentialInStream> *streamSpec = new 
          CComObjectNoLock<CLimitedSequentialInStream>;
      CComPtr<ISequentialInStream> inStream(streamSpec);
      
      UINT64 aPos;
      _stream->Seek(itemInfo.DataPosition, STREAM_SEEK_SET, &aPos);

      streamSpec->Init(_stream, itemInfo.PackSize);


      CComObjectNoLock<CLocalProgress> *aLocalProgressSpec = new  CComObjectNoLock<CLocalProgress>;
      CComPtr<ICompressProgressInfo> aProgress = aLocalProgressSpec;
      aLocalProgressSpec->Init(extractCallback, false);


      CComObjectNoLock<CLocalCompressProgressInfo> *aLocalCompressProgressSpec = 
          new  CComObjectNoLock<CLocalCompressProgressInfo>;
      CComPtr<ICompressProgressInfo> aCompressProgress = aLocalCompressProgressSpec;
      aLocalCompressProgressSpec->Init(aProgress, 
          &currentTotalPacked,
          &currentTotalUnPacked);

      if (itemInfo.IsEncrypted())
      {
        RINOK(extractCallback->SetOperationResult(
          NArchive::NExtract::NOperationResult::kUnSupportedMethod));
        continue;
      }

      HRESULT result;

      switch(itemInfo.Method)
      {
        case NFileHeader::NCompressionMethod::kStored:
          {
            if(copyCoderSpec == NULL)
            {
              copyCoderSpec = new CComObjectNoLock<NCompression::CCopyCoder>;
              copyCoder = copyCoderSpec;
            }
            try
            {
              if (itemInfo.IsEncrypted())
              {
                RINOK(extractCallback->SetOperationResult(
                  NArchive::NExtract::NOperationResult::kUnSupportedMethod));
                continue;
              }
              else
              {
                result = copyCoder->Code(inStream, outStream,
                    NULL, NULL, aCompressProgress);
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
        case NFileHeader::NCompressionMethod::kCompressed1a:
        case NFileHeader::NCompressionMethod::kCompressed1b:
        case NFileHeader::NCompressionMethod::kCompressed1c:
          {
            if(!arj1Decoder)
            {
              arj1Decoder = new CComObjectNoLock<NCompress::NArj::NDecoder1::CCoder>;
            }
            try
            {
              if (itemInfo.IsEncrypted())
              {
                RINOK(extractCallback->SetOperationResult(
                  NArchive::NExtract::NOperationResult::kUnSupportedMethod));
                continue;
              }
              else
              {
                result = arj1Decoder->Code(inStream, outStream,
                    NULL, &currentItemUnPacked, aCompressProgress);
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
        case NFileHeader::NCompressionMethod::kCompressed2:
          {
            if(!arj2Decoder)
            {
              arj2Decoder = new CComObjectNoLock<NCompress::NArj::NDecoder2::CCoder>;
            }
            try
            {
              if (itemInfo.IsEncrypted())
              {
                RINOK(extractCallback->SetOperationResult(
                  NArchive::NExtract::NOperationResult::kUnSupportedMethod));
                continue;
              }
              else
              {
                result = arj2Decoder->Code(inStream, outStream,
                    NULL, &currentItemUnPacked, aCompressProgress);
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
      bool aCRC_Ok = anOutStreamSpec->GetCRC() == itemInfo.FileCRC;
      outStream.Release();
      if(aCRC_Ok)
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK))
      else
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kCRCError))
    }
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::ExtractAllItems(INT32 testMode,
      IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  CRecordVector<UINT32> indices;
  for(int i = 0; i < _items.Size(); i++)
    indices.Add(i);
  return Extract(&indices.Front(), _items.Size(), testMode, extractCallback);
  COM_TRY_END
}

}}
