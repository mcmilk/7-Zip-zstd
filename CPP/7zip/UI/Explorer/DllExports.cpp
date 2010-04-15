// DLLExports.cpp
//
// Notes:
// Win2000:
// If I register at HKCR\Folder\ShellEx then DLL is locked.
// otherwise it unloads after explorer closing.
// but if I call menu for desktop items it's locked all the time

#include "StdAfx.h"

#include "Common/MyInitGuid.h"

#include <ShlGuid.h>
#include <OleCtl.h>

#include "Common/ComTry.h"
#include "Common/StringConvert.h"

#include "Windows/DLL.h"
#include "Windows/Error.h"
#include "Windows/NtCheck.h"
#include "Windows/Registry.h"

#include "../FileManager/IFolder.h"
#include "../FileManager/LangUtils.h"

#include "ContextMenu.h"

// {23170F69-40C1-278A-1000-000100020000}
DEFINE_GUID(CLSID_CZipContextMenu,
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00);

using namespace NWindows;

HINSTANCE g_hInstance = 0;
HWND g_HWND = 0;

UString HResultToMessage(HRESULT errorCode)
{
  UString message;
  if (!NError::MyFormatMessage(errorCode, message))
    message.Empty();
  if (message.IsEmpty())
    message = L"Error";
  return message;
}

LONG g_DllRefCount = 0; // Reference count of this DLL.

static LPCWSTR kShellExtName = L"7-Zip Shell Extension";
static LPCTSTR kClsidMask = TEXT("CLSID\\%s");
static LPCTSTR kClsidInprocMask = TEXT("CLSID\\%s\\InprocServer32");
static LPCTSTR kApprovedKeyPath = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved");

// #define ODS(sz) OutputDebugString(L#sz)

class CShellExtClassFactory:
  public IClassFactory,
  public CMyUnknownImp
{
public:
  CShellExtClassFactory() { InterlockedIncrement(&g_DllRefCount); }
  ~CShellExtClassFactory() { InterlockedDecrement(&g_DllRefCount); }

  MY_UNKNOWN_IMP1_MT(IClassFactory)
  
  STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, void**);
  STDMETHODIMP LockServer(BOOL);
};

STDMETHODIMP CShellExtClassFactory::CreateInstance(LPUNKNOWN pUnkOuter,
    REFIID riid, void **ppvObj)
{
  // ODS("CShellExtClassFactory::CreateInstance()\r\n");
  *ppvObj = NULL;
  if (pUnkOuter)
    return CLASS_E_NOAGGREGATION;
  
  CZipContextMenu *shellExt;
  try
  {
    shellExt = new CZipContextMenu();
  }
  catch(...) { return E_OUTOFMEMORY; }
  if (shellExt == NULL)
    return E_OUTOFMEMORY;
  
  HRESULT res = shellExt->QueryInterface(riid, ppvObj);
  if (res != S_OK)
    delete shellExt;
  return res;
}


STDMETHODIMP CShellExtClassFactory::LockServer(BOOL /* fLock */)
{
  return S_OK; // Check it
}


#define NT_CHECK_FAIL_ACTION return FALSE;

extern "C"
BOOL WINAPI DllMain(
  #ifdef UNDER_CE
  HANDLE hInstance
  #else
  HINSTANCE hInstance
  #endif
  , DWORD dwReason, LPVOID)
{
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    g_hInstance = (HINSTANCE)hInstance;
    // ODS("In DLLMain, DLL_PROCESS_ATTACH\r\n");
    NT_CHECK
  }
  else if (dwReason == DLL_PROCESS_DETACH)
  {
    // ODS("In DLLMain, DLL_PROCESS_DETACH\r\n");
  }
  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
  // ODS("In DLLCanUnloadNow\r\n");
  return (g_DllRefCount == 0 ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
  // ODS("In DllGetClassObject\r\n");
  *ppv = NULL;
  if (IsEqualIID(rclsid, CLSID_CZipContextMenu))
  {
    CShellExtClassFactory *cf;
    try
    {
      cf = new CShellExtClassFactory;
    }
    catch(...) { return E_OUTOFMEMORY; }
    if (cf == 0)
      return E_OUTOFMEMORY;
    HRESULT res = cf->QueryInterface(riid, ppv);
    if (res != S_OK)
      delete cf;
    return res;
  }
  return CLASS_E_CLASSNOTAVAILABLE;
  // return _Module.GetClassObject(rclsid, riid, ppv);
}

static BOOL GetStringFromIID(CLSID clsid, LPTSTR s, int size)
{
  LPWSTR pwsz;
  if (StringFromIID(clsid, &pwsz) != S_OK)
    return FALSE;
  if (!pwsz)
    return FALSE;
  #ifdef UNICODE
  for (int i = 0; i < size; i++)
  {
    s[i] = pwsz[i];
    if (pwsz[i] == 0)
      break;
  }
  s[size - 1] = 0;
  #else
  WideCharToMultiByte(CP_ACP, 0, pwsz, -1, s, size, NULL, NULL);
  #endif
  CoTaskMemFree(pwsz);
  s[size - 1] = 0;
  return TRUE;
}

typedef struct
{
  HKEY hRootKey;
  LPCTSTR SubKey;
  LPCWSTR ValueName;
  LPCWSTR Data;
} CRegItem;

static BOOL RegisterServer(CLSID clsid, LPCWSTR title)
{
  TCHAR clsidString[MAX_PATH];
  if (!GetStringFromIID(clsid, clsidString, MAX_PATH))
    return FALSE;
  
  UString modulePath;
  if (!NDLL::MyGetModuleFileName(g_hInstance, modulePath))
    return FALSE;
  
  CRegItem clsidEntries[] =
  {
    HKEY_CLASSES_ROOT, kClsidMask,        NULL,              title,
    HKEY_CLASSES_ROOT, kClsidInprocMask,  NULL,              modulePath,
    HKEY_CLASSES_ROOT, kClsidInprocMask,  L"ThreadingModel", L"Apartment",
    NULL,              NULL,              NULL,              NULL
  };
  
  //register the CLSID entries
  for (int i = 0; clsidEntries[i].hRootKey; i++)
  {
    TCHAR subKey[MAX_PATH];
    const CRegItem &r = clsidEntries[i];
    wsprintf(subKey, r.SubKey, clsidString);
    NRegistry::CKey key;
    if (key.Create(r.hRootKey, subKey, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE) != NOERROR)
      return FALSE;
    key.SetValue(clsidEntries[i].ValueName, clsidEntries[i].Data);
  }
 
  #if !defined(_WIN64) && !defined(UNDER_CE)
  if (IsItWindowsNT())
  #endif
  {
    NRegistry::CKey key;
    if (key.Create(HKEY_LOCAL_MACHINE, kApprovedKeyPath, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE) == NOERROR)
      key.SetValue(GetUnicodeString(clsidString), title);
  }
  return TRUE;
}

STDAPI DllRegisterServer(void)
{
  return RegisterServer(CLSID_CZipContextMenu, kShellExtName) ?  S_OK: SELFREG_E_CLASS;
}

static BOOL UnregisterServer(CLSID clsid)
{
  TCHAR clsidString[MAX_PATH];
  if (!GetStringFromIID(clsid, clsidString, MAX_PATH))
    return FALSE;

  TCHAR subKey[MAX_PATH];
  wsprintf(subKey, kClsidInprocMask, clsidString);
  RegDeleteKey(HKEY_CLASSES_ROOT, subKey);
  
  wsprintf (subKey, kClsidMask, clsidString);
  RegDeleteKey(HKEY_CLASSES_ROOT, subKey);

  #if !defined(_WIN64) && !defined(UNDER_CE)
  if (IsItWindowsNT())
  #endif
  {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, kApprovedKeyPath, 0, KEY_SET_VALUE, &hKey) == NOERROR)
    {
      RegDeleteValue(hKey, clsidString);
      RegCloseKey(hKey);
    }
  }
  return TRUE;
}

STDAPI DllUnregisterServer(void)
{
  return UnregisterServer(CLSID_CZipContextMenu) ? S_OK: SELFREG_E_CLASS;
}
