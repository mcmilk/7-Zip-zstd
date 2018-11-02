// XXH32Reg.cpp

#include "StdAfx.h"

#include "../../C/CpuArch.h"

#define XXH_STATIC_LINKING_ONLY
#include "../../C/zstd/xxhash.h"
#include "../../C/hashes/md2.h"
#include "../../C/hashes/md4.h"
#include "../../C/hashes/md5.h"
#include "../../C/hashes/sha.h"

#include "../Common/MyCom.h"
#include "../7zip/Common/RegisterCodec.h"

// XXH32
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

REGISTER_HASHER(CXXH32Hasher, 0x203, "XXH32", 4)

// XXH64
class CXXH64Hasher:
  public IHasher,
  public CMyUnknownImp
{
  XXH64_state_t *_ctx;
  Byte mtDummy[1 << 7];

public:
  CXXH64Hasher() { _ctx = XXH64_createState(); }
  ~CXXH64Hasher() { XXH64_freeState(_ctx); }

  MY_UNKNOWN_IMP1(IHasher)
  INTERFACE_IHasher(;)
};

STDMETHODIMP_(void) CXXH64Hasher::Init() throw()
{
  XXH64_reset(_ctx, 0);
}

STDMETHODIMP_(void) CXXH64Hasher::Update(const void *data, UInt32 size) throw()
{
  XXH64_update(_ctx, data, size);
}

STDMETHODIMP_(void) CXXH64Hasher::Final(Byte *digest) throw()
{
  UInt64 val = XXH64_digest(_ctx);
  SetUi64(digest, val);
}
REGISTER_HASHER(CXXH64Hasher, 0x204, "XXH64", 8)

// MD2
class CMD2Hasher:
  public IHasher,
  public CMyUnknownImp
{
  MD2_CTX _ctx;
  Byte mtDummy[1 << 7];

public:
  CMD2Hasher() { MD2_Init(&_ctx); }

  MY_UNKNOWN_IMP1(IHasher)
  INTERFACE_IHasher(;)
};

STDMETHODIMP_(void) CMD2Hasher::Init() throw()
{
  MD2_Init(&_ctx);
}

STDMETHODIMP_(void) CMD2Hasher::Update(const void *data, UInt32 size) throw()
{
  MD2_Update(&_ctx, (const Byte *)data, size);
}

STDMETHODIMP_(void) CMD2Hasher::Final(Byte *digest) throw()
{
  MD2_Final(digest, &_ctx);
}
REGISTER_HASHER(CMD2Hasher, 0x205, "MD2", MD2_DIGEST_LENGTH)

// MD4
class CMD4Hasher:
  public IHasher,
  public CMyUnknownImp
{
  MD4_CTX _ctx;
  Byte mtDummy[1 << 7];

public:
  CMD4Hasher() { MD4_Init(&_ctx); }

  MY_UNKNOWN_IMP1(IHasher)
  INTERFACE_IHasher(;)
};

STDMETHODIMP_(void) CMD4Hasher::Init() throw()
{
  MD4_Init(&_ctx);
}

STDMETHODIMP_(void) CMD4Hasher::Update(const void *data, UInt32 size) throw()
{
  MD4_Update(&_ctx, (const Byte *)data, size);
}

STDMETHODIMP_(void) CMD4Hasher::Final(Byte *digest) throw()
{
  MD4_Final(digest, &_ctx);
}
REGISTER_HASHER(CMD4Hasher, 0x206, "MD4", MD4_DIGEST_LENGTH)

// MD5
class CMD5Hasher:
  public IHasher,
  public CMyUnknownImp
{
  MD5_CTX _ctx;
  Byte mtDummy[1 << 7];

public:
  CMD5Hasher() { MD5_Init(&_ctx); }

  MY_UNKNOWN_IMP1(IHasher)
  INTERFACE_IHasher(;)
};

STDMETHODIMP_(void) CMD5Hasher::Init() throw()
{
  MD5_Init(&_ctx);
}

STDMETHODIMP_(void) CMD5Hasher::Update(const void *data, UInt32 size) throw()
{
  MD5_Update(&_ctx, (const Byte *)data, size);
}

STDMETHODIMP_(void) CMD5Hasher::Final(Byte *digest) throw()
{
  MD5_Final(digest, &_ctx);
}
REGISTER_HASHER(CMD5Hasher, 0x207, "MD5", MD5_DIGEST_LENGTH)

// SHA384
class CSHA384Hasher:
  public IHasher,
  public CMyUnknownImp
{
  SHA384_CTX _ctx;
  Byte mtDummy[1 << 7];

public:
  CSHA384Hasher() { SHA384_Init(&_ctx); }

  MY_UNKNOWN_IMP1(IHasher)
  INTERFACE_IHasher(;)
};

STDMETHODIMP_(void) CSHA384Hasher::Init() throw()
{
  SHA384_Init(&_ctx);
}

STDMETHODIMP_(void) CSHA384Hasher::Update(const void *data, UInt32 size) throw()
{
  SHA384_Update(&_ctx, (const Byte *)data, size);
}

STDMETHODIMP_(void) CSHA384Hasher::Final(Byte *digest) throw()
{
  SHA384_Final(digest, &_ctx);
}
REGISTER_HASHER(CSHA384Hasher, 0x208, "SHA384", SHA384_DIGEST_LENGTH)

// SHA512
class CSHA512Hasher:
  public IHasher,
  public CMyUnknownImp
{
  SHA512_CTX _ctx;
  Byte mtDummy[1 << 7];

public:
  CSHA512Hasher() { SHA512_Init(&_ctx); }

  MY_UNKNOWN_IMP1(IHasher)
  INTERFACE_IHasher(;)
};

STDMETHODIMP_(void) CSHA512Hasher::Init() throw()
{
  SHA512_Init(&_ctx);
}

STDMETHODIMP_(void) CSHA512Hasher::Update(const void *data, UInt32 size) throw()
{
  SHA512_Update(&_ctx, (const Byte *)data, size);
}

STDMETHODIMP_(void) CSHA512Hasher::Final(Byte *digest) throw()
{
  SHA512_Final(digest, &_ctx);
}
REGISTER_HASHER(CSHA512Hasher, 0x209, "SHA512", SHA512_DIGEST_LENGTH)
