// Sha3-384Reg.cpp /TR 2023-06-18

#include "StdAfx.h"

#include "../../C/CpuArch.h"

EXTERN_C_BEGIN
#include "../../C/hashes/sha3.h"
EXTERN_C_END

#include "../Common/MyCom.h"
#include "../7zip/Common/RegisterCodec.h"

// SHA3-384
class CSHA3_384Hasher:
  public IHasher,
  public CMyUnknownImp
{
  SHA3_CTX _ctx;
  Byte mtDummy[1 << 7];

public:
  CSHA3_384Hasher() { SHA3_Init(&_ctx, 384); }

  MY_UNKNOWN_IMP1(IHasher)
  INTERFACE_IHasher(;)
};

STDMETHODIMP_(void) CSHA3_384Hasher::Init() throw()
{
  SHA3_Init(&_ctx, 384);
}

STDMETHODIMP_(void) CSHA3_384Hasher::Update(const void *data, UInt32 size) throw()
{
  SHA3_Update(&_ctx, (const Byte *)data, size);
}

STDMETHODIMP_(void) CSHA3_384Hasher::Final(Byte *digest) throw()
{
  SHA3_Final(digest, &_ctx);
}
REGISTER_HASHER(CSHA3_384Hasher, 0x20b, "SHA3-384", SHA3_384_DIGEST_LENGTH)
