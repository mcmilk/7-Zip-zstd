// DllExports.cpp

#include "StdAfx.h"

#include "../../Common/MyInitGuid.h"

#include "../ICoder.h"

#include "../Common/RegisterCodec.h"

extern "C"
BOOL WINAPI DllMain(HINSTANCE /* hInstance */, DWORD /* dwReason */, LPVOID /*lpReserved*/)
{
  return TRUE;
}

static const UInt16 kDecodeId = 0x2790;

DEFINE_GUID(CLSID_CCodec, 
0x23170F69, 0x40C1, kDecodeId, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

STDAPI CreateCoder(const GUID *clsid, const GUID *iid, void **outObject);

STDAPI CreateObject(const GUID *clsid, const GUID *iid, void **outObject)
{
  return CreateCoder(clsid, iid, outObject);
}

