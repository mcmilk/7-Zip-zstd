// Crypto/Zip/Encoder.h

#ifndef __CRYPTO_ZIP_ENCODER_H
#define __CRYPTO_ZIP_ENCODER_H

#include "Interface/ICoder.h"
#include "../Common/CipherInterface.h"
#include "Common/Random.h"

#include "Common/Types.h"
#include "Crypto.h"

// {23170F69-40C1-278A-1000-000250030100}
DEFINE_GUID(CLSID_CCryptoZipEncoder, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x02, 0x50, 0x03, 0x01, 0x00);

// {23170F69-40C1-278A-1000-000250030000}
DEFINE_GUID(CLSID_CCryptoZipDecoder, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x02, 0x50, 0x03, 0x00, 0x00);

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
DECLARE_REGISTRY(CEncoder, TEXT("Crypto.ZipEncoder.1"), 
    TEXT("Crypto.ZipEncoder"), UINT(0), THREADFLAGS_APARTMENT)

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
DECLARE_REGISTRY(CDecoder, TEXT("Crypto.ZipDecoder.1"), 
                 TEXT("Crypto.ZipDecoder"), UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, UINT64 const *inSize, 
      const UINT64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(CryptoSetPassword)(const BYTE *data, UINT32 size);
};

}}

#endif