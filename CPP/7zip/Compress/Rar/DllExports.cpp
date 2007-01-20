// DLLExports.cpp

#include "StdAfx.h"

#include "Common/MyInitGuid.h"
#include "Common/ComTry.h"

#include "Rar1Decoder.h"
#include "Rar2Decoder.h"
#include "Rar3Decoder.h"
// #include "Rar29Decoder.h"

#define RarClassId(ver) CLSID_CCompressRar ## ver ## Decoder

#define MyClassRar(ver) DEFINE_GUID(RarClassId(ver), \
0x23170F69, 0x40C1, 0x278B, 0x04, 0x03, ver, 0x00, 0x00, 0x00, 0x00, 0x00);

MyClassRar(1);
MyClassRar(2);
MyClassRar(3);

#define CreateCoder(ver) if (*clsid == RarClassId(ver)) \
{ if (!correctInterface) return E_NOINTERFACE; \
coder = (ICompressCoder *)new NCompress::NRar ## ver::CDecoder; }

extern "C"
BOOL WINAPI DllMain(HINSTANCE /* hInstance */, DWORD /* dwReason */, LPVOID /*lpReserved*/)
{
  return TRUE;
}

STDAPI CreateObject(const GUID *clsid, const GUID *iid, void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  int correctInterface = (*iid == IID_ICompressCoder);
  CMyComPtr<ICompressCoder> coder;
  CreateCoder(1) else
  CreateCoder(2) else
  CreateCoder(3) else    
  return CLASS_E_CLASSNOTAVAILABLE;
  *outObject = coder.Detach();
  COM_TRY_END
  return S_OK;
}

struct CRarMethodItem
{
  char ID[3];
  const wchar_t *UserName;
  const GUID *Decoder;
};

static CRarMethodItem g_Methods[] =
{
  { { 0x04, 0x03, 0x01 }, L"Rar15", &RarClassId(1) },
  { { 0x04, 0x03, 0x02 }, L"Rar20", &RarClassId(2) },
  { { 0x04, 0x03, 0x03 }, L"Rar29", &RarClassId(3) }
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
  const CRarMethodItem &method = g_Methods[index];
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
  }
  return S_OK;
}
