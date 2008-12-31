// ByteSwap.h

#ifndef __COMPRESS_BYTE_SWAP_H
#define __COMPRESS_BYTE_SWAP_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"

class CByteSwap2:
  public ICompressFilter,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP
  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
};

class CByteSwap4:
  public ICompressFilter,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP
  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
};

#endif
