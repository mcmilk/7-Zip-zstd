// x86_2.h

#pragma once

#ifndef __X86_2_H
#define __X86_2_H

#include "Coder.h"
#include "Compression/AriBitCoder.h"

// {23170F69-40C1-278B-0303-010100000100}
#define MyClass2_a(Name, anId, aSubId, anEncodingId)  \
DEFINE_GUID(CLSID_CCompressConvert ## Name,  \
0x23170F69, 0x40C1, 0x278B, 0x03, 0x03, anId, aSubId, 0x00, 0x00, anEncodingId, 0x00);

#define MyClass_a(Name, anId, aSubId)  \
MyClass2_a(Name ## _Encoder, anId, aSubId, 0x01) \
MyClass2_a(Name ## _Decoder, anId, aSubId, 0x00) 

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
  CBCJ2_x86_Encoder(): m_MainStream(1 << 16) {}

  NStream::COutByte m_MainStream;
  NStream::COutByte m_E8Stream;
  NStream::COutByte m_JumpStream;
  NCompression::NArithmetic::CRangeEncoder m_RangeEncoder;
  NCompression::NArithmetic::CBitEncoder<kNumMoveBits> m_StatusE8Encoder[256];
  NCompression::NArithmetic::CBitEncoder<kNumMoveBits> m_StatusE9Encoder;
  NCompression::NArithmetic::CBitEncoder<kNumMoveBits> m_StatusJccEncoder;

  void ReleaseStreams()
  {
    m_MainStream.ReleaseStream();
    m_E8Stream.ReleaseStream();
    m_JumpStream.ReleaseStream();
    m_RangeEncoder.ReleaseStream();
  }
  HRESULT Flush();
  class CCoderReleaser
  {
    CBCJ2_x86_Encoder *m_Coder;
  public:
    CCoderReleaser(CBCJ2_x86_Encoder *aCoder): m_Coder(aCoder) {}
    ~CCoderReleaser()
    {
      m_Coder->ReleaseStreams();
    }
  };
  friend class CCoderReleaser;

public: 
BEGIN_COM_MAP(CBCJ2_x86_Encoder)
  COM_INTERFACE_ENTRY(ICompressCoder2)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CBCJ2_x86_Encoder)
  DECLARE_REGISTRY(CBCJ2_x86_Encoder, TEXT("Compress.ConvertBranch.1"), 
  TEXT("Compress.ConvertBranch"), 0, THREADFLAGS_APARTMENT)
  HRESULT CodeReal(ISequentialInStream **anInStreams,
      const UINT64 **anInSizes,
      UINT32 aNumInStreams,
      ISequentialOutStream **anOutStreams,
      const UINT64 **anOutSizes,
      UINT32 aNumOutStreams,
      ICompressProgressInfo *aProgress);
  STDMETHOD(Code)(ISequentialInStream **anInStreams,
      const UINT64 **anInSizes,
      UINT32 aNumInStreams,
      ISequentialOutStream **anOutStreams,
      const UINT64 **anOutSizes,
      UINT32 aNumOutStreams,
      ICompressProgressInfo *aProgress);
}; 

#endif

class CBCJ2_x86_Decoder:
  public ICompressCoder2,
  public CComObjectRoot,
  public CComCoClass<CBCJ2_x86_Decoder, & CLSID_CCompressConvertBCJ2_x86_Decoder> 
{ 
public:
  CBCJ2_x86_Decoder(): m_OutStream(1 << 16), m_MainInStream(1 << 16) {}

  NStream::CInByte m_MainInStream;
  NStream::CInByte m_E8Stream;
  NStream::CInByte m_JumpStream;
  NCompression::NArithmetic::CRangeDecoder m_RangeDecoder;
  NCompression::NArithmetic::CBitDecoder<kNumMoveBits> m_StatusE8Decoder[256];
  NCompression::NArithmetic::CBitDecoder<kNumMoveBits> m_StatusE9Decoder;
  NCompression::NArithmetic::CBitDecoder<kNumMoveBits> m_StatusJccDecoder;

  NStream::COutByte m_OutStream;

  void ReleaseStreams()
  {
    m_MainInStream.ReleaseStream();
    m_E8Stream.ReleaseStream();
    m_JumpStream.ReleaseStream();
    m_RangeDecoder.ReleaseStream();
    m_OutStream.ReleaseStream();
  }
  HRESULT Flush() { return m_OutStream.Flush(); }
  class CCoderReleaser
  {
    CBCJ2_x86_Decoder *m_Coder;
  public:
    CCoderReleaser(CBCJ2_x86_Decoder *aCoder): m_Coder(aCoder) {}
    ~CCoderReleaser()  { m_Coder->ReleaseStreams(); }
  };
  friend class CCoderReleaser;

public: 
BEGIN_COM_MAP(CBCJ2_x86_Decoder)
  COM_INTERFACE_ENTRY(ICompressCoder2)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CBCJ2_x86_Decoder)
  DECLARE_REGISTRY(CBCJ2_x86_Decoder, TEXT("Compress.ConvertBranch.1"), 
  TEXT("Compress.ConvertBranch"), 0, THREADFLAGS_APARTMENT)
  HRESULT CodeReal(ISequentialInStream **anInStreams,
      const UINT64 **anInSizes,
      UINT32 aNumInStreams,
      ISequentialOutStream **anOutStreams,
      const UINT64 **anOutSizes,
      UINT32 aNumOutStreams,
      ICompressProgressInfo *aProgress);
  STDMETHOD(Code)(ISequentialInStream **anInStreams,
      const UINT64 **anInSizes,
      UINT32 aNumInStreams,
      ISequentialOutStream **anOutStreams,
      const UINT64 **anOutSizes,
      UINT32 aNumOutStreams,
      ICompressProgressInfo *aProgress);
}; 

#endif
