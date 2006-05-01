// Tar/Handler.cpp

#include "StdAfx.h"

#include "TarHandler.h"
#include "TarIn.h"

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
namespace NTar {

STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},
  { NULL, kpidLastWriteTime, VT_FILETIME},
  { NULL, kpidUser, VT_BSTR},
  { NULL, kpidGroup, VT_BSTR},
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
  return E_INVALIDARG;
}

STDMETHODIMP CHandler::Open(IInStream *stream, 
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  // try
  {
    CInArchive archive;

    if(archive.Open(stream) != S_OK)
      return S_FALSE;

    _items.Clear();

    if (openArchiveCallback != NULL)
    {
      RINOK(openArchiveCallback->SetTotal(NULL, NULL));
      UInt64 numFiles = _items.Size();
      RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
    }

    while(true)
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
      archive.SkeepDataRecords(item.Size);
      if (openArchiveCallback != NULL)
      {
        UInt64 numFiles = _items.Size();
        RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
      }
    }
    if (_items.Size() == 0)
      return S_FALSE;

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
  _items.Clear();
  _inStream.Release();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _items.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
  const NArchive::NTar::CItemEx &item = _items[index];

  switch(propID)
  {
    case kpidPath:
      propVariant = (const wchar_t *)NItemName::GetOSName2(
          MultiByteToUnicodeString(item.Name, CP_OEMCP));
      break;
    case kpidIsFolder:
      propVariant = item.IsDirectory();
      break;
    case kpidSize:
    case kpidPackedSize:
      propVariant = (UInt64)item.Size;
      break;
    case kpidLastWriteTime:
    {
      FILETIME utcFileTime;
      if (item.ModificationTime != 0)
        NTime::UnixTimeToFileTime((UInt32)item.ModificationTime, utcFileTime);
      else
      {
        utcFileTime.dwLowDateTime = 0;
        utcFileTime.dwHighDateTime = 0;
      }
      propVariant = utcFileTime;
      break;
    }
    case kpidUser:
      propVariant = (const wchar_t *)
          MultiByteToUnicodeString(item.UserName, CP_OEMCP);
      break;
    case kpidGroup:
      propVariant = (const wchar_t *)
          MultiByteToUnicodeString(item.GroupName, CP_OEMCP);
      break;
  }
  propVariant.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 _aTestMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool testMode = (_aTestMode != 0);
  bool allFilesMode = (numItems == UInt32(-1));
  if (allFilesMode)
    numItems = _items.Size();
  UInt64 totalSize = 0;
  if(numItems == 0)
    return S_OK;
  UInt32 i;
  for(i = 0; i < numItems; i++)
    totalSize += _items[allFilesMode ? i : indices[i]].Size;
  extractCallback->SetTotal(totalSize);

  UInt64 currentTotalSize = 0;
  UInt64 currentItemSize;
  
  CMyComPtr<ICompressCoder> copyCoder;

  for(i = 0; i < numItems; i++, currentTotalSize += currentItemSize)
  {
    RINOK(extractCallback->SetCompleted(&currentTotalSize));
    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode;
    askMode = testMode ? NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];
    const CItemEx &item = _items[index];
    
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    currentItemSize = item.Size;

    if(item.IsDirectory())
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
      continue;
    }
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

      RINOK(_inStream->Seek(item.GetDataPosition(), STREAM_SEEK_SET, NULL));
      CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
      CMyComPtr<ISequentialInStream> inStream(streamSpec);
      streamSpec->Init(_inStream, item.Size);

      CLocalProgress *localProgressSpec = new CLocalProgress;
      CMyComPtr<ICompressProgressInfo> progress = localProgressSpec;
      localProgressSpec->Init(extractCallback, false);


      CLocalCompressProgressInfo *localCompressProgressSpec = 
          new CLocalCompressProgressInfo;
      CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
      localCompressProgressSpec->Init(progress, 
          &currentTotalSize, &currentTotalSize);

      if(!copyCoder)
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
