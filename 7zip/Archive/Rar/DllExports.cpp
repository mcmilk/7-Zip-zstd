// DLLExports.cpp

#include "StdAfx.h"

#include "Common/MyInitGuid.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "../../ICoder.h"
#include "../../IPassword.h"
#include "../Common/CodecsPath.h"

// {23170F69-40C1-278B-0601-010000000000}
DEFINE_GUID(CLSID_CCrypto_AES128_Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);

#include "RarHandler.h"

HINSTANCE g_hInstance;

void GetCryptoFolderPrefix(TCHAR *path)
{
  CSysString s = GetCodecsFolderPrefix();
  lstrcpy(path, s);
}


// {23170F69-40C1-278B-0403-010000000000}
DEFINE_GUID(CLSID_CCompressRar15Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-0403-020000000000}
DEFINE_GUID(CLSID_CCompressRar20Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-0403-030000000000}
DEFINE_GUID(CLSID_CCompressRar29Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00);

/*
// {23170F69-40C1-278B-06F1-0302000000000}
DEFINE_GUID(CLSID_CCryptoRar20Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278B-06F1-0303000000000}
DEFINE_GUID(CLSID_CCryptoRar29Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00);
*/

// {23170F69-40C1-278A-1000-000110030000}
DEFINE_GUID(CLSID_CRarHandler, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x03, 0x00, 0x00);

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
  if (dwReason == DLL_PROCESS_ATTACH)
    g_hInstance = hInstance;
  return TRUE;
}

STDAPI CreateObject(
    const GUID *classID, 
    const GUID *interfaceID, 
    void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  if (*classID != CLSID_CRarHandler)
    return CLASS_E_CLASSNOTAVAILABLE;
  int needIn = *interfaceID == IID_IInArchive;
  if (needIn)
  {
    NArchive::NRar::CHandler *temp = new NArchive::NRar::CHandler;
    if (needIn)
    {
      CMyComPtr<IInArchive> inArchive = (IInArchive *)temp;
      *outObject = inArchive.Detach();
    }
    else
    {
      CMyComPtr<IOutArchive> outArchive = (IOutArchive *)temp;
      *outObject = outArchive.Detach();
    }
  }
  else
    return E_NOINTERFACE;
  COM_TRY_END
  return S_OK;
}

STDAPI GetHandlerProperty(PROPID propID, PROPVARIANT *value)
{
  NWindows::NCOM::CPropVariant propVariant;
  switch(propID)
  {
    case NArchive::kName:
      propVariant = L"Rar";
      break;
    case NArchive::kClassID:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CRarHandler, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NArchive::kExtension:
      propVariant = L"rar";
      break;
    case NArchive::kUpdate:
      propVariant = false;
      break;
    case NArchive::kKeepName:
      propVariant = false;
      break;
    case NArchive::kStartSignature:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)NArchive::NRar::NHeader::kMarker, 
          NArchive::NRar::NHeader::kMarkerSize)) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
  }
  propVariant.Detach(value);
  return S_OK;
}
