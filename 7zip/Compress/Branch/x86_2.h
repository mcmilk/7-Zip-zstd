// x86_2.h

#pragma once

#ifndef __X86_2_H
#define __X86_2_H

#include "Common/MyCom.h"
#include "Coder.h"
#include "../RangeCoder/RangeCoderBit.h"
// #include "../../Common/InBuffer.h"

// {23170F69-40C1-278B-0303-010100000100}
#define MyClass2_a(Name, id, subId, encodingId)  \
DEFINE_GUID(CLSID_CCompressConvert ## Name,  \
0x23170F69, 0x40C1, 0x278B, 0x03, 0x03, id, subId, 0x00, 0x00, encodingId, 0x00);

#define MyClass_a(Name, id, subId)  \
MyClass2_a(Name ## _Encoder, id, subId, 0x01) \
MyClass2_a(Name ## _Decoder, id, subId, 0x00) 

MyClass_a(BCJ2_x86, 0x01, 0x1B)

const int kNumMoveBits = 5;

#ifndef EXTRACT_ONLY

class CBCJ2_x86_Encoder:
  public ICompressCoder2,
  public CDataBuffer,
  public CMyUnknownImp
{
public:
  CBCJ2_x86_Encoder(): _mainStream(1 << 16) {}

  COutBuffer _mainStream;
  COutBuffer _callStream;
  COutBuffer _jumpStream;
  NCompress::NRangeCoder::CEncoder _rangeEncoder;
  NCompress::NRangeCoder::CBitEncoder<kNumMoveBits> _statusE8Encoder[256];
  NCompress::NRangeCoder::CBitEncoder<kNumMoveBits> _statusE9Encoder;
  NCompress::NRangeCoder::CBitEncoder<kNumMoveBits> _statusJccEncoder;

  HRESULT Flush();
  /*
  void ReleaseStreams()
  {
    _mainStream.ReleaseStream();
    _callStream.ReleaseStream();
    _jumpStream.ReleaseStream();
    _rangeEncoder.ReleaseStream();
  }

  class CCoderReleaser
  {
    CBCJ2_x86_Encoder *_coder;
  public:
    CCoderReleaser(CBCJ2_x86_Encoder *aCoder): _coder(aCoder) {}
    ~CCoderReleaser()
    {
      _coder->ReleaseStreams();
    }
  };
  friend class CCoderReleaser;
  */

public: 

  MY_UNKNOWN_IMP

  HRESULT CodeReal(ISequentialInStream **inStreams,
      const UINT64 **inSizes,
      UINT32 numInStreams,
      ISequentialOutStream **outStreams,
      const UINT64 **outSizes,
      UINT32 numOutStreams,
      ICompressProgressInfo *progress);
  STDMETHOD(Code)(ISequentialInStream **inStreams,
      const UINT64 **inSizes,
      UINT32 numInStreams,
      ISequentialOutStream **outStreams,
      const UINT64 **outSizes,
      UINT32 numOutStreams,
      ICompressProgressInfo *progress);
}; 

#endif

class CBCJ2_x86_Decoder:
  public ICompressCoder2,
  public CMyUnknownImp
{ 
public:
  CBCJ2_x86_Decoder(): _outStream(1 << 16), _mainInStream(1 << 16) {}

  CInBuffer _mainInStream;
  CInBuffer _callStream;
  CInBuffer _jumpStream;
  NCompress::NRangeCoder::CDecoder _rangeDecoder;
  NCompress::NRangeCoder::CBitDecoder<kNumMoveBits> _statusE8Decoder[256];
  NCompress::NRangeCoder::CBitDecoder<kNumMoveBits> _statusE9Decoder;
  NCompress::NRangeCoder::CBitDecoder<kNumMoveBits> _statusJccDecoder;

  COutBuffer _outStream;

  /*
  void ReleaseStreams()
  {
    _mainInStream.ReleaseStream();
    _callStream.ReleaseStream();
    _jumpStream.ReleaseStream();
    _rangeDecoder.ReleaseStream();
    _outStream.ReleaseStream();
  }
  */

  HRESULT Flush() { return _outStream.Flush(); }
  /*
  class CCoderReleaser
  {
    CBCJ2_x86_Decoder *_coder;
  public:
    CCoderReleaser(CBCJ2_x86_Decoder *aCoder): _coder(aCoder) {}
    ~CCoderReleaser()  { _coder->ReleaseStreams(); }
  };
  friend class CCoderReleaser;
  */

public: 
  MY_UNKNOWN_IMP
  HRESULT CodeReal(ISequentialInStream **inStreams,
      const UINT64 **inSizes,
      UINT32 numInStreams,
      ISequentialOutStream **outStreams,
      const UINT64 **outSizes,
      UINT32 numOutStreams,
      ICompressProgressInfo *progress);
  STDMETHOD(Code)(ISequentialInStream **inStreams,
      const UINT64 **inSizes,
      UINT32 numInStreams,
      ISequentialOutStream **outStreams,
      const UINT64 **outSizes,
      UINT32 numOutStreams,
      ICompressProgressInfo *progress);
}; 

#endif
