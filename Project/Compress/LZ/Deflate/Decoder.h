// Deflate/Decoder.h

#pragma once

#ifndef __ARCHIVE_ZIP_DEFLATEDECODER_H
#define __ARCHIVE_ZIP_DEFLATEDECODER_H

#include "Interface/ICoder.h"
#include "../../Interface/CompressInterface.h"

#include "Stream/WindowOut.h"
#include "Stream/LSBFDecoder.h"
#include "Stream/InByte.h"

#include "Compression/HuffmanDecoder.h"

#include "ExtConst.h"

// {23170F69-40C1-278B-0401-080000000000}
DEFINE_GUID(CLSID_CCompressDeflateDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-0401-090000000000}
DEFINE_GUID(CLSID_CCompressDeflate64Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00);

namespace NDeflate{
namespace NDecoder{

class CException
{
public:
  enum ECauseType
  {
    kData
  } m_Cause;
  CException(ECauseType aCause): m_Cause(aCause) {}
};

typedef NStream::NLSBF::CDecoder<NStream::CInByte> CInBit;
typedef NCompression::NHuffman::CDecoder<kNumHuffmanBits> CHuffmanDecoder;

class CCoder
{
  NStream::NWindow::COut m_OutWindowStream;
  CInBit m_InBitStream;
  CHuffmanDecoder m_MainDecoder;
  CHuffmanDecoder m_DistDecoder;
  CHuffmanDecoder m_LevelDecoder; // table for decoding other tables;

  bool m_FinalBlock;
  bool m_StoredMode;
  UINT32 m_StoredBlockSize;

  bool _deflate64Mode;

  void DeCodeLevelTable(BYTE *newLevels, int numLevels);
  void ReadTables();
  
  void CCoder::ReleaseStreams()
  {
    m_OutWindowStream.ReleaseStream();
    m_InBitStream.ReleaseStream();
  }
  class CCoderReleaser
  {
    CCoder *m_Coder;
  public:
    CCoderReleaser(CCoder *coder): m_Coder(coder) {}
    ~CCoderReleaser()
    {
      m_Coder->m_OutWindowStream.Flush();
      m_Coder->ReleaseStreams();
    }
  };
  friend class CCoderReleaser;

public:
  CCoder(bool deflate64Mode = false);

  HRESULT CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  HRESULT BaseCode(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  // IGetInStreamProcessedSize
  HRESULT BaseGetInStreamProcessedSize(UINT64 *aValue);
};

class CCOMCoder :
  public ICompressCoder,
  public IGetInStreamProcessedSize,
  public CComObjectRoot,
  public CComCoClass<CCOMCoder, &CLSID_CCompressDeflateDecoder>,
  public CCoder
{
public:
  BEGIN_COM_MAP(CCOMCoder)
    COM_INTERFACE_ENTRY(ICompressCoder)
    COM_INTERFACE_ENTRY(IGetInStreamProcessedSize)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(CCOMCoder)

  // DECLARE_NO_REGISTRY()

  DECLARE_REGISTRY(CCOMCoder, 
    // TEXT("Compress.DeflateDecoder.1"), TEXT("Compress.DeflateDecoder"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  // IGetInStreamProcessedSize
  STDMETHOD(GetInStreamProcessedSize)(UINT64 *aValue);

  CCOMCoder(): CCoder(false) {}
};

class CCOMCoder64 :
  public ICompressCoder,
  public IGetInStreamProcessedSize,
  public CComObjectRoot,
  public CComCoClass<CCOMCoder64, &CLSID_CCompressDeflate64Decoder>,
  public CCoder
{
public:
  BEGIN_COM_MAP(CCOMCoder64)
    COM_INTERFACE_ENTRY(ICompressCoder)
    COM_INTERFACE_ENTRY(IGetInStreamProcessedSize)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(CCOMCoder64)

  // DECLARE_NO_REGISTRY()

  DECLARE_REGISTRY(CCOMCoder64, 
    // TEXT("Compress.DeflateDecoder.1"), TEXT("Compress.DeflateDecoder"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  // IGetInStreamProcessedSize
  STDMETHOD(GetInStreamProcessedSize)(UINT64 *aValue);

  CCOMCoder64(): CCoder(true) {}
};

}}

#endif