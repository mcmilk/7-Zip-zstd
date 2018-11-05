// Lzma2Encoder.h

#ifndef __LZMA2_ENCODER_H
#define __LZMA2_ENCODER_H

#include "../../../C/Lzma2Enc.h"
#include "../../../C/fast-lzma2/fast-lzma2.h"

#include "../../Common/MyCom.h"
#include "../../Common/MyBuffer.h"

#include "../ICoder.h"

namespace NCompress {
namespace NLzma2 {

class CEncoder:
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public ICompressSetCoderPropertiesOpt,
  public CMyUnknownImp
{
  CLzma2EncHandle _encoder;
public:
  MY_UNKNOWN_IMP4(
      ICompressCoder,
      ICompressSetCoderProperties,
      ICompressWriteCoderProperties,
      ICompressSetCoderPropertiesOpt)
 
  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);
  STDMETHOD(SetCoderPropertiesOpt)(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);

  CEncoder();
  virtual ~CEncoder();
};

class CFastEncoder :
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public CMyUnknownImp
{
  FL2_CCtx* _encoder;
  CByteBuffer inBuffer;
  UInt64 reduceSize;
  UInt32 dictSize;

public:
  MY_UNKNOWN_IMP3(
    ICompressCoder,
    ICompressSetCoderProperties,
    ICompressWriteCoderProperties)

    STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);

  CFastEncoder();
  virtual ~CFastEncoder();
};

}}

#endif
