// DLLExports.cpp

#include "StdAfx.h"

#include "Common/MyInitGuid.h"
#include "Common/ComTry.h"
#include "MyAES.h"

extern "C"
BOOL WINAPI DllMain(HINSTANCE /* hInstance */, DWORD /* dwReason */, LPVOID /* lpReserved */)
{
  return TRUE;
}

#define MY_CreateClass(n) \
if (*clsid == CLSID_CCrypto_ ## n ## _Encoder) { \
    if (!correctInterface) return E_NOINTERFACE; \
    filter = (ICompressFilter *)new C ## n ## _Encoder(); \
  } else if (*clsid == CLSID_CCrypto_ ## n ## _Decoder){ \
    if (!correctInterface) return E_NOINTERFACE; \
    filter = (ICompressFilter *)new C ## n ## _Decoder(); \
  }

STDAPI CreateObject(
    const GUID *clsid, 
    const GUID *interfaceID, 
    void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  int correctInterface = (*interfaceID == IID_ICompressFilter);
  CMyComPtr<ICompressFilter> filter;

  MY_CreateClass(AES_CBC)
  else
  MY_CreateClass(AES_ECB)
  else
    return CLASS_E_CLASSNOTAVAILABLE;
  *outObject = filter.Detach();
  return S_OK;
  COM_TRY_END
}

struct CAESMethodItem
{
  char ID[3];
  const wchar_t *UserName;
  const GUID *Decoder;
  const GUID *Encoder;
};

#define METHOD_ITEM(Name, id, UserName) \
  { { 0x06, 0x01, id }, UserName, \
  &CLSID_CCrypto_ ## Name ## _Decoder, \
  &CLSID_CCrypto_ ## Name ## _Encoder }


static CAESMethodItem g_Methods[] =
{
  METHOD_ITEM(AES_ECB, (char)(unsigned char)0xC0, L"AES-ECB"),
  METHOD_ITEM(AES_CBC, (char)(unsigned char)0xC1, L"AES")
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
  VariantClear((tagVARIANT *)value);
  const CAESMethodItem &method = g_Methods[index];
  switch(propID)
  {
    case NMethodPropID::kID:
      if ((value->bstrVal = ::SysAllocStringByteLen(method.ID, 
          sizeof(method.ID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    case NMethodPropID::kName:
      if ((value->bstrVal = ::SysAllocString(method.UserName)) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    case NMethodPropID::kDecoder:
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)method.Decoder, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    case NMethodPropID::kEncoder:
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)method.Encoder, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
  }
  return S_OK;
}

