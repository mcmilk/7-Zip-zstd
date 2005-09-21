// Rar29Decoder.h
// According to unRAR license,
// this code may not be used to develop a 
// RAR (WinRAR) compatible archiver

#ifndef __RAR_DECODER_H
#define __RAR_DECODER_H

#include "Common/MyCom.h"

#include "../../ICoder.h"

#include "Original/rar.hpp"

namespace NCompress {

namespace NRar29 {

class CDecoder :
  public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public CMyUnknownImp
{
  Unpack *Unp;
  bool m_IsSolid;
public:
  ComprDataIO DataIO;
  CDecoder();
  ~CDecoder();
  /*
  class CCoderReleaser
  {
    CCoder *m_Coder;
  public:
    CCoderReleaser(CCoder *aCoder): m_Coder(aCoder) {}
    ~CCoderReleaser()
    {
      m_Coder->DataIO.ReleaseStreams();
    }
  };
  */
  MY_UNKNOWN_IMP1(ICompressSetDecoderProperties2)

  // void ReleaseStreams();
  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);
};

}

namespace NRar15{

class CDecoder :
  public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public CMyUnknownImp
{
  Unpack *Unp;
  bool m_IsSolid;
public:
  ComprDataIO DataIO;
  CDecoder();
  ~CDecoder();
  /*
  class CCoderReleaser
  {
    CDecoder *m_Coder;
  public:
    CCoderReleaser(CDecoder *coder): m_Coder(coder) {}
    ~CCoderReleaser()
    {
      m_Coder->DataIO.ReleaseStreams();
    }
  };
  */
  MY_UNKNOWN_IMP1(ICompressSetDecoderProperties2)

  // void ReleaseStreams();
  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);
};

}

}

#endif
