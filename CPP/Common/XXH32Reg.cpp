// XXH32Reg.cpp

#include "StdAfx.h"

#define XXH_STATIC_LINKING_ONLY
#include "../../C/CpuArch.h"
#include "../../C/zstd/xxhash.h"

#include "../Common/MyCom.h"

#include "../7zip/Common/RegisterCodec.h"

class CXXH32Hasher:
  public IHasher,
  public CMyUnknownImp
{
  XXH32_state_t *_ctx;
  Byte mtDummy[1 << 7];

public:
  CXXH32Hasher() { _ctx = XXH32_createState(); }
  ~CXXH32Hasher() { XXH32_freeState(_ctx); }

  MY_UNKNOWN_IMP1(IHasher)
  INTERFACE_IHasher(;)
};

STDMETHODIMP_(void) CXXH32Hasher::Init() throw()
{
  XXH32_reset(_ctx, 0);
}

STDMETHODIMP_(void) CXXH32Hasher::Update(const void *data, UInt32 size) throw()
{
  XXH32_update(_ctx, data, size);
}

STDMETHODIMP_(void) CXXH32Hasher::Final(Byte *digest) throw()
{
  UInt32 val = XXH32_digest(_ctx);
  SetUi32(digest, val);
}

REGISTER_HASHER(CXXH32Hasher, 0x202, "XXH32", 4)
