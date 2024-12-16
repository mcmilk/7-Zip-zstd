// Sha512Reg.cpp /TR 2018-11-02

#include "StdAfx.h"

#include "../../C/CpuArch.h"

EXTERN_C_BEGIN
#include "../../C/hashes/sha512.h"
EXTERN_C_END

#include "../Common/MyCom.h"
#include "../7zip/Common/RegisterCodec.h"

// SHA512
Z7_CLASS_IMP_COM_1(
  CSHA512Hasher
  , IHasher
)
  SHA512_CTX _ctx;
  Byte mtDummy[1 << 7];

public:
  CSHA512Hasher() { SHA512_Init(&_ctx); }
};

Z7_COM7F_IMF2(void, CSHA512Hasher::Init())
{
  SHA512_Init(&_ctx);
}

Z7_COM7F_IMF2(void, CSHA512Hasher::Update(const void *data, UInt32 size))
{
  SHA512_Update(&_ctx, (const Byte *)data, size);
}

Z7_COM7F_IMF2(void, CSHA512Hasher::Final(Byte *digest))
{
  SHA512_Final(digest, &_ctx);
}
REGISTER_HASHER(CSHA512Hasher, 0x209, "SHA512", SHA512_DIGEST_LENGTH)
