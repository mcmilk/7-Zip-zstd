// Arj/Decoder2.h

#pragma once

#ifndef __COMPRESS_ARJ_DECODER2_H
#define __COMPRESS_ARJ_DECODER2_H

#include "Common/MyCom.h"
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
  
  /*
  void CCoder::ReleaseStreams()
  {
    m_OutWindowStream.ReleaseStream();
    m_InBitStream.ReleaseStream();
  }
  */

  class CCoderReleaser
  {
    CCoder *m_Coder;
  public:
    CCoderReleaser(CCoder *aCoder): m_Coder(aCoder) {}
    ~CCoderReleaser()
    {
      m_Coder->m_OutWindowStream.Flush();
      // m_Coder->ReleaseStreams();
    }
  };
  friend class CCoderReleaser;

public:
  CCoder();

  MY_UNKNOWN_IMP

  STDMETHOD(CodeReal)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

};

}}}

#endif