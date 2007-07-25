// RarAES.h

#ifndef __CRYPTO_RARAES_H
#define __CRYPTO_RARAES_H

#include "Common/MyCom.h"
#include "Common/Types.h"
#include "Common/Buffer.h"

#include "../../ICoder.h"
#include "../../IPassword.h"

extern "C" 
{ 
#include "../../../../C/Crypto/Aes.h"
}

namespace NCrypto {
namespace NRar29 {

const UInt32 kRarAesKeySize = 16;

class CDecoder: 
  public ICompressFilter,
  public ICompressSetDecoderProperties2,
  public ICryptoSetPassword,
  public CMyUnknownImp
{
  Byte _salt[8];
  bool _thereIsSalt;
  CByteBuffer buffer;
  Byte aesKey[kRarAesKeySize];
  Byte aesInit[AES_BLOCK_SIZE];
  bool _needCalculate;

  CAesCbc Aes;

  bool _rar350Mode;

  void Calculate();

public:

  MY_UNKNOWN_IMP2(
    ICryptoSetPassword,
    ICompressSetDecoderProperties2)

  STDMETHOD(Init)();

  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);

  STDMETHOD(CryptoSetPassword)(const Byte *aData, UInt32 aSize);

  // ICompressSetDecoderProperties
  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);

  CDecoder();
  void SetRar350Mode(bool rar350Mode) { _rar350Mode = rar350Mode; }
};

}}

#endif
