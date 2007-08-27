// ArjHandler.cpp

#include "StdAfx.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/ComTry.h"

#include "Windows/Time.h"
#include "Windows/PropVariant.h"

#include "ArjHandler.h"

#include "../../ICoder.h"

#include "../../Common/StreamObjects.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/LimitedStreams.h"

#include "../../Compress/Copy/CopyCoder.h"
#include "../../Compress/Arj/ArjDecoder1.h"
#include "../../Compress/Arj/ArjDecoder2.h"

#include "../Common/ItemNameUtils.h"
#include "../Common/OutStreamWithCRC.h"

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


const int kNumHostOSes = sizeof(kHostOS) / sizeof(kHostOS[0]);

const wchar_t *kUnknownOS = L"Unknown";


STATPROPSTG kProps[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},
  { NULL, kpidLastWriteTime, VT_FILETIME},
  { NULL, kpidAttributes, VT_UI4},
  { NULL, kpidEncrypted, VT_BOOL},
  { NULL, kpidCRC, VT_UI4},
  { NULL, kpidMethod, VT_UI1},
  // { NULL, kpidUnpackVer, VT_UI1},
  { NULL, kpidHostOS, VT_BSTR}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _items.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CItemEx &item = _items[index];
  switch(propID)
  {
    case kpidPath:  prop = NItemName::GetOSName(MultiByteToUnicodeString(item.Name, CP_OEMCP)); break;
    case kpidIsFolder:  prop = item.IsDirectory(); break;
    case kpidSize:  prop = item.Size; break;
    case kpidPackedSize:  prop = item.PackSize; break;
    case kpidAttributes:  prop = item.GetWinAttributes(); break;
    case kpidEncrypted:  prop = item.IsEncrypted(); break;
    case kpidCRC:  prop = item.FileCRC; break;
    case kpidMethod:  prop = item.Method; break;
    case kpidHostOS:  prop = (item.HostOS < kNumHostOSes) ? (kHostOS[item.HostOS]) : kUnknownOS; break;
    case kpidLastWriteTime:
    {
      FILETIME localFileTime, utcFileTime;
      if (DosTimeToFileTime(item.ModifiedTime, localFileTime))
      {
        if (!LocalFileTimeToFileTime(&localFileTime, &utcFileTime))
          utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
      }
      else
        utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
      prop = utcFileTime;
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Open(IInStream *inStream, 
    const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *callback)
{
  COM_TRY_BEGIN
  try
  {
    _items.Clear();
    CInArchive archive;
    if(!archive.Open(inStream, maxCheckStartPosition))
      return S_FALSE;
    if (callback != NULL)
    {
      RINOK(callback->SetTotal(NULL, NULL));
      UInt64 numFiles = _items.Size();
      RINOK(callback->SetCompleted(&numFiles, NULL));
    }
    for (;;)
    {
      CItemEx item;
      bool filled;
      HRESULT result = archive.GetNextItem(filled, item);
      if (result == S_FALSE)
        return S_FALSE;
      if (result != S_OK)
        return S_FALSE;
      if (!filled)
        break;
      _items.Add(item);
      archive.IncreaseRealPosition(item.PackSize);
      if (callback != NULL)
      {
        UInt64 numFiles = _items.Size();
        RINOK(callback->SetCompleted(&numFiles, NULL));
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
  _items.Clear();
  _stream.Release();
  return S_OK;
}



//////////////////////////////////////
// CHandler::DecompressItems

STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 testModeSpec, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool testMode = (testModeSpec != 0);
  UInt64 totalUnPacked = 0, totalPacked = 0;
  bool allFilesMode = (numItems == UInt32(-1));
  if (allFilesMode)
    numItems = _items.Size();
  if(numItems == 0)
    return S_OK;
  UInt32 i;
  for(i = 0; i < numItems; i++)
  {
    const CItemEx &item = _items[allFilesMode ? i : indices[i]];
    totalUnPacked += item.Size;
    totalPacked += item.PackSize;
  }
  extractCallback->SetTotal(totalUnPacked);

  UInt64 currentTotalUnPacked = 0, currentTotalPacked = 0;
  UInt64 currentItemUnPacked, currentItemPacked;
  
  CMyComPtr<ICompressCoder> arj1Decoder;
  CMyComPtr<ICompressCoder> arj2Decoder;
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  streamSpec->SetStream(_stream);

  for(i = 0; i < numItems; i++, currentTotalUnPacked += currentItemUnPacked,
      currentTotalPacked += currentItemPacked)
  {
    lps->InSize = currentTotalPacked;
    lps->OutSize = currentTotalUnPacked;
    RINOK(lps->SetCur());

    currentItemUnPacked = currentItemPacked = 0;

    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ? 
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];
    const CItemEx &item = _items[index];
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    if(item.IsDirectory())
    {
      // if (!testMode)
      {
        RINOK(extractCallback->PrepareOperation(askMode));
        RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      }
      continue;
    }

    if (!testMode && (!realOutStream)) 
      continue;

    RINOK(extractCallback->PrepareOperation(askMode));
    currentItemUnPacked = item.Size;
    currentItemPacked = item.PackSize;

    {
      COutStreamWithCRC *outStreamSpec = new COutStreamWithCRC;
      CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
      outStreamSpec->SetStream(realOutStream);
      outStreamSpec->Init();
      realOutStream.Release();

      streamSpec->Init(item.PackSize);
      
      UInt64 pos;
      _stream->Seek(item.DataPosition, STREAM_SEEK_SET, &pos);

      HRESULT result = S_OK;
      Int32 opRes = NExtract::NOperationResult::kOK;

      if (item.IsEncrypted())
      {
        opRes = NExtract::NOperationResult::kUnSupportedMethod;
      }
      else
      {
        switch(item.Method)
        {
          case NFileHeader::NCompressionMethod::kStored:
          {
            result = copyCoder->Code(inStream, outStream, NULL, NULL, progress);
            if (result == S_OK && copyCoderSpec->TotalSize != item.PackSize)
              result = S_FALSE;
            break;
          }
          case NFileHeader::NCompressionMethod::kCompressed1a:
          case NFileHeader::NCompressionMethod::kCompressed1b:
          case NFileHeader::NCompressionMethod::kCompressed1c:
          {
            if (!arj1Decoder)
              arj1Decoder = new NCompress::NArj::NDecoder1::CCoder;
            result = arj1Decoder->Code(inStream, outStream, NULL, &currentItemUnPacked, progress);
            break;
          }
          case NFileHeader::NCompressionMethod::kCompressed2:
          {
            if (!arj2Decoder)
              arj2Decoder = new NCompress::NArj::NDecoder2::CCoder;
            result = arj2Decoder->Code(inStream, outStream, NULL, &currentItemUnPacked, progress);
            break;
          }
          default:
            opRes = NExtract::NOperationResult::kUnSupportedMethod;
        }
      }
      if (opRes == NExtract::NOperationResult::kOK)
      {
        if (result == S_FALSE)
          opRes = NExtract::NOperationResult::kDataError;
        else
        {
          RINOK(result);
          opRes = (outStreamSpec->GetCRC() == item.FileCRC) ? 
              NExtract::NOperationResult::kOK:
              NExtract::NOperationResult::kCRCError;
        }
      }
      outStream.Release();
      RINOK(extractCallback->SetOperationResult(opRes));
    }
  }
  return S_OK;
  COM_TRY_END
}


}}
