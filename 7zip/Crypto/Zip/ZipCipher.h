// Crypto/ZipCipher.h

#ifndef __CRYPTO_ZIPCIPHER_H
#define __CRYPTO_ZIPCIPHER_H

#include "Common/MyCom.h"
#include "Common/Random.h"
#include "Common/Types.h"

#include "../../ICoder.h"
#include "../../IPassword.h"

#include "ZipCrypto.h"

namespace NCrypto {
namespace NZip {

/*
class CBuffer2
{
protected:
  Byte *_buffer;
public:
  CBuffer2();
  ~CBuffer2();
};
*/
class CEncoder : 
  public ICompressFilter,
  public ICryptoSetPassword,
  public ICryptoSetCRC,
  public CMyUnknownImp
  // public CBuffer2
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
  // public CBuffer2
{
  CCipher _cipher;
public:

  MY_UNKNOWN_IMP1(ICryptoSetPassword)

  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);

  HRESULT ReadHeader(ISequentialInStream *inStream);
  STDMETHOD(CryptoSetPassword)(const Byte *data, UInt32 size);
};

}}

#endif
