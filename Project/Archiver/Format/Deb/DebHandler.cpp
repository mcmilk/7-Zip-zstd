// DebHandler.cpp

#include "StdAfx.h"

#include "DebHandler.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/NewHandler.h"

#include "Interface/ProgressUtils.h"
#include "Interface/LimitedStreams.h"
#include "Interface/EnumStatProp.h"

#include "Windows/Time.h"
#include "Windows/PropVariant.h"
#include "Windows/COMTry.h"

#include "Compression/CopyCoder.h"

#include "Archive/Common/ItemNameUtils.h"
#include "Archive/Deb/InEngine.h"

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

STDMETHODIMP CHandler::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  COM_TRY_BEGIN
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
  COM_TRY_END
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
      CItemInfoEx itemInfo;
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
  const CItemInfoEx &item = _items[index];

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

//////////////////////////////////////
// CHandler::DecompressItems

STDMETHODIMP CHandler::Extract(const UINT32* indices, UINT32 numItems,
    INT32 _aTestMode, IArchiveExtractCallback *_anExtractCallback)
{
  COM_TRY_BEGIN
  if(numItems == 0)
    return S_OK;
  bool testMode = (_aTestMode != 0);
  CComPtr<IArchiveExtractCallback> extractCallback = _anExtractCallback;
  UINT64 totalSize = 0;
  UINT32 i;
  for(i = 0; i < numItems; i++)
    totalSize += _items[indices[i]].Size;
  extractCallback->SetTotal(totalSize);

  UINT64 currentTotalSize = 0;
  UINT64 currentItemSize;
  
  CComObjectNoLock<NCompression::CCopyCoder> *copyCoderSpec = NULL;
  CComPtr<ICompressCoder> copyCoder;

  for(i = 0; i < numItems; i++, currentTotalSize += currentItemSize)
  {
    RINOK(extractCallback->SetCompleted(&currentTotalSize));
    CComPtr<ISequentialOutStream> realOutStream;
    INT32 askMode;
    askMode = testMode ? NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
    INT32 index = indices[i];
    const CItemInfoEx &itemInfo = _items[index];
    
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
      CComObjectNoLock<CLimitedSequentialInStream> *streamSpec = new 
          CComObjectNoLock<CLimitedSequentialInStream>;
      CComPtr<ISequentialInStream> inStream(streamSpec);
      streamSpec->Init(_inStream, itemInfo.Size);

      CComObjectNoLock<CLocalProgress> *localProgressSpec = new  CComObjectNoLock<CLocalProgress>;
      CComPtr<ICompressProgressInfo> progress = localProgressSpec;
      localProgressSpec->Init(extractCallback, false);

      CComObjectNoLock<CLocalCompressProgressInfo> *localCompressProgressSpec = 
          new  CComObjectNoLock<CLocalCompressProgressInfo>;
      CComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
      localCompressProgressSpec->Init(progress, 
          &currentTotalSize, &currentTotalSize);

      if(copyCoderSpec == NULL)
      {
        copyCoderSpec = new CComObjectNoLock<NCompression::CCopyCoder>;
        copyCoder = copyCoderSpec;
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

STDMETHODIMP CHandler::ExtractAllItems(INT32 testMode,
      IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  CRecordVector<UINT32> indices;
  indices.Reserve(_items.Size());
  for(int i = 0; i < _items.Size(); i++)
    indices.Add(i);
  return Extract(&indices.Front(), _items.Size(), testMode,
      extractCallback);
  COM_TRY_END
}

}}
