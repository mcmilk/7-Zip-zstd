// DLLExports.cpp : Implementation of DLL Exports.

#include "StdAfx.h"

// #include <locale.h>

#include <initguid.h>
#include <ShlGuid.h>
#include <windows.h>

// #include "../../Compress/Interface/CompressInterface.h"
#include "../../IPassword.h"
#include "../Agent/Agent.h"
#include "Common/ComTry.h"

#include "ContextMenu.h"

#include "OptionsDialog.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	// OBJECT_ENTRY(CLSID_CAgentArchiveHandler, CAgent)
  OBJECT_ENTRY(CLSID_CZipContextMenu, CZipContextMenu)
  // OBJECT_ENTRY(CLSID_CSevenZipOptions, CSevenZipOptions)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

HINSTANCE g_hInstance;

static bool IsItWindowsNT()
{
  OSVERSIONINFO aVersionInfo;
  aVersionInfo.dwOSVersionInfoSize = sizeof(aVersionInfo);
  if (!::GetVersionEx(&aVersionInfo)) 
    return false;
  return (aVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID)
{
  // setlocale(LC_COLLATE, ".ACP");
  g_hInstance = hInstance;
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    #ifdef UNICODE
    if (!IsItWindowsNT())
      return FALSE;
    #endif    
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

STDAPI CreateObject(
    const GUID *classID, 
    const GUID *interfaceID, 
    void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  if (*classID == CLSID_CAgentArchiveHandler)
  {
    if (*interfaceID == IID_IFolderManager)
    {
      CMyComPtr<IFolderManager> manager = new CArchiveFolderManager;
      *outObject = manager.Detach();
      return S_OK;
    }
    return E_NOINTERFACE;
  }
  if (*classID == CLSID_CSevenZipOptions)
  {
    if (*interfaceID == IID_IPluginOptions)
    {
      CMyComPtr<IPluginOptions> options = new CSevenZipOptions;
      *outObject = options.Detach();
      return S_OK;
    }
    return E_NOINTERFACE;
  }
  return CLASS_E_CLASSNOTAVAILABLE;
  COM_TRY_END
}

STDAPI GetPluginProperty(PROPID propID, PROPVARIANT *value)
{
  ::VariantClear((tagVARIANT *)value);
  switch(propID)
  {
    case NPlugin::kName:
      if ((value->bstrVal = ::SysAllocString(L"7-Zip")) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    case NPlugin::kClassID:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CAgentArchiveHandler, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NPlugin::kOptionsClassID:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CSevenZipOptions, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    /*
    case NArchive::kType:
      propVariant = UINT32(0);
      break;
    */
  }
  return S_OK;
}
