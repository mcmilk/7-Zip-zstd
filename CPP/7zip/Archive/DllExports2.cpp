// DLLExports2.cpp

#include "StdAfx.h"

#include "../../Common/MyInitGuid.h"

#if defined(_7ZIP_LARGE_PAGES)
#include "../../../C/Alloc.h"
#endif

#include "../../Common/ComTry.h"

#include "../../Windows/NtCheck.h"
#include "../../Windows/PropVariant.h"

#include "../ICoder.h"
#include "../IPassword.h"

#include "IArchive.h"

HINSTANCE g_hInstance;

#define NT_CHECK_FAIL_ACTION return FALSE;

extern "C"
BOOL WINAPI DllMain(
  #ifdef UNDER_CE
  HANDLE
  #else
  HINSTANCE
  #endif
  hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    g_hInstance = (HINSTANCE)hInstance;
    NT_CHECK;
  }
  return TRUE;
}

DEFINE_GUID(CLSID_CArchiveHandler,
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00);

static const UInt16 kDecodeId = 0x2790;

DEFINE_GUID(CLSID_CCodec,
0x23170F69, 0x40C1, kDecodeId, 0, 0, 0, 0, 0, 0, 0, 0);

STDAPI CreateCoder(const GUID *clsid, const GUID *iid, void **outObject);
STDAPI CreateHasher(const GUID *clsid, IHasher **hasher);
STDAPI CreateArchiver(const GUID *classID, const GUID *iid, void **outObject);

STDAPI CreateObject(const GUID *clsid, const GUID *iid, void **outObject)
{
  // COM_TRY_BEGIN
  *outObject = 0;
  if (*iid == IID_ICompressCoder ||
      *iid == IID_ICompressCoder2 ||
      *iid == IID_ICompressFilter)
    return CreateCoder(clsid, iid, outObject);
  if (*iid == IID_IHasher)
    return CreateHasher(clsid, (IHasher **)outObject);
  return CreateArchiver(clsid, iid, outObject);
  // COM_TRY_END
}

STDAPI SetLargePageMode()
{
  #if defined(_7ZIP_LARGE_PAGES)
  SetLargePageSize();
  #endif
  return S_OK;
}

extern bool g_CaseSensitive;

STDAPI SetCaseSensitive(Int32 caseSensitive)
{
  g_CaseSensitive = (caseSensitive != 0);
  return S_OK;
}
