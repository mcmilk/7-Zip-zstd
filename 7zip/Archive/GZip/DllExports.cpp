// DLLExports.cpp

#include "StdAfx.h"

#define INITGUID

#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "../../ICoder.h"
#include "GZipHandler.h"

// {23170F69-40C1-278A-1000-000110030000}
DEFINE_GUID(CLSID_CGZipHandler, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x03, 0x00, 0x00);

// {23170F69-40C1-278B-0401-080000000100}
DEFINE_GUID(CLSID_CCompressDeflateEncoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00);

// {23170F69-40C1-278B-0401-080000000000}
DEFINE_GUID(CLSID_CCompressDeflateDecoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00);

HINSTANCE g_hInstance;

#ifndef COMPRESS_BZIP2
#include "../Common/CodecsPath.h"
CSysString GetDeflateCodecPath()
{
  return GetCodecsFolderPrefix() + TEXT("Deflate.dll");
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
  if (*classID != CLSID_CGZipHandler)
    return CLASS_E_CLASSNOTAVAILABLE;
  int needIn = *interfaceID == IID_IInArchive;
  int needOut = *interfaceID == IID_IOutArchive;
  if (needIn || needOut)
  {
    NArchive::NGZip::CHandler *temp = new NArchive::NGZip::CHandler;
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
      propVariant = L"GZip";
      break;
    case NArchive::kClassID:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CGZipHandler, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NArchive::kExtension:
      propVariant = L"gz tgz";
      break;
    case NArchive::kAddExtension:
      propVariant = L"* .tar";
      break;
    case NArchive::kUpdate:
      propVariant = true;
      break;
    case NArchive::kKeepName:
      propVariant = true;
      break;
    case NArchive::kStartSignature:
    {
      const unsigned char sig[] = { 0x1F, 0x8B };
      if ((value->bstrVal = ::SysAllocStringByteLen((const char *)sig, 2)) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
  }
  propVariant.Detach(value);
  return S_OK;
}
