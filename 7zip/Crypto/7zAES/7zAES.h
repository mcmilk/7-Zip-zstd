// 7z_AES.h

#ifndef __CRYPTO_7Z_AES_H
#define __CRYPTO_7Z_AES_H

#include "Common/MyCom.h"
#include "Common/Types.h"
#include "Common/Buffer.h"
#include "Common/Vector.h"

#include "../../ICoder.h"
#include "../../IPassword.h"

#ifndef CRYPTO_AES
#include "../../Archive/Common/CoderLoader.h"
#endif

DEFINE_GUID(CLSID_CCrypto_AES_CBC_Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, 0xC1, 0x00, 0x00, 0x00, 0x01, 0x00);

DEFINE_GUID(CLSID_CCrypto_AES_CBC_Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, 0xC1, 0x00, 0x00, 0x00, 0x00, 0x00);

namespace NCrypto {
namespace NSevenZ {

const int kKeySize = 32;

class CKeyInfo
{
public:
  int NumCyclesPower;
  UInt32 SaltSize;
  Byte Salt[16];
  CByteBuffer Password;
  Byte Key[kKeySize];

  bool IsEqualTo(const CKeyInfo &a) const;
  void CalculateDigest();

  CKeyInfo() { Init(); }
  void Init()
  {
    NumCyclesPower = 0;
    SaltSize = 0;
    for (int i = 0; i < sizeof(Salt); i++)
      Salt[i] = 0;
  }
};

class CKeyInfoCache
{
  int Size;
  CObjectVector<CKeyInfo> Keys;
public:
  CKeyInfoCache(int size): Size(size) {}
  bool Find(CKeyInfo &key);
  // HRESULT Calculate(CKeyInfo &key);
  void Add(CKeyInfo &key);
};

class CBase
{
  CKeyInfoCache _cachedKeys;
protected:
  CKeyInfo _key;
  Byte _iv[16];
  // int _ivSize;
  void CalculateDigest();
  CBase();
};

class CBaseCoder: 
  public ICompressFilter,
  public ICryptoSetPassword,
  public CMyUnknownImp,
  public CBase
{
protected:
  #ifndef CRYPTO_AES
  CCoderLibrary _aesLibrary;
  #endif
  CMyComPtr<ICompressFilter> _aesFilter;

  virtual HRESULT CreateFilter() = 0;
  #ifndef CRYPTO_AES
  HRESULT CreateFilterFromDLL(REFCLSID clsID);
  #endif
public:
  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
  
  STDMETHOD(CryptoSetPassword)(const Byte *data, UInt32 size);
};

class CEncoder: 
  public CBaseCoder,
  public ICompressWriteCoderProperties
{
  virtual HRESULT CreateFilter();
public:
  MY_UNKNOWN_IMP2(
      ICryptoSetPassword,
      ICompressWriteCoderProperties)
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);
};

class CDecoder: 
  public CBaseCoder,
  public ICompressSetDecoderProperties2
{
  virtual HRESULT CreateFilter();
public:
  MY_UNKNOWN_IMP2(
      ICryptoSetPassword,
      ICompressSetDecoderProperties2)
  STDMETHOD(SetDecoderProperties2)(const Byte *data, UInt32 size);
};

}}

#endif
