// Crypto/Rar20Cipher.h

#ifndef __CRYPTO_RAR20_CIPHER_H
#define __CRYPTO_RAR20_CIPHER_H

#include "../../ICoder.h"
#include "../../IPassword.h"
#include "Common/MyCom.h"

#include "Common/Types.h"
#include "Rar20Crypto.h"

namespace NCrypto {
namespace NRar20 {

class CDecoder: 
  public ICompressCoder,
  public ICryptoSetPassword,
  public CMyUnknownImp
{
  BYTE *_buffer;
public:
  CData _coder;

  CDecoder();
  ~CDecoder();

  MY_UNKNOWN_IMP1(ICryptoSetPassword)

  STDMETHOD(Code)(ISequentialInStream *inStream,
      ISequentialOutStream *outStream, UINT64 const *inSize, const UINT64 *outSize,
      ICompressProgressInfo *progress);

  STDMETHOD(CryptoSetPassword)(const BYTE *data, UINT32 size);

};

}}

#endif