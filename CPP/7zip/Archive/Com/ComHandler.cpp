// ComHandler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"

#include "Windows/PropVariant.h"

#include "../../Common/LimitedStreams.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/StreamUtils.h"

#include "../../Compress/CopyCoder.h"

#include "ComHandler.h"

namespace NArchive {
namespace NCom {

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsDir, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidMTime, VT_FILETIME}
};

STATPROPSTG kArcProps[] =
{
  { NULL, kpidClusterSize, VT_UI4},
  { NULL, kpidSectorSize, VT_UI4}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidClusterSize: prop = (UInt32)1 << _db.SectorSizeBits; break;
    case kpidSectorSize: prop = (UInt32)1 << _db.MiniSectorSizeBits; break;
    case kpidMainSubfile: if (_db.MainSubfile >= 0) prop = (UInt32)_db.MainSubfile; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CRef &ref = _db.Refs[index];
  const CItem &item = _db.Items[ref.Did];
    
  switch(propID)
  {
    case kpidPath:  prop = _db.GetItemPath(index); break;
    case kpidIsDir:  prop = item.IsDir(); break;
    case kpidCTime:  prop = item.CTime; break;
    case kpidMTime:  prop = item.MTime; break;
    case kpidPackSize:  if (!item.IsDir()) prop = _db.GetItemPackSize(item.Size); break;
    case kpidSize:  if (!item.IsDir()) prop = item.Size; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Open(IInStream *inStream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */)
{
  COM_TRY_BEGIN
  Close();
  try
  {
    if (_db.Open(inStream) != S_OK)
      return S_FALSE;
    _stream = inStream;
  }
  catch(...) { return S_FALSE; }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _db.Clear();
  _stream.Release();
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = _db.Refs.Size();
  if (numItems == 0)
    return S_OK;
  UInt32 i;
  UInt64 totalSize = 0;
  for(i = 0; i < numItems; i++)
  {
    const CItem &item = _db.Items[_db.Refs[allFilesMode ? i : indices[i]].Did];
    if (!item.IsDir())
      totalSize += item.Size;
  }
  RINOK(extractCallback->SetTotal(totalSize));

  UInt64 totalPackSize;
  totalSize = totalPackSize = 0;
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  for (i = 0; i < numItems; i++)
  {
    lps->InSize = totalPackSize;
    lps->OutSize = totalSize;
    RINOK(lps->SetCur());
    Int32 index = allFilesMode ? i : indices[i];
    const CItem &item = _db.Items[_db.Refs[index].Did];

    CMyComPtr<ISequentialOutStream> outStream;
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    RINOK(extractCallback->GetStream(index, &outStream, askMode));

    if (item.IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      continue;
    }

    totalPackSize += _db.GetItemPackSize(item.Size);
    totalSize += item.Size;
    
    if (!testMode && !outStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));
    Int32 res = NExtract::NOperationResult::kDataError;
    CMyComPtr<ISequentialInStream> inStream;
    HRESULT hres = GetStream(index, &inStream);
    if (hres == S_FALSE)
      res = NExtract::NOperationResult::kDataError;
    else if (hres == E_NOTIMPL)
      res = NExtract::NOperationResult::kUnSupportedMethod;
    else
    {
      RINOK(hres);
      if (inStream)
      {
        RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress));
        if (copyCoderSpec->TotalSize == item.Size)
          res = NExtract::NOperationResult::kOK;
      }
    }
    outStream.Release();
    RINOK(extractCallback->SetOperationResult(res));
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _db.Refs.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetStream(UInt32 index, ISequentialInStream **stream)
{
  COM_TRY_BEGIN
  *stream = 0;
  const CItem &item = _db.Items[_db.Refs[index].Did];
  CClusterInStream *streamSpec = new CClusterInStream;
  CMyComPtr<ISequentialInStream> streamTemp = streamSpec;
  streamSpec->Stream = _stream;
  streamSpec->StartOffset = 0;

  bool isLargeStream = _db.IsLargeStream(item.Size);
  int bsLog = isLargeStream ? _db.SectorSizeBits : _db.MiniSectorSizeBits;
  streamSpec->BlockSizeLog = bsLog;
  streamSpec->Size = item.Size;

  UInt32 clusterSize = (UInt32)1 << bsLog;
  UInt64 numClusters64 = (item.Size + clusterSize - 1) >> bsLog;
  if (numClusters64 >= ((UInt32)1 << 31))
    return E_NOTIMPL;
  streamSpec->Vector.Reserve((int)numClusters64);
  UInt32 sid = item.Sid;
  UInt64 size = item.Size;

  if (size != 0)
  {
    for (;; size -= clusterSize)
    {
      if (isLargeStream)
      {
        if (sid >= _db.FatSize)
          return S_FALSE;
        streamSpec->Vector.Add(sid + 1);
        sid = _db.Fat[sid];
      }
      else
      {
        UInt64 val;
        if (sid >= _db.MatSize || !_db.GetMiniCluster(sid, val) || val >= (UInt64)1 << 32)
          return S_FALSE;
        streamSpec->Vector.Add((UInt32)val);
        sid = _db.Mat[sid];
      }
      if (size <= clusterSize)
        break;
    }
  }
  if (sid != NFatID::kEndOfChain)
    return S_FALSE;
  RINOK(streamSpec->InitAndSeek());
  *stream = streamTemp.Detach();
  return S_OK;
  COM_TRY_END
}

}}
