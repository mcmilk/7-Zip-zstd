// ArchiverInfo.cpp

#include "StdAfx.h"

#include "ArchiverInfo.h"

#ifndef EXCLUDE_COM

#include "Windows/FileFind.h"
#include "Windows/FileName.h"
#include "Windows/DLL.h"
#include "Windows/Registry.h"
#include "Windows/PropVariant.h"
#include "../../Archive/IArchive.h"

using namespace NWindows;
using namespace NFile;

#endif

extern HINSTANCE g_hInstance;

#ifndef EXCLUDE_COM

typedef UINT32 (WINAPI * GetHandlerPropertyFunc)(
    PROPID propID, PROPVARIANT *value);

CSysString GetCurrentModulePath()
{
  TCHAR fullPath[MAX_PATH + 1];
  ::GetModuleFileName(g_hInstance, fullPath, MAX_PATH);
  return fullPath;
}

static CSysString GetModuleFolderPrefix()
{
  CSysString path = GetCurrentModulePath();
  int pos = path.ReverseFind(TEXT('\\'));
  return path.Left(pos + 1);
}

static LPCTSTR kFormatFolderName = TEXT("Formats");
static LPCTSTR kRegistryPath = TEXT("Software\\7-zip");
static LPCTSTR kProgramPathValue = TEXT("Path");

CSysString GetBaseFolderPrefix()
{
  CSysString moduleFolderPrefix = GetModuleFolderPrefix();
  NFind::CFileInfo fileInfo;
  if (NFind::FindFile(moduleFolderPrefix + kFormatFolderName, fileInfo))
    if (fileInfo.IsDirectory())
      return moduleFolderPrefix;
  CSysString path;
  {
    NRegistry::CKey key;
    if(key.Open(HKEY_CURRENT_USER, kRegistryPath, KEY_READ) == ERROR_SUCCESS)
      if (key.QueryValue(kProgramPathValue, path) == ERROR_SUCCESS)
      {
        NName::NormalizeDirPathPrefix(path);
        return path;
      }
  }
  {
    NRegistry::CKey key;
    if(key.Open(HKEY_LOCAL_MACHINE, kRegistryPath, KEY_READ) == ERROR_SUCCESS)
      if (key.QueryValue(kProgramPathValue, path) == ERROR_SUCCESS)
      {
        NName::NormalizeDirPathPrefix(path);
        return path;
      }
  }
  return moduleFolderPrefix;
}

// static const TCHAR *kExtension = TEXT("Extension");
// static const TCHAR *kAddExtension = TEXT("AddExtension");
// static const TCHAR *kUpdate = TEXT("Update");
// static const TCHAR *kKeepName = TEXT("KeepName");

typedef UINT32 (WINAPI *CreateObjectPointer)(
    const GUID *clsID, 
    const GUID *interfaceID, 
    void **outObject);

#endif

void ReadArchiverInfoList(CObjectVector<CArchiverInfo> &archivers)
{
  archivers.Clear();
  
  #ifdef EXCLUDE_COM
  
  CArchiverInfo item;
  #ifdef FORMAT_7Z
  item.UpdateEnabled = true;
  item.KeepName = false;
  item.Name = L"7z";
  item.Extension = L"7z";
  archivers.Add(item);
  #endif

  #ifdef FORMAT_BZIP2
  item.UpdateEnabled = true;
  item.KeepName = true;
  item.Name = L"BZip2";
  item.Extension = L"bz2";
  archivers.Add(item);
  #endif

  #ifdef FORMAT_GZIP
  item.UpdateEnabled = true;
  item.KeepName = false;
  item.Name = L"GZip";
  item.Extension = L"gz";
  archivers.Add(item);
  #endif

  #ifdef FORMAT_TAR
  item.UpdateEnabled = true;
  item.KeepName = false;
  item.Name = L"Tar";
  item.Extension = L"tar";
  archivers.Add(item);
  #endif

  #ifdef FORMAT_ZIP
  item.UpdateEnabled = true;
  item.KeepName = false;
  item.Name = L"Zip";
  item.Extension = L"zip";
  archivers.Add(item);
  #endif

  #ifdef FORMAT_CPIO
  item.UpdateEnabled = false;
  item.Name = L"cpio";
  item.Extension = L"cpio";
  archivers.Add(item);
  #endif

  #ifdef FORMAT_RPM
  item.UpdateEnabled = false;
  item.Name = L"RPM";
  item.Extension = L"rpm";
  item.AddExtension = L".cpio.gz");
  archivers.Add(item);
  #endif

  #ifdef FORMAT_ARJ
  item.UpdateEnabled = false;
  item.Name = L"arj";
  item.Extension = L"arj";
  archivers.Add(item);
  #endif
  
  #else

  CSysString folderPath = GetBaseFolderPrefix() + 
      kFormatFolderName + TEXT("\\");
  NFind::CEnumerator enumerator(folderPath + TEXT("*"));
  NFind::CFileInfo fileInfo;
  while (enumerator.Next(fileInfo))
  {
    if (fileInfo.IsDirectory())
      continue;
    CSysString filePath = folderPath + fileInfo.Name;
    {
      NDLL::CLibrary library;
      if (!library.LoadEx(filePath, LOAD_LIBRARY_AS_DATAFILE))
        continue;
    }

    NDLL::CLibrary library;
    if (!library.Load(filePath))
      continue;
    GetHandlerPropertyFunc getHandlerProperty = (GetHandlerPropertyFunc)
        library.GetProcAddress("GetHandlerProperty");
    if (getHandlerProperty == NULL)
      continue;

    CArchiverInfo item;
    item.FilePath = filePath;
    
    NWindows::NCOM::CPropVariant prop;
    if (getHandlerProperty(NArchive::kName, &prop) != S_OK)
      continue;
    if (prop.vt != VT_BSTR)
      continue;
    item.Name = prop.bstrVal;
    prop.Clear();

    if (getHandlerProperty(NArchive::kClassID, &prop) != S_OK)
      continue;
    if (prop.vt != VT_BSTR)
      continue;
    item.ClassID = *(const GUID *)prop.bstrVal;
    prop.Clear();

    if (getHandlerProperty(NArchive::kExtension, &prop) != S_OK)
      continue;
    if (prop.vt != VT_BSTR)
      continue;
    item.Extension = prop.bstrVal;
    prop.Clear();

    if (getHandlerProperty(NArchive::kAddExtension, &prop) != S_OK)
      continue;
    if (prop.vt == VT_BSTR)
      item.AddExtension = prop.bstrVal;
    else if (prop.vt != VT_EMPTY)
      continue;
    prop.Clear();

    if (getHandlerProperty(NArchive::kUpdate, &prop) != S_OK)
      continue;
    if (prop.vt != VT_BOOL)
      continue;
    item.UpdateEnabled = VARIANT_BOOLToBool(prop.boolVal);
    prop.Clear();

    if (item.UpdateEnabled)
    {
      if (getHandlerProperty(NArchive::kKeepName, &prop) != S_OK)
        continue;
      if (prop.vt != VT_BOOL)
        continue;
      item.KeepName = VARIANT_BOOLToBool(prop.boolVal);
      prop.Clear();
    }

    archivers.Add(item);
  }

  #endif
}


