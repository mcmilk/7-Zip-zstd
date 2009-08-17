// DLLExports.cpp

#include "StdAfx.h"

#include "../../Common/MyInitGuid.h"
#include "../../Common/ComTry.h"
#include "../../Common/Types.h"

#include "../../Windows/NtCheck.h"
#include "../../Windows/PropVariant.h"

#include "IArchive.h"
#include "../ICoder.h"
#include "../IPassword.h"

HINSTANCE g_hInstance;

#define NT_CHECK_FAIL_ACTION return FALSE;

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    g_hInstance = hInstance;
    NT_CHECK;
  }
  return TRUE;
}

DEFINE_GUID(CLSID_CArchiveHandler,
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00);

STDAPI CreateArchiver(const GUID *classID, const GUID *iid, void **outObject);

STDAPI CreateObject(const GUID *clsid, const GUID *iid, void **outObject)
{
  return CreateArchiver(clsid, iid, outObject);
}

STDAPI SetLargePageMode()
{
  #if defined(_WIN32) && defined(_7ZIP_LARGE_PAGES)
  SetLargePageSize();
  #endif
  return S_OK;
}
