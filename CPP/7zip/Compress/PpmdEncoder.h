// PpmdEncoder.h
// 2009-03-11 : Igor Pavlov : Public domain

#ifndef __COMPRESS_PPMD_ENCODER_H
#define __COMPRESS_PPMD_ENCODER_H

#include "../../../C/Ppmd7.h"

#include "../../Common/MyCom.h"

#include "../ICoder.h"

#include "../Common/CWrappers.h"

namespace NCompress {
namespace NPpmd {

class CEncoder :
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public CMyUnknownImp
{
  Byte *_inBuf;
  CByteOutBufWrap _outStream;
  CPpmd7z_RangeEnc _rangeEnc;
  CPpmd7 _ppmd;

  UInt32 _usedMemSize;
  Byte _order;

public:
  MY_UNKNOWN_IMP2(
      ICompressSetCoderProperties,
      ICompressWriteCoderProperties)

  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);

  CEncoder();
  ~CEncoder();
};

}}

#endif
