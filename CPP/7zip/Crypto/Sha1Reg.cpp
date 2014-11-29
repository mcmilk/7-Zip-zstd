// Sha1Reg.cpp

#include "StdAfx.h"

#include "../../Common/MyCom.h"

#include "../ICoder.h"
#include "../Common/RegisterCodec.h"

#include "Sha1.h"

using namespace NCrypto;
using namespace NSha1;

class CSha1Hasher:
  public IHasher,
  public CMyUnknownImp
{
  CContext _sha;
public:
  CSha1Hasher() { Init(); }

  MY_UNKNOWN_IMP

  STDMETHOD_(void, Init)();
  STDMETHOD_(void, Update)(const void *data, UInt32 size);
  STDMETHOD_(void, Final)(Byte *digest);
  STDMETHOD_(UInt32, GetDigestSize)();
};

STDMETHODIMP_(void) CSha1Hasher::Init()
{
  _sha.Init();
}

STDMETHODIMP_(void) CSha1Hasher::Update(const void *data, UInt32 size)
{
  _sha.Update((const Byte *)data, size);
}

STDMETHODIMP_(void) CSha1Hasher::Final(Byte *digest)
{
  _sha.Final(digest);
}

STDMETHODIMP_(UInt32) CSha1Hasher::GetDigestSize()
{
  return kDigestSize;
}

static IHasher *CreateHasher() { return new CSha1Hasher; }

static CHasherInfo g_HasherInfo = { CreateHasher, 0x201, L"SHA1", kDigestSize };

REGISTER_HASHER(Sha1)
