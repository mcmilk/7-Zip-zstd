// OutStreamWithSha1.h

#ifndef __OUT_STREAM_WITH_SHA1_H
#define __OUT_STREAM_WITH_SHA1_H

#include "../../Crypto/Sha1.h"

#include "../../../Common/MyCom.h"

#include "../../IStream.h"

class COutStreamWithSha1:
  public ISequentialOutStream,
  public CMyUnknownImp
{
  CMyComPtr<ISequentialOutStream> _stream;
  UInt64 _size;
  NCrypto::NSha1::CContext _sha;
  bool _calculate;
public:
  MY_UNKNOWN_IMP
  STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);
  void SetStream(ISequentialOutStream *stream) { _stream = stream; }
  void ReleaseStream() { _stream.Release(); }
  void Init(bool calculate = true)
  {
    _size = 0;
    _calculate = calculate;
    _sha.Init();
  }
  void InitSha1() { _sha.Init(); }
  UInt64 GetSize() const { return _size; }
  void Final(Byte *digest) { _sha.Final(digest); }
};

#endif
