// Arj/Decoder1.h

#pragma once

#ifndef __COMPRESS_ARJ_DECODER1_H
#define __COMPRESS_ARJ_DECODER1_H

#include "Interface/ICoder.h"

#include "Stream/WindowOut.h"
#include "Stream/MSBFDecoder.h"
#include "Stream/InByte.h"

// {23170F69-40C1-278B-0404-010000000000}
DEFINE_GUID(CLSID_CCompressArjDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);

namespace NCompress {
namespace NArj {
namespace NDecoder1 {

typedef NStream::NMSBF::CDecoder<NStream::CInByte> CInBit;

#define CODE_BIT          16

#define THRESHOLD    3
#define DDICSIZ      26624
#define MAXDICBIT   16
#define MATCHBIT     8
#define MAXMATCH   256
#define NC          (UCHAR_MAX + MAXMATCH + 2 - THRESHOLD)
#define NP          (MAXDICBIT + 1)
#define CBIT         9
#define NT          (CODE_BIT + 3)
#define PBIT         5
#define TBIT         5

#if NT > NP
#define NPT NT
#else
#define NPT NP
#endif

#define CTABLESIZE  4096
#define PTABLESIZE   256


class CCoder :
  public ICompressCoder,
  public CComObjectRoot,
  public CComCoClass<CCoder, &CLSID_CCompressArjDecoder>
{
  NStream::NWindow::COut m_OutWindowStream;
  CInBit m_InBitStream;

  UINT32 left[2 * NC - 1];
  UINT32 right[2 * NC - 1];
  BYTE c_len[NC];
  BYTE pt_len[NPT];

  UINT32 c_table[CTABLESIZE];
  UINT32 pt_table[PTABLESIZE];
  

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

  void make_table(int nchar, BYTE *bitlen, int tablebits, 
      UINT32 *table, int tablesize);
  
  void read_c_len();
  void read_pt_len(int nn, int nbit, int i_special);
  UINT32 decode_c();
  UINT32 decode_p();


public:
  CCoder();

  BEGIN_COM_MAP(CCoder)
    COM_INTERFACE_ENTRY(ICompressCoder)
  END_COM_MAP()

  DECLARE_NOT_AGGREGATABLE(CCoder)

  // DECLARE_NO_REGISTRY()

  DECLARE_REGISTRY(CEncoder, TEXT("Compress.Arj2Decoder.1"), 
  TEXT("Compress.Arj2Decoder"), 0, THREADFLAGS_APARTMENT)

  STDMETHOD(CodeReal)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

};

}}}

#endif
