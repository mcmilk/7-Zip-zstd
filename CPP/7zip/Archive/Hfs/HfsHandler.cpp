// HfsHandler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "../../Common/StreamUtils.h"
#include "HfsHandler.h"

namespace NArchive {
namespace NHfs {

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsDir, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidATime, VT_FILETIME}
};

STATPROPSTG kArcProps[] =
{
  { NULL, kpidMethod, VT_BSTR},
  { NULL, kpidClusterSize, VT_UI4},
  { NULL, kpidFreeSpace, VT_UI8},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidMTime, VT_FILETIME}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

static void HfsTimeToProp(UInt32 hfsTime, NWindows::NCOM::CPropVariant &prop)
{
  FILETIME ft;
  HfsTimeToFileTime(hfsTime, ft);
  prop = ft;
}

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidMethod: prop = _db.Header.IsHfsX() ? L"HFSX" : L"HFS+"; break;
    case kpidClusterSize: prop = (UInt32)1 << _db.Header.BlockSizeLog; break;
    case kpidFreeSpace: prop = (UInt64)_db.Header.NumFreeBlocks << _db.Header.BlockSizeLog; break;
    case kpidMTime: HfsTimeToProp(_db.Header.MTime, prop); break;
    case kpidCTime:
    {
      FILETIME localFt, ft;
      HfsTimeToFileTime(_db.Header.CTime, localFt);
      if (LocalFileTimeToFileTime(&localFt, &ft))
        prop = ft;
      break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CItem &item = _db.Items[index];
  switch(propID)
  {
    case kpidPath: prop = _db.GetItemPath(index); break;
    case kpidIsDir: prop = item.IsDir(); break;

    case kpidCTime:  HfsTimeToProp(item.CTime, prop); break;
    case kpidMTime:  HfsTimeToProp(item.MTime, prop); break;
    case kpidATime:  HfsTimeToProp(item.ATime, prop); break;

    case kpidPackSize: if (!item.IsDir()) prop = (UInt64)item.NumBlocks << _db.Header.BlockSizeLog; break;
    case kpidSize:     if (!item.IsDir()) prop = item.Size; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

class CProgressImp: public CProgressVirt
{
  CMyComPtr<IArchiveOpenCallback> _callback;
public:
  HRESULT SetTotal(UInt64 numFiles);
  HRESULT SetCompleted(UInt64 numFiles);
  CProgressImp(IArchiveOpenCallback *callback): _callback(callback) {}
};

HRESULT CProgressImp::SetTotal(UInt64 numFiles)
{
  if (_callback)
    return _callback->SetTotal(&numFiles, NULL);
  return S_OK;
}

HRESULT CProgressImp::SetCompleted(UInt64 numFiles)
{
  if (_callback)
    return _callback->SetCompleted(&numFiles, NULL);
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *inStream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback *callback)
{
  COM_TRY_BEGIN
  Close();
  try
  {
    CProgressImp progressImp(callback);
    HRESULT res = _db.Open(inStream, &progressImp);
    if (res == E_ABORT)
      return res;
    if (res != S_OK)
      return S_FALSE;
    _stream = inStream;
  }
  catch(...) { return S_FALSE; }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _stream.Release();
  _db.Clear();
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = _db.Items.Size();
  if (numItems == 0)
    return S_OK;
  UInt32 i;
  UInt64 totalSize = 0;
  for (i = 0; i < numItems; i++)
  {
    const CItem &item = _db.Items[allFilesMode ? i : indices[i]];
    if (!item.IsDir())
      totalSize += item.Size;
  }
  RINOK(extractCallback->SetTotal(totalSize));

  UInt64 currentTotalSize = 0, currentItemSize = 0;
  
  CByteBuffer buf;
  const UInt32 kBufSize = (1 << 16);
  buf.SetCapacity(kBufSize);

  for (i = 0; i < numItems; i++, currentTotalSize += currentItemSize)
  {
    RINOK(extractCallback->SetCompleted(&currentTotalSize));
    Int32 index = allFilesMode ? i : indices[i];
    const CItem &item = _db.Items[index];
    currentItemSize = 0;
    if (!item.IsDir())
      currentItemSize = item.Size;

    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    if (item.IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      continue;
    }
    if (!testMode && !realOutStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));
    UInt64 pos = 0;
    int res = NExtract::NOperationResult::kOK;
    int i;
    for (i = 0; i < item.Extents.Size(); i++)
    {
      if (item.Size == pos)
        break;
      if (res != NExtract::NOperationResult::kOK)
        break;
      const CExtent &e = item.Extents[i];
      RINOK(_stream->Seek((UInt64)e.Pos << _db.Header.BlockSizeLog, STREAM_SEEK_SET, NULL));
      UInt64 extentSize = (UInt64)e.NumBlocks << _db.Header.BlockSizeLog;
      for (;;)
      {
        if (extentSize == 0)
          break;
        UInt64 rem = item.Size - pos;
        if (rem == 0)
        {
          if (extentSize >= (UInt64)((UInt32)1 << _db.Header.BlockSizeLog))
            res = NExtract::NOperationResult::kDataError;
          break;
        }
        UInt32 curSize = kBufSize;
        if (curSize > rem)
          curSize = (UInt32)rem;
        if (curSize > extentSize)
          curSize = (UInt32)extentSize;
        RINOK(ReadStream_FALSE(_stream, buf, curSize));
        if (realOutStream)
        {
          RINOK(WriteStream(realOutStream, buf, curSize));
        }
        pos += curSize;
        extentSize -= curSize;
        UInt64 processed = currentTotalSize + pos;
        RINOK(extractCallback->SetCompleted(&processed));
      }
    }
    if (i != item.Extents.Size() || item.Size != pos)
      res = NExtract::NOperationResult::kDataError;
    realOutStream.Release();
    RINOK(extractCallback->SetOperationResult(res));
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _db.Items.Size();
  return S_OK;
}

}}
