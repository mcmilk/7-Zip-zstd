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

class CCoder :
  public ICompressCoder,
  public IGetInStreamProcessedSize,
  public CComObjectRoot,
  public CComCoClass<CCoder, &CLSID_CCompressDeflateDecoder>
{
  NStream::NWindow::COut m_OutWindowStream;
  CInBit m_InBitStream;
  CHuffmanDecoder m_MainDecoder;
  CHuffmanDecoder m_DistDecoder;
  CHuffmanDecoder m_LevelDecoder; // table for decoding other tables;

  bool m_FinalBlock;
  bool m_StoredMode;
  UINT32 m_StoredBlockSize;

  void DeCodeLevelTable(BYTE *aNewLevels, int aNumLevels);
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
    CCoderReleaser(CCoder *aCoder): m_Coder(aCoder) {}
    ~CCoderReleaser()
    {
      m_Coder->m_OutWindowStream.Flush();
      m_Coder->ReleaseStreams();
    }
  };
  friend class CCoderReleaser;

public:
  CCoder();

  BEGIN_COM_MAP(CCoder)
    COM_INTERFACE_ENTRY(ICompressCoder)
    COM_INTERFACE_ENTRY(IGetInStreamProcessedSize)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(CCoder)

  // DECLARE_NO_REGISTRY()

  DECLARE_REGISTRY(CEncoder, "Compress.DeflateDecoder.1", "Compress.DeflateDecoder", 0, THREADFLAGS_APARTMENT)

  STDMETHOD(CodeReal)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  // IGetInStreamProcessedSize
  STDMETHOD(GetInStreamProcessedSize)(UINT64 *aValue);
};

}}

#endif