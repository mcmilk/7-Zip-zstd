// ArjDecoder2.h

#ifndef __COMPRESS_ARJ_DECODER2_H
#define __COMPRESS_ARJ_DECODER2_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/InBuffer.h"

#include "BitmDecoder.h"
#include "LzOutWindow.h"

namespace NCompress {
namespace NArj {
namespace NDecoder2 {

class CCoder :
  public ICompressCoder,
  public CMyUnknownImp
{
  CLzOutWindow m_OutWindowStream;
  NBitm::CDecoder<CInBuffer> m_InBitStream;
  
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

  HRESULT CodeReal(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
public:
  MY_UNKNOWN_IMP

  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);

};

}}}

#endif
