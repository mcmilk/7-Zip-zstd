// DLLExports.cpp

#include "StdAfx.h"

// #include <initguid.h>
#define INITGUID

#include "LZMAEncoder.h"
#include "LZMADecoder.h"
#include "../../../Common/ComTry.h"

// {23170F69-40C1-278B-0301-010000000000}
DEFINE_GUID(CLSID_CLZMADecoder, 
0x23170F69, 0x40C1, 0x278B, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-0301-010000000100}
DEFINE_GUID(CLSID_CLZMAEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00);

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	return TRUE;
}

STDAPI CreateObject(const GUID *clsid, const GUID *iid, void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  int correctInterface = (*iid == IID_ICompressCoder);
  CMyComPtr<ICompressCoder> coder;
  if (*clsid == CLSID_CLZMADecoder)
  {
    if (!correctInterface)
      return E_NOINTERFACE;
    coder = (ICompressCoder *)new NCompress::NLZMA::CDecoder();
  }
  else if (*clsid == CLSID_CLZMAEncoder)
  {
    if (!correctInterface)
      return E_NOINTERFACE;
    coder = (ICompressCoder *)new NCompress::NLZMA::CEncoder();
  }
  else
    return CLASS_E_CLASSNOTAVAILABLE;
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
  // ::VariantClear((tagVARIANT *)value);
  switch(propID)
  {
    case NMethodPropID::kID:
    {
      const char id[] = { 0x03, 0x01, 0x01 };
      if ((value->bstrVal = ::SysAllocStringByteLen(id, sizeof(id))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NMethodPropID::kName:
      if ((value->bstrVal = ::SysAllocString(L"LZMA")) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    case NMethodPropID::kDecoder:
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CLZMADecoder, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    case NMethodPropID::kEncoder:
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CLZMAEncoder, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
  }
  return S_OK;
}

