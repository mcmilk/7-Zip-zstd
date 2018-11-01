//  Blake2sReg.cpp

#include "StdAfx.h"

#define XXH_STATIC_LINKING_ONLY
#include "../../C/CpuArch.h"
#include "../../C/Blake2.h"

#include "../Common/MyCom.h"

#include "../7zip/Common/RegisterCodec.h"


class CBlake2spHasher:
  public IHasher,
  public CMyUnknownImp
{
  CBlake2sp _blake;
  Byte mtDummy[1 << 7];

public:
  CBlake2spHasher() { Init(); }

  MY_UNKNOWN_IMP
  INTERFACE_IHasher(;)
};

STDMETHODIMP_(void) CBlake2spHasher::Init() throw()
{
  Blake2sp_Init(&_blake);
}

STDMETHODIMP_(void) CBlake2spHasher::Update(const void *data, UInt32 size) throw()
{
  Blake2sp_Update(&_blake, (const Byte *)data, size);
}

STDMETHODIMP_(void) CBlake2spHasher::Final(Byte *digest) throw()
{
  Blake2sp_Final(&_blake, digest);
}

REGISTER_HASHER(CBlake2spHasher, 0x202, "BLAKE2sp", BLAKE2S_DIGEST_SIZE)
