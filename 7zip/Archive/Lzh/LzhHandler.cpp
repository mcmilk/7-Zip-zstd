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

STATPROPSTG kProperties[] = 
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


CHandler::CHandler()
{}

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

STDMETHODIMP CHandler::GetArchivePropertyInfo(UInt32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  return E_NOTIMPL;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _items.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID,  PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
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
        propVariant = s;
      }
      break;
    }
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
      propVariant = utcFileTime;
      break;
    }
    /*
    case kpidAttributes:
      propVariant = (UInt32)item.Attributes;
      break;
    case kpidCommented:
      propVariant = item.IsCommented();
      break;
    */
    case kpidCRC:
      propVariant = (UInt32)item.CRC;
      break;
    case kpidMethod:
    {
      wchar_t method2[kMethodIdSize + 1];
      method2[kMethodIdSize] = 0;
      for (int i = 0; i < kMethodIdSize; i++)
        method2[i] = item.Method[i];
      propVariant = method2;
      break;
    }
    case kpidHostOS:
      propVariant = GetOS(item.OsId);
      break;
  }
  propVariant.Detach(value);
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
    const UInt64 *maxCheckStartPosition, IArchiveOpenCallback *callback)
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
    while(true)
    {
      CItemEx itemInfo;
      bool filled;
      HRESULT result = archive.GetNextItem(filled, itemInfo);
      if (result == S_FALSE)
        return S_FALSE;
      if (result != S_OK)
        return S_FALSE;
      if (!filled)
        break;
      _items.Add(itemInfo);
      archive.Skeep(itemInfo.PackSize);
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
    const CItemEx &itemInfo = _items[allFilesMode ? i : indices[i]];
    totalUnPacked += itemInfo.Size;
    totalPacked += itemInfo.PackSize;
  }
  extractCallback->SetTotal(totalUnPacked);

  UInt64 currentTotalUnPacked = 0, currentTotalPacked = 0;
  UInt64 currentItemUnPacked, currentItemPacked;
  
  NCompress::NLzh::NDecoder::CCoder *lzhDecoderSpec;
  CMyComPtr<ICompressCoder> lzhDecoder;
  CMyComPtr<ICompressCoder> lzh1Decoder;
  CMyComPtr<ICompressCoder> arj2Decoder;
  CMyComPtr<ICompressCoder> copyCoder;

  for(i = 0; i < numItems; i++, currentTotalUnPacked += currentItemUnPacked,
      currentTotalPacked += currentItemPacked)
  {
    currentItemUnPacked = 0;
    currentItemPacked = 0;

    RINOK(extractCallback->SetCompleted(&currentTotalUnPacked));
    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode;
    askMode = testMode ? NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];
    const CItemEx &itemInfo = _items[index];
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
      COutStreamWithCRC *outStreamSpec = new COutStreamWithCRC;
      CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
      outStreamSpec->Init(realOutStream);
      realOutStream.Release();
      
      CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
      CMyComPtr<ISequentialInStream> inStream(streamSpec);
      
      UInt64 pos;
      _stream->Seek(itemInfo.DataPosition, STREAM_SEEK_SET, &pos);

      streamSpec->Init(_stream, itemInfo.PackSize);


      CLocalProgress *localProgressSpec = new CLocalProgress;
      CMyComPtr<ICompressProgressInfo> progress = localProgressSpec;
      localProgressSpec->Init(extractCallback, false);


      CLocalCompressProgressInfo *localCompressProgressSpec = 
          new CLocalCompressProgressInfo;
      CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
      localCompressProgressSpec->Init(progress, 
          &currentTotalPacked,
          &currentTotalUnPacked);

      HRESULT result;

      if (itemInfo.IsCopyMethod())
      {
        if(!copyCoder)
          copyCoder = new NCompress::CCopyCoder;
        try
        {
          result = copyCoder->Code(inStream, outStream, NULL, NULL, compressProgress);
          if (result == S_FALSE)
            throw "data error";
          if (result != S_OK)
            return result;
        }
        catch(...)
        {
          outStream.Release();
          RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kDataError));
          continue;
        }
      }
      else if (itemInfo.IsLh4GroupMethod())
      {
        if(!lzhDecoder)
        {
          lzhDecoderSpec = new NCompress::NLzh::NDecoder::CCoder;
          lzhDecoder = lzhDecoderSpec;
        }
        try
        {
          lzhDecoderSpec->SetDictionary(itemInfo.GetNumDictBits());
          result = lzhDecoder->Code(inStream, outStream, NULL, &currentItemUnPacked, compressProgress);
          if (result == S_FALSE)
            throw "data error";
          if (result != S_OK)
            return result;
        }
        catch(...)
        {
          outStream.Release();
          RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kDataError));
          continue;
        }
      }
      /*
      else if (itemInfo.IsLh1GroupMethod())
      {
        if(!lzh1Decoder)
        {
          lzh1DecoderSpec = new NCompress::NLzh1::NDecoder::CCoder;
          lzh1Decoder = lzh1DecoderSpec;
        }
        try
        {
          lzh1DecoderSpec->SetDictionary(itemInfo.GetNumDictBits());
          result = lzh1Decoder->Code(inStream, outStream, NULL, &currentItemUnPacked, compressProgress);
          if (result == S_FALSE)
            throw "data error";
          if (result != S_OK)
            return result;
        }
        catch(...)
        {
          outStream.Release();
          RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kDataError));
          continue;
        }
      }
      */
      else
      {
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kUnSupportedMethod));
        continue;
      }

      bool crcOK = (outStreamSpec->GetCRC() == itemInfo.CRC);
      outStream.Release();
      if(crcOK)
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK))
      else
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kCRCError))
    }
  }
  return S_OK;
  COM_TRY_END
}


}}
