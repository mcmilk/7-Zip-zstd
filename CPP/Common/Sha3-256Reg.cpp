// Sha3-256Reg.cpp /TR 2023-06-18

#include "StdAfx.h"

#include "../../C/CpuArch.h"

EXTERN_C_BEGIN
#include "../../C/hashes/sha3.h"
EXTERN_C_END

#include "../Common/MyCom.h"
#include "../7zip/Common/RegisterCodec.h"

// SHA3-256
class CSHA3_256Hasher:
  public IHasher,
  public CMyUnknownImp
{
  SHA3_CTX _ctx;
  Byte mtDummy[1 << 7];

public:
  CSHA3_256Hasher() { SHA3_Init(&_ctx, 256); }

  MY_UNKNOWN_IMP1(IHasher)
  INTERFACE_IHasher(;)
};

STDMETHODIMP_(void) CSHA3_256Hasher::Init() throw()
{
  SHA3_Init(&_ctx, 256);
}

STDMETHODIMP_(void) CSHA3_256Hasher::Update(const void *data, UInt32 size) throw()
{
  SHA3_Update(&_ctx, (const Byte *)data, size);
}

STDMETHODIMP_(void) CSHA3_256Hasher::Final(Byte *digest) throw()
{
  SHA3_Final(digest, &_ctx);
}
REGISTER_HASHER(CSHA3_256Hasher, 0x20a, "SHA3-256", SHA3_256_DIGEST_LENGTH)
