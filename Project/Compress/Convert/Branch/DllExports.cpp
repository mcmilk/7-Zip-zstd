// DLLExports.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Coder.h"
#include "x86.h"
#include "PPC.h"
#include "Alpha.h"
#include "IA64.h"
#include "ARM.h"
#include "M68.h"
#include "Interface/ICoder.h"
#include "x86_2.h"

CComModule _Module;

#define MyOBJECT_ENTRY(Name) \
  OBJECT_ENTRY(CLSID_CCompressConvert ## Name ## _Encoder, C ## Name ## _Encoder) \
  OBJECT_ENTRY(CLSID_CCompressConvert ## Name ## _Decoder, C ## Name ## _Decoder) \


BEGIN_OBJECT_MAP(ObjectMap)
  MyOBJECT_ENTRY(BCJ_x86)
  MyOBJECT_ENTRY(BCJ2_x86)
  MyOBJECT_ENTRY(BC_PPC_B)
  MyOBJECT_ENTRY(BC_Alpha)
  MyOBJECT_ENTRY(BC_IA64)
  MyOBJECT_ENTRY(BC_ARM)
  MyOBJECT_ENTRY(BC_M68_B)
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
