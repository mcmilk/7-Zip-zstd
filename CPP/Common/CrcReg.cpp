// CrcReg.cpp

#include "StdAfx.h"

#include "../../C/7zCrc.h"
#include "../../C/CpuArch.h"

#include "../Common/MyCom.h"

#include "../7zip/ICoder.h"
#include "../7zip/Common/RegisterCodec.h"

EXTERN_C_BEGIN

typedef UInt32 (MY_FAST_CALL *CRC_FUNC)(UInt32 v, const void *data, size_t size, const UInt32 *table);

extern CRC_FUNC g_CrcUpdate;

#ifdef MY_CPU_X86_OR_AMD64
  UInt32 MY_FAST_CALL CrcUpdateT8(UInt32 v, const void *data, size_t size, const UInt32 *table);
#endif

#ifndef MY_CPU_BE
  UInt32 MY_FAST_CALL CrcUpdateT4(UInt32 v, const void *data, size_t size, const UInt32 *table);
#endif

EXTERN_C_END

class CCrcHasher:
  public IHasher,
  public ICompressSetCoderProperties,
  public CMyUnknownImp
{
  UInt32 _crc;
  CRC_FUNC _updateFunc;
  bool SetFunctions(UInt32 tSize);
public:
  CCrcHasher(): _crc(CRC_INIT_VAL) { SetFunctions(0); }

  MY_UNKNOWN_IMP1(ICompressSetCoderProperties)

  STDMETHOD_(void, Init)();
  STDMETHOD_(void, Update)(const void *data, UInt32 size);
  STDMETHOD_(void, Final)(Byte *digest);
  STDMETHOD_(UInt32, GetDigestSize)();
  STDMETHOD(SetCoderProperties)(const PROPID *propIDs, const PROPVARIANT *props, UInt32 numProps);
};

STDMETHODIMP_(void) CCrcHasher::Init()
{
  _crc = CRC_INIT_VAL;
}

STDMETHODIMP_(void) CCrcHasher::Update(const void *data, UInt32 size)
{
  _crc = _updateFunc(_crc, data, size, g_CrcTable);
}

STDMETHODIMP_(void) CCrcHasher::Final(Byte *digest)
{
  UInt32 val = CRC_GET_DIGEST(_crc);
  SetUi32(digest, val);
}

STDMETHODIMP_(UInt32) CCrcHasher::GetDigestSize()
{
  return 4;
}

bool CCrcHasher::SetFunctions(UInt32 tSize)
{
  _updateFunc = g_CrcUpdate;
  if (tSize == 4)
  {
    #ifndef MY_CPU_BE
    _updateFunc = CrcUpdateT4;
    #endif
  }
  else if (tSize == 8)
  {
    #ifdef MY_CPU_X86_OR_AMD64
      _updateFunc = CrcUpdateT8;
    #else
      return false;
    #endif
  }
  return true;
}

STDMETHODIMP CCrcHasher::SetCoderProperties(const PROPID *propIDs,
    const PROPVARIANT *coderProps, UInt32 numProps)
{
  for (UInt32 i = 0; i < numProps; i++)
  {
    const PROPVARIANT &prop = coderProps[i];
    if (propIDs[i] == NCoderPropID::kDefaultProp)
    {
      if (prop.vt != VT_UI4)
        return E_INVALIDARG;
      if (!SetFunctions(prop.ulVal))
        return E_NOTIMPL;
    }
  }
  return S_OK;
}

static IHasher *CreateHasher() { return new CCrcHasher(); }

static CHasherInfo g_HasherInfo = { CreateHasher, 0x1, L"CRC32", 4 };

REGISTER_HASHER(Crc32)
