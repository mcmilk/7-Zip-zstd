// Crypto/Zip/Encoder.h

#ifndef __CRYPTO_ZIP_ENCODER_H
#define __CRYPTO_ZIP_ENCODER_H

#include "Interface/ICoder.h"
#include "../Common/CipherInterface.h"

#include "Common/Types.h"
#include "Crypto.h"

// {23170F69-40C1-278A-1000-000205020000}
DEFINE_GUID(CLSID_CZipCryptoEncoder, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x02, 0x50, 0x02, 0x00, 0x00);

// {23170F69-40C1-278A-1000-000250030000}
DEFINE_GUID(CLSID_CCryptoZipDecoder, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x02, 0x50, 0x03, 0x00, 0x00);

namespace NCrypto {
namespace NZip {

class CBuffer2
{
protected:
  BYTE *m_Buffer;
public:
  CBuffer2();
  ~CBuffer2();
};

/*
class CEncoder : 
  public ICompressCoder,
  public CBuffer,
  public CComObjectRoot,
  public CComCoClass<CEncoder, &CLSID_CZipCryptoEncoder>
{
public:
BEGIN_COM_MAP(CEncoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CEncoder)

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CEncoder, "Compress.ZipCryptoEncoder.1", "Compress.ZipCryptoEncoder", 0, THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);
};
*/


class CDecoder: 
  public ICompressCoder,
  public ICryptoSetPassword,
  public CBuffer2,
  public CComObjectRoot,
  public CComCoClass<CDecoder, &CLSID_CCryptoZipDecoder>
{
public:
  CData m_Data;

BEGIN_COM_MAP(CDecoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
  COM_INTERFACE_ENTRY(ICryptoSetPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDecoder)

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CDecoder, "Crypto.ZipDecoder.1", "Crypto.ZipDecoder", 0, THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, UINT64 const *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  STDMETHOD(CryptoSetPassword)(const BYTE *aData, UINT32 aSize);

};

}}

#endif