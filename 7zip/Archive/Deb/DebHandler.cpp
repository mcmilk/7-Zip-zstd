// DebHandler.cpp

#include "StdAfx.h"

#include "DebHandler.h"
#include "DebIn.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/NewHandler.h"
#include "Common/ComTry.h"

#include "Windows/Time.h"
#include "Windows/PropVariant.h"

#include "../../Common/ProgressUtils.h"
#include "../../Common/LimitedStreams.h"

#include "../../Compress/Copy/CopyCoder.h"

#include "../Common/ItemNameUtils.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NDeb {

STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},
  { NULL, kpidLastWriteTime, VT_FILETIME}
};

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  value->vt = VT_EMPTY;
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfProperties(UINT32 *numProperties)
{
  *numProperties = sizeof(kProperties) / sizeof(kProperties[0]);
  return S_OK;
}

STDMETHODIMP CHandler::GetPropertyInfo(UINT32 index,     
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

STDMETHODIMP CHandler::GetNumberOfArchiveProperties(UINT32 *numProperties)
{
  *numProperties = 0;
  return S_OK;
}

STDMETHODIMP CHandler::GetArchivePropertyInfo(UINT32 index,     
      BSTR *name, PROPID *propID, VARTYPE *varType)
{
  return E_INVALIDARG;
}


STDMETHODIMP CHandler::Open(IInStream *stream, 
    const UINT64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  bool mustBeClosed = true;
  // try
  {
    CInArchive archive;
    if(archive.Open(stream) != S_OK)
      return S_FALSE;
    _items.Clear();

    if (openArchiveCallback != NULL)
    {
      RINOK(openArchiveCallback->SetTotal(NULL, NULL));
      UINT64 numFiles = _items.Size();
      RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
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
      archive.SkeepData(itemInfo.Size);
      if (openArchiveCallback != NULL)
      {
        UINT64 numFiles = _items.Size();
        RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
      }
    }
    _inStream = stream;
  }
  /*
  catch(...)
  {
    return S_FALSE;
  }
  */
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _inStream.Release();
  _items.Clear();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UINT32 *numItems)
{
  *numItems = _items.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UINT32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
  const CItemEx &item = _items[index];

  switch(propID)
  {
    case kpidPath:
      propVariant = (const wchar_t *)NItemName::GetOSName2(
          MultiByteToUnicodeString(item.Name, CP_OEMCP));
      break;
    case kpidIsFolder:
      propVariant = false;
      break;
    case kpidSize:
    case kpidPackedSize:
      propVariant = item.Size;
      break;
    case kpidLastWriteTime:
    {
      FILETIME utcFileTime;
      if (item.ModificationTime != 0)
        NTime::UnixTimeToFileTime(item.ModificationTime, utcFileTime);
      else
      {
        utcFileTime.dwLowDateTime = 0;
        utcFileTime.dwHighDateTime = 0;
      }
      propVariant = utcFileTime;
      break;
    }
  }
  propVariant.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Extract(const UINT32* indices, UINT32 numItems,
    INT32 _aTestMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == UINT32(-1));
  if (allFilesMode)
    numItems = _items.Size();
  if(numItems == 0)
    return S_OK;
  bool testMode = (_aTestMode != 0);
  UINT64 totalSize = 0;
  UINT32 i;
  for(i = 0; i < numItems; i++)
    totalSize += _items[allFilesMode ? i : indices[i]].Size;
  extractCallback->SetTotal(totalSize);

  UINT64 currentTotalSize = 0;
  UINT64 currentItemSize;
  
  CMyComPtr<ICompressCoder> copyCoder;

  for(i = 0; i < numItems; i++, currentTotalSize += currentItemSize)
  {
    RINOK(extractCallback->SetCompleted(&currentTotalSize));
    CMyComPtr<ISequentialOutStream> realOutStream;
    INT32 askMode;
    askMode = testMode ? NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
    INT32 index = allFilesMode ? i : indices[i];
    const CItemEx &itemInfo = _items[index];
    
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    currentItemSize = itemInfo.Size;

    if(!testMode && (!realOutStream))
    {
      continue;
    }
    RINOK(extractCallback->PrepareOperation(askMode));
    {
      if (testMode)
      {
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
        continue;
      }

      RINOK(_inStream->Seek(itemInfo.GetDataPosition(), STREAM_SEEK_SET, NULL));
      CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
      CMyComPtr<ISequentialInStream> inStream(streamSpec);
      streamSpec->Init(_inStream, itemInfo.Size);

      CLocalProgress *localProgressSpec = new CLocalProgress;
      CMyComPtr<ICompressProgressInfo> progress = localProgressSpec;
      localProgressSpec->Init(extractCallback, false);

      CLocalCompressProgressInfo *localCompressProgressSpec = 
          new CLocalCompressProgressInfo;
      CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
      localCompressProgressSpec->Init(progress, 
          &currentTotalSize, &currentTotalSize);

      if(copyCoder == NULL)
      {
        copyCoder = new NCompress::CCopyCoder;
      }
      try
      {
        RINOK(copyCoder->Code(inStream, realOutStream,
            NULL, NULL, compressProgress));
      }
      catch(...)
      {
        realOutStream.Release();
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kDataError));
        continue;
      }
      realOutStream.Release();
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
    }
  }
  return S_OK;
  COM_TRY_END
}

}}
