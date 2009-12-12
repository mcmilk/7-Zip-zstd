// Crypto/MyAes.h

#ifndef __CRYPTO_MY_AES_H
#define __CRYPTO_MY_AES_H

#include "../../../C/Aes.h"

#include "../../Common/MyCom.h"

#include "../ICoder.h"

namespace NCrypto {

class CAesCbcCoder:
  public ICompressFilter,
  public ICryptoProperties,
  public CMyUnknownImp
{
protected:
  AES_CODE_FUNC _codeFunc;
  AES_SET_KEY_FUNC _setKeyFunc;
  unsigned _offset;
  UInt32 _aes[AES_NUM_IVMRK_WORDS + 3];
public:
  CAesCbcCoder();
  MY_UNKNOWN_IMP1(ICryptoProperties)
  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
  STDMETHOD(SetKey)(const Byte *data, UInt32 size);
  STDMETHOD(SetInitVector)(const Byte *data, UInt32 size);
};

struct CAesCbcEncoder: public CAesCbcCoder { CAesCbcEncoder(); };
struct CAesCbcDecoder: public CAesCbcCoder { CAesCbcDecoder(); };

}

#endif
