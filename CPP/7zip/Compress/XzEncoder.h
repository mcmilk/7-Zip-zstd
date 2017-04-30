// XzEncoder.h

#ifndef __XZ_ENCODER_H
#define __XZ_ENCODER_H

#include "../../../C/XzEnc.h"

#include "../../Common/MyCom.h"

#include "../ICoder.h"

namespace NCompress {
namespace NXz {


class CEncoder:
  public ICompressCoder,
  public ICompressSetCoderProperties,
  public CMyUnknownImp
{
  // CXzEncHandle _encoder;
public:
  CLzma2EncProps _lzma2Props;

  CXzProps xzProps;
  CXzFilterProps filter;

  MY_UNKNOWN_IMP2(ICompressCoder, ICompressSetCoderProperties)

  void InitCoderProps();
  
  HRESULT SetCoderProp(PROPID propID, const PROPVARIANT &prop);
  
  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);

  CEncoder();
  virtual ~CEncoder();
};

}}

#endif
