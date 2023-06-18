// Sha3-512Reg.cpp /TR 2023-06-18

#include "StdAfx.h"

#include "../../C/CpuArch.h"

EXTERN_C_BEGIN
#include "../../C/hashes/sha3.h"
EXTERN_C_END

#include "../Common/MyCom.h"
#include "../7zip/Common/RegisterCodec.h"

// SHA3-512
class CSHA3_512Hasher:
  public IHasher,
  public CMyUnknownImp
{
  SHA3_CTX _ctx;
  Byte mtDummy[1 << 7];

public:
  CSHA3_512Hasher() { SHA3_Init(&_ctx, 512); }

  MY_UNKNOWN_IMP1(IHasher)
  INTERFACE_IHasher(;)
};

STDMETHODIMP_(void) CSHA3_512Hasher::Init() throw()
{
  SHA3_Init(&_ctx, 512);
}

STDMETHODIMP_(void) CSHA3_512Hasher::Update(const void *data, UInt32 size) throw()
{
  SHA3_Update(&_ctx, (const Byte *)data, size);
}

STDMETHODIMP_(void) CSHA3_512Hasher::Final(Byte *digest) throw()
{
  SHA3_Final(digest, &_ctx);
}
REGISTER_HASHER(CSHA3_512Hasher, 0x20c, "SHA3-512", SHA3_512_DIGEST_LENGTH)
