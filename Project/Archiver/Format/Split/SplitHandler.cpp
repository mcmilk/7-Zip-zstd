// Tar/Handler.cpp

#include "StdAfx.h"

#include "SplitHandler.h"

#include "Common/Defs.h"
#include "Common/StringConvert.h"
#include "Common/NewHandler.h"

#include "Interface/ProgressUtils.h"
#include "Interface/EnumStatProp.h"

#include "Windows/Time.h"
#include "Windows/PropVariant.h"
#include "Windows/COMTry.h"

#include "Compression/CopyCoder.h"

#include "Archive/Common/ItemNameUtils.h"

using namespace NWindows;
using namespace NTime;

namespace NArchive {
namespace NSplit {

STATPROPSTG kProperties[] = 
{
  { NULL, kpidPath, VT_BSTR},
  { NULL, kpidIsFolder, VT_BOOL},
  { NULL, kpidSize, VT_UI8},
  { NULL, kpidPackedSize, VT_UI8},
};

STDMETHODIMP CHandler::EnumProperties(IEnumSTATPROPSTG **enumerator)
{
  COM_TRY_BEGIN
  return CStatPropEnumerator::CreateEnumerator(kProperties, 
      sizeof(kProperties) / sizeof(kProperties[0]), enumerator);
  COM_TRY_END
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
    const UINT64 *maxCheckStartPosition,
    IArchiveOpenCallback *openArchiveCallback)
{
  COM_TRY_BEGIN
  Close();
  if (openArchiveCallback == 0)
    return S_FALSE;
  bool mustBeClosed = true;
  // try
  {
    CComPtr<IArchiveOpenVolumeCallback> openVolumeCallback;
    CComPtr<IArchiveOpenCallback> openArchiveCallbackWrap = openArchiveCallback;
    if (openArchiveCallbackWrap.QueryInterface(&openVolumeCallback) != S_OK)
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
    UINT64 size;
    {
      NCOM::CPropVariant propVariant;
      RINOK(openVolumeCallback->GetProperty(kpidSize, &propVariant));
      if (propVariant.vt != VT_UI8)
        return E_INVALIDARG;
      size = *(UINT64 *)(&propVariant.uhVal);
    }
    _totalSize += size;
    
    if (openArchiveCallback != NULL)
    {
      RINOK(openArchiveCallback->SetTotal(NULL, NULL));
      UINT64 numFiles = _streams.Size();
      RINOK(openArchiveCallback->SetCompleted(&numFiles, NULL));
    }

    while (true)
    {
      UString fullName = seqName.GetNextName();
      CComPtr<IInStream> nextStream;
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
        size = *(UINT64 *)(&propVariant.uhVal);
      }
      _totalSize += size;
      _streams.Add(nextStream);
      if (openArchiveCallback != NULL)
      {
        UINT64 numFiles = _streams.Size();
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

STDMETHODIMP CHandler::GetNumberOfItems(UINT32 *numItems)
{
  *numItems = _streams.IsEmpty() ? 0 : 1;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UINT32 index, PROPID propID, PROPVARIANT *value)
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

//////////////////////////////////////
// CHandler::DecompressItems

STDMETHODIMP CHandler::Extract(const UINT32* indices, UINT32 numItems,
    INT32 _aTestMode, IArchiveExtractCallback *_anExtractCallback)
{
  COM_TRY_BEGIN

  if (numItems != 1)
    return E_INVALIDARG;
  if (indices[0] != 0)
    return E_INVALIDARG;


  bool testMode = (_aTestMode != 0);
  CComPtr<IArchiveExtractCallback> extractCallback = _anExtractCallback;
  extractCallback->SetTotal(_totalSize);
  
  /*
  CComPtr<IArchiveVolumeExtractCallback> volumeExtractCallback;
  if (extractCallback.QueryInterface(&volumeExtractCallback) != S_OK)
    return E_FAIL;
  */

  UINT64 currentTotalSize = 0;
  UINT64 currentItemSize;

  RINOK(extractCallback->SetCompleted(&currentTotalSize));
  CComPtr<ISequentialOutStream> realOutStream;
  INT32 askMode;
  askMode = testMode ? NArchive::NExtract::NAskMode::kTest :
  NArchive::NExtract::NAskMode::kExtract;
  INT32 index = 0;
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

  
  CComObjectNoLock<NCompression::CCopyCoder> *copyCoderSpec = new CComObjectNoLock<NCompression::CCopyCoder>;
  CComPtr<ICompressCoder> copyCoder = copyCoderSpec;

  for(int i = 0; i < _streams.Size(); i++, currentTotalSize += currentItemSize)
  {
    // CComPtr<ISequentialInStream> inStream;
    // RINOK(volumeExtractCallback->GetInStream(_names[i], &inStream));

    CComObjectNoLock<CLocalProgress> *localProgressSpec = new  CComObjectNoLock<CLocalProgress>;
    CComPtr<ICompressProgressInfo> progress = localProgressSpec;
    localProgressSpec->Init(extractCallback, false);
    CComObjectNoLock<CLocalCompressProgressInfo> *localCompressProgressSpec = 
      new  CComObjectNoLock<CLocalCompressProgressInfo>;
    CComPtr<ICompressProgressInfo> compressProgress = localCompressProgressSpec;
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

STDMETHODIMP CHandler::ExtractAllItems(INT32 testMode,
      IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  CRecordVector<UINT32> indices;
  indices.Add(0);
  return Extract(&indices.Front(), 1, testMode, extractCallback);
  COM_TRY_END
}

}}
