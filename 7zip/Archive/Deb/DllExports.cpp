// DLLExports.cpp

#include "StdAfx.h"

#define INITGUID

#include "Common/ComTry.h"
#include "Windows/PropVariant.h"

#include "DebHandler.h"
#include "../../ICoder.h"

// {23170F69-40C1-278A-1000-0001100C0000}
DEFINE_GUID(CLSID_CDebHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x0C, 0x00, 0x00);

// HINSTANCE g_hInstance;

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	/*
  if (dwReason == DLL_PROCESS_ATTACH)
    g_hInstance = hInstance;
  */
	return TRUE;
}

STDAPI CreateObject(
    const GUID *classID, 
    const GUID *interfaceID, 
    void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  if (*classID != CLSID_CDebHandler)
    return CLASS_E_CLASSNOTAVAILABLE;
  if (*interfaceID != IID_IInArchive)
    return E_NOINTERFACE;
  CMyComPtr<IInArchive> inArchive = (IInArchive *)new NArchive::NDeb::CHandler;
  *outObject = inArchive.Detach();
  COM_TRY_END
  return S_OK;
}

STDAPI GetHandlerProperty(PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant propVariant;
  switch(propID)
  {
    case NArchive::kName:
      propVariant = L"Deb";
      break;
    case NArchive::kClassID:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CDebHandler, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NArchive::kExtension:
      propVariant = L"deb";
      break;
    case NArchive::kUpdate:
      propVariant = false;
      break;
    case NArchive::kKeepName:
      propVariant = false;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
}
