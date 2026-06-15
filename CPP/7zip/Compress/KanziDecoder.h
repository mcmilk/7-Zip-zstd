// KanziDecoder.h

#ifndef ZIP7_INC_COMPRESS_KANZI_DECODER_H
#define ZIP7_INC_COMPRESS_KANZI_DECODER_H

#include "../../Common/Common.h"
#include "../../Common/MyCom.h"

#include "../ICoder.h"
#include "../Common/StreamUtils.h"

#include "KanziCommon.h"

namespace NCompress {
namespace NKANZI {

class CDecoder Z7_final:
  public ICompressCoder,
  public ICompressSetDecoderProperties2,
  public ICompressSetCoderMt,
  public CMyUnknownImp
{
  CProps _props;
  UInt64 _processedIn;
  UInt64 _processedOut;
  UInt32 _numThreads;

public:
  CDecoder();

  Z7_COM_QI_BEGIN2(ICompressCoder)
  Z7_COM_QI_ENTRY(ICompressSetDecoderProperties2)
  Z7_COM_QI_ENTRY(ICompressSetCoderMt)
  Z7_COM_QI_END
  Z7_COM_ADDREF_RELEASE

  Z7_IFACE_COM7_IMP(ICompressCoder)
  Z7_IFACE_COM7_IMP(ICompressSetDecoderProperties2)
  Z7_IFACE_COM7_IMP(ICompressSetCoderMt)
};

}}

#endif
