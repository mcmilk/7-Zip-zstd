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
  public ICompressFilter,
  public ICryptoSetPassword,
  public CMyUnknownImp
{
public:
  CData _coder;

  MY_UNKNOWN_IMP1(ICryptoSetPassword)

  STDMETHOD(Init)();
  STDMETHOD_(UInt32, Filter)(Byte *data, UInt32 size);
  
  STDMETHOD(CryptoSetPassword)(const Byte *data, UInt32 size);

};

}}

#endif
