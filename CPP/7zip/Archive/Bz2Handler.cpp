// Bz2Handler.cpp

#include "StdAfx.h"

#include "Common/ComTry.h"

#include "Windows/PropVariant.h"

#ifndef _7ZIP_ST
#include "../../Windows/System.h"
#endif

#include "../Common/CreateCoder.h"
#include "../Common/ProgressUtils.h"
#include "../Common/RegisterArc.h"
#include "../Common/StreamUtils.h"

#include "../Compress/BZip2Decoder.h"
#include "../Compress/BZip2Encoder.h"
#include "../Compress/CopyCoder.h"

#include "Common/DummyOutStream.h"
#include "Common/ParseProperties.h"

using namespace NWindows;

namespace NArchive {
namespace NBz2 {

static const UInt32 kNumPassesX1 = 1;
static const UInt32 kNumPassesX7 = 2;
static const UInt32 kNumPassesX9 = 7;

static const UInt32 kDicSizeX1 = 100000;
static const UInt32 kDicSizeX3 = 500000;
static const UInt32 kDicSizeX5 = 900000;

class CHandler:
  public IInArchive,
  public IArchiveOpenSeq,
  public IOutArchive,
  public ISetProperties,
  public CMyUnknownImp
{
  CMyComPtr<IInStream> _stream;
  CMyComPtr<ISequentialInStream> _seqStream;
  UInt64 _packSize;
  UInt64 _startPosition;
  bool _packSizeDefined;

  UInt32 _level;
  UInt32 _dicSize;
  UInt32 _numPasses;
  #ifndef _7ZIP_ST
  UInt32 _numThreads;
  #endif

  void InitMethodProperties()
  {
    _level = 5;
    _dicSize =
    _numPasses = 0xFFFFFFFF;
    #ifndef _7ZIP_ST
    _numThreads = NWindows::NSystem::GetNumberOfProcessors();;
    #endif
  }

public:
  MY_UNKNOWN_IMP4(IInArchive, IArchiveOpenSeq, IOutArchive, ISetProperties)

  INTERFACE_IInArchive(;)
  INTERFACE_IOutArchive(;)
  STDMETHOD(OpenSeq)(ISequentialInStream *stream);
  STDMETHOD(SetProperties)(const wchar_t **names, const PROPVARIANT *values, Int32 numProps);

  CHandler() { InitMethodProperties(); }
};

STATPROPSTG kProps[] =
{
  { NULL, kpidPackSize, VT_UI8}
};

IMP_IInArchive_Props
IMP_IInArchive_ArcProps_NO_Table

STDMETHODIMP CHandler::GetArchiveProperty(PROPID propID, PROPVARIANT *value)
{
  NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidPhySize: if (_packSizeDefined) prop = _packSize; break;
  }
  prop.Detach(value);
  return S_OK;
}

STDMETHODIMP CHandler::GetNumberOfItems(UInt32 *numItems)
{
  *numItems = 1;
  return S_OK;
}

STDMETHODIMP CHandler::GetProperty(UInt32 /* index */, PROPID propID,  PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant prop;
  switch(propID)
  {
    case kpidPackSize: if (_packSizeDefined) prop = _packSize; break;
  }
  prop.Detach(value);
  return S_OK;
}

STDMETHODIMP CHandler::Open(IInStream *stream,
    const UInt64 * /* maxCheckStartPosition */,
    IArchiveOpenCallback * /* openArchiveCallback */)
{
  COM_TRY_BEGIN
  try
  {
    Close();
    RINOK(stream->Seek(0, STREAM_SEEK_CUR, &_startPosition));
    const int kSignatureSize = 3;
    Byte buf[kSignatureSize];
    RINOK(ReadStream_FALSE(stream, buf, kSignatureSize));
    if (buf[0] != 'B' || buf[1] != 'Z' || buf[2] != 'h')
      return S_FALSE;

    UInt64 endPosition;
    RINOK(stream->Seek(0, STREAM_SEEK_END, &endPosition));
    _packSize = endPosition - _startPosition;
    _packSizeDefined = true;
    _stream = stream;
    _seqStream = stream;
  }
  catch(...) { return S_FALSE; }
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
  _packSizeDefined = false;
  _seqStream.Release();
  _stream.Release();
  return S_OK;
}

STDMETHODIMP CHandler::Extract(const UInt32 *indices, UInt32 numItems,
    Int32 testMode, IArchiveExtractCallback *extractCallback)
{
  COM_TRY_BEGIN
  if (numItems == 0)
    return S_OK;
  if (numItems != (UInt32)-1 && (numItems != 1 || indices[0] != 0))
    return E_INVALIDARG;

  if (_stream)
    extractCallback->SetTotal(_packSize);
  UInt64 currentTotalPacked = 0;
  RINOK(extractCallback->SetCompleted(&currentTotalPacked));
  CMyComPtr<ISequentialOutStream> realOutStream;
  Int32 askMode = testMode ?
      NExtract::NAskMode::kTest :
      NExtract::NAskMode::kExtract;
  RINOK(extractCallback->GetStream(0, &realOutStream, askMode));
  if (!testMode && !realOutStream)
    return S_OK;

  extractCallback->PrepareOperation(askMode);

  NCompress::NBZip2::CDecoder *decoderSpec = new NCompress::NBZip2::CDecoder;
  CMyComPtr<ICompressCoder> decoder = decoderSpec;

  if (_stream)
  {
    RINOK(_stream->Seek(_startPosition, STREAM_SEEK_SET, NULL));
  }

  decoderSpec->SetInStream(_seqStream);

  #ifndef _7ZIP_ST
  RINOK(decoderSpec->SetNumberOfThreads(_numThreads));
  #endif

  CDummyOutStream *outStreamSpec = new CDummyOutStream;
  CMyComPtr<ISequentialOutStream> outStream(outStreamSpec);
  outStreamSpec->SetStream(realOutStream);
  outStreamSpec->Init();
  
  realOutStream.Release();

  CLocalProgress *lps = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> progress = lps;
  lps->Init(extractCallback, true);

  HRESULT result = S_OK;

  bool firstItem = true;
  for (;;)
  {
    lps->InSize = currentTotalPacked;
    lps->OutSize = outStreamSpec->GetSize();

    RINOK(lps->SetCur());

    bool isBz2;
    result = decoderSpec->CodeResume(outStream, isBz2, progress);

    if (result != S_OK)
      break;
    if (!isBz2)
    {
      if (firstItem)
        result = S_FALSE;
      break;
    }
    firstItem = false;

    _packSize = currentTotalPacked = decoderSpec->GetInputProcessedSize();
    _packSizeDefined = true;
  }
  decoderSpec->ReleaseInStream();
  outStream.Release();

  Int32 retResult;
  if (result == S_OK)
    retResult = NExtract::NOperationResult::kOK;
  else if (result == S_FALSE)
    retResult = NExtract::NOperationResult::kDataError;
  else
    return result;
  return extractCallback->SetOperationResult(retResult);

  COM_TRY_END
}

static HRESULT UpdateArchive(
    UInt64 unpackSize,
    ISequentialOutStream *outStream,
    int indexInClient,
    UInt32 dictionary,
    UInt32 numPasses,
    #ifndef _7ZIP_ST
    UInt32 numThreads,
    #endif
    IArchiveUpdateCallback *updateCallback)
{
  RINOK(updateCallback->SetTotal(unpackSize));
  UInt64 complexity = 0;
  RINOK(updateCallback->SetCompleted(&complexity));

  CMyComPtr<ISequentialInStream> fileInStream;

  RINOK(updateCallback->GetStream(indexInClient, &fileInStream));

  CLocalProgress *localProgressSpec = new CLocalProgress;
  CMyComPtr<ICompressProgressInfo> localProgress = localProgressSpec;
  localProgressSpec->Init(updateCallback, true);
  
  NCompress::NBZip2::CEncoder *encoderSpec = new NCompress::NBZip2::CEncoder;
  CMyComPtr<ICompressCoder> encoder = encoderSpec;
  {
    NWindows::NCOM::CPropVariant properties[] =
    {
      dictionary,
      numPasses
      #ifndef _7ZIP_ST
      , numThreads
      #endif
    };
    PROPID propIDs[] =
    {
      NCoderPropID::kDictionarySize,
      NCoderPropID::kNumPasses
      #ifndef _7ZIP_ST
      , NCoderPropID::kNumThreads
      #endif
    };
    RINOK(encoderSpec->SetCoderProperties(propIDs, properties, sizeof(propIDs) / sizeof(propIDs[0])));
  }
  
  RINOK(encoder->Code(fileInStream, outStream, NULL, NULL, localProgress));
  
  return updateCallback->SetOperationResult(NArchive::NUpdate::NOperationResult::kOK);
}

STDMETHODIMP CHandler::GetFileTimeType(UInt32 *type)
{
  *type = NFileTimeType::kUnix;
  return S_OK;
}

STDMETHODIMP CHandler::UpdateItems(ISequentialOutStream *outStream, UInt32 numItems,
    IArchiveUpdateCallback *updateCallback)
{
  if (numItems != 1)
    return E_INVALIDARG;

  Int32 newData, newProps;
  UInt32 indexInArchive;
  if (!updateCallback)
    return E_FAIL;
  RINOK(updateCallback->GetUpdateItemInfo(0, &newData, &newProps, &indexInArchive));
 
  if (IntToBool(newProps))
  {
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidIsDir, &prop));
      if (prop.vt == VT_BOOL)
      {
        if (prop.boolVal != VARIANT_FALSE)
          return E_INVALIDARG;
      }
      else if (prop.vt != VT_EMPTY)
        return E_INVALIDARG;
    }
  }
  
  if (IntToBool(newData))
  {
    UInt64 size;
    {
      NCOM::CPropVariant prop;
      RINOK(updateCallback->GetProperty(0, kpidSize, &prop));
      if (prop.vt != VT_UI8)
        return E_INVALIDARG;
      size = prop.uhVal.QuadPart;
    }
  
    UInt32 dicSize = _dicSize;
    if (dicSize == 0xFFFFFFFF)
      dicSize = (_level >= 5 ? kDicSizeX5 :
                (_level >= 3 ? kDicSizeX3 :
                               kDicSizeX1));

    UInt32 numPasses = _numPasses;
    if (numPasses == 0xFFFFFFFF)
      numPasses = (_level >= 9 ? kNumPassesX9 :
                  (_level >= 7 ? kNumPassesX7 :
                                 kNumPassesX1));

    return UpdateArchive(
        size, outStream, 0, dicSize, numPasses,
        #ifndef _7ZIP_ST
        _numThreads,
        #endif
        updateCallback);
  }
  if (indexInArchive != 0)
    return E_INVALIDARG;
  if (_stream)
    RINOK(_stream->Seek(_startPosition, STREAM_SEEK_SET, NULL));
  return NCompress::CopyStream(_stream, outStream, NULL);
}

STDMETHODIMP CHandler::SetProperties(const wchar_t **names, const PROPVARIANT *values, Int32 numProps)
{
  InitMethodProperties();
  #ifndef _7ZIP_ST
  const UInt32 numProcessors = NSystem::GetNumberOfProcessors();
  _numThreads = numProcessors;
  #endif

  for (int i = 0; i < numProps; i++)
  {
    UString name = names[i];
    name.MakeUpper();
    if (name.IsEmpty())
      return E_INVALIDARG;
    const PROPVARIANT &prop = values[i];
    if (name[0] == L'X')
    {
      UInt32 level = 9;
      RINOK(ParsePropValue(name.Mid(1), prop, level));
      _level = level;
    }
    else if (name[0] == L'D')
    {
      UInt32 dicSize = kDicSizeX5;
      RINOK(ParsePropDictionaryValue(name.Mid(1), prop, dicSize));
      _dicSize = dicSize;
    }
    else if (name.Left(4) == L"PASS")
    {
      UInt32 num = kNumPassesX9;
      RINOK(ParsePropValue(name.Mid(4), prop, num));
      _numPasses = num;
    }
    else if (name.Left(2) == L"MT")
    {
      #ifndef _7ZIP_ST
      RINOK(ParseMtProp(name.Mid(2), prop, numProcessors, _numThreads));
      #endif
    }
    else
      return E_INVALIDARG;
  }
  return S_OK;
}

static IInArchive *CreateArc() { return new CHandler; }
#ifndef EXTRACT_ONLY
static IOutArchive *CreateArcOut() { return new CHandler; }
#else
#define CreateArcOut 0
#endif

static CArcInfo g_ArcInfo =
  { L"bzip2", L"bz2 bzip2 tbz2 tbz", L"* * .tar .tar", 2, { 'B', 'Z', 'h' }, 3, true, CreateArc, CreateArcOut };

REGISTER_ARC(BZip2)

}}
