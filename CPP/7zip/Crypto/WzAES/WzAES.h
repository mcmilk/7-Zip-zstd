// WzAES.h
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

#include "../Hash/HmacSha1.h"

#include "Common/MyCom.h"
#include "Common/Buffer.h"
#include "Common/MyVector.h"

#include "../../ICoder.h"
#include "../../IPassword.h"

extern "C" 
{ 
#include "../../../../C/Crypto/Aes.h"
}

namespace NCrypto {
namespace NWzAES {

const unsigned int kSaltSizeMax = 16;
const unsigned int kMacSize = 10;

const UInt32 kPasswordSizeMax = 99; // 128;

// Password Verification Code Size
const unsigned int kPwdVerifCodeSize = 2;

class CKeyInfo
{
public:
  Byte KeySizeMode; // 1 - 128-bit , 2 - 192-bit , 3 - 256-bit
  Byte Salt[kSaltSizeMax];
  Byte PwdVerifComputed[kPwdVerifCodeSize];

  CByteBuffer Password;

  UInt32 GetKeySize() const  { return (8 * (KeySizeMode & 3) + 8); }
  UInt32 GetSaltSize() const { return (4 * (KeySizeMode & 3) + 4); }

  CKeyInfo() { Init(); }
  void Init() { KeySizeMode = 3; }
};

class CBaseCoder: 
  public ICompressFilter,
  public ICryptoSetPassword,
  public CMyUnknownImp
{
protected:
  CKeyInfo _key;
  UInt32 _counter[AES_BLOCK_SIZE / 4];
  Byte _buffer[AES_BLOCK_SIZE];
  NSha1::CHmac _hmac;
  unsigned int _blockPos;
  Byte _pwdVerifFromArchive[kPwdVerifCodeSize];

  void EncryptData(Byte *data, UInt32 size);

  CAes Aes;

public:
  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size) = 0;
  
  STDMETHOD(CryptoSetPassword)(const Byte *data, UInt32 size);

  UInt32 GetHeaderSize() const { return _key.GetSaltSize() + kPwdVerifCodeSize; }
};

class CEncoder: 
  public CBaseCoder
  // public ICompressWriteCoderProperties
{
public:
  MY_UNKNOWN_IMP1(ICryptoSetPassword)
  //  ICompressWriteCoderProperties
  // STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
  HRESULT WriteHeader(ISequentialOutStream *outStream);
  HRESULT WriteFooter(ISequentialOutStream *outStream);
  bool SetKeyMode(Byte mode)
  {
    if (mode < 1 || mode > 3)
      return false;
    _key.KeySizeMode = mode;
    return true;
  }
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
