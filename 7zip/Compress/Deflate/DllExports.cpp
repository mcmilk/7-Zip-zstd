// DLLExports.cpp

#include "StdAfx.h"

#include "Common/MyInitGuid.h"
#include "Common/ComTry.h"

#include "DeflateEncoder.h"
#include "DeflateDecoder.h"


// {23170F69-40C1-278B-0401-080000000000}
DEFINE_GUID(CLSID_CCompressDeflateDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-0401-090000000000}
DEFINE_GUID(CLSID_CCompressDeflate64Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-0401-080000000100}
DEFINE_GUID(CLSID_CCompressDeflateEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00);

// {23170F69-40C1-278B-0401-090000000100}
DEFINE_GUID(CLSID_CCompressDeflate64Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x09, 0x00, 0x00, 0x00, 0x01, 0x00);

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
  if (*clsid == CLSID_CCompressDeflateDecoder)
  {
    if (!correctInterface)
      return E_NOINTERFACE;
    coder = (ICompressCoder *)new NCompress::NDeflate::NDecoder::CCOMCoder();
  }
  else if (*clsid == CLSID_CCompressDeflateEncoder)
  {
    if (!correctInterface)
      return E_NOINTERFACE;
    coder = (ICompressCoder *)new NCompress::NDeflate::NEncoder::CCOMCoder();
  }
  else if (*clsid == CLSID_CCompressDeflate64Decoder)
  {
    if (!correctInterface)
      return E_NOINTERFACE;
    coder = (ICompressCoder *)new NCompress::NDeflate::NDecoder::CCOMCoder64();
  }
  else if (*clsid == CLSID_CCompressDeflate64Encoder)
  {
    if (!correctInterface)
      return E_NOINTERFACE;
    coder = (ICompressCoder *)new NCompress::NDeflate::NEncoder::CCOMCoder64();
  }
  else
    return CLASS_E_CLASSNOTAVAILABLE;
  *outObject = coder.Detach();
  COM_TRY_END
  return S_OK;
}

struct CDeflateMethodItem
{
  char ID[3];
  const wchar_t *UserName;
  const GUID *Decoder;
  const GUID *Encoder;
};

#define METHOD_ITEM(Name, id, UserName) \
  { { 0x04, 0x01, id }, UserName, \
  &CLSID_CCompress ## Name ## Decoder, \
  &CLSID_CCompress ## Name ## Encoder }


static CDeflateMethodItem g_Methods[] =
{
  METHOD_ITEM(Deflate,  0x08, L"Deflate"),
  METHOD_ITEM(Deflate64, 0x09, L"Deflate64")
};

STDAPI GetNumberOfMethods(UINT32 *numMethods)
{
  *numMethods = sizeof(g_Methods) / sizeof(g_Methods[0]);
  return S_OK;
}

STDAPI GetMethodProperty(UINT32 index, PROPID propID, PROPVARIANT *value)
{
  if (index > sizeof(g_Methods) / sizeof(g_Methods[0]))
    return E_INVALIDARG;
  VariantClear((tagVARIANT *)value);
  const CDeflateMethodItem &method = g_Methods[index];
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
