// Crypto/ZipCrypto.h

#ifndef __CRYPTO_ZIP_CRYPTO_H
#define __CRYPTO_ZIP_CRYPTO_H

#include "Common/MyCom.h"

#include "../ICoder.h"
#include "../IPassword.h"

namespace NCrypto {
namespace NZip {

const unsigned kHeaderSize = 12;

class CCipher
{
  UInt32 Keys[3];

  void UpdateKeys(Byte b);
  Byte DecryptByteSpec();
public:
  void SetPassword(const Byte *password, UInt32 passwordLen);
  Byte DecryptByte(Byte b);
  Byte EncryptByte(Byte b);
  void DecryptHeader(Byte *buf);
  void EncryptHeader(Byte *buf);
};

class CEncoder :
  public ICompressFilter,
  public ICryptoSetPassword,
  public ICryptoSetCRC,
  public CMyUnknownImp
{
  CCipher _cipher;
  UInt32 _crc;
public:
  MY_UNKNOWN_IMP2(
      ICryptoSetPassword,
      ICryptoSetCRC
  )
  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);

  STDMETHOD(CryptoSetPassword)(const Byte *data, UInt32 size);
  STDMETHOD(CryptoSetCRC)(UInt32 crc);
  HRESULT WriteHeader(ISequentialOutStream *outStream);
};


class CDecoder:
  public ICompressFilter,
  public ICryptoSetPassword,
  public CMyUnknownImp
{
  CCipher _cipher;
public:
  MY_UNKNOWN_IMP1(ICryptoSetPassword)

  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
  STDMETHOD(CryptoSetPassword)(const Byte *data, UInt32 size);

  HRESULT ReadHeader(ISequentialInStream *inStream);
};


}}

#endif
