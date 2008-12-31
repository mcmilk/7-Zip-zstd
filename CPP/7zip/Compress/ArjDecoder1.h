// ArjDecoder1.h

#ifndef __COMPRESS_ARJ_DECODER1_H
#define __COMPRESS_ARJ_DECODER1_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/InBuffer.h"

#include "BitmDecoder.h"
#include "LzOutWindow.h"

namespace NCompress {
namespace NArj {
namespace NDecoder1 {

#define CODE_BIT          16

#define THRESHOLD    3
#define DDICSIZ      26624
#define MAXDICBIT   16
#define MATCHBIT     8
#define MAXMATCH   256
#define NC          (0xFF + MAXMATCH + 2 - THRESHOLD)
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
  public CMyUnknownImp
{
  CLzOutWindow m_OutWindowStream;
  NBitm::CDecoder<CInBuffer> m_InBitStream;

  UInt32 left[2 * NC - 1];
  UInt32 right[2 * NC - 1];
  Byte c_len[NC];
  Byte pt_len[NPT];

  UInt32 c_table[CTABLESIZE];
  UInt32 pt_table[PTABLESIZE];
  
  void ReleaseStreams()
  {
    m_OutWindowStream.ReleaseStream();
    m_InBitStream.ReleaseStream();
  }

  class CCoderReleaser
  {
    CCoder *m_Coder;
  public:
    bool NeedFlush;
    CCoderReleaser(CCoder *coder): m_Coder(coder), NeedFlush(true) {}
    ~CCoderReleaser()
    {
      if (NeedFlush)
        m_Coder->m_OutWindowStream.Flush();
      m_Coder->ReleaseStreams();
    }
  };
  friend class CCoderReleaser;

  void MakeTable(int nchar, Byte *bitlen, int tablebits, UInt32 *table, int tablesize);
  
  void read_c_len();
  void read_pt_len(int nn, int nbit, int i_special);
  UInt32 decode_c();
  UInt32 decode_p();

  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
public:
  MY_UNKNOWN_IMP

  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

};

}}}

#endif
