// Crypto/Rar20/Encoder.h

#ifndef __CRYPTO_RAR20_ENCODER_H
#define __CRYPTO_RAR20_ENCODER_H

#include "Interface/ICoder.h"
#include "../Common/CipherInterface.h"

#include "Common/Types.h"
#include "Crypto.h"

// {23170F69-40C1-278A-1000-000205000000}
DEFINE_GUID(CLSID_CCopmpressRar20CryptoEncoder, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x02, 0x50, 0x00, 0x00, 0x00);

// {23170F69-40C1-278A-1000-000250010000}
DEFINE_GUID(CLSID_CCryptoRar20Decoder, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x02, 0x50, 0x01, 0x00, 0x00);

class CBuffer
{
protected:
  BYTE *m_Buffer;
public:
  CBuffer();
  ~CBuffer();
};

namespace NCrypto {
namespace NRar20 {


/*
class CEncoder : 
  public ICompressCoder,
  public CBuffer,
  public CComObjectRoot,
  public CComCoClass<CEncoder, &CLSID_CCopmpressRar20CryptoEncoder>
{
public:
BEGIN_COM_MAP(CEncoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CEncoder)

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CEncoder, "Compress.Rar20CryptoEncoder.1", "Compress.Rar20CryptoEncoder", 0, THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, const UINT64 *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);
};
*/


class CDecoder: 
  public ICompressCoder,
  public ICryptoSetPassword,
  public CBuffer,
  public CComObjectRoot,
  public CComCoClass<CDecoder, &CLSID_CCryptoRar20Decoder>
{
public:
  CData m_Data;

BEGIN_COM_MAP(CDecoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
  COM_INTERFACE_ENTRY(ICryptoSetPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDecoder)

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CDecoder, "Crypto.Rar20Decoder.1", "Crypto.Rar20Decoder", 0, THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *anInStream,
      ISequentialOutStream *anOutStream, UINT64 const *anInSize, const UINT64 *anOutSize,
      ICompressProgressInfo *aProgress);

  STDMETHOD(CryptoSetPassword)(const BYTE *aData, UINT32 aSize);

};

}}

#endif