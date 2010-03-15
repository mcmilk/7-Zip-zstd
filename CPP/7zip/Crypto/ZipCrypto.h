// Crypto/ZipCrypto.h

#ifndef __CRYPTO_ZIP_CRYPTO_H
#define __CRYPTO_ZIP_CRYPTO_H

#include "Common/MyCom.h"

#include "../ICoder.h"
#include "../IPassword.h"

namespace NCrypto {
namespace NZip {

const unsigned kHeaderSize = 12;

class CCipher:
  public ICompressFilter,
  public ICryptoSetPassword,
  public CMyUnknownImp
{
  UInt32 Keys[3];
  UInt32 Keys2[3];

protected:
  void UpdateKeys(Byte b);
  Byte DecryptByteSpec();
  void RestoreKeys()
  {
    for (int i = 0; i < 3; i++)
      Keys[i] = Keys2[i];
  }

public:
  STDMETHOD(Init)();
  STDMETHOD(CryptoSetPassword)(const Byte *data, UInt32 size);
};

class CEncoder: public CCipher
{
public:
  MY_UNKNOWN_IMP1(ICryptoSetPassword)
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
  HRESULT WriteHeader(ISequentialOutStream *outStream, UInt32 crc);
};

class CDecoder: public CCipher
{
public:
  MY_UNKNOWN_IMP1(ICryptoSetPassword)
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
  HRESULT ReadHeader(ISequentialInStream *inStream);
};

}}

#endif
