// DLLExports.cpp

#include "StdAfx.h"

#define INITGUID

#include "Common/ComTry.h"
#include "ImplodeDecoder.h"

// {23170F69-40C1-278B-0401-060000000000}
DEFINE_GUID(CLSID_CCompressImplodeDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00);

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	return TRUE;
}

STDAPI CreateObject(const GUID *clsid, const GUID *iid, void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  if (*clsid != CLSID_CCompressImplodeDecoder)
    return CLASS_E_CLASSNOTAVAILABLE;
  if (*iid != IID_ICompressCoder)
    return E_NOINTERFACE;
  CMyComPtr<ICompressCoder> coder = (ICompressCoder *)new 
      NCompress::NImplode::NDecoder::CCoder;
  *outObject = coder.Detach();
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
      const char id[] = { 0x04, 0x01, 0x06 };
      if ((value->bstrVal = ::SysAllocStringByteLen(id, sizeof(id))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NMethodPropID::kName:
      if ((value->bstrVal = ::SysAllocString(L"Implode")) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    case NMethodPropID::kDecoder:
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CCompressImplodeDecoder, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
  }
  return S_OK;
}
