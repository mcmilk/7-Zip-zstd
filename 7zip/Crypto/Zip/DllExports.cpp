// DLLExports.cpp

#include "StdAfx.h"

#define INITGUID

#include "Common/ComTry.h"
#include "../../ICoder.h"
#include "../../ICoderProperties.h"

#include "ZipCipher.h"

// {23170F69-40C1-278B-06F1-0101000000100}
DEFINE_GUID(CLSID_CCryptoZipEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00);

// {23170F69-40C1-278B-06F1-0101000000000}
DEFINE_GUID(CLSID_CCryptoZipDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00);

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
  if (*clsid == CLSID_CCryptoZipDecoder)
  {
    if (!correctInterface)
      return E_NOINTERFACE;
    coder = (ICompressCoder *)new NCrypto::NZip::CDecoder();
  }
  else if (*clsid == CLSID_CCryptoZipEncoder)
  {
    if (!correctInterface)
      return E_NOINTERFACE;
    coder = (ICompressCoder *)new NCrypto::NZip::CEncoder();
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
  ::VariantClear((tagVARIANT *)value);
  switch(propID)
  {
    case NCompress::kID:
    {
      const char id[] = { 0x06, (char)0xF1, 0x01, 0x01 };
      if ((value->bstrVal = ::SysAllocStringByteLen(id, sizeof(id))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NCompress::kName:
      if ((value->bstrVal = ::SysAllocString(L"CryptoZip")) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    case NCompress::kDecoder:
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CCryptoZipDecoder, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    case NCompress::kEncoder:
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CCryptoZipEncoder, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
  }
  return S_OK;
}

