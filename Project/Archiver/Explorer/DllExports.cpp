// DLLExports.cpp : Implementation of DLL Exports.

#include "StdAfx.h"

#include <locale.h>

#include <initguid.h>
#include <ShlGuid.h>
#include <windows.h>

#include "../../Compress/Interface./CompressInterface.h"
#include "../Format/Common/FormatCryptoInterface.h"
#include "../Agent/Handler.h"

#include "ZipFolder.h"
#include "ContextMenu.h"
#include "ZipViewObject.h"

/*
// {23170F69-40C1-278A-1000-000100030000}
DEFINE_GUID(CLSID_CAgentArchiveHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00);
*/

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
  OBJECT_ENTRY(CLSID_CZipFolder, CZipFolder)
  OBJECT_ENTRY(CLSID_CZipContextMenu, CZipContextMenu)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

HINSTANCE g_hInstance;

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID)
{
  setlocale(LC_COLLATE, ".ACP");
  g_hInstance = hInstance;
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    _Module.Init(ObjectMap, hInstance);
    //DisableThreadLibraryCalls(hInstance);
  }
  else if (dwReason == DLL_PROCESS_DETACH)
    _Module.Term();
  return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
  return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
  return _Module.GetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer(void)
{
  return _Module.RegisterServer(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
  return _Module.UnregisterServer();
}
