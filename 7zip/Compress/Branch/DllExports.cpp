// DLLExports.cpp

#include "StdAfx.h"

#define INITGUID

#include "Common/ComTry.h"

#include "Coder.h"
#include "x86.h"
#include "PPC.h"
#include "Alpha.h"
#include "IA64.h"
#include "ARM.h"
#include "ARMThumb.h"
#include "M68.h"
#include "x86_2.h"

#include "../../ICoder.h"


#define MY_CreateClass(n) \
if (*clsid == CLSID_CCompressConvert ## n ## _Encoder) { \
    if (!correctInterface) \
      return E_NOINTERFACE; \
    coder = (ICompressCoder *)new C ## n ## _Encoder(); \
  } else if (*clsid == CLSID_CCompressConvert ## n ## _Decoder){ \
    if (!correctInterface) \
      return E_NOINTERFACE; \
    coder = (ICompressCoder *)new C ## n ## _Decoder(); \
  }

/*
#define MyOBJECT_ENTRY(Name) \
  OBJECT_ENTRY(CLSID_CCompressConvert ## Name ## _Encoder, C ## Name ## _Encoder) \
  OBJECT_ENTRY(CLSID_CCompressConvert ## Name ## _Decoder, C ## Name ## _Decoder) \


BEGIN_OBJECT_MAP(ObjectMap)
  MyOBJECT_ENTRY(BCJ_x86)
  MyOBJECT_ENTRY(BCJ2_x86)
  MyOBJECT_ENTRY(BC_PPC_B)
  MyOBJECT_ENTRY(BC_Alpha)
  MyOBJECT_ENTRY(BC_IA64)
  MyOBJECT_ENTRY(BC_ARM)
  MyOBJECT_ENTRY(BC_ARMThumb)
  MyOBJECT_ENTRY(BC_M68_B)
END_OBJECT_MAP()
*/

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	return TRUE;
}

STDAPI CreateObject(
    const GUID *clsid, 
    const GUID *interfaceID, 
    void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  int correctInterface = (*interfaceID == IID_ICompressCoder);
  CMyComPtr<ICompressCoder> coder;

  MY_CreateClass(BCJ_x86)
  else
  MY_CreateClass(BC_PPC_B)
  else
  MY_CreateClass(BC_Alpha)
  else
  MY_CreateClass(BC_IA64)
  else
  MY_CreateClass(BC_ARM)
  else
  MY_CreateClass(BC_ARMThumb)
  else
  MY_CreateClass(BC_M68_B)
  else
  {
    CMyComPtr<ICompressCoder2> coder2;
    correctInterface = (*interfaceID == IID_ICompressCoder2);
    if (*clsid == CLSID_CCompressConvertBCJ2_x86_Encoder)
    {
      if (!correctInterface)
        return E_NOINTERFACE;
      coder2 = (ICompressCoder2 *)new CBCJ2_x86_Encoder();
    }
    else if (*clsid == CLSID_CCompressConvertBCJ2_x86_Decoder)
    {
      if (!correctInterface)
        return E_NOINTERFACE;
      coder2 = (ICompressCoder2 *)new CBCJ2_x86_Decoder();
    }
    else
      return CLASS_E_CLASSNOTAVAILABLE;
    *outObject = coder2.Detach();
    return S_OK;
  }
  *outObject = coder.Detach();
  return S_OK;
  COM_TRY_END
}

struct CBranchMethodItem
{
  char ID[4];
  const wchar_t *UserName;
  const GUID *Decoder;
  const GUID *Encoder;
  UINT32 NumInStreams;
};

#define METHOD_ITEM(Name, id, subId, UserName, NumInStreams) \
  { { 0x03, 0x03, id, subId }, UserName, \
  &CLSID_CCompressConvert ## Name ## _Decoder, \
  &CLSID_CCompressConvert ## Name ## _Encoder, NumInStreams }


static CBranchMethodItem g_Methods[] =
{
  METHOD_ITEM(BCJ_x86,  0x01, 0x03, L"BCJ", 1),
  METHOD_ITEM(BCJ2_x86, 0x01, 0x1B, L"BCJ2", 4),
  METHOD_ITEM(BC_PPC_B, 0x02, 0x05, L"BC_PPC_B", 1),
  METHOD_ITEM(BC_Alpha, 0x03, 1, L"BC_Alpha", 1),
  METHOD_ITEM(BC_IA64,  0x04, 1, L"BC_IA64", 1),
  METHOD_ITEM(BC_ARM,   0x05, 1, L"BC_ARM", 1),
  METHOD_ITEM(BC_M68_B, 0x06, 5, L"BC_M68_B", 1),
  METHOD_ITEM(BC_ARMThumb, 0x07, 1, L"BC_ARMThumb", 1)
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
  const CBranchMethodItem &method = g_Methods[index];
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
    case NMethodPropID::kInStreams:
    {
      if (method.NumInStreams != 1)
      {
        value->vt = VT_UI4;
        value->ulVal = method.NumInStreams;
      }
      return S_OK;
    }
  }
  return S_OK;
}

