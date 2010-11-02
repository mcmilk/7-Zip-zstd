// IsoHandler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"
#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#include "Windows/PropVariant.h"
#include "Windows/Time.h"

#include "../../Common/LimitedStreams.h"
#include "../../Common/ProgressUtils.h"

#include "../../Compress/CopyCoder.h"

#include "../Common/ItemNameUtils.h"

#include "IsoHandler.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NIso {

static const STATPROPSTG kProps[] =
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsDir, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackSize, VT_UI8},
  { NULL, kpidMTime, VT_FILETIME}
};

static const STATPROPSTG kArcProps[] =
{
  { NULL, kpidComment, VT_BSTR},
  { NULL, kpidCTime, VT_FILETIME},
  { NULL, kpidMTime, VT_FILETIME}
  // { NULL, kpidPhySize, VT_UI8},
  // { NULL, kpidHeadersSize, VT_UI8}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */)
{
  COM_TRY_BEGIN
  Close();
  // try
  {
    if (_archive.Open(stream) != S_OK)
      return S_FALSE;
    _stream = stream;
  }
  // catch(...) { return S_FALSE; }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _archive.Clear();
  _stream.Release();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _archive.Refs.Size() + _archive.BootEntries.Size();
  return S_OK;
}

static void AddString(AString &s, const char *name, const Byte *p, int size)
{
  int i;
  for (i = 0; i < size && p[i]; i++);
  for (; i > 0 && p[i - 1] == ' '; i--);
  if (i != 0)
  {
    AString d;
    memcpy(d.GetBuffer(i), p, i);
    d.ReleaseBuffer(i);
    s += '\n';
    s += name;
    s += ": ";
    s += d;
  }
}

#define ADD_STRING(n, v) AddString(s, n, vol. ## v, sizeof(vol. ## v))

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  const CVolumeDescriptor &vol = _archive.VolDescs[_archive.MainVolDescIndex];
  switch(propID)
  {
    case kpidComment:
    {
      AString s;
      ADD_STRING("System", SystemId);
      ADD_STRING("Volume", VolumeId);
      ADD_STRING("VolumeSet", VolumeSetId);
      ADD_STRING("Publisher", PublisherId);
      ADD_STRING("Preparer", DataPreparerId);
      ADD_STRING("Application", ApplicationId);
      ADD_STRING("Copyright", CopyrightFileId);
      ADD_STRING("Abstract", AbstractFileId);
      ADD_STRING("Bib", BibFileId);
      prop = s;
      break;
    }
    case kpidCTime: { FILETIME utc; if (vol.CTime.GetFileTime(utc)) prop = utc; break; }
    case kpidMTime: { FILETIME utc; if (vol.MTime.GetFileTime(utc)) prop = utc; break; }
    // case kpidPhySize: break;
    // case kpidHeadersSize: break;
    case kpidError: if (_archive.IncorrectBigEndian) prop = "Incorrect big-endian headers"; break;
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant prop;
  if (index >= (UInt32)_archive.Refs.Size())
  {
    index -= _archive.Refs.Size();
    const CBootInitialEntry &be = _archive.BootEntries[index];
    switch(propID)
    {
      case kpidPath:
      {
        // wchar_t name[32];
        // ConvertUInt64ToString(index + 1, name);
        UString s = L"[BOOT]" WSTRING_PATH_SEPARATOR;
        // s += name;
        // s += L"-";
        s += be.GetName();
        prop = (const wchar_t *)s;
        break;
      }
      case kpidIsDir: prop = false; break;
      case kpidSize:
      case kpidPackSize:
        prop = (UInt64)_archive.GetBootItemSize(index);
        break;
    }
  }
  else
  {
    const CRef &ref = _archive.Refs[index];
    const CDir &item = ref.Dir->_subItems[ref.Index];
    switch(propID)
    {
      case kpidPath:
        // if (item.FileId.GetCapacity() >= 0)
        {
          UString s;
          if (_archive.IsJoliet())
            s = item.GetPathU();
          else
            s = MultiByteToUnicodeString(item.GetPath(_archive.IsSusp, _archive.SuspSkipSize), CP_OEMCP);

          int pos = s.ReverseFind(L';');
          if (pos >= 0 && pos == s.Length() - 2)
              if (s[s.Length() - 1] == L'1')
                s = s.Left(pos);
          if (!s.IsEmpty())
            if (s[s.Length() - 1] == L'.')
              s = s.Left(s.Length() - 1);
          prop = (const wchar_t *)NItemName::GetOSName2(s);
        }
        break;
      case kpidIsDir: prop = item.IsDir(); break;
      case kpidSize:
      case kpidPackSize:
        if (!item.IsDir())
          prop = (UInt64)item.DataLength;
        break;
      case kpidMTime:
      {
        FILETIME utc;
        if (item.DateTime.GetFileTime(utc))
          prop = utc;
        break;
      }
    }
  }
  prop.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  bool allFilesMode = (numItems == (UInt32)-1);
  if (allFilesMode)
    numItems = _archive.Refs.Size();
  if (numItems == 0)
    return S_OK;
  UInt64 totalSize = 0;
  UInt32 i;
  for(i = 0; i < numItems; i++)
  {
    UInt32 index = (allFilesMode ? i : indices[i]);
    if (index < (UInt32)_archive.Refs.Size())
    {
      const CRef &ref = _archive.Refs[index];
      const CDir &item = ref.Dir->_subItems[ref.Index];
      totalSize += item.DataLength;
    }
    else
    {
      totalSize += _archive.GetBootItemSize(index - _archive.Refs.Size());
    }
  }
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
  streamSpec->SetStream(_stream);

  CLimitedSequentialOutStream *outStreamSpec = new CLimitedSequentialOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);

  for (i = 0; i < numItems; i++, currentTotalSize += currentItemSize)
  {
    lps->InSize = lps->OutSize = currentTotalSize;
    RINOK(lps->SetCur());
    currentItemSize = 0;
    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode = testMode ?
        NExtract::NAskMode::kTest :
        NExtract::NAskMode::kExtract;
    UInt32 index = allFilesMode ? i : indices[i];
    
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    UInt64 blockIndex;
    if (index < (UInt32)_archive.Refs.Size())
    {
      const CRef &ref = _archive.Refs[index];
      const CDir &item = ref.Dir->_subItems[ref.Index];
      if (item.IsDir())
      {
        RINOK(extractCallback->PrepareOperation(askMode));
        RINOK(extractCallback->SetOperationResult(NExtract::NOperationResult::kOK));
        continue;
      }
      currentItemSize = item.DataLength;
      blockIndex = item.ExtentLocation;
    }
    else
    {
      int bootIndex = index - _archive.Refs.Size();
      const CBootInitialEntry &be = _archive.BootEntries[bootIndex];
      currentItemSize = _archive.GetBootItemSize(bootIndex);
      blockIndex = be.LoadRBA;
    }
   
    if (!testMode && !realOutStream)
      continue;
    RINOK(extractCallback->PrepareOperation(askMode));
    outStreamSpec->SetStream(realOutStream);
    realOutStream.Release();
    outStreamSpec->Init(currentItemSize);
    RINOK(_stream->Seek(blockIndex * _archive.BlockSize, STREAM_SEEK_SET, NULL));
    streamSpec->Init(currentItemSize);
    RINOK(copyCoder->Code(inStream, outStream, NULL, NULL, progress));
    outStreamSpec->ReleaseStream();
    RINOK(extractCallback->SetOperationResult(outStreamSpec->IsFinishedOK() ?
        NExtract::NOperationResult::kOK:
        NExtract::NOperationResult::kDataError));
  }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetStream(UInt32 index, ISequentialInStream **stream)
{
  COM_TRY_BEGIN
  *stream = 0;
  UInt64 blockIndex;
  UInt64 currentItemSize;
  if (index < (UInt32)_archive.Refs.Size())
  {
    const CRef &ref = _archive.Refs[index];
    const CDir &item = ref.Dir->_subItems[ref.Index];
    if (item.IsDir())
      return S_FALSE;
    currentItemSize = item.DataLength;
    blockIndex = item.ExtentLocation;
  }
  else
  {
    int bootIndex = index - _archive.Refs.Size();
    const CBootInitialEntry &be = _archive.BootEntries[bootIndex];
    currentItemSize = _archive.GetBootItemSize(bootIndex);
    blockIndex = be.LoadRBA;
  }
  return CreateLimitedInStream(_stream, blockIndex * _archive.BlockSize, currentItemSize, stream);
  COM_TRY_END
}

}}
