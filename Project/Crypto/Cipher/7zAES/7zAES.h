// 7z_AES.h

#ifndef __CRYPTO_7Z_AES_H
#define __CRYPTO_7Z_AES_H

#include "Interface/ICoder.h"
#include "../../Cipher/Common/CipherInterface.h"
#include "../../../Compress/Interface/CompressInterface.h"

#include "Common/Types.h"
#include "Common/Buffer.h"
#include "Common/Vector.h"

// {23170F69-40C1-278B-0703-000000000000}
DEFINE_GUID(CLSID_CCrypto_Hash_SHA256, 
0x23170F69, 0x40C1, 0x278B, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-06F1-070100000100}
DEFINE_GUID(CLSID_CCrypto7zAESEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x07, 0x01, 0x00, 0x00, 0x01, 0x00);

// {23170F69-40C1-278B-06F1-070100000000}
DEFINE_GUID(CLSID_CCrypto7zAESDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-0601-810000000100}
DEFINE_GUID(CLSID_CCrypto_AES256_Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, 0x81, 0x00, 0x00, 0x00, 0x01, 0x00);

// {23170F69-40C1-278B-0601-810000000000}
DEFINE_GUID(CLSID_CCrypto_AES256_Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00);

namespace NCrypto {
namespace NSevenZ {

const kKeySize = 32;

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
  public CComObjectRoot,
  public CComCoClass<CEncoder, &CLSID_CCrypto7zAESEncoder>,
  public CBase
{
BEGIN_COM_MAP(CEncoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
  COM_INTERFACE_ENTRY(ICryptoSetPassword)
  COM_INTERFACE_ENTRY(ICompressSetDecoderProperties)
  COM_INTERFACE_ENTRY(ICompressWriteCoderProperties)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CEncoder)

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CEncoder, 
    // "Crypto.7zAESEncoder.1", "Crypto.7zAESEncoder", 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, UINT64 const *inSize, 
      const UINT64 *outSize,ICompressProgressInfo *progress);
  
  STDMETHOD(CryptoSetPassword)(const BYTE *aData, UINT32 aSize);

  // ICompressSetDecoderProperties
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *inStream);

  // ICompressWriteCoderProperties
  STDMETHOD(WriteCoderProperties)(ISequentialOutStream *outStream);
};

class CDecoder: 
  public ICompressCoder,
  public ICompressSetDecoderProperties,
  public ICryptoSetPassword,
  public CComObjectRoot,
  public CComCoClass<CDecoder, &CLSID_CCrypto7zAESDecoder>,
  public CBase
{
BEGIN_COM_MAP(CDecoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
  COM_INTERFACE_ENTRY(ICryptoSetPassword)
  COM_INTERFACE_ENTRY(ICompressSetDecoderProperties)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDecoder)

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CDecoder, 
    // "Crypto.7zAESDecoder.1", "Crypto.7zAESDecoder", 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, UINT64 const *inSize, 
      const UINT64 *outSize,ICompressProgressInfo *progress);
  
  STDMETHOD(CryptoSetPassword)(const BYTE *aData, UINT32 aSize);
  // ICompressSetDecoderProperties
  
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *inStream);
};

}}

#endif