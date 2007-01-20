// DLLExports.cpp

#include "StdAfx.h"

#include "Common/MyInitGuid.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "../../ICoder.h"
#include "LzhHandler.h"

// {23170F69-40C1-278A-1000-000110060000}
DEFINE_GUID(CLSID_CLzhHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x06, 0x00, 0x00);

extern "C"
BOOL WINAPI DllMain(HINSTANCE /* hInstance */, DWORD /* dwReason */, LPVOID /* lpReserved */)
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
  if (*classID != CLSID_CLzhHandler)
    return CLASS_E_CLASSNOTAVAILABLE;
  if (*interfaceID != IID_IInArchive)
    return E_NOINTERFACE;
  CMyComPtr<IInArchive> inArchive = (IInArchive *)new NArchive::NLzh::CHandler;
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
      propVariant = L"Lzh";
      break;
    case NArchive::kClassID:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CLzhHandler, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NArchive::kExtension:
      propVariant = L"lzh lha";
      break;
    case NArchive::kUpdate:
      propVariant = false;
      break;
    case NArchive::kKeepName:
      propVariant = false;
      break;
    case NArchive::kStartSignature:
    {
      const unsigned char sig[] = { '-', 'l' };
      if ((value->bstrVal = ::SysAllocStringByteLen((const char *)sig, 2)) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
  }
  propVariant.Detach(value);
  return S_OK;
}
