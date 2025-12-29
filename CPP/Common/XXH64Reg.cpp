// XXH64Reg.cpp /TR 2018-11-02

#include "StdAfx.h"

#include "../../C/CpuArch.h"

#define XXH_STATIC_LINKING_ONLY
#include "../../C/hashes/xxhash.h"

#include "../Common/MyCom.h"
#include "../7zip/Common/RegisterCodec.h"

// XXH64
Z7_CLASS_IMP_COM_1(
  CXXH64Hasher
  , IHasher
)
  XXH64_state_t *_ctx;

public:
  CXXH64Hasher() { _ctx = XXH64_createState(); }
  ~CXXH64Hasher() { XXH64_freeState(_ctx); }
};

Z7_COM7F_IMF2(void, CXXH64Hasher::Init())
{
  XXH64_reset(_ctx, 0);
}

Z7_COM7F_IMF2(void, CXXH64Hasher::Update(const void *data, UInt32 size))
{
  XXH64_update(_ctx, data, size);
}

Z7_COM7F_IMF2(void, CXXH64Hasher::Final(Byte *digest))
{
  UInt64 val = XXH64_digest(_ctx);
  SetUi64(digest, val);
}

REGISTER_HASHER(CXXH64Hasher, 0x211, "XXH64", 8)
