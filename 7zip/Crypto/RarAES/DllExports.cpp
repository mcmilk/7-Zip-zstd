// DLLExports.cpp

#include "StdAfx.h"

#define INITGUID

#include "RarAES.h"
#include "Windows/COMTRY.h"

HINSTANCE g_hInstance;

// {23170F69-40C1-278B-06F1-0303000000000}
DEFINE_GUID(CLSID_CCryptoRar29Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00);

bool GetCryptoFolderPrefix(TCHAR *path)
{
  TCHAR fullPath[MAX_PATH + 1];
  if (::GetModuleFileName(g_hInstance, fullPath, MAX_PATH) == 0)
    return false;
  LPTSTR fileNamePointer;
  DWORD needLength = ::GetFullPathName(fullPath, MAX_PATH + 1, 
      path, &fileNamePointer);
  if (needLength == 0 || needLength >= MAX_PATH)
    return false;
  *fileNamePointer = 0;
  return true;
}

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
  if (dwReason == DLL_PROCESS_ATTACH)
    g_hInstance = hInstance;
  return TRUE;
}

STDAPI CreateObject(const GUID *clsid, const GUID *iid, void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  int correctInterface = (*iid == IID_ICompressCoder);
  CMyComPtr<ICompressCoder> coder;
  if (*clsid == CLSID_CCryptoRar29Decoder)
  {
    if (!correctInterface)
      return E_NOINTERFACE;
    coder = (ICompressCoder *)new NCrypto::NRar29::CDecoder();
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
      const char id[] = { 0x06, (char)0xF1, 0x03, 0x03 };
      if ((value->bstrVal = ::SysAllocStringByteLen(id, sizeof(id))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NCompress::kName:
      if ((value->bstrVal = ::SysAllocString(L"RarAES")) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    case NCompress::kDecoder:
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CCryptoRar29Decoder, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
  }
  return S_OK;
}

