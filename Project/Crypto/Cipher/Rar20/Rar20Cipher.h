// Crypto/Rar20Cipher.h

#ifndef __CRYPTO_RAR20_CIPHER_H
#define __CRYPTO_RAR20_CIPHER_H

#include "Interface/ICoder.h"
#include "../Common/CipherInterface.h"

#include "Common/Types.h"
#include "Rar20Crypto.h"

// {23170F69-40C1-278B-06F1-0302000000000}
DEFINE_GUID(CLSID_CCryptoRar20Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00);

namespace NCrypto {
namespace NRar20 {

class CDecoder: 
  public ICompressCoder,
  public ICryptoSetPassword,
  public CComObjectRoot,
  public CComCoClass<CDecoder, &CLSID_CCryptoRar20Decoder>
{
  BYTE *_buffer;
public:
  CData _coder;

  CDecoder();
  ~CDecoder();

BEGIN_COM_MAP(CDecoder)
  COM_INTERFACE_ENTRY(ICompressCoder)
  COM_INTERFACE_ENTRY(ICryptoSetPassword)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDecoder)

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CDecoder, 
    // "Crypto.Rar20Decoder.1", "Crypto.Rar20Decoder", 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, UINT64 const *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(CryptoSetPassword)(const BYTE *data, UINT32 size);

};

}}

#endif