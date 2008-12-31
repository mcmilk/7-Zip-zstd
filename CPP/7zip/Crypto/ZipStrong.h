// Crypto/ZipStrong.h

#ifndef __CRYPTO_ZIP_STRONG_H
#define __CRYPTO_ZIP_STRONG_H

#include "Common/MyCom.h"
#include "Common/Buffer.h"

#include "../ICoder.h"
#include "../IPassword.h"

namespace NCrypto {
namespace NZipStrong {

struct CKeyInfo
{
  Byte MasterKey[32];
  UInt32 KeySize;
  void SetPassword(const Byte *data, UInt32 size);
};

class CBaseCoder:
  public ICompressFilter,
  public ICryptoSetPassword,
  public CMyUnknownImp
{
protected:
  CKeyInfo _key;
  CMyComPtr<ICompressFilter> _aesFilter;
  CByteBuffer _buf;
public:
  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size) = 0;
  
  STDMETHOD(CryptoSetPassword)(const Byte *data, UInt32 size);
};

class CDecoder:
  public CBaseCoder
{
  UInt32 _ivSize;
  Byte _iv[16];
  UInt32 _remSize;
public:
  MY_UNKNOWN_IMP1(ICryptoSetPassword)
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
  HRESULT ReadHeader(ISequentialInStream *inStream, UInt32 crc, UInt64 unpackSize);
  HRESULT CheckPassword(bool &passwOK);
};

}}

#endif
