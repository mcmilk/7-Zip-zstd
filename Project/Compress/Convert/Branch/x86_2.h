// x86_2.h

#pragma once

#ifndef __X86_2_H
#define __X86_2_H

#include "Coder.h"
#include "Compression/AriBitCoder.h"

// {23170F69-40C1-278B-0303-010100000100}
#define MyClass2_a(Name, id, subId, encodingId)  \
DEFINE_GUID(CLSID_CCompressConvert ## Name,  \
0x23170F69, 0x40C1, 0x278B, 0x03, 0x03, id, subId, 0x00, 0x00, encodingId, 0x00);

#define MyClass_a(Name, id, subId)  \
MyClass2_a(Name ## _Encoder, id, subId, 0x01) \
MyClass2_a(Name ## _Decoder, id, subId, 0x00) 

MyClass_a(BCJ2_x86, 0x01, 0x1B)

const kNumMoveBits = 5;

#ifndef EXTRACT_ONLY

class CBCJ2_x86_Encoder:
  public ICompressCoder2,
  public CDataBuffer,
  public CComObjectRoot,
  public CComCoClass<CBCJ2_x86_Encoder, & CLSID_CCompressConvertBCJ2_x86_Encoder> 
{
public:
  CBCJ2_x86_Encoder(): _mainStream(1 << 16) {}

  NStream::COutByte _mainStream;
  NStream::COutByte _callStream;
  NStream::COutByte _jumpStream;
  NCompression::NArithmetic::CRangeEncoder _rangeEncoder;
  NCompression::NArithmetic::CBitEncoder<kNumMoveBits> _statusE8Encoder[256];
  NCompression::NArithmetic::CBitEncoder<kNumMoveBits> _statusE9Encoder;
  NCompression::NArithmetic::CBitEncoder<kNumMoveBits> _statusJccEncoder;

  void ReleaseStreams()
  {
    _mainStream.ReleaseStream();
    _callStream.ReleaseStream();
    _jumpStream.ReleaseStream();
    _rangeEncoder.ReleaseStream();
  }
  HRESULT Flush();
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

public: 
BEGIN_COM_MAP(CBCJ2_x86_Encoder)
  COM_INTERFACE_ENTRY(ICompressCoder2)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CBCJ2_x86_Encoder)
  DECLARE_REGISTRY(CBCJ2_x86_Encoder, TEXT("Compress.ConvertBranch.1"), 
  TEXT("Compress.ConvertBranch"), UINT(0), THREADFLAGS_APARTMENT)
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
  public CComObjectRoot,
  public CComCoClass<CBCJ2_x86_Decoder, & CLSID_CCompressConvertBCJ2_x86_Decoder> 
{ 
public:
  CBCJ2_x86_Decoder(): _outStream(1 << 16), _mainInStream(1 << 16) {}

  NStream::CInByte _mainInStream;
  NStream::CInByte _callStream;
  NStream::CInByte _jumpStream;
  NCompression::NArithmetic::CRangeDecoder _rangeDecoder;
  NCompression::NArithmetic::CBitDecoder<kNumMoveBits> _statusE8Decoder[256];
  NCompression::NArithmetic::CBitDecoder<kNumMoveBits> _statusE9Decoder;
  NCompression::NArithmetic::CBitDecoder<kNumMoveBits> _statusJccDecoder;

  NStream::COutByte _outStream;

  void ReleaseStreams()
  {
    _mainInStream.ReleaseStream();
    _callStream.ReleaseStream();
    _jumpStream.ReleaseStream();
    _rangeDecoder.ReleaseStream();
    _outStream.ReleaseStream();
  }
  HRESULT Flush() { return _outStream.Flush(); }
  class CCoderReleaser
  {
    CBCJ2_x86_Decoder *_coder;
  public:
    CCoderReleaser(CBCJ2_x86_Decoder *aCoder): _coder(aCoder) {}
    ~CCoderReleaser()  { _coder->ReleaseStreams(); }
  };
  friend class CCoderReleaser;

public: 
BEGIN_COM_MAP(CBCJ2_x86_Decoder)
  COM_INTERFACE_ENTRY(ICompressCoder2)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CBCJ2_x86_Decoder)
  DECLARE_REGISTRY(CBCJ2_x86_Decoder, TEXT("Compress.ConvertBranch.1"), 
  TEXT("Compress.ConvertBranch"), UINT(0), THREADFLAGS_APARTMENT)
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
