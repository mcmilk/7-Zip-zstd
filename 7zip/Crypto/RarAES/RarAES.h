// Crypto/CRarAES/RarAES.h

#ifndef __CRYPTO_RARAES_H
#define __CRYPTO_RARAES_H

#include "Common/MyCom.h"
#include "../../ICoder.h"
#include "../../IPassword.h"

#include "Common/Types.h"
#include "Common/Buffer.h"

namespace NCrypto {
namespace NRar29 {

class CDecoder: 
  public ICompressCoder,
  public ICompressSetDecoderProperties,
  public ICryptoSetPassword,
  public CMyUnknownImp
{
  BYTE _salt[8];
  bool _thereIsSalt;
  CByteBuffer buffer;
  BYTE aesKey[16];
  BYTE aesInit[16];
  bool _needCalculate;
public:

  MY_UNKNOWN_IMP2(
    ICryptoSetPassword,
    ICompressSetDecoderProperties)

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