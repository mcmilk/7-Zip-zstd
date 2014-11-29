// Sha256Reg.cpp

#include "StdAfx.h"

#include "../../C/Sha256.h"

#include "../Common/MyCom.h"

#include "../7zip/ICoder.h"
#include "../7zip/Common/RegisterCodec.h"

class CSha256Hasher:
  public IHasher,
  public CMyUnknownImp
{
  CSha256 _sha;
public:
  CSha256Hasher() { Init(); };

  MY_UNKNOWN_IMP

  STDMETHOD_(void, Init)();
  STDMETHOD_(void, Update)(const void *data, UInt32 size);
  STDMETHOD_(void, Final)(Byte *digest);
  STDMETHOD_(UInt32, GetDigestSize)();
};

STDMETHODIMP_(void) CSha256Hasher::Init()
{
  Sha256_Init(&_sha);
}

STDMETHODIMP_(void) CSha256Hasher::Update(const void *data, UInt32 size)
{
  Sha256_Update(&_sha, (const Byte *)data, size);
}

STDMETHODIMP_(void) CSha256Hasher::Final(Byte *digest)
{
  Sha256_Final(&_sha, digest);
}

STDMETHODIMP_(UInt32) CSha256Hasher::GetDigestSize()
{
  return SHA256_DIGEST_SIZE;
}

static IHasher *CreateHasher() { return new CSha256Hasher; }

static CHasherInfo g_HasherInfo = { CreateHasher, 0xA, L"SHA256", SHA256_DIGEST_SIZE };

REGISTER_HASHER(Sha256)
