// DLLExports.cpp

#include "StdAfx.h"

#define INITGUID

#include "Common/ComTry.h"
#include "ByteSwap.h"
#include "../../ICoder.h"

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
  if (*clsid == CLSID_CCompressConvertByteSwap2)
  {
    if (!correctInterface)
      return E_NOINTERFACE;
    coder = (ICompressCoder *)new CByteSwap2();
  }
  else if (*clsid == CLSID_CCompressConvertByteSwap4)
  {
    if (!correctInterface)
      return E_NOINTERFACE;
    coder = (ICompressCoder *)new CByteSwap4();
  }
  else
    return CLASS_E_CLASSNOTAVAILABLE;
  *outObject = coder.Detach();
  COM_TRY_END
  return S_OK;
}

struct CSwapMethodInfo
{
  char ID[3];
  const wchar_t *Name;
  const GUID *clsid;
};

static CSwapMethodInfo g_Methods[] =
{
  { { 0x2, 0x03, 0x02 }, L"Swap2", &CLSID_CCompressConvertByteSwap2 },
  { { 0x2, 0x03, 0x04 }, L"Swap4", &CLSID_CCompressConvertByteSwap4 }
};

STDAPI GetNumberOfMethods(UINT32 *numMethods)
{
  *numMethods = sizeof(g_Methods) / sizeof(g_Methods[1]);
  return S_OK;
}
  
STDAPI GetMethodProperty(UINT32 index, PROPID propID, PROPVARIANT *value)
{
  if (index > sizeof(g_Methods) / sizeof(g_Methods[1]))
    return E_INVALIDARG;
  ::VariantClear((tagVARIANT *)value);
  const CSwapMethodInfo &method = g_Methods[index];
  switch(propID)
  {
    case NMethodPropID::kID:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(method.ID, 
          sizeof(method.ID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NMethodPropID::kName:
    {
      if ((value->bstrVal = ::SysAllocString(method.Name)) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NMethodPropID::kDecoder:
    case NMethodPropID::kEncoder:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)method.clsid, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
  }
  return S_OK;
}
