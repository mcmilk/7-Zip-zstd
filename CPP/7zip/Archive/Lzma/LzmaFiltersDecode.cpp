// LzmaFiltersDecode.cpp

#include "StdAfx.h"

#include "LzmaFiltersDecode.h"

namespace NArchive {
namespace NLzma {

static const UInt64 k_LZMA = 0x030101;
static const UInt64 k_BCJ = 0x03030103;
  
HRESULT CDecoder::Code(
    DECL_EXTERNAL_CODECS_LOC_VARS
    const CHeader &block,
    ISequentialInStream *inStream, ISequentialOutStream *outStream,
    UInt64 *inProcessedSize, ICompressProgressInfo *progress)
{
  *inProcessedSize = (UInt64)(Int64)-1;

  if (block.FilterMethod > 1)
    return E_NOTIMPL;

  if (!_lzmaDecoder)
  {
    RINOK(CreateCoder(EXTERNAL_CODECS_LOC_VARS k_LZMA, _lzmaDecoder, false));
    if (_lzmaDecoder == 0)
      return E_NOTIMPL;
  }

  {
    CMyComPtr<ICompressSetDecoderProperties2> setDecoderProperties;
    _lzmaDecoder.QueryInterface(IID_ICompressSetDecoderProperties2, &setDecoderProperties);
    if (!setDecoderProperties)
      return E_NOTIMPL;
    RINOK(setDecoderProperties->SetDecoderProperties2(block.LzmaProps, 5));
  }

  bool filteredMode = (block.FilterMethod == 1);

  CMyComPtr<ICompressSetOutStream> setOutStream;

  if (filteredMode)
  {
    if (!_bcjStream)
    {
      CMyComPtr<ICompressCoder> coder;
      RINOK(CreateCoder(EXTERNAL_CODECS_LOC_VARS k_BCJ, coder, false));
      if (!coder)
        return E_NOTIMPL;
      coder.QueryInterface(IID_ISequentialOutStream, &_bcjStream);
      if (!_bcjStream)
        return E_NOTIMPL;
    }

    _bcjStream.QueryInterface(IID_ICompressSetOutStream, &setOutStream);
    if (!setOutStream)
      return E_NOTIMPL;
    RINOK(setOutStream->SetOutStream(outStream));
    outStream = _bcjStream;
  }

  const UInt64 *unpackSize = block.HasUnpackSize() ? &block.UnpackSize : NULL;
  RINOK(_lzmaDecoder->Code(inStream, outStream, NULL, unpackSize, progress));

  if (filteredMode)
  {
    CMyComPtr<IOutStreamFlush> flush;
    _bcjStream.QueryInterface(IID_IOutStreamFlush, &flush);
    if (flush)
    {
      RINOK(flush->Flush());
    }
    RINOK(setOutStream->ReleaseOutStream());
  }

  CMyComPtr<ICompressGetInStreamProcessedSize> getInStreamProcessedSize;
  _lzmaDecoder.QueryInterface(IID_ICompressGetInStreamProcessedSize, &getInStreamProcessedSize);
  if (getInStreamProcessedSize)
  {
    RINOK(getInStreamProcessedSize->GetInStreamProcessedSize(inProcessedSize));
  }
  return S_OK;
}

}}
