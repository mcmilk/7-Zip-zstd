// Crypto/MyAes.h

#ifndef __CRYPTO_MY_AES_H
#define __CRYPTO_MY_AES_H

#include "../../../C/Aes.h"

#include "../../Common/MyCom.h"
#include "../../Common/Types.h"

#include "../ICoder.h"

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
