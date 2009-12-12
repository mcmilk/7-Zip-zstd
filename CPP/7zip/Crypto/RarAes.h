// Crypto/RarAes.h

#ifndef __CRYPTO_RAR_AES_H
#define __CRYPTO_RAR_AES_H

#include "../../../C/Aes.h"

#include "Common/Buffer.h"

#include "../IPassword.h"

#include "MyAes.h"

namespace NCrypto {
namespace NRar29 {

const UInt32 kRarAesKeySize = 16;

class CDecoder:
  public CAesCbcDecoder,
  public ICompressSetDecoderProperties2,
  public ICryptoSetPassword
{
  Byte _salt[8];
  bool _thereIsSalt;
  CByteBuffer buffer;
  Byte aesKey[kRarAesKeySize];
  Byte _aesInit[AES_BLOCK_SIZE];
  bool _needCalculate;
  bool _rar350Mode;

  void Calculate();
public:
  MY_UNKNOWN_IMP2(
    ICryptoSetPassword,
    ICompressSetDecoderProperties2)
  STDMETHOD(Init)();
  STDMETHOD(CryptoSetPassword)(const Byte *aData, UInt32 aSize);
  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);

  CDecoder();
  void SetRar350Mode(bool rar350Mode) { _rar350Mode = rar350Mode; }
};

}}

#endif
