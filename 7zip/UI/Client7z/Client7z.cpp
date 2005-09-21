// Client7z.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Common/StringConvert.h"
#include "../../Common/FileStreams.h"
#include "../../Archive/IArchive.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include "Windows/DLL.h"

// {23170F69-40C1-278A-1000-000110070000}
DEFINE_GUID(CLSID_CFormat7z, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00);

typedef UINT32 (WINAPI * CreateObjectFunc)(
    const GUID *clsID, 
    const GUID *interfaceID, 
    void **outObject);

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    printf("Use Client7z.exe file.7z");
    return 1;
  }
  NWindows::NDLL::CLibrary library;
  if (!library.Load("7za.dll"))
  {
    printf("Can not load library");
    return 1;
  }
  CreateObjectFunc
      createObjectFunc = 
      (CreateObjectFunc)library.GetProcAddress("CreateObject");
  if (createObjectFunc == 0)
  {
    printf("Can not get CreateObject");
    return 1;
  }
  CMyComPtr<IInArchive> archive;
  if (createObjectFunc(&CLSID_CFormat7z, 
        &IID_IInArchive, (void **)&archive) != S_OK)
  {
    printf("Can not get class object");
    return 1;
  }

  CInFileStream *fileSpec = new CInFileStream;
  CMyComPtr<IInStream> file = fileSpec;

  if (!fileSpec->Open(argv[1]))
  {
    printf("Can not open");
    return 1;
  }
  if (archive->Open(file, 0, 0) != S_OK)
    return 0;
  UInt32 numItems = 0;
  archive->GetNumberOfItems(&numItems);  
  for (UInt32 i = 0; i < numItems; i++)
  {
    NWindows::NCOM::CPropVariant propVariant;
    archive->GetProperty(i, kpidPath, &propVariant);
    UString s = ConvertPropVariantToString(propVariant);
    printf("%s\n", (LPCSTR)GetOemString(s));
  }
  return 0;
}
