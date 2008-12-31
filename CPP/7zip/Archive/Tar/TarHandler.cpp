// TarHandler.cpp

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

#include "../Common/DummyOutStream.h"
#include "../Common/ItemNameUtils.h"

#include "TarHandler.h"
#include "TarIn.h"

using namespace NWindows;

namespace NArchive {
namespace NTar {

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsDir, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidUser, VT_BSTR},
  { NULL, kpidGroup, VT_BSTR}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO

HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *callback)
{
  UInt64 endPos = 0;
  if (callback != NULL)
  {
    RINOK(stream->Seek(0, STREAM_SEEK_END, &endPos));
    RINOK(stream->Seek(0, STREAM_SEEK_SET, NULL));
  }
  
  UInt64 pos = 0;
  for (;;)
  {
    CItemEx item;
    bool filled;
    item.HeaderPosition = pos;
    RINOK(ReadItem(stream, filled, item));
    if (!filled)
      break;
    _items.Add(item);
    
    RINOK(stream->Seek(item.GetPackSize(), STREAM_SEEK_CUR, &pos));
    if (pos >= endPos)
      return S_FALSE;
    if (callback != NULL)
    {
      if (_items.Size() == 1)
      {
        RINOK(callback->SetTotal(NULL, &endPos));
      }
      if (_items.Size() % 100 == 0)
      {
        UInt64 numFiles = _items.Size();
        RINOK(callback->SetCompleted(&numFiles, &pos));
      }
    }
  }

  if (_items.Size() == 0)
  {
    CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
    if (!callback)
      return S_FALSE;
    callback->QueryInterface(IID_IArchiveOpenVolumeCallback, (void **)&openVolumeCallback);
    if (!openVolumeCallback)
      return S_FALSE;
    NCOM::CPropVariant prop;
    if (openVolumeCallback->GetProperty(kpidName, &prop) != S_OK)
      return S_FALSE;
    if (prop.vt != VT_BSTR)
      return S_FALSE;
    UString baseName = prop.bstrVal;
    baseName = baseName.Right(4);
    if (baseName.CompareNoCase(L".tar") != 0)
      return S_FALSE;
  }
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  {
    Close();
    RINOK(Open2(stream, openArchiveCallback));
    _inStream = stream;
  }
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
    case kpidPath:  prop = NItemName::GetOSName2(MultiByteToUnicodeString(item.Name, CP_OEMCP)); break;
    case kpidIsDir:  prop = item.IsDir(); break;
    case kpidSize:  prop = item.Size; break;
    case kpidPackSize:  prop = item.GetPackSize(); break;
    case kpidMTime:
      if (item.MTime != 0)
      {
        FILETIME ft;
        NTime::UnixTimeToFileTime(item.MTime, ft);
        prop = ft;
      }
      break;
    case kpidUser:  prop = MultiByteToUnicodeString(item.UserName, CP_OEMCP); break;
    case kpidGroup:  prop = MultiByteToUnicodeString(item.GroupName, CP_OEMCP); break;
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

  UInt64 totalPackSize, curPackSize, curSize;
  totalSize = totalPackSize = 0;
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  streamSpec->SetStream(_inStream);

  CDummyOutStream *outStreamSpec = new CDummyOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);

  for (i = 0; i < numItems; i++, totalSize += curSize, totalPackSize += curPackSize)
  {
    lps->InSize = totalPackSize;
    lps->OutSize = totalSize;
    RINOK(lps->SetCur());
    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ?
        NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];
    const CItemEx &item = _items[index];
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
    curSize = item.Size;
    curPackSize = item.GetPackSize();
    if (item.IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
      continue;
    }
    if (!testMode && (!realOutStream))
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));

    outStreamSpec->SetStream(realOutStream);
    realOutStream.Release();
    outStreamSpec->Init();

    RINOK(_inStream->Seek(item.GetDataPosition(), STREAM_SEEK_SET, NULL));
    streamSpec->Init(item.Size);
    RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress));
    outStreamSpec->ReleaseStream();
    RINOK(extractCallback->SetOperationResult(copyCoderSpec->TotalSize == item.Size ?
        NArchive::NExtract::NOperationResult::kOK:
        NArchive::NExtract::NOperationResult::kDataError));
  }
  return S_OK;
  COM_TRY_END
}

}}
