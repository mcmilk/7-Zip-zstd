// Sha384Reg.cpp /TR 2018-11-02

#include "StdAfx.h"

#include "../../C/CpuArch.h"

EXTERN_C_BEGIN
#include "../../C/hashes/sha512.h"
EXTERN_C_END

#include "../Common/MyCom.h"
#include "../7zip/Common/RegisterCodec.h"

// SHA384
Z7_CLASS_IMP_COM_1(
  CSHA384Hasher
  , IHasher
)
  SHA384_CTX _ctx;
  Byte mtDummy[1 << 7];

public:
  CSHA384Hasher() { SHA384_Init(&_ctx); }
};

Z7_COM7F_IMF2(void, CSHA384Hasher::Init())
{
  SHA384_Init(&_ctx);
}

Z7_COM7F_IMF2(void, CSHA384Hasher::Update(const void *data, UInt32 size))
{
  SHA384_Update(&_ctx, (const Byte *)data, size);
}

Z7_COM7F_IMF2(void, CSHA384Hasher::Final(Byte *digest))
{
  SHA384_Final(digest, &_ctx);
}
REGISTER_HASHER(CSHA384Hasher, 0x218, "SHA384", SHA384_DIGEST_LENGTH)
