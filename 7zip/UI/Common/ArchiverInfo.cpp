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

static void SplitString(const UString &srcString, UStringVector &destStrings)
{
  destStrings.Clear();
  UString string;
  int len = srcString.Length();
  if (len == 0)
    return;
  for (int i = 0; i < len; i++)
  {
    wchar_t c = srcString[i];
    if (c == L' ')
    {
      if (!string.IsEmpty())
      {
        destStrings.Add(string);
        string.Empty();
      }
    }
    else
      string += c;
  }
  if (!string.IsEmpty())
    destStrings.Add(string);
}

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
  
  #ifdef FORMAT_7Z
  {
    CArchiverInfo item;
    item.UpdateEnabled = true;
    item.KeepName = false;
    item.Name = L"7z";
    item.Extensions.Add(CArchiverExtInfo(L"7z"));
    archivers.Add(item);
  }
  #endif

  #ifdef FORMAT_BZIP2
  {
    CArchiverInfo item;
    item.UpdateEnabled = true;
    item.KeepName = true;
    item.Name = L"BZip2";
    item.Extensions.Add(CArchiverExtInfo(L"bz2"));
    item.Extensions.Add(CArchiverExtInfo(L"tbz2", L".tar"));
    archivers.Add(item);
  }
  #endif

  #ifdef FORMAT_GZIP
  {
    CArchiverInfo item;
    item.UpdateEnabled = true;
    item.KeepName = false;
    item.Name = L"GZip";
    item.Extensions.Add(CArchiverExtInfo(L"gz"));
    item.Extensions.Add(CArchiverExtInfo(L"tgz", L".tar"));
    archivers.Add(item);
  }
  #endif

  #ifdef FORMAT_TAR
  {
    CArchiverInfo item;
    item.UpdateEnabled = true;
    item.KeepName = false;
    item.Name = L"Tar";
    item.Extensions.Add(CArchiverExtInfo(L"tar"));
    archivers.Add(item);
  }
  #endif

  #ifdef FORMAT_ZIP
  {
    CArchiverInfo item;
    item.UpdateEnabled = true;
    item.KeepName = false;
    item.Name = L"Zip";
    item.Extensions.Add(CArchiverExtInfo(L"zip"));
    archivers.Add(item);
  }
  #endif

  #ifdef FORMAT_CPIO
  {
    CArchiverInfo item;
    item.UpdateEnabled = false;
    item.Name = L"Cpio";
    item.Extensions.Add(CArchiverExtInfo(L"cpio"));
    archivers.Add(item);
  }
  #endif

  #ifdef FORMAT_RPM
  {
    CArchiverInfo item;
    item.UpdateEnabled = false;
    item.Name = L"Rpm";
    item.Extensions.Add(CArchiverExtInfo(L"rpm", L".cpio.gz"));
    archivers.Add(item);
  }
  #endif

  #ifdef FORMAT_ARJ
  {
    CArchiverInfo item;
    item.UpdateEnabled = false;
    item.Name = L"Arj";
    item.Extensions.Add(CArchiverExtInfo(L"arj"));
    archivers.Add(item);
  }
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

    UString ext  = prop.bstrVal;
    // item.Extension = prop.bstrVal;

    UString addExt;

    if (getHandlerProperty(NArchive::kAddExtension, &prop) != S_OK)
      continue;
    if (prop.vt == VT_BSTR)
    {
      addExt = prop.bstrVal;
    }
    else if (prop.vt != VT_EMPTY)
      continue;
    prop.Clear();

    UStringVector exts, addExts;
    SplitString(ext, exts);
    SplitString(addExt, addExts);

    prop.Clear();
    for (int i = 0; i < exts.Size(); i++)
    {
      CArchiverExtInfo extInfo;
      extInfo.Extension = exts[i];
      if (addExts.Size() > 0)
        extInfo.AddExtension = addExts[i];
      if (extInfo.AddExtension == L"*")
        extInfo.AddExtension.Empty();
      item.Extensions.Add(extInfo);
    }

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


