// Compress/PPMD/Encoder.h

#ifndef __COMPRESS_PPMD_ENCODER_H
#define __COMPRESS_PPMD_ENCODER_H

#include "../../../Common/MyCom.h"

#include "../../ICoder.h"
#include "../../Common/InBuffer.h"
#include "../RangeCoder/RangeCoder.h"

#include "PPMDEncode.h"

namespace NCompress {
namespace NPPMD {

class CEncoder : 
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public ICompressWriteCoderProperties,
  public CMyUnknownImp
{
public:
  CInBuffer _inStream;

  NRangeCoder::CEncoder _rangeEncoder;

  CEncodeInfo _info;
  UInt32 _usedMemorySize;
  Byte _order;

public:

  MY_UNKNOWN_IMP2(
      ICompressSetCoderProperties,
      ICompressWriteCoderProperties)

  // ICoder interface
  HRESULT Flush();
  void ReleaseStreams()
  {
    _inStream.ReleaseStream();
    _rangeEncoder.ReleaseStream();
  }

  HRESULT CodeReal(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);
  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UInt64 *inSize, const UInt64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, 
      const PROPVARIANT *properties, UInt32 numProperties);

  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);

  CEncoder();

};

}}

#endif
