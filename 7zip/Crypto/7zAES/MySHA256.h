// Crypto/SHA256/MySHA256.h

#ifndef __CRYPTO_MYSHA256_H
#define __CRYPTO_MYSHA256_H

#include "Common/MyCom.h"
#include "../../ICoder.h"
#include "../ICryptoHash.h"

#include "SHA256.h"

namespace NCrypto {
namespace NSHA256 {

class CHash: 
  public ICryptoHash,
  public CMyUnknownImp
{
  SHA256 _SHA;
public:

  MY_UNKNOWN_IMP

  STDMETHOD(Init)();
  STDMETHOD(Update)(const void *data, UINT32 size);
  STDMETHOD(GetDigest)(BYTE *aDigestData);
	STDMETHOD(GetDigestSize)(UINT32 *size);
};

}}

#endif