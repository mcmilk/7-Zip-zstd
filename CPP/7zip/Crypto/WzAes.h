// Crypto/WzAes.h
/*
This code implements Brian Gladman's scheme
specified in password Based File Encryption Utility:
  - AES encryption (128,192,256-bit) in Counter (CTR) mode.
  - HMAC-SHA1 authentication for encrypted data (10 bytes)
  - Keys are derived by PPKDF2(RFC2898)-HMAC-SHA1 from ASCII password and
    Salt (saltSize = aesKeySize / 2).
  - 2 bytes contain Password Verifier's Code
*/

#ifndef __CRYPTO_WZ_AES_H
#define __CRYPTO_WZ_AES_H

#include "../../../C/Aes.h"

#include "Common/Buffer.h"
#include "Common/MyCom.h"
#include "Common/MyVector.h"

#include "../ICoder.h"
#include "../IPassword.h"

#include "HmacSha1.h"

namespace NCrypto {
namespace NWzAes {

const unsigned kSaltSizeMax = 16;
const unsigned kMacSize = 10;

const UInt32 kPasswordSizeMax = 99; // 128;

// Password Verification Code Size
const unsigned kPwdVerifCodeSize = 2;

enum EKeySizeMode
{
  kKeySizeMode_AES128 = 1,
  kKeySizeMode_AES192 = 2,
  kKeySizeMode_AES256 = 3
};

class CKeyInfo
{
public:
  EKeySizeMode KeySizeMode;
  Byte Salt[kSaltSizeMax];
  Byte PwdVerifComputed[kPwdVerifCodeSize];

  CByteBuffer Password;

  UInt32 GetKeySize() const  { return (8 * (KeySizeMode & 3) + 8); }
  UInt32 GetSaltSize() const { return (4 * (KeySizeMode & 3) + 4); }

  CKeyInfo() { Init(); }
  void Init() { KeySizeMode = kKeySizeMode_AES256; }
};

struct CAesCtr2
{
  unsigned pos;
  unsigned offset;
  UInt32 aes[4 + AES_NUM_IVMRK_WORDS + 3];
  CAesCtr2();
};

void AesCtr2_Init(CAesCtr2 *p);
void AesCtr2_Code(CAesCtr2 *p, Byte *data, SizeT size);

class CBaseCoder:
  public ICompressFilter,
  public ICryptoSetPassword,
  public CMyUnknownImp
{
protected:
  CKeyInfo _key;
  NSha1::CHmac _hmac;
  Byte _pwdVerifFromArchive[kPwdVerifCodeSize];
  CAesCtr2 _aes;

public:
  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size) = 0;
  
  STDMETHOD(CryptoSetPassword)(const Byte *data, UInt32 size);

  UInt32 GetHeaderSize() const { return _key.GetSaltSize() + kPwdVerifCodeSize; }
  bool SetKeyMode(unsigned mode)
  {
    if (mode < kKeySizeMode_AES128 || mode > kKeySizeMode_AES256)
      return false;
    _key.KeySizeMode = (EKeySizeMode)mode;
    return true;
  }
};

class CEncoder:
  public CBaseCoder
{
public:
  MY_UNKNOWN_IMP1(ICryptoSetPassword)
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
  HRESULT WriteHeader(ISequentialOutStream *outStream);
  HRESULT WriteFooter(ISequentialOutStream *outStream);
};

class CDecoder:
  public CBaseCoder,
  public ICompressSetDecoderProperties2
{
public:
  MY_UNKNOWN_IMP2(
      ICryptoSetPassword,
      ICompressSetDecoderProperties2)
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);
  HRESULT ReadHeader(ISequentialInStream *inStream);
  bool CheckPasswordVerifyCode();
  HRESULT CheckMac(ISequentialInStream *inStream, bool &isOK);
};

}}

#endif
