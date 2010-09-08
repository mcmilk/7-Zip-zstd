// NsisDecode.cpp

#include "StdAfx.h"

#include "NsisDecode.h"

#include "../../Common/StreamUtils.h"

#include "../../Common/MethodId.h"

#include "../../Compress/BZip2Decoder.h"
#include "../../Compress/DeflateDecoder.h"
#include "../../Compress/LzmaDecoder.h"

namespace NArchive {
namespace NNsis {

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
    switch (method)
    {
      // case NMethodType::kCopy: return E_NOTIMPL;
      case NMethodType::kDeflate: _codecInStream = new NCompress::NDeflate::NDecoder::CNsisCOMCoder(); break;
      case NMethodType::kBZip2: _codecInStream = new NCompress::NBZip2::CNsisDecoder(); break;
      case NMethodType::kLZMA: _codecInStream = new NCompress::NLzma::CDecoder(); break;
      default: return E_NOTIMPL;
    }
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

HRESULT CDecoder::Read(void *data, size_t *processedSize)
{
  return ReadStream(_decoderInStream, data, processedSize);;
}

}}
