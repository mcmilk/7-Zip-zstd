// DLLExports.cpp

#include "StdAfx.h"

#define INITGUID

#include "Common/ComTry.h"
#include "../../ICoder.h"
#include "MyAES.h"

/*
// {23170F69-40C1-278B-0601-000000000100}
DEFINE_GUID(CLSID_CCrypto_AES_Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00);

// {23170F69-40C1-278B-0601-000000000000}
DEFINE_GUID(CLSID_CCrypto_AES_Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
*/

/*
#include "Interface/ICoder.h"

#include "Alien/Crypto/CryptoPP/crc.h"
#include "Alien/Crypto/CryptoPP/sha.h"
#include "Alien/Crypto/CryptoPP/md2.h"
#include "Alien/Crypto/CryptoPP/md5.h"
#include "Alien/Crypto/CryptoPP/ripemd.h"
#include "Alien/Crypto/CryptoPP/haval.h"
// #include "Alien/Crypto/CryptoPP/tiger.h"


using namespace CryptoPP;

#define CLSIDName(Name) CLSID_CCryptoHash ## Name
#define ClassName(Name) aClass ## Name

// {23170F69-40C1-278B-17??-000000000000}
#define MyClassID(Name, anID) DEFINE_GUID(CLSIDName(Name), \
  0x23170F69, 0x40C1, 0x278B, 0x17, anID, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

#define Pair(Name, anID) MyClassID(Name, anID)\
typedef CHash<Name, &CLSIDName(Name)> ClassName(Name);

Pair(CRC32, 1)
Pair(SHA1, 2)
Pair(SHA256, 3)
Pair(SHA384, 4)
Pair(SHA512, 5)
Pair(MD2, 6)
Pair(MD5, 7)
Pair(RIPEMD160, 8)
Pair(HAVAL, 9)
// Pair(Tiger, 18)

#define My_OBJECT_ENTRY(ID) OBJECT_ENTRY(CLSIDName(ID), ClassName(ID))

BEGIN_OBJECT_MAP(ObjectMap)
  My_OBJECT_ENTRY(CRC32)
  My_OBJECT_ENTRY(SHA1)
  My_OBJECT_ENTRY(SHA256)
  My_OBJECT_ENTRY(SHA384)
  My_OBJECT_ENTRY(SHA512)
  My_OBJECT_ENTRY(MD2)
  My_OBJECT_ENTRY(MD5)
  My_OBJECT_ENTRY(RIPEMD160)
  My_OBJECT_ENTRY(HAVAL)
//  My_OBJECT_ENTRY(Tiger)
END_OBJECT_MAP()
*/

/*
#define MyOBJECT_ENTRY(Name) \
  OBJECT_ENTRY(CLSID_CCrypto ## Name ## _Encoder, C ## Name ## _Encoder) \
  OBJECT_ENTRY(CLSID_CCrypto ## Name ## _Decoder, C ## Name ## _Decoder) \

BEGIN_OBJECT_MAP(ObjectMap)
  MyOBJECT_ENTRY(_AES128_CBC)
  MyOBJECT_ENTRY(_AES256_CBC)
END_OBJECT_MAP()
*/

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	return TRUE;
}

#define MY_CreateClass(n) \
if (*clsid == CLSID_CCrypto_ ## n ## _Encoder) { \
    if (!correctInterface) \
      return E_NOINTERFACE; \
    coder = (ICompressCoder2 *)new C ## n ## _Encoder(); \
  } else if (*clsid == CLSID_CCrypto_ ## n ## _Decoder){ \
    if (!correctInterface) \
      return E_NOINTERFACE; \
    coder = (ICompressCoder2 *)new C ## n ## _Decoder(); \
  }

STDAPI CreateObject(
    const GUID *clsid, 
    const GUID *interfaceID, 
    void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  int correctInterface = (*interfaceID == IID_ICompressCoder2);
  CMyComPtr<ICompressCoder2> coder;

  MY_CreateClass(AES128_CBC)
  else
  MY_CreateClass(AES256_CBC)
  else
    return CLASS_E_CLASSNOTAVAILABLE;
  *outObject = coder.Detach();
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
  METHOD_ITEM(AES128_CBC, 0x01, L"AES128"),
  METHOD_ITEM(AES256_CBC, char(0x81), L"AES256")
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
    case NMethodPropID::kInStreams:
    {
      value->vt = VT_UI4;
      value->ulVal = 3;
      return S_OK;
    }
  }
  return S_OK;
}

