// DLLExports.cpp

#include "StdAfx.h"

#include "Common/MyInitGuid.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "CpioHandler.h"
#include "../../ICoder.h"

// {23170F69-40C1-278A-1000-000110080000}
DEFINE_GUID(CLSID_CCpioHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x08, 0x00, 0x00);

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
  if (*classID != CLSID_CCpioHandler)
    return CLASS_E_CLASSNOTAVAILABLE;
  if (*interfaceID != IID_IInArchive)
    return E_NOINTERFACE;
  CMyComPtr<IInArchive> inArchive = (IInArchive *)new NArchive::NCpio::CHandler;
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
      propVariant = L"Cpio";
      break;
    case NArchive::kClassID:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CCpioHandler, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NArchive::kExtension:
      propVariant = L"cpio";
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
