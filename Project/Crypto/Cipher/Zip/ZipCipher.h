// Crypto/ZipCipher.h

#ifndef __CRYPTO_ZIPCIPHER_H
#define __CRYPTO_ZIPCIPHER_H

#include "Interface/ICoder.h"
#include "../Common/CipherInterface.h"

#include "Common/Random.h"
#include "Common/Types.h"

#include "ZipCrypto.h"

// {23170F69-40C1-278B-06F1-0101000000100}
DEFINE_GUID(CLSID_CCryptoZipEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00);

// {23170F69-40C1-278B-06F1-0101000000000}
DEFINE_GUID(CLSID_CCryptoZipDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00);

namespace NCrypto {
namespace NZip {

class CBuffer2
{
protected:
  BYTE *_buffer;
public:
  CBuffer2();
  ~CBuffer2();
};

class CEncoder : 
  public ICompressCoder,
  public ICryptoSetPassword,
  public ICryptoSetCRC,
  public CBuffer2,
  public CComObjectRoot,
  public CComCoClass<CEncoder, &CLSID_CCryptoZipEncoder>
{
  CCipher _cipher;
  UINT32 _crc;
public:
BEGIN_COM_MAP(CEncoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
  COM_INTERFACE_ENTRY(ICryptoSetPassword)
  COM_INTERFACE_ENTRY(ICryptoSetCRC)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CEncoder)

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CEncoder, 
    // TEXT("Crypto.ZipEncoder.1"), TEXT("Crypto.ZipEncoder"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, const UINT64 *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(CryptoSetPassword)(const BYTE *data, UINT32 size);
  STDMETHOD(CryptoSetCRC)(UINT32 crc);

  CEncoder();
};


class CDecoder: 
  public ICompressCoder,
  public ICryptoSetPassword,
  public CBuffer2,
  public CComObjectRoot,
  public CComCoClass<CDecoder, &CLSID_CCryptoZipDecoder>
{
  CCipher _cipher;
public:

BEGIN_COM_MAP(CDecoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
  COM_INTERFACE_ENTRY(ICryptoSetPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDecoder)

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CDecoder, 
    // TEXT("Crypto.ZipDecoder.1"), TEXT("Crypto.ZipDecoder"), 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, UINT64 const *inSize, 
      const UINT64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(CryptoSetPassword)(const BYTE *data, UINT32 size);
};

}}

#endif