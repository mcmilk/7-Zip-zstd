// Client7z.cpp : Defines the entry point for the console application.

#include "stdafx.h"

#include <initguid.h>

#include "../../Format/Common/ArchiveInterface.h"
#include "Interface/FileStreams.h"
#include "Windows/PropVariant.h"
#include "Windows/PropVariantConversions.h"
#include "Windows/DLL.h"

// {23170F69-40C1-278A-1000-000110050000}
DEFINE_GUID(CLSID_CFormat7z, 
  0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x05, 0x00, 0x00);

typedef (WINAPI * DllGetClassObjectFunctionPointer)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    printf("Use test.exe file.7z");
    return 1;
  }
  // CoInitialize(NULL);

  NWindows::NDLL::CLibrary library;
  if (!library.Load("7za.dll"))
  {
    printf("Can not load library");
    return 1;
  }
  DllGetClassObjectFunctionPointer 
      getClassObject = 
      (DllGetClassObjectFunctionPointer)library.GetProcAddress("DllGetClassObject");
  if (getClassObject == 0)
  {
    printf("Can not get GetProcAddress");
    return 1;
  }
  CComPtr<IInArchive> archive;
  CComPtr<IClassFactory> classFactory;
  HRESULT result;
  result = getClassObject(CLSID_CFormat7z, IID_IClassFactory, 
      (void **)&classFactory);
  if (result != 0)
  {
    printf("Can not get GetProcAddress");
    return 1;
  }
  result = classFactory->CreateInstance(0, IID_IInArchive, (void **)&archive);
  if (result != 0)
  {
    printf("Can not get GetProcAddress");
    return 1;
  }

  /*
  HRESULT result = archive.CoCreateInstance(CLSID_CFormat7z);
    if (result != S_OK)
      return 1;
  */

  CComObjectNoLock<CInFileStream> *fileSpec = new CComObjectNoLock<CInFileStream>;
  CComPtr<IInStream> file = fileSpec;

  if (!fileSpec->Open(argv[1]))
  {
    printf("Can not open");
    return 1;
  }
  result = archive->Open(file, 0, 0);
  if (result != S_OK)
    return 0;
  UINT32 numItems = 0;
  archive->GetNumberOfItems(&numItems);  
  for (UINT32 i = 0; i < numItems; i++)
  {
    NWindows::NCOM::CPropVariant propVariant;
    archive->GetProperty(i, kpidPath, &propVariant);
    CSysString string = ConvertPropVariantToString(propVariant);
    printf("%s\n", (LPCTSTR)string);
  }
	return 0;
}
