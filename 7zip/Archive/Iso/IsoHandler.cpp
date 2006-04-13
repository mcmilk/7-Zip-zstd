// Iso/Handler.cpp

#include "StdAfx.h"

#include "IsoHandler.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/IntToString.h"
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
namespace NIso {

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
  bool mustBeClosed = true;
  Close();
  // try
  {
    if(_archive.Open(stream) != S_OK)
      return S_FALSE;
    _inStream = stream;
  }
  // catch(...) { return S_FALSE; }
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Close()
{
  _archive.Clear();
  _inStream.Release();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _archive.Refs.Size() + _archive.BootEntries.Size();
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;
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
        propVariant = (const wchar_t *)s;
        break;
      }
      case kpidIsFolder:
        propVariant = false;
        break;
      case kpidSize:
      case kpidPackedSize:
      {
        propVariant = (UInt64)_archive.GetBootItemSize(index);
        break;
      }
    }
  }
  else
  {
    const CRef &ref = _archive.Refs[index];
    const CDir &item = ref.Dir->_subItems[ref.Index];
    switch(propID)
    {
      case kpidPath:
        if (item.FileId.GetCapacity() >= 0)
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
          propVariant = (const wchar_t *)NItemName::GetOSName2(s);
        }
        break;
      case kpidIsFolder:
        propVariant = item.IsDir();
        break;
      case kpidSize:
      case kpidPackedSize:
        if (!item.IsDir())
          propVariant = (UInt64)item.DataLength;
        break;
      case kpidLastWriteTime:
      {
        FILETIME utcFileTime;
        if (item.DateTime.GetFileTime(utcFileTime))
          propVariant = utcFileTime;
        /*
        else
        {
          utcFileTime.dwLowDateTime = 0;
          utcFileTime.dwHighDateTime = 0;
        }
        */
        break;
      }
    }
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
    numItems = _archive.Refs.Size();
  UInt64 totalSize = 0;
  if(numItems == 0)
    return S_OK;
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
  
  CMyComPtr<ICompressCoder> copyCoder;

  for (i = 0; i < numItems; i++, currentTotalSize += currentItemSize)
  {
    currentItemSize = 0;
    RINOK(extractCallback->SetCompleted(&currentTotalSize));
    CMyComPtr<ISequentialOutStream> realOutStream;
    Int32 askMode;
    askMode = testMode ? NArchive::NExtract::NAskMode::kTest : NArchive::NExtract::NAskMode::kExtract;
    UInt32 index = allFilesMode ? i : indices[i];
    
    RINOK(extractCallback->GetStream(index, &realOutStream, askMode));

    UInt64 blockIndex;
    if (index < (UInt32)_archive.Refs.Size())
    {
      const CRef &ref = _archive.Refs[index];
      const CDir &item = ref.Dir->_subItems[ref.Index];
      if(item.IsDir())
      {
        RINOK(extractCallback->PrepareOperation(askMode));
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
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
   
    if(!testMode && (!realOutStream))
      continue;

    RINOK(extractCallback->PrepareOperation(askMode));
    {
      if (testMode)
      {
        RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
        continue;
      }

      RINOK(_inStream->Seek(blockIndex * _archive.BlockSize, STREAM_SEEK_SET, NULL));
      CLimitedSequentialInStream *streamSpec = new CLimitedSequentialInStream;
      CMyComPtr<ISequentialInStream> inStream(streamSpec);
      streamSpec->Init(_inStream, currentItemSize);

      CLocalProgress *localProgressSpec = new CLocalProgress;
      CMyComPtr<ICompressProgressInfo> progress = localProgressSpec;
      localProgressSpec->Init(extractCallback, false);

      CLocalCompressProgressInfo *localCompressProgressSpec = new CLocalCompressProgressInfo;
      CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
      localCompressProgressSpec->Init(progress, &currentTotalSize, &currentTotalSize);

      Int32 res = NArchive::NExtract::NOperationResult::kOK;
      if(!copyCoder)
      {
        copyCoder = new NCompress::CCopyCoder;
      }
      try
      {
        RINOK(copyCoder->Code(inStream, realOutStream, NULL, NULL, compressProgress));
      }
      catch(...)
      {
        res = NArchive::NExtract::NOperationResult::kDataError;
      }
      realOutStream.Release();
      RINOK(extractCallback->SetOperationResult(res));
    }
  }
  return S_OK;
  COM_TRY_END
}

}}
