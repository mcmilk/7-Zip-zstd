// LZMA/Encoder.h

#ifndef __LZMA_ENCODER_H
#define __LZMA_ENCODER_H

#include "../../../Common/MyCom.h"
#include "../../ICoder.h"

extern "C"
{
  #include "../../../../C/LzmaEnc.h"
}

namespace NCompress {
namespace NLZMA {

struct CSeqInStream
{
  ISeqInStream SeqInStream;
  ISequentialInStream *RealStream;
};

struct CSeqOutStream
{
  ISeqOutStream SeqOutStream;
  CMyComPtr<ISequentialOutStream> RealStream;
  HRESULT Res;
};

class CEncoder : 
  public ICompressCoder,
  public ICompressSetOutStream,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public CMyUnknownImp
{
  CLzmaEncHandle _encoder;
  
  CSeqInStream _seqInStream;
  CSeqOutStream _seqOutStream;

public:
  CEncoder();

  MY_UNKNOWN_IMP3(
      ICompressSetOutStream,
      ICompressSetCoderProperties,
      ICompressWriteCoderProperties
      )
    
 
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, 
      const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  // ICompressSetCoderProperties2
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UInt32 numProperties);
  
  // ICompressWriteCoderProperties
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);

  STDMETHOD(SetOutStream)(ISequentialOutStream *outStream);
  STDMETHOD(ReleaseOutStream)();

  virtual ~CEncoder();
};

}}

#endif
