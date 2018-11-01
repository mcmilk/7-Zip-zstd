// MD5Reg.cpp

#include "StdAfx.h"

#include "../../C/md5.h"

#include "../Common/MyCom.h"

#include "../7zip/Common/RegisterCodec.h"

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
  MD5_Final(&_ctx, digest);
}

REGISTER_HASHER(CMD5Hasher, 0x205, "MD5", 16)
