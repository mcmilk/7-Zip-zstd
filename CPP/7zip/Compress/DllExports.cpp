// DllExports.cpp

#include "StdAfx.h"

#include "../../Common/MyInitGuid.h"

#include "../ICoder.h"

#include "../Common/RegisterCodec.h"

static const unsigned int kNumCodecsMax = 32;
unsigned int g_NumCodecs = 0;
const CCodecInfo *g_Codecs[kNumCodecsMax];
void RegisterCodec(const CCodecInfo *codecInfo)
{
  if (g_NumCodecs < kNumCodecsMax)
    g_Codecs[g_NumCodecs++] = codecInfo;
}

#ifdef _WIN32
extern "C"
BOOL WINAPI DllMain(
  #ifdef UNDER_CE
  HANDLE
  #else
  HINSTANCE
  #endif
  , DWORD /* dwReason */, LPVOID /*lpReserved*/)
{
  return TRUE;
}
#endif

static const UInt16 kDecodeId = 0x2790;

DEFINE_GUID(CLSID_CCodec,
0x23170F69, 0x40C1, kDecodeId, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

STDAPI CreateCoder(const GUID *clsid, const GUID *iid, void **outObject);

STDAPI CreateObject(const GUID *clsid, const GUID *iid, void **outObject)
{
  return CreateCoder(clsid, iid, outObject);
}

