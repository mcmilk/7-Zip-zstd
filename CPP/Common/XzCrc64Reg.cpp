// XzCrc64Reg.cpp

#include "StdAfx.h"

#include "../../C/CpuArch.h"
#include "../../C/XzCrc64.h"

#include "../Common/MyCom.h"

#include "../7zip/ICoder.h"
#include "../7zip/Common/RegisterCodec.h"

class CXzCrc64Hasher:
  public IHasher,
  public CMyUnknownImp
{
  UInt64 _crc;
public:
  CXzCrc64Hasher(): _crc(CRC64_INIT_VAL) {}

  MY_UNKNOWN_IMP

  STDMETHOD_(void, Init)();
  STDMETHOD_(void, Update)(const void *data, UInt32 size);
  STDMETHOD_(void, Final)(Byte *digest);
  STDMETHOD_(UInt32, GetDigestSize)();
};

STDMETHODIMP_(void) CXzCrc64Hasher::Init()
{
  _crc = CRC64_INIT_VAL;
}

STDMETHODIMP_(void) CXzCrc64Hasher::Update(const void *data, UInt32 size)
{
  _crc = Crc64Update(_crc, data, size);
}

STDMETHODIMP_(void) CXzCrc64Hasher::Final(Byte *digest)
{
  UInt64 val = CRC64_GET_DIGEST(_crc);
  SetUi64(digest, val);
}

STDMETHODIMP_(UInt32) CXzCrc64Hasher::GetDigestSize()
{
  return 8;
}

static IHasher *CreateHasher() { return new CXzCrc64Hasher; }

static CHasherInfo g_HasherInfo = { CreateHasher, 0x4, L"CRC64", 8 };

REGISTER_HASHER(Crc64)
