// PpmdZip.h
// 2010-03-11 : Igor Pavlov : Public domain

#ifndef __COMPRESS_PPMD_ZIP_H
#define __COMPRESS_PPMD_ZIP_H

#include "../../../C/Alloc.h"
#include "../../../C/Ppmd8.h"

#include "../../Common/MyCom.h"

#include "../Common/CWrappers.h"

#include "../ICoder.h"

namespace NCompress {
namespace NPpmdZip {

static const UInt32 kBufSize = (1 << 20);

struct CBuf
{
  Byte *Buf;
  
  CBuf(): Buf(0) {}
  ~CBuf() { ::MidFree(Buf); }
  bool Alloc()
  {
    if (!Buf)
      Buf = (Byte *)::MidAlloc(kBufSize);
    return (Buf != 0);
  }
};

class CDecoder :
  public ICompressCoder,
  public CMyUnknownImp
{
  CByteInBufWrap _inStream;
  CBuf _outStream;
  CPpmd8 _ppmd;
  bool _fullFileMode;
public:
  MY_UNKNOWN_IMP
  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  CDecoder(bool fullFileMode);
  ~CDecoder();
};

class CEncoder :
  public ICompressCoder,
  public CMyUnknownImp
{
  CByteOutBufWrap _outStream;
  CBuf _inStream;
  CPpmd8 _ppmd;
  UInt32 _usedMemInMB;
  unsigned _order;
  unsigned _restor;
public:
  MY_UNKNOWN_IMP
  STDMETHOD(Code)(ISequentialInStream *inStream, ISequentialOutStream *outStream,
      const UInt64 *inSize, const UInt64 *outSize, ICompressProgressInfo *progress);
  HRESULT SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);
  CEncoder();
  ~CEncoder();
};

}}

#endif
