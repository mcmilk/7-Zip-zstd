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

// {23170F69-40C1-278B-0601-810000000100}
DEFINE_GUID(CLSID_CCrypto_AES256_Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, 0x81, 0x00, 0x00, 0x00, 0x01, 0x00);

// {23170F69-40C1-278B-0601-810000000000}
DEFINE_GUID(CLSID_CCrypto_AES256_Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00);

namespace NCrypto {
namespace NSevenZ {

const int kKeySize = 32;

class CKeyInfo
{
public:
  int NumCyclesPower;
  UINT32 SaltSize;
  BYTE Salt[16];
  CByteBuffer Password;
  BYTE Key[kKeySize];

  bool IsEqualTo(const CKeyInfo &a) const;
  void CalculateDigest();

  CKeyInfo()
  {
    Init();
  }
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
  BYTE _iv[16];
  // int _ivSize;
  void CalculateDigest();
  CBase();
};

class CEncoder: 
  public ICompressCoder,
  public ICompressSetDecoderProperties,
  public ICryptoSetPassword,
  public ICompressWriteCoderProperties,
  public CMyUnknownImp,
  public CBase
{
  MY_UNKNOWN_IMP3(
      ICryptoSetPassword,
      ICompressSetDecoderProperties,
      ICompressWriteCoderProperties
      )

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, UINT64 const *inSize, 
      const UINT64 *outSize,ICompressProgressInfo *progress);
  
  STDMETHOD(CryptoSetPassword)(const BYTE *aData, UINT32 aSize);

  // ICompressSetDecoderProperties
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *inStream);

  // ICompressWriteCoderProperties
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);

  #ifndef CRYPTO_AES
  CCoderLibrary _aesEncoderLibrary;
  #endif
  CMyComPtr<ICompressCoder2> _aesEncoder;
};

class CDecoder: 
  public ICompressCoder,
  public ICompressSetDecoderProperties,
  public ICryptoSetPassword,
  public CMyUnknownImp,
  public CBase
{
  MY_UNKNOWN_IMP2(
      ICryptoSetPassword,
      ICompressSetDecoderProperties
      )

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, UINT64 const *inSize, 
      const UINT64 *outSize,ICompressProgressInfo *progress);
  
  STDMETHOD(CryptoSetPassword)(const BYTE *aData, UINT32 aSize);
  // ICompressSetDecoderProperties
  
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *inStream);

  #ifndef CRYPTO_AES
  CCoderLibrary _aesDecoderLibrary;
  #endif
  CMyComPtr<ICompressCoder2> _aesDecoder;
};

}}

#endif
