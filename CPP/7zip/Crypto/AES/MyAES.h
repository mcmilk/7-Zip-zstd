// Crypto/AES/MyAES.h

#ifndef __CIPHER_MYAES_H
#define __CIPHER_MYAES_H

#include "Common/Types.h"
#include "Common/MyCom.h"

#include "../../ICoder.h"

extern "C" 
{ 
#include "../../../../C/Crypto/Aes.h"
}

namespace NCrypto {

class CAesCbcEncoder:
  public ICompressFilter,
  public ICryptoProperties,
  public CMyUnknownImp
{ 
  CAesCbc Aes;
public:
  MY_UNKNOWN_IMP1(ICryptoProperties)
  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
  STDMETHOD(SetKey)(const Byte *data, UInt32 size);
  STDMETHOD(SetInitVector)(const Byte *data, UInt32 size);
};

class CAesCbcDecoder:
  public ICompressFilter,
  public ICryptoProperties,
  public CMyUnknownImp
{ 
  CAesCbc Aes;
public:
  MY_UNKNOWN_IMP1(ICryptoProperties)
  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
  STDMETHOD(SetKey)(const Byte *data, UInt32 size);
  STDMETHOD(SetInitVector)(const Byte *data, UInt32 size);
};

}

#endif
