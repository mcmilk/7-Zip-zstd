// Blake3Reg.cpp /TR 2021-04-06

#include "StdAfx.h"

#include "../../C/CpuArch.h"

EXTERN_C_BEGIN
#include "../../C/hashes/blake3.h"
EXTERN_C_END

#include "../Common/MyCom.h"
#include "../7zip/Common/RegisterCodec.h"

// BLAKE3
Z7_CLASS_IMP_COM_1(
  CBLAKE3Hasher
  , IHasher
)
  blake3_hasher _ctx;

public:
  CBLAKE3Hasher() { blake3_hasher_init(&_ctx); }
};

Z7_COM7F_IMF2(void, CBLAKE3Hasher::Init())
{
  blake3_hasher_init(&_ctx);
}

Z7_COM7F_IMF2(void, CBLAKE3Hasher::Update(const void *data, UInt32 size))
{
  blake3_hasher_update(&_ctx, data, size);
}

Z7_COM7F_IMF2(void, CBLAKE3Hasher::Final(Byte *digest))
{
  blake3_hasher_finalize(&_ctx, digest, BLAKE3_OUT_LEN);
}
REGISTER_HASHER(CBLAKE3Hasher, 0x204, "BLAKE3", BLAKE3_OUT_LEN)
