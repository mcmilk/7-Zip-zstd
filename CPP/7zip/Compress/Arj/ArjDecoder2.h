// Arj/Decoder2.h

#ifndef __COMPRESS_ARJ_DECODER2_H
#define __COMPRESS_ARJ_DECODER2_H

#include "../../../Common/MyCom.h"
#include "../../ICoder.h"
#include "../../Common/MSBFDecoder.h"
#include "../../Common/InBuffer.h"
#include "../LZ/LZOutWindow.h"

/*
// {23170F69-40C1-278B-0404-020000000000}
DEFINE_GUID(CLSID_CCompressArj2Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00);
*/

namespace NCompress {
namespace NArj {
namespace NDecoder2 {

class CCoder :
  public ICompressCoder,
  public CMyUnknownImp
{
  CLZOutWindow m_OutWindowStream;
  NStream::NMSBF::CDecoder<CInBuffer> m_InBitStream;
  
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

public:
  MY_UNKNOWN_IMP

  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

};

}}}

#endif
