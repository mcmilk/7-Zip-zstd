// DLLExports.cpp : Implementation of DLL Exports.

#include "StdAfx.h"

#include <initguid.h>

#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "SplitHandler.h"
#include "../../ICoder.h"

// {23170F69-40C1-278A-1000-0001100B0000}
DEFINE_GUID(CLSID_CSplitHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x0B, 0x00, 0x00);

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	return TRUE;
}

STDAPI CreateObject(
    const GUID *classID, 
    const GUID *interfaceID, 
    void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  if (*classID != CLSID_CSplitHandler)
    return CLASS_E_CLASSNOTAVAILABLE;
  if (*interfaceID != IID_IInArchive)
    return E_NOINTERFACE;
  CMyComPtr<IInArchive> inArchive = (IInArchive *)new NArchive::NSplit::CHandler;
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
      propVariant = L"Split";
      break;
    case NArchive::kClassID:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CSplitHandler, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NArchive::kExtension:
      propVariant = L"001";
      break;
    case NArchive::kUpdate:
      propVariant = false;
      break;
    case NArchive::kKeepName:
      propVariant = true;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
}
