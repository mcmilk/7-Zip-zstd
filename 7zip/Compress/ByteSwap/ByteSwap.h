// ByteSwap.h

#ifndef __BYTESWAP_H
#define __BYTESWAP_H

#include "../../ICoder.h"
#include "Common/MyCom.h"

// {23170F69-40C1-278B-0203-020000000000}
DEFINE_GUID(CLSID_CCompressConvertByteSwap2, 
0x23170F69, 0x40C1, 0x278B, 0x02, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-0203-040000000000}
DEFINE_GUID(CLSID_CCompressConvertByteSwap4, 
0x23170F69, 0x40C1, 0x278B, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00);

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
