// Crypto/CRarAES/RarAES.h

#ifndef __CRYPTO_RARAES_H
#define __CRYPTO_RARAES_H

#include "Interface/ICoder.h"
#include "../../Cipher/Common/CipherInterface.h"
#include "../../../Compress/Interface/CompressInterface.h"

#include "Common/Types.h"
#include "Common/Buffer.h"

// {23170F69-40C1-278B-06F1-0303000000000}
DEFINE_GUID(CLSID_CCryptoRar29Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-0601-010000000000}
DEFINE_GUID(CLSID_CCrypto_AES128_Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);

namespace NCrypto {
namespace NRar29 {

class CDecoder: 
  public ICompressCoder,
  public ICompressSetDecoderProperties,
  public ICryptoSetPassword,
  public CComObjectRoot,
  public CComCoClass<CDecoder, &CLSID_CCryptoRar29Decoder>
{
  BYTE _salt[8];
  bool _thereIsSalt;
  CByteBuffer buffer;
  BYTE aesKey[16];
  BYTE aesInit[16];
  bool _needCalculate;
public:

BEGIN_COM_MAP(CDecoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
  COM_INTERFACE_ENTRY(ICryptoSetPassword)
  COM_INTERFACE_ENTRY(ICompressSetDecoderProperties)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDecoder)

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CDecoder, 
    // "Crypto.Rar29Decoder.1", "Crypto.Rar29Decoder", 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, UINT64 const *inSize, 
      const UINT64 *outSize,ICompressProgressInfo *progress);

  STDMETHOD(CryptoSetPassword)(const BYTE *aData, UINT32 aSize);

  // ICompressSetDecoderProperties
  STDMETHOD(SetDecoderProperties)(ISequentialInStream *inStream);

  CDecoder();
};

}}

#endif