// DLLExports.cpp : Implementation of DLL Exports.

#include "StdAfx.h"

#include <initguid.h>

#include "Interface/ICoder.h"
// #include "HC2.h"
#include "HC3.h"
#include "HC4.h"
// #include "HC4b.h"

CComModule _Module;

#define MyOBJECT_ENTRY(Name) \
  OBJECT_ENTRY(N ## Name :: CLSID_CMatchFinder ## Name, N ## Name ::CMatchFinderHC)

BEGIN_OBJECT_MAP(ObjectMap)
  // MyOBJECT_ENTRY(HC2)
  MyOBJECT_ENTRY(HC3)
  MyOBJECT_ENTRY(HC4)
  // MyOBJECT_ENTRY(HC4b)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
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

STDAPI DllUnregisterServer(void)
{
  return _Module.UnregisterServer();
}
