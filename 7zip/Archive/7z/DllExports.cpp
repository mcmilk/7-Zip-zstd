// DLLExports.cpp

#include "StdAfx.h"

#include "../../../Common/MyInitGuid.h"
#include "../../../Common/ComTry.h"

#include "../../ICoder.h"

#include "7zHandler.h"

#ifndef EXCLUDE_COM
// {23170F69-40C1-278B-06F1-070100000100}
DEFINE_GUID(CLSID_CCrypto7zAESEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x06, 0xF1, 0x07, 0x01, 0x00, 0x00, 0x01, 0x00);
#else
#include "../../Compress/LZ/IMatchFinder.h"
#endif

HINSTANCE g_hInstance;

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
  if (*classID != NArchive::N7z::CLSID_CFormat7z)
    return CLASS_E_CLASSNOTAVAILABLE;
  int needIn = *interfaceID == IID_IInArchive;
  int needOut = *interfaceID == IID_IOutArchive;
  if (needIn || needOut)
  {
    NArchive::N7z::CHandler *temp = new NArchive::N7z::CHandler;
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
      propVariant = L"7z";
      break;
    case NArchive::kClassID:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&NArchive::N7z::CLSID_CFormat7z, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NArchive::kExtension:
      propVariant = L"7z";
      break;
    case NArchive::kUpdate:
      propVariant = true;
      break;
    case NArchive::kKeepName:
      propVariant = false;
      break;
    case NArchive::kStartSignature:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen((const char *)NArchive::N7z::kSignature, 
          NArchive::N7z::kSignatureSize)) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
  }
  propVariant.Detach(value);
  return S_OK;
}
