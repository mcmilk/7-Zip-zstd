// Crypto/SHA/MySHA256.h

#ifndef __CRYPTO_MYSHA256_H
#define __CRYPTO_MYSHA256_H

#include "Interface/ICoder.h"
#include "../Common/CryptoHashInterface.h"

#include "SHA256.h"

// {23170F69-40C1-278B-0703-000000000000}
DEFINE_GUID(CLSID_CCrypto_Hash_SHA256, 
0x23170F69, 0x40C1, 0x278B, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

namespace NCrypto {
namespace NSHA256 {

class CHash: 
  public ICryptoHash,
  public CComObjectRoot,
  public CComCoClass<CHash, &CLSID_CCrypto_Hash_SHA256>
{
  SHA256 _SHA;
  // CryptoPP::RIPEMD160 _SHA;
public:

BEGIN_COM_MAP(CHash)
  COM_INTERFACE_ENTRY(ICryptoHash)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CHash)

//DECLARE_NO_REGISTRY()
DECLARE_REGISTRY(CHash, 
    // "Crypto.HashSHA.1", "Crypto.HashSHA", 
    TEXT("SevenZip.1"), TEXT("SevenZip"),
    UINT(0), THREADFLAGS_APARTMENT)

  STDMETHOD(Init)();
  STDMETHOD(Update)(const void *data, UINT32 size);
  STDMETHOD(GetDigest)(BYTE *aDigestData);
	STDMETHOD(GetDigestSize)(UINT32 *size);

};

}}

#endif