// DLLExports.cpp

#include "StdAfx.h"

#include "Common/MyInitGuid.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "ZHandler.h"
#include "../../ICoder.h"

// {23170F69-40C1-278B-0402-050000000000}
DEFINE_GUID(CLSID_CCompressZDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x02, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278A-1000-0001100D0000}
DEFINE_GUID(CLSID_CZHandler, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x0D, 0x00, 0x00);

HINSTANCE g_hInstance;

#ifndef COMPRESS_BZIP2
#include "../Common/CodecsPath.h"
CSysString GetBZip2CodecPath()
{
  return GetCodecsFolderPrefix() + TEXT("BZip2.dll");
}
#endif

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
  if (*classID != CLSID_CZHandler)
    return CLASS_E_CLASSNOTAVAILABLE;
  int needIn = *interfaceID == IID_IInArchive;
  int needOut = *interfaceID == IID_IOutArchive;
  if (needIn || needOut)
  {
    NArchive::NZ::CHandler *temp = new NArchive::NZ::CHandler;
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
      propVariant = L"Z";
      break;
    case NArchive::kClassID:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CZHandler, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NArchive::kExtension:
      propVariant = L"z";
      break;
    case NArchive::kUpdate:
      propVariant = false;
      break;
    case NArchive::kKeepName:
      propVariant = true;
      break;
    case NArchive::kStartSignature:
    {
      const unsigned char sig[] = { 0x1F, 0x9D };
      if ((value->bstrVal = ::SysAllocStringByteLen((const char *)sig, 2)) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }

  }
  propVariant.Detach(value);
  return S_OK;
}
