// Sha3-256Reg.cpp /TR 2023-06-18

#include "StdAfx.h"

#include "../../C/CpuArch.h"

EXTERN_C_BEGIN
#include "../../C/hashes/sha3c.h"
EXTERN_C_END

#include "../Common/MyCom.h"
#include "../7zip/Common/RegisterCodec.h"

// SHA3-256
Z7_CLASS_IMP_COM_1(
  CSHA3_256Hasher
  , IHasher
)
  SHA3_CTX _ctx;
  Byte mtDummy[1 << 7];

public:
  CSHA3_256Hasher() { SHA3_Init(&_ctx, 256); }
};

Z7_COM7F_IMF2(void, CSHA3_256Hasher::Init())
{
  SHA3_Init(&_ctx, 256);
}

Z7_COM7F_IMF2(void, CSHA3_256Hasher::Update(const void *data, UInt32 size))
{
  SHA3_Update(&_ctx, (const Byte *)data, size);
}

Z7_COM7F_IMF2(void, CSHA3_256Hasher::Final(Byte *digest))
{
  SHA3_Final(digest, &_ctx);
}
REGISTER_HASHER(CSHA3_256Hasher, 0x20a, "SHA3-256", SHA3_256_DIGEST_LENGTH)
