// XXH3Reg.cpp /TR

#include "StdAfx.h"

#include "../../C/CpuArch.h"

#define XXH_STATIC_LINKING_ONLY
#include "../../C/hashes/xxhash.h"

#include "../Common/MyCom.h"
#include "../7zip/Common/RegisterCodec.h"

// XXH3-128
Z7_CLASS_IMP_COM_1(
  CXXH3Hasher128
  , IHasher
)
  XXH3_state_t *_ctx;

public:
  CXXH3Hasher128() { _ctx = XXH3_createState(); }
  ~CXXH3Hasher128() { XXH3_freeState(_ctx); }
};

Z7_COM7F_IMF2(void, CXXH3Hasher128::Init())
{
  XXH3_128bits_reset(_ctx);
}

Z7_COM7F_IMF2(void, CXXH3Hasher128::Update(const void *data, UInt32 size))
{
  XXH3_128bits_update(_ctx, data, size);
}

Z7_COM7F_IMF2(void, CXXH3Hasher128::Final(Byte *digest))
{
  XXH128_canonicalFromHash((XXH128_canonical_t *)digest, XXH3_128bits_digest(_ctx));
}
REGISTER_HASHER(CXXH3Hasher128, 0x213, "XXH3-128", 16)
