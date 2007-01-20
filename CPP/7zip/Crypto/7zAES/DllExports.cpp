// DLLExports.cpp

#include "StdAfx.h"

#include "Common/MyInitGuid.h"
#include "Common/ComTry.h"
#include "7zAES.h"

/*
// {23170F69-40C1-278B-0703-000000000000}
DEFINE_GUID(CLSID_CCrypto_Hash_SHA256, 
0x23170F69, 0x40C1, 0x278B, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
*/

// {23170F69-40C1-278B-06F1-070100000100}
DEFINE_GUID(CLSID_CCrypto7zAESEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x07, 0x01, 0x00, 0x00, 0x01, 0x00);

// {23170F69-40C1-278B-06F1-070100000000}
DEFINE_GUID(CLSID_CCrypto7zAESDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00);

HINSTANCE g_hInstance;
#ifndef _UNICODE
bool g_IsNT = false;
static bool IsItWindowsNT()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo)) 
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}
#endif

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    g_hInstance = hInstance;
    #ifndef _UNICODE
    g_IsNT = IsItWindowsNT();
    #endif
  }
  return TRUE;
}

STDAPI CreateObject(const GUID *clsid, const GUID *iid, void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  int correctInterface = (*iid == IID_ICompressFilter);
  CMyComPtr<ICompressFilter> filter;
  if (*clsid == CLSID_CCrypto7zAESDecoder)
  {
    if (!correctInterface)
      return E_NOINTERFACE;
    filter = (ICompressFilter *)new NCrypto::NSevenZ::CDecoder();
  }
  else if (*clsid == CLSID_CCrypto7zAESEncoder)
  {
    if (!correctInterface)
      return E_NOINTERFACE;
    filter = (ICompressFilter *)new NCrypto::NSevenZ::CEncoder();
  }
  else
    return CLASS_E_CLASSNOTAVAILABLE;
  *outObject = filter.Detach();
  COM_TRY_END
  return S_OK;
}

STDAPI GetNumberOfMethods(UINT32 *numMethods)
{
  *numMethods = 1;
  return S_OK;
}

STDAPI GetMethodProperty(UINT32 index, PROPID propID, PROPVARIANT *value)
{
  if (index != 0)
    return E_INVALIDARG;
  ::VariantClear((tagVARIANT *)value);
  switch(propID)
  {
    case NMethodPropID::kID:
    {
      const char id[] = { 0x06, (char)(unsigned char)0xF1, 0x07, 0x01 };
      if ((value->bstrVal = ::SysAllocStringByteLen(id, sizeof(id))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NMethodPropID::kName:
      if ((value->bstrVal = ::SysAllocString(L"7zAES")) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    case NMethodPropID::kDecoder:
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CCrypto7zAESDecoder, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    case NMethodPropID::kEncoder:
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CCrypto7zAESEncoder, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
  }
  return S_OK;
}

