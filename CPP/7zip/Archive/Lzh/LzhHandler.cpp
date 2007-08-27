// LzhHandler.cpp

#include "StdAfx.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/ComTry.h"

#include "Windows/Time.h"
#include "Windows/PropVariant.h"

#include "LzhHandler.h"
#include "LzhOutStreamWithCRC.h"

#include "../../ICoder.h"

#include "../../Common/ProgressUtils.h"
#include "../../Common/LimitedStreams.h"

#include "../../Compress/Copy/CopyCoder.h"
#include "../../Compress/Lzh/LzhDecoder.h"

#include "../Common/ItemNameUtils.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NLzh{

struct COsPair
{
  Byte Id;
  const wchar_t *Name;
};

COsPair g_OsPairs[] = 
{
  { 'M', L"MS-DOS" },
  { '2', L"OS/2" },
  { '9', L"OS9" },
  { 'K', L"OS/68K" },
  { '3', L"OS/386" },
  { 'H', L"HUMAN" },
  { 'U', L"UNIX" },
  { 'C', L"CP/M" },
  { 'F', L"FLEX" },
  { 'm', L"Mac" },
  { 'R', L"Runser" },
  { 'T', L"TownsOS" },
  { 'X', L"XOSK" },
  { 'w', L"Windows95" },
  { 'W', L"WindowsNT" },
  {  0,  L"MS-DOS" },
  { 'J', L"Java VM" }
};

const wchar_t *kUnknownOS = L"Unknown";

const int kNumHostOSes = sizeof(g_OsPairs) / sizeof(g_OsPairs[0]);

static const wchar_t *GetOS(Byte osId)
{
  for (int i = 0; i < kNumHostOSes; i++)
    if (g_OsPairs[i].Id == osId)
      return g_OsPairs[i].Name;
  return kUnknownOS;
};

STATPROPSTG kProps[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},
  { NULL, kpidLastWriteTime, VT_FILETIME},
  { NULL, kpidAttributes, VT_UI4},

  // { NULL, kpidCommented, VT_BOOL},
    
  { NULL, kpidCRC, VT_UI4},

  { NULL, kpidMethod, VT_UI1},
  { NULL, kpidHostOS, VT_BSTR}

};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

CHandler::CHandler() {}

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
    case kpidPath:
    {
      UString s = NItemName::WinNameToOSName(MultiByteToUnicodeString(item.GetName(), CP_OEMCP));
      if (!s.IsEmpty())
      {
        if (s[s.Length() - 1] == WCHAR_PATH_SEPARATOR)
           s.Delete(s.Length() - 1);
        prop = s;
      }
      break;
    }
    case kpidIsFolder:
      prop = item.IsDirectory();
      break;
    case kpidSize:
      prop = item.Size;
      break;
    case kpidPackedSize:
      prop = item.PackSize;
      break;
    case kpidLastWriteTime:
    {
      FILETIME utcFileTime;
      UInt32 unixTime;
      if (item.GetUnixTime(unixTime))
      {
        NTime::UnixTimeToFileTime(unixTime, utcFileTime);
      }
      else
      {
        FILETIME localFileTime;
        if (DosTimeToFileTime(item.ModifiedTime, localFileTime))
        {
          if (!LocalFileTimeToFileTime(&localFileTime, &utcFileTime))
            utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
        }
        else
          utcFileTime.dwHighDateTime = utcFileTime.dwLowDateTime = 0;
      }
      prop = utcFileTime;
      break;
    }
    /*
    case kpidAttributes:
      prop = (UInt32)item.Attributes;
      break;
    case kpidCommented:
      prop = item.IsCommented();
      break;
    */
    case kpidCRC:
      prop = (UInt32)item.CRC;
      break;
    case kpidMethod:
    {
      wchar_t method2[kMethodIdSize + 1];
      method2[kMethodIdSize] = 0;
      for (int i = 0; i < kMethodIdSize; i++)
        method2[i] = item.Method[i];
      prop = method2;
      break;
    }
    case kpidHostOS:
      prop = GetOS(item.OsId);
      break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

/*
class CPropgressImp: public CProgressVirt
{
public:
  CMyComPtr<IArchiveOpenCallback> Callback;
  STDMETHOD(SetCompleted)(const UInt64 *numFiles);
};

STDMETHODIMP CPropgressImp::SetCompleted(const UInt64 *numFiles)
{
  if (Callback)
    return Callback->SetCompleted(numFiles, NULL);
  return S_OK;
}
*/

STDMETHODIMP CHandler::Open(IInStream *inStream, 
    const UInt64 * /* maxCheckStartPosition */, IArchiveOpenCallback *callback)
{
  COM_TRY_BEGIN
  try
  {
    _items.Clear();
    CInArchive archive;
    RINOK(archive.Open(inStream));
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
      archive.Skeep(item.PackSize);
      if (callback != NULL)
      {
        UInt64 numFiles = _items.Size();
        RINOK(callback->SetCompleted(&numFiles, NULL));
      }
    }
    if (_items.IsEmpty())
      return S_FALSE;

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
  
  NCompress::NLzh::NDecoder::CCoder *lzhDecoderSpec = 0;
  CMyComPtr<ICompressCoder> lzhDecoder;
  CMyComPtr<ICompressCoder> lzh1Decoder;
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
    currentItemUnPacked = 0;
    currentItemPacked = 0;

    lps->InSize = currentTotalPacked;
    lps->OutSize = currentTotalUnPacked;
    RINOK(lps->SetCur());

    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode;
    askMode = testMode ? NExtract::NAskMode::kTest :
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
      outStreamSpec->Init(realOutStream);
      realOutStream.Release();
      
      UInt64 pos;
      _stream->Seek(item.DataPosition, STREAM_SEEK_SET, &pos);

      streamSpec->Init(item.PackSize);

      HRESULT result = S_OK;
      Int32 opRes = NExtract::NOperationResult::kOK;

      if (item.IsCopyMethod())
      {
        result = copyCoder->Code(inStream, outStream, NULL, NULL, progress);
        if (result == S_OK && copyCoderSpec->TotalSize != item.PackSize)
          result = S_FALSE;
      }
      else if (item.IsLh4GroupMethod())
      {
        if(!lzhDecoder)
        {
          lzhDecoderSpec = new NCompress::NLzh::NDecoder::CCoder;
          lzhDecoder = lzhDecoderSpec;
        }
        lzhDecoderSpec->SetDictionary(item.GetNumDictBits());
        result = lzhDecoder->Code(inStream, outStream, NULL, &currentItemUnPacked, progress);
      }
      /*
      else if (item.IsLh1GroupMethod())
      {
        if(!lzh1Decoder)
        {
          lzh1DecoderSpec = new NCompress::NLzh1::NDecoder::CCoder;
          lzh1Decoder = lzh1DecoderSpec;
        }
        lzh1DecoderSpec->SetDictionary(item.GetNumDictBits());
        result = lzh1Decoder->Code(inStream, outStream, NULL, &currentItemUnPacked, progress);
      }
      */
      else
        opRes = NExtract::NOperationResult::kUnSupportedMethod;

      if (opRes == NExtract::NOperationResult::kOK)
      {
        if (result == S_FALSE)
          opRes = NExtract::NOperationResult::kDataError;
        else
        {
          RINOK(result);
          if (outStreamSpec->GetCRC() != item.CRC)
            opRes = NExtract::NOperationResult::kCRCError;
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
