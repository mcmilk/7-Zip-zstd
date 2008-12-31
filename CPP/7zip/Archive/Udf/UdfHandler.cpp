// UdfHandler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/NewHandler.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../../Common/ProgressUtils.h"

#include "../../Compress/CopyCoder.h"

#include "../Common/DummyOutStream.h"

#include "UdfHandler.h"

namespace NArchive {
namespace NUdf {

void UdfTimeToFileTime(const CTime &t, NWindows::NCOM::CPropVariant &prop)
{
  UInt64 numSecs;
  const Byte *d = t.Data;
  if (!NWindows::NTime::GetSecondsSince1601(t.GetYear(), d[4], d[5], d[6], d[7], d[8], numSecs))
    return;
  if (t.IsLocal())
    numSecs -= t.GetMinutesOffset() * 60;
  FILETIME ft;
  UInt64 v = (((numSecs * 100 + d[9]) * 100 + d[10]) * 100 + d[11]) * 10;
  ft.dwLowDateTime = (UInt32)v;
  ft.dwHighDateTime = (UInt32)(v >> 32);
  prop = ft;
}

STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsDir, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidATime, VT_FILETIME}
};

STATPROPSTG kArcProps[] =
{
  { NULL, kpidComment, VT_BSTR},
  { NULL, kpidClusterSize, VT_UI4},
  { NULL, kpidCTime, VT_FILETIME}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidComment:
    {
      UString comment = _archive.GetComment();
      if (!comment.IsEmpty())
        prop = comment;
      break;
    }

    case kpidClusterSize:
      if (_archive.LogVols.Size() > 0)
      {
        UInt32 blockSize = _archive.LogVols[0].BlockSize;
        int i;
        for (i = 1; i < _archive.LogVols.Size(); i++)
          if (_archive.LogVols[i].BlockSize != blockSize)
            break;
        if (i == _archive.LogVols.Size())
          prop = blockSize;
      }
      break;

    case kpidCTime:
      if (_archive.LogVols.Size() == 1)
      {
        const CLogVol &vol = _archive.LogVols[0];
        if (vol.FileSets.Size() >= 1)
          UdfTimeToFileTime(vol.FileSets[0].RecodringTime, prop);
      }
      break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

class CProgressImp: public CProgressVirt
{
  CMyComPtr<IArchiveOpenCallback> _callback;
  UInt64 _numFiles;
  UInt64 _numBytes;
public:
  HRESULT SetTotal(UInt64 numBytes);
  HRESULT SetCompleted(UInt64 numFiles, UInt64 numBytes);
  HRESULT SetCompleted();
  CProgressImp(IArchiveOpenCallback *callback): _callback(callback), _numFiles(0), _numBytes(0) {}
};

HRESULT CProgressImp::SetTotal(UInt64 numBytes)
{
  if (_callback)
    return _callback->SetTotal(NULL, &numBytes);
  return S_OK;
}

HRESULT CProgressImp::SetCompleted(UInt64 numFiles, UInt64 numBytes)
{
  _numFiles = numFiles;
  _numBytes = numBytes;
  return SetCompleted();
}

HRESULT CProgressImp::SetCompleted()
{
  if (_callback)
    return _callback->SetCompleted(&_numFiles, &_numBytes);
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback *callback)
{
  COM_TRY_BEGIN
  {
    Close();
    CProgressImp progressImp(callback);
    RINOK(_archive.Open(stream, &progressImp));
    bool showVolName = (_archive.LogVols.Size() > 1);
    for (int volIndex = 0; volIndex < _archive.LogVols.Size(); volIndex++)
    {
      const CLogVol &vol = _archive.LogVols[volIndex];
      bool showFileSetName = (vol.FileSets.Size() > 1);
      for (int fsIndex = 0; fsIndex < vol.FileSets.Size(); fsIndex++)
      {
        const CFileSet &fs = vol.FileSets[fsIndex];
        for (int i = ((showVolName || showFileSetName) ? 0 : 1); i < fs.Refs.Size(); i++)
        {
          CRef2 ref2;
          ref2.Vol = volIndex;
          ref2.Fs = fsIndex;
          ref2.Ref = i;
          _refs2.Add(ref2);
        }
      }
    }
    _inStream = stream;
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _inStream.Release();
  _archive.Clear();
  _refs2.Clear();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _refs2.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  {
    const CRef2 &ref2 = _refs2[index];
    const CLogVol &vol = _archive.LogVols[ref2.Vol];
    const CRef &ref = vol.FileSets[ref2.Fs].Refs[ref2.Ref];
    const CFile &file = _archive.Files[ref.FileIndex];
    const CItem &item = _archive.Items[file.ItemIndex];
    switch(propID)
    {
      case kpidPath:  prop = _archive.GetItemPath(ref2.Vol, ref2.Fs, ref2.Ref,
            _archive.LogVols.Size() > 1, vol.FileSets.Size() > 1); break;
      case kpidIsDir:  prop = item.IsDir(); break;
      case kpidSize:      if (!item.IsDir()) prop = (UInt64)item.Size; break;
      case kpidPackSize:  if (!item.IsDir()) prop = (UInt64)item.NumLogBlockRecorded * vol.BlockSize; break;
      case kpidMTime:  UdfTimeToFileTime(item.MTime, prop); break;
      case kpidATime:  UdfTimeToFileTime(item.ATime, prop); break;
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

class CUdfInStream:
  public ISequentialInStream,
  public CMyUnknownImp
{
  MY_UNKNOWN_IMP

  STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize);
  UInt64 _rem;
public:
  CInArchive *_archive;
  CMyComPtr<IInStream> _stream;
  CRef2 _ref2;
  int _extentIndex;
  UInt32 _offsetInExtent;

  void Init(UInt64 size)
  {
    _extentIndex = 0;
    _offsetInExtent = 0;
    _rem = size;
  }
  void ReleaseStream() { _stream.Release(); }
};

STDMETHODIMP CUdfInStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;
  if (size > _rem)
    size = (UInt32)_rem;
  while (size > 0)
  {
    const CLogVol &vol = _archive->LogVols[_ref2.Vol];
    const CRef &ref = vol.FileSets[_ref2.Fs].Refs[_ref2.Ref];
    const CFile &file = _archive->Files[ref.FileIndex];
    const CItem &item = _archive->Items[file.ItemIndex];

    HRESULT res = S_OK;
    if (item.IsInline)
    {
      size_t rem = item.InlineData.GetCapacity() - _offsetInExtent;
      if (rem == 0)
        return S_OK;
      if (rem > _rem)
        rem = (size_t)_rem;
      memcpy(data, (const Byte *)item.InlineData + _offsetInExtent, rem);
    }
    else
    {
      if (_extentIndex >= item.Extents.Size())
        return S_OK;
      const CMyExtent &extent = item.Extents[_extentIndex];
      UInt32 rem = extent.GetLen() - _offsetInExtent;
      if (rem == 0)
      {
        _extentIndex++;
        _offsetInExtent = 0;
        continue;
      }
      if (size > rem)
        size = rem;
      
      int partitionIndex = vol.PartitionMaps[extent.PartitionRef].PartitionIndex;
      UInt32 logBlockNumber = extent.Pos;
      const CPartition &partition = _archive->Partitions[partitionIndex];
      UInt64 offset = ((UInt64)partition.Pos << _archive->SecLogSize) +
        (UInt64)logBlockNumber * vol.BlockSize + _offsetInExtent;
      
      RINOK(_stream->Seek(offset, STREAM_SEEK_SET, NULL));
      res = _stream->Read(data, size, &size);
    }
    _offsetInExtent += size;
    _rem -= size;
    if (processedSize)
      *processedSize = size;
    return res;
  }
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 _aTestMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool testMode = (_aTestMode != 0);
  bool allFilesMode = (numItems == UInt32(-1));
  if (allFilesMode)
    numItems = _refs2.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;

  for (i = 0; i < numItems; i++)
  {
    UInt32 index = (allFilesMode ? i : indices[i]);
    const CRef2 &ref2 = _refs2[index];
    const CRef &ref = _archive.LogVols[ref2.Vol].FileSets[ref2.Fs].Refs[ref2.Ref];
    const CFile &file = _archive.Files[ref.FileIndex];
    const CItem &item = _archive.Items[file.ItemIndex];
    if (!item.IsDir())
      totalSize += item.Size;
  }
  extractCallback->SetTotal(totalSize);

  UInt64 currentTotalSize = 0;
  UInt64 currentItemSize;
  
  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder();
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  CUdfInStream *udfInStreamSpec = new CUdfInStream();
  CMyComPtr<ISequentialInStream> udfInStream = udfInStreamSpec;

  udfInStreamSpec->_archive = &_archive;
  udfInStreamSpec->_stream = _inStream;

  CDummyOutStream *outStreamSpec = new CDummyOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);

  for (i = 0; i < numItems; i++, currentTotalSize += currentItemSize)
  {
    lps->InSize = lps->OutSize = currentTotalSize;
    RINOK(lps->SetCur());
    currentItemSize = 0;
    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ?
        NArchive::NExtract::NAskMode::kTest :
        NArchive::NExtract::NAskMode::kExtract;
    UInt32 index = allFilesMode ? i : indices[i];
    
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    const CRef2 &ref2 = _refs2[index];
    const CRef &ref = _archive.LogVols[ref2.Vol].FileSets[ref2.Fs].Refs[ref2.Ref];
    const CFile &file = _archive.Files[ref.FileIndex];
    const CItem &item = _archive.Items[file.ItemIndex];

    if (item.IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
      continue;
    }
    currentItemSize = item.Size;

    if (!testMode && (!realOutStream))
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));

    outStreamSpec->SetStream(realOutStream);
    realOutStream.Release();
    outStreamSpec->Init();
    Int32 opRes = NArchive::NExtract::NOperationResult::kUnSupportedMethod;
    if (item.IsRecAndAlloc() && item.CheckChunkSizes() && _archive.CheckItemExtents(ref2.Vol, item))
    {
      udfInStreamSpec->_ref2 = ref2;
      udfInStreamSpec->Init(item.Size);
      RINOK(copyCoder->Code(udfInStream, outStream, NULL, NULL, progress));
      opRes = (outStreamSpec->GetSize() == currentItemSize) ?
          NArchive::NExtract::NOperationResult::kOK:
          NArchive::NExtract::NOperationResult::kDataError;
    }
    outStreamSpec->ReleaseStream();
    RINOK(extractCallback->SetOperationResult(opRes));
  }
  return S_OK;
  COM_TRY_END
}

}}
