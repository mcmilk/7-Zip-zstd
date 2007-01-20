// DLLExports.cpp

#include "StdAfx.h"

#include "Common/MyInitGuid.h"
#include "Common/ComTry.h"
#include "Windows/PropVariant.h"
#include "BZip2Handler.h"
#include "../../ICoder.h"

// {23170F69-40C1-278B-0402-020000000100}
DEFINE_GUID(CLSID_CCompressBZip2Encoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00);

// {23170F69-40C1-278B-0402-020000000000}
DEFINE_GUID(CLSID_CCompressBZip2Decoder, 
0x23170F69, 0x40C1, 0x278B, 0x04, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00);

// {23170F69-40C1-278A-1000-000110020000}
DEFINE_GUID(CLSID_CBZip2Handler, 
0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x02, 0x00, 0x00);

HINSTANCE g_hInstance;
#ifndef _UNICODE
bool g_IsNT = false;
static bool IsItWindowsNT()
{
  OSVERSIONINFO versionInfo;
  versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
  if (!::GetVersionEx(&versionInfo)) 
    return false;
  return (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT);
}
#endif

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
  {
    g_hInstance = hInstance;
    #ifndef _UNICODE
    g_IsNT = IsItWindowsNT();
    #endif
  }
  return TRUE;
}

STDAPI CreateObject(
    const GUID *classID, 
    const GUID *interfaceID, 
    void **outObject)
{
  COM_TRY_BEGIN
  *outObject = 0;
  if (*classID != CLSID_CBZip2Handler)
    return CLASS_E_CLASSNOTAVAILABLE;
  int needIn = *interfaceID == IID_IInArchive;
  int needOut = *interfaceID == IID_IOutArchive;
  if (needIn || needOut)
  {
    NArchive::NBZip2::CHandler *temp = new NArchive::NBZip2::CHandler;
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
      propVariant = L"BZip2";
      break;
    case NArchive::kClassID:
    {
      if ((value->bstrVal = ::SysAllocStringByteLen(
          (const char *)&CLSID_CBZip2Handler, sizeof(GUID))) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }
    case NArchive::kExtension:
      propVariant = L"bz2 bzip2 tbz2 tbz";
      break;
    case NArchive::kAddExtension:
      propVariant = L"* * .tar .tar";
      break;
    case NArchive::kUpdate:
      propVariant = true;
      break;
    case NArchive::kKeepName:
      propVariant = true;
      break;
    case NArchive::kStartSignature:
    {
      const char sig[] = { 'B', 'Z', 'h' };
      if ((value->bstrVal = ::SysAllocStringByteLen(sig, 3)) != 0)
        value->vt = VT_BSTR;
      return S_OK;
    }

  }
  propVariant.Detach(value);
  return S_OK;
}
