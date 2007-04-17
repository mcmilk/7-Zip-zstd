// NsisDecode.cpp

#include "StdAfx.h"

#include "NsisDecode.h"

#include "../../Common/StreamUtils.h"

#include "../../Common/MethodId.h"
#include "../../Common/CreateCoder.h"

namespace NArchive {
namespace NNsis {

static const CMethodId k_Copy    = 0x0;
static const CMethodId k_Deflate = 0x040901;
static const CMethodId k_BZip2   = 0x040902;
static const CMethodId k_LZMA    = 0x030101;
static const CMethodId k_BCJ_X86 = 0x03030103;

HRESULT CDecoder::Init(
    DECL_EXTERNAL_CODECS_LOC_VARS
    IInStream *inStream, NMethodType::EEnum method, bool thereIsFilterFlag, bool &useFilter)
{
  useFilter = false;
  CObjectVector< CMyComPtr<ISequentialInStream> > inStreams;

  if (_decoderInStream)
    if (method != _method)
      Release();
  _method = method;
  if (!_codecInStream)
  {
    CMethodId methodID;
    switch (method)
    {
      case NMethodType::kCopy: methodID = k_Copy; break;
      case NMethodType::kDeflate: methodID = k_Deflate; break;
      case NMethodType::kBZip2: methodID = k_BZip2; break;
      case NMethodType::kLZMA: methodID = k_LZMA; break;
      default: return E_NOTIMPL;
    }
    CMyComPtr<ICompressCoder> coder;
    RINOK(CreateCoder(
        EXTERNAL_CODECS_LOC_VARS
        methodID, coder, false));
    if (!coder)
      return E_NOTIMPL;
    coder.QueryInterface(IID_ISequentialInStream, &_codecInStream);
    if (!_codecInStream)
      return E_NOTIMPL;
  }

  if (thereIsFilterFlag)
  {
    UInt32 processedSize;
    BYTE flag;
    RINOK(inStream->Read(&flag, 1, &processedSize));
    if (processedSize != 1)
      return E_FAIL;
    if (flag > 1)
      return E_NOTIMPL;
    useFilter = (flag != 0);
  }
  
  if (useFilter)
  {
    if (!_filterInStream)
    {
      CMyComPtr<ICompressCoder> coder;
      RINOK(CreateCoder(
          EXTERNAL_CODECS_LOC_VARS
          k_BCJ_X86, coder, false));
      if (!coder)
        return E_NOTIMPL;
      coder.QueryInterface(IID_ISequentialInStream, &_filterInStream);
      if (!_filterInStream)
        return E_NOTIMPL;
    }
    CMyComPtr<ICompressSetInStream> setInStream;
    _filterInStream.QueryInterface(IID_ICompressSetInStream, &setInStream);
    if (!setInStream)
      return E_NOTIMPL;
    RINOK(setInStream->SetInStream(_codecInStream));
    _decoderInStream = _filterInStream;
  }
  else
    _decoderInStream = _codecInStream;

  if (method == NMethodType::kLZMA)
  {
    CMyComPtr<ICompressSetDecoderProperties2> setDecoderProperties;
    _codecInStream.QueryInterface(IID_ICompressSetDecoderProperties2, &setDecoderProperties);
    if (setDecoderProperties)
    {
      static const UInt32 kPropertiesSize = 5;
      BYTE properties[kPropertiesSize];
      UInt32 processedSize;
      RINOK(inStream->Read(properties, kPropertiesSize, &processedSize));
      if (processedSize != kPropertiesSize)
        return E_FAIL;
      RINOK(setDecoderProperties->SetDecoderProperties2((const Byte *)properties, kPropertiesSize));
    }
  }

  {
    CMyComPtr<ICompressSetInStream> setInStream;
    _codecInStream.QueryInterface(IID_ICompressSetInStream, &setInStream);
    if (!setInStream)
      return E_NOTIMPL;
    RINOK(setInStream->SetInStream(inStream));
  }

  {
    CMyComPtr<ICompressSetOutStreamSize> setOutStreamSize;
    _codecInStream.QueryInterface(IID_ICompressSetOutStreamSize, &setOutStreamSize);
    if (!setOutStreamSize)
      return E_NOTIMPL;
    RINOK(setOutStreamSize->SetOutStreamSize(NULL));
  }

  if (useFilter)
  {
    /*
    CMyComPtr<ICompressSetOutStreamSize> setOutStreamSize;
    _filterInStream.QueryInterface(IID_ICompressSetOutStreamSize, &setOutStreamSize);
    if (!setOutStreamSize)
      return E_NOTIMPL;
    RINOK(setOutStreamSize->SetOutStreamSize(NULL));
    */
  }

  return S_OK;
}

HRESULT CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  return ReadStream(_decoderInStream, data, size, processedSize);;
}

}}
