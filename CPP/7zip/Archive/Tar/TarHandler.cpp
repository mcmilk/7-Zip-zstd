// TarHandler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../../Common/LimitedStreams.h"
#include "../../Common/ProgressUtils.h"
#include "../../Common/StreamObjects.h"
#include "../../Common/StreamUtils.h"

#include "../Common/ItemNameUtils.h"

#include "TarHandler.h"
#include "TarIn.h"

using namespace NWindows;

namespace NArchive {
namespace NTar {

static const char *kUnexpectedEnd = "Unexpected end of archive";

static const STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsDir, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME},
  { NULL, kpidPosixAttrib, VT_UI4},
  { NULL, kpidUser, VT_BSTR},
  { NULL, kpidGroup, VT_BSTR},
  { NULL, kpidLink, VT_BSTR}
};

static const STATPROPSTG kArcProps[] =
{
  { NULL, kpidPhySize, VT_UI8},
  { NULL, kpidHeadersSize, VT_UI8}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidPhySize: if (_phySizeDefined) prop = _phySize; break;
    case kpidHeadersSize: if (_phySizeDefined) prop = _headersSize; break;
    case kpidError: if (!_errorMessage.IsEmpty()) prop = _errorMessage; break;
  }
  prop.Detach(value);
  return S_OK;
}

HRESULT CHandler::ReadItem2(ISequentialInStream *stream, bool &filled, CItemEx &item)
{
  item.HeaderPos = _phySize;
  RINOK(ReadItem(stream, filled, item, _errorMessage));
  _phySize += item.HeaderSize;
  _headersSize += item.HeaderSize;
  return S_OK;
}

HRESULT CHandler::Open2(IInStream *stream, IArchiveOpenCallback *callback)
{
  UInt64 endPos = 0;
  {
    RINOK(stream->Seek(0, STREAM_SEEK_END, &endPos));
    RINOK(stream->Seek(0, STREAM_SEEK_SET, NULL));
  }
  
  _phySizeDefined = true;
  for (;;)
  {
    CItemEx item;
    bool filled;
    RINOK(ReadItem2(stream, filled, item));
    if (!filled)
      break;
    _items.Add(item);
    
    RINOK(stream->Seek(item.GetPackSize(), STREAM_SEEK_CUR, &_phySize));
    if (_phySize > endPos)
    {
      _errorMessage = kUnexpectedEnd;
      break;
    }
    /*
    if (_phySize == endPos)
    {
      _errorMessage = "There are no trailing zero-filled records";
      break;
    }
    */
    if (callback != NULL)
    {
      if (_items.Size() == 1)
      {
        RINOK(callback->SetTotal(NULL, &endPos));
      }
      if (_items.Size() % 100 == 0)
      {
        UInt64 numFiles = _items.Size();
        RINOK(callback->SetCompleted(&numFiles, &_phySize));
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

STDMETHODIMP CHandler::Open(IInStream *stream, const UInt64 *, IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  {
    Close();
    RINOK(Open2(stream, openArchiveCallback));
    _stream = stream;
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::OpenSeq(ISequentialInStream *stream)
{
  Close();
  _seqStream = stream;
  return S_OK;
}

STDMETHODIMP CHandler::Close()
{
  _errorMessage.Empty();
  _phySizeDefined = false;
  _phySize = 0;
  _headersSize = 0;
  _curIndex = 0;
  _latestIsRead = false;
  _items.Clear();
  _seqStream.Release();
  _stream.Release();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = (_stream ? _items.Size() : (UInt32)(Int32)-1);
  return S_OK;
}

CHandler::CHandler()
{
  copyCoderSpec = new NCompress::CCopyCoder();
  copyCoder = copyCoderSpec;
}

HRESULT CHandler::SkipTo(UInt32 index)
{
  while (_curIndex < index || !_latestIsRead)
  {
    if (_latestIsRead)
    {
      UInt64 packSize = _latestItem.GetPackSize();
      RINOK(copyCoderSpec->Code(_seqStream, NULL, &packSize, &packSize, NULL));
      _phySize += copyCoderSpec->TotalSize;
      if (copyCoderSpec->TotalSize != packSize)
      {
        _errorMessage = kUnexpectedEnd;
        return S_FALSE;
      }
      _latestIsRead = false;
      _curIndex++;
    }
    else
    {
      bool filled;
      RINOK(ReadItem2(_seqStream, filled, _latestItem));
      if (!filled)
      {
        _phySizeDefined = true;
        return E_INVALIDARG;
      }
      _latestIsRead = true;
    }
  }
  return S_OK;
}

static UString TarStringToUnicode(const AString &s)
{
  return MultiByteToUnicodeString(s, CP_OEMCP);
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;

  const CItemEx *item;
  if (_stream)
    item = &_items[index];
  else
  {
    if (index < _curIndex)
      return E_INVALIDARG;
    else
    {
      RINOK(SkipTo(index));
      item = &_latestItem;
    }
  }

  switch(propID)
  {
    case kpidPath: prop = NItemName::GetOSName2(TarStringToUnicode(item->Name)); break;
    case kpidIsDir: prop = item->IsDir(); break;
    case kpidSize: prop = item->GetUnpackSize(); break;
    case kpidPackSize: prop = item->GetPackSize(); break;
    case kpidMTime:
      if (item->MTime != 0)
      {
        FILETIME ft;
        NTime::UnixTimeToFileTime(item->MTime, ft);
        prop = ft;
      }
      break;
    case kpidPosixAttrib: prop = item->Mode; break;
    case kpidUser: prop = TarStringToUnicode(item->User); break;
    case kpidGroup: prop = TarStringToUnicode(item->Group); break;
    case kpidLink: prop = TarStringToUnicode(item->LinkName); break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

HRESULT CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  ISequentialInStream *stream = _seqStream;
  bool seqMode = (_stream == NULL);
  if (!seqMode)
    stream = _stream;

  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = _items.Size();
  if (_stream && numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for (i = 0; i < numItems; i++)
    totalSize += _items[allFilesMode ? i : indices[i]].GetUnpackSize();
  extractCallback->SetTotal(totalSize);

  UInt64 totalPackSize;
  totalSize = totalPackSize = 0;
  
  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, false);

  CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
  CMyComPtr<ISequentialInStream> inStream(streamSpec);
  streamSpec->SetStream(stream);

  CLimitedSequentialOutStream *outStreamSpec = new CLimitedSequentialOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);

  for (i = 0; i < numItems || seqMode; i++)
  {
    lps->InSize = totalPackSize;
    lps->OutSize = totalSize;
    RINOK(lps->SetCur());
    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    Int32 index = allFilesMode ? i : indices[i];
    const CItemEx *item;
    if (seqMode)
    {
      HRESULT res = SkipTo(index);
      if (res == E_INVALIDARG)
        break;
      RINOK(res);
      item = &_latestItem;
    }
    else
      item = &_items[index];

    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
    UInt64 unpackSize = item->GetUnpackSize();
    totalSize += unpackSize;
    totalPackSize += item->GetPackSize();
    if (item->IsDir())
    {
      RINOK(extractCallback->PrepareOperation(askMode));
      RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
      continue;
    }
    bool skipMode = false;
    if (!testMode && !realOutStream)
    {
      if (!seqMode)
        continue;
      skipMode = true;
      askMode = NExtract::NAskMode::kSkip;
    }
    RINOK(extractCallback->PrepareOperation(askMode));

    outStreamSpec->SetStream(realOutStream);
    realOutStream.Release();
    outStreamSpec->Init(skipMode ? 0 : unpackSize, true);

    if (item->IsLink())
    {
      RINOK(WriteStream(outStreamSpec, (const char *)item->LinkName, item->LinkName.Length()));
    }
    else
    {
      if (!seqMode)
      {
        RINOK(_stream->Seek(item->GetDataPosition(), STREAM_SEEK_SET, NULL));
      }
      streamSpec->Init(item->GetPackSize());
      RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress));
    }
    if (seqMode)
    {
      _latestIsRead = false;
      _curIndex++;
    }
    outStreamSpec->ReleaseStream();
    RINOK(extractCallback->SetOperationResult(outStreamSpec->GetRem() == 0 ?
        NExtract::NOperationResult::kOK:
        NExtract::NOperationResult::kDataError));
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetStream(UInt32 index, ISequentialInStream **stream)
{
  COM_TRY_BEGIN
  const CItemEx &item = _items[index];
  if (item.IsLink())
  {
    CBufInStream *streamSpec = new CBufInStream;
    CMyComPtr<IInStream> streamTemp = streamSpec;
    streamSpec->Init((const Byte *)(const char *)item.LinkName, item.LinkName.Length(), (IInArchive *)this);
    *stream = streamTemp.Detach();
    return S_OK;
  }
  return CreateLimitedInStream(_stream, item.GetDataPosition(), item.Size, stream);
  COM_TRY_END
}

}}
