// CpioHandler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/Defs.h"
#include "Common/NewHandler.h"
#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../../Common/LimitedStreams.h"
#include "../../Common/ProgressUtils.h"

#include "../../Compress/CopyCoder.h"

#include "../Common/ItemNameUtils.h"

#include "CpioHandler.h"
#include "CpioIn.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NCpio {

/*
enum
{
  kpidinode = kpidUserDefined,
  kpidiChkSum
};
*/

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsDir, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME},
  // { NULL, kpidUser, VT_BSTR},
  // { NULL, kpidGroup, VT_BSTR},
  // { L"inode", kpidinode, VT_UI4}
  // { L"CheckSum", kpidiChkSum, VT_UI4}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback *callback)
{
  COM_TRY_BEGIN
  // try
  {
    CInArchive archive;

    UInt64 endPos = 0;
    bool needSetTotal = true;

    if (callback != NULL)
    {
      RINOK(stream->Seek(0, STREAM_SEEK_END, &endPos));
      RINOK(stream->Seek(0, STREAM_SEEK_SET, NULL));
    }

    RINOK(archive.Open(stream));

    _items.Clear();

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
      archive.SkeepDataRecords(item.Size, item.Align);
      if (callback != NULL)
      {
        if (needSetTotal)
        {
          RINOK(callback->SetTotal(NULL, &endPos));
          needSetTotal = false;
        }
        if (_items.Size() % 100 == 0)
        {
          UInt64 numFiles = _items.Size();
          UInt64 numBytes = item.HeaderPosition;
          RINOK(callback->SetCompleted(&numFiles, &numBytes));
        }
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
  NWindows::NCOM::CPropVariant prop;
  const CItemEx &item = _items[index];

  switch(propID)
  {
    case kpidPath:
      prop = NItemName::GetOSName(MultiByteToUnicodeString(item.Name, CP_OEMCP));
      break;
    case kpidIsDir:
      prop = item.IsDir();
      break;
    case kpidSize:
    case kpidPackSize:
      prop = (UInt64)item.Size;
      break;
    case kpidMTime:
    {
      FILETIME utcFileTime;
      if (item.ModificationTime != 0)
        NTime::UnixTimeToFileTime(item.ModificationTime, utcFileTime);
      else
      {
        utcFileTime.dwLowDateTime = 0;
        utcFileTime.dwHighDateTime = 0;
      }
      prop = utcFileTime;
      break;
    }
    /*
    case kpidinode:  prop = item.inode; break;
    case kpidiChkSum:  prop = item.ChkSum; break;
    */
  }
  prop.Detach(value);
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
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
    totalSize += _items[allFilesMode ? i : indices[i]].Size;
  extractCallback->SetTotal(totalSize);

  UInt64 currentTotalSize = 0;
  UInt64 currentItemSize;
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  streamSpec->SetStream(_inStream);

  for (i = 0; i < numItems; i++, currentTotalSize += currentItemSize)
  {
    lps->InSize = lps->OutSize = currentTotalSize;
    RINOK(lps->SetCur());
    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ?
        NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];
    const CItemEx &item = _items[index];
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
    currentItemSize = item.Size;
    if (item.IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
      continue;
    }
    if (!testMode && (!realOutStream))
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));
    if (testMode)
    {
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
      continue;
    }
    RINOK(_inStream->Seek(item.GetDataPosition(), STREAM_SEEK_SET, NULL));
    streamSpec->Init(item.Size);
    RINOK(copyCoder->Code(inStream, realOutStream, NULL, NULL, progress));
    realOutStream.Release();
    RINOK(extractCallback->SetOperationResult((copyCoderSpec->TotalSize == item.Size) ?
        NArchive::NExtract::NOperationResult::kOK:
        NArchive::NExtract::NOperationResult::kDataError));
  }
  return S_OK;
  COM_TRY_END
}

}}
