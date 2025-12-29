// XXH3Reg.cpp /TR

#include "StdAfx.h"

#include "../../C/CpuArch.h"

#define XXH_STATIC_LINKING_ONLY
#include "../../C/hashes/xxhash.h"

#include "../Common/MyCom.h"
#include "../7zip/Common/RegisterCodec.h"

// XXH3-64
Z7_CLASS_IMP_COM_1(
  CXXH3Hasher64
  , IHasher
)
  XXH3_state_t *_ctx;

public:
  CXXH3Hasher64() { _ctx = XXH3_createState(); }
  ~CXXH3Hasher64() { XXH3_freeState(_ctx); }
};

Z7_COM7F_IMF2(void, CXXH3Hasher64::Init())
{
  XXH3_64bits_reset(_ctx);
}

Z7_COM7F_IMF2(void, CXXH3Hasher64::Update(const void *data, UInt32 size))
{
  XXH3_64bits_update(_ctx, data, size);
}

Z7_COM7F_IMF2(void, CXXH3Hasher64::Final(Byte *digest))
{
  XXH64_canonicalFromHash((XXH64_canonical_t *)digest, XXH3_64bits_digest(_ctx));
}
REGISTER_HASHER(CXXH3Hasher64, 0x212, "XXH3-64", 8)
