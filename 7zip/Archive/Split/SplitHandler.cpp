// Tar/Handler.cpp

#include "StdAfx.h"

#include "SplitHandler.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/NewHandler.h"
#include "Common/ComTry.h"

#include "Windows/Time.h"
#include "Windows/PropVariant.h"

#include "../../Common/ProgressUtils.h"
#include "../../Compress/Copy/CopyCoder.h"
#include "../Common/ItemNameUtils.h"
#include "../Common/MultiStream.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NSplit {

STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
//  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},
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

class CSeqName
{
public:
  UString _unchangedPart;
  UString _changedPart;    
  bool _splitStyle;
  UString GetNextName()
  {
    UString newName; 
    if (_splitStyle)
    {
      int i;
      int numLetters = _changedPart.Length();
      for (i = numLetters - 1; i >= 0; i--)
      {
        wchar_t c = _changedPart[i];
        if (c == 'z')
        {
          c = 'a';
          newName = c + newName;
          continue;
        }
        else if (c == 'Z')
        {
          c = 'A';
          newName = c + newName;
          continue;
        }
        c++;
        if ((c == 'z' || c == 'Z') && i == 0)
        {
          _unchangedPart += c;
          wchar_t newChar = (c == 'z') ? L'a' : L'A';
          newName.Empty();
          numLetters++;
          for (int k = 0; k < numLetters; k++)
            newName += newChar;
          break;
        }
        newName = c + newName;
        i--;
        for (; i >= 0; i--)
          newName = _changedPart[i] + newName;
        break;
      }
    }
    else
    {
      int i;
      int numLetters = _changedPart.Length();
      for (i = numLetters - 1; i >= 0; i--)
      {
        wchar_t c = _changedPart[i];
        if (c == L'9')
        {
          c = L'0';
          newName = c + newName;
          if (i == 0)
            newName = UString(L'1') + newName;
          continue;
        }
        c++;
        newName = c + newName;
        i--;
        for (; i >= 0; i--)
          newName = _changedPart[i] + newName;
        break;
      }
    }
    _changedPart = newName;
    return _unchangedPart + _changedPart;
  }
};

STDMETHODIMP CHandler::Open(IInStream *stream, 
    const UInt64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  Close();
  if (openArchiveCallback == 0)
    return S_FALSE;
  bool mustBeClosed = true;
  // try
  {
    CMyComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
    CMyComPtr<IArchiveOpenCallback> openArchiveCallbackWrap = openArchiveCallback;
    if (openArchiveCallbackWrap.QueryInterface(IID_IArchiveOpenVolumeCallback, 
        &openVolumeCallback) != S_OK)
      return S_FALSE;
    
    {
      NCOM::CPropVariant propVariant;
      RINOK(openVolumeCallback->GetProperty(kpidName, &propVariant));
      if (propVariant.vt != VT_BSTR)
        return S_FALSE;
      _name = propVariant.bstrVal;
    }
    
    int dotPos = _name.ReverseFind('.');
    UString prefix, ext;
    if (dotPos >= 0)
    {
      prefix = _name.Left(dotPos + 1);
      ext = _name.Mid(dotPos + 1);
    }
    else
      ext = _name;
    UString extBig = ext;
    extBig.MakeUpper();

    CSeqName seqName;

    int numLetters = 2;
    bool splitStyle = false;
    if (extBig.Right(2) == L"AA")
    {
      splitStyle = true;
      while (numLetters < extBig.Length())
      {
        if (extBig[extBig.Length() - numLetters - 1] != 'A')
          break;
        numLetters++;
      }
    }
    else if (ext.Right(2) == L"01")
    {
      while (numLetters < extBig.Length())
      {
        if (extBig[extBig.Length() - numLetters - 1] != '0')
          break;
        numLetters++;
      }
      if (numLetters != ext.Length())
        return S_FALSE;
    }
    else 
      return S_FALSE;

    _streams.Add(stream);

    seqName._unchangedPart = prefix + ext.Left(extBig.Length() - numLetters);
    seqName._changedPart = ext.Right(numLetters);
    seqName._splitStyle = splitStyle;

    if (prefix.Length() < 1)
      _subName = L"file";
    else
      _subName = prefix.Left(prefix.Length() - 1);

    _totalSize = 0;
    UInt64 size;
    {
      NCOM::CPropVariant propVariant;
      RINOK(openVolumeCallback->GetProperty(kpidSize, &propVariant));
      if (propVariant.vt != VT_UI8)
        return E_INVALIDARG;
      size = *(UInt64 *)(&propVariant.uhVal);
    }
    _totalSize += size;
    _sizes.Add(size);
    
    if (openArchiveCallback != NULL)
    {
      RINOK(openArchiveCallback->SetTotal(NULL, NULL));
      UInt64 numFiles = _streams.Size();
      RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
    }

    while (true)
    {
      UString fullName = seqName.GetNextName();
      CMyComPtr<IInStream> nextStream;
      HRESULT result = openVolumeCallback->GetStream(fullName, &nextStream);
      if (result == S_FALSE)
        break;
      if (result != S_OK)
        return result;
      if (!stream)
        break;
      {
        NCOM::CPropVariant propVariant;
        RINOK(openVolumeCallback->GetProperty(kpidSize, &propVariant));
        if (propVariant.vt != VT_UI8)
          return E_INVALIDARG;
        size = *(UInt64 *)(&propVariant.uhVal);
      }
      _totalSize += size;
      _sizes.Add(size);
      _streams.Add(nextStream);
      if (openArchiveCallback != NULL)
      {
        UInt64 numFiles = _streams.Size();
        RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
      }
    }
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
  _streams.Clear();
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = _streams.IsEmpty() ? 0 : 1;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
  COM_TRY_BEGIN
  NWindows::NCOM::CPropVariant propVariant;

  switch(propID)
  {
    case kpidPath:
      propVariant = _subName;
      break;
    case kpidIsFolder:
      propVariant = false;
      break;
    case kpidSize:
    case kpidPackedSize:
      propVariant = _totalSize;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::Extract(const UInt32* indices, UInt32 numItems,
    Int32 _aTestMode, IArchiveExtractCallback *_anExtractCallback)
{
  COM_TRY_BEGIN

  if (numItems != UInt32(-1))
  {
    if (numItems != 1)
      return E_INVALIDARG;
    if (indices[0] != 0)
      return E_INVALIDARG;
  }
  bool testMode = (_aTestMode != 0);
  CMyComPtr<IArchiveExtractCallback> extractCallback = _anExtractCallback;
  extractCallback->SetTotal(_totalSize);
  
  /*
  CMyComPtr<IArchiveVolumeExtractCallback> volumeExtractCallback;
  if (extractCallback.QueryInterface(&volumeExtractCallback) != S_OK)
    return E_FAIL;
  */

  UInt64 currentTotalSize = 0;
  UInt64 currentItemSize;

  RINOK(extractCallback->SetCompleted(&currentTotalSize));
  CMyComPtr<ISequentialOutStream> realOutStream;
  Int32 askMode;
  askMode = testMode ? NArchive::NExtract::NAskMode::kTest :
  NArchive::NExtract::NAskMode::kExtract;
  Int32 index = 0;
  RINOK(extractCallback->GetStream(index, &realOutStream, askMode));
  
  RINOK(extractCallback->PrepareOperation(askMode));
  if (testMode)
  {
    RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
    return S_OK;
  }
  // currentItemSize = itemInfo.Size;
  
  if(!testMode && (!realOutStream))
  {
    return S_OK;
  }

  NCompress::CCopyCoder *copyCoderSpec = new NCompress::CCopyCoder;
  CMyComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  for(int i = 0; i < _streams.Size(); i++, currentTotalSize += currentItemSize)
  {
    // CMyComPtr<ISequentialInStream> inStream;
    // RINOK(volumeExtractCallback->GetInStream(_names[i], &inStream));

    CLocalProgress *localProgressSpec = new CLocalProgress;
    CMyComPtr<ICompressProgressInfo> progress = localProgressSpec;
    localProgressSpec->Init(extractCallback, false);
    CLocalCompressProgressInfo *localCompressProgressSpec = 
        new CLocalCompressProgressInfo;
    CMyComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
    localCompressProgressSpec->Init(progress, 
      &currentTotalSize, &currentTotalSize);
    IInStream *inStream = _streams[i];
    RINOK(inStream->Seek(0, STREAM_SEEK_SET, NULL));
    try
    {
      RINOK(copyCoder->Code(inStream, realOutStream,
          NULL, NULL, compressProgress));
    }
    catch(...)
    {
      realOutStream.Release();
      RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kDataError));
      return S_OK;;
    }
    currentItemSize = copyCoderSpec->TotalSize;
  }
  realOutStream.Release();
  RINOK(extractCallback->SetOperationResult(NArchive::NExtract::NOperationResult::kOK));
  return S_OK;
  COM_TRY_END
}

STDMETHODIMP CHandler::GetStream(UInt32 index, ISequentialInStream **stream)
{
  if (index != 0)
    return E_INVALIDARG;
  *stream = 0;
  CMultiStream *streamSpec = new CMultiStream;
  CMyComPtr<ISequentialInStream> streamTemp = streamSpec;
  for (int i = 0; i < _streams.Size(); i++)
  {
    CMultiStream::CSubStreamInfo subStreamInfo;
    subStreamInfo.Stream = _streams[i];
    subStreamInfo.Pos = 0;
    subStreamInfo.Size = _sizes[i];
    streamSpec->Streams.Add(subStreamInfo);
  }
  streamSpec->Init();
  *stream = streamTemp.Detach();
  return S_OK;
}

}}
