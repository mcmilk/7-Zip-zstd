// DLLExports.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "../../ICoder.h"
#include "TarHandler.h"

// {23170F69-40C1-278A-1000-000110040000}
DEFINE_GUID(CLSID_CTarHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x04, 0x00, 0x00);

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
  if (*classID != CLSID_CTarHandler)
    return CLASS_E_CLASSNOTAVAILABLE;
  int needIn = *interfaceID == IID_IInArchive;
  int needOut = *interfaceID == IID_IOutArchive;
  if (needIn || needOut)
  {
    NArchive::NTar::CHandler *temp = new NArchive::NTar::CHandler;
    if (needIn)
    {
      CMyComPtr<IInArchive> inArchive = (IInArchive *)temp;
      *outObject = inArchive.Detach();
    }
    else
    {
      CMyComPtr<IOutArchive> outArchive = (IOutArchive *)temp;
      *outObject = outArchive.Detach();
    }
  }
  else
    return E_NOINTERFACE;
  COM_TRY_END
  return S_OK;
}

STDAPI GetHandlerProperty(PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant propVariant;
  switch(propID)
  {
    case NArchive::kName:
      propVariant = L"Tar";
      break;
    case NArchive::kClassID:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CTarHandler, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NArchive::kExtension:
      propVariant = L"tar";
      break;
    case NArchive::kUpdate:
      propVariant = true;
      break;
    case NArchive::kKeepName:
      propVariant = false;
      break;
  }
  propVariant.Detach(value);
  return S_OK;
}
