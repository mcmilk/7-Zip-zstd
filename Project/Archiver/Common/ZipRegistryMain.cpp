// ZipRegistryMain.cpp

#include "StdAfx.h"

#include "ZipRegistryMain.h"

#ifndef NO_REGISTRY

#include "Windows/COM.h"
#include "Windows/Synchronization.h"
#include "Windows/Registry.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"

using namespace NZipSettings;

using namespace NWindows;
using namespace NCOM;
using namespace NRegistry;

#endif

namespace NZipRootRegistry {

#ifndef NO_REGISTRY
//////////////////////////////////
static const TCHAR *kLMBasePath = _T("Software\\7-ZIP");
//  m_LMKey.Create(HKEY_LOCAL_MACHINE, kLMBasePath);

static const TCHAR *kArchiversKeyName = _T("Archivers");

namespace NArchiveType
{
  static const TCHAR *kExtension = _T("Extension");
  static const TCHAR *kAddExtension = _T("AddExtension");
  static const TCHAR *kUpdate = _T("Update");
  static const TCHAR *kKeepName = _T("KeepName");
}

static NSynchronization::CCriticalSection g_RegistryOperationsCriticalSection;

static CSysString GetArchiversKeyName()
{
  return CSysString(kLMBasePath) + CSysString(kKeyNameDelimiter) + 
    CSysString(kArchiversKeyName);
}
#endif

void ReadArchiverInfoList(CObjectVector<CArchiverInfo> &infoList)
{
  infoList.Clear();
  
  #ifdef NO_REGISTRY
  
  CArchiverInfo itemInfo;
  #ifdef FORMAT_7Z
  itemInfo.UpdateEnabled = true;
  itemInfo.KeepName = false;
  itemInfo.Name = TEXT("7z");
  itemInfo.Extension = TEXT("7z");
  infoList.Add(itemInfo);
  #endif

  #ifdef FORMAT_BZIP2
  itemInfo.UpdateEnabled = true;
  itemInfo.KeepName = true;
  itemInfo.Name = TEXT("BZip2");
  itemInfo.Extension = TEXT("bz2");
  infoList.Add(itemInfo);
  #endif

  #ifdef FORMAT_GZIP
  itemInfo.UpdateEnabled = true;
  itemInfo.KeepName = false;
  itemInfo.Name = TEXT("GZip");
  itemInfo.Extension = TEXT("gz");
  infoList.Add(itemInfo);
  #endif

  #ifdef FORMAT_TAR
  itemInfo.UpdateEnabled = true;
  itemInfo.KeepName = false;
  itemInfo.Name = TEXT("Tar");
  itemInfo.Extension = TEXT("tar");
  infoList.Add(itemInfo);
  #endif

  #ifdef FORMAT_ZIP
  itemInfo.UpdateEnabled = true;
  itemInfo.KeepName = false;
  itemInfo.Name = TEXT("Zip");
  itemInfo.Extension = TEXT("zip");
  infoList.Add(itemInfo);
  #endif

  #ifdef FORMAT_CPIO
  itemInfo.UpdateEnabled = false;
  itemInfo.Name = TEXT("cpio");
  itemInfo.Extension = TEXT("cpio");
  infoList.Add(itemInfo);
  #endif

  #ifdef FORMAT_RPM
  itemInfo.UpdateEnabled = false;
  itemInfo.Name = TEXT("RPM");
  itemInfo.Extension = TEXT("rpm");
  itemInfo.AddExtension = TEXT(".cpio.gz");
  infoList.Add(itemInfo);
  #endif

  #ifdef FORMAT_ARJ
  itemInfo.UpdateEnabled = false;
  itemInfo.Name = TEXT("arj");
  itemInfo.Extension = TEXT("arj");
  infoList.Add(itemInfo);
  #endif
  
  #else

  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);

  CKey archiversKey;
  if(archiversKey.Open(HKEY_LOCAL_MACHINE, GetArchiversKeyName(), KEY_READ) != ERROR_SUCCESS)
    return;
 
  CSysStringVector classIDs;
  archiversKey.EnumKeys(classIDs);
  for(int i = 0; i < classIDs.Size(); i++)
  {
    const CSysString classIDString = classIDs[i];
    CArchiverInfo itemInfo;
    itemInfo.UpdateEnabled = false;
    itemInfo.KeepName = false;
    CKey classIDKey;
    if(classIDKey.Open(archiversKey, classIDString, KEY_READ) != ERROR_SUCCESS)
      return;

    if(StringToGUID(classIDString, itemInfo.ClassID) != NOERROR)
      return; // test it maybe creation;
    classIDKey.QueryValue(NULL, itemInfo.Name);
    classIDKey.QueryValue(NArchiveType::kExtension, itemInfo.Extension);
    classIDKey.QueryValue(NArchiveType::kAddExtension, itemInfo.AddExtension);
    classIDKey.QueryValue(NArchiveType::kUpdate, itemInfo.UpdateEnabled);
    classIDKey.QueryValue(NArchiveType::kKeepName, itemInfo.KeepName);
    infoList.Add(itemInfo);
  }
  #endif
}

}

