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

void ReadArchiverInfoList(CObjectVector<CArchiverInfo> &anInfoList)
{
  anInfoList.Clear();
  
  #ifdef NO_REGISTRY
  
  CArchiverInfo anItemInfo;
  #ifdef FORMAT_7Z
  anItemInfo.UpdateEnabled = true;
  anItemInfo.KeepName = false;
  anItemInfo.Name = TEXT("7z");
  anItemInfo.Extension = TEXT("7z");
  anInfoList.Add(anItemInfo);
  #endif

  #ifdef FORMAT_BZIP2
  anItemInfo.UpdateEnabled = true;
  anItemInfo.KeepName = true;
  anItemInfo.Name = TEXT("BZip2");
  anItemInfo.Extension = TEXT("bz2");
  anInfoList.Add(anItemInfo);
  #endif

  #ifdef FORMAT_GZIP
  anItemInfo.UpdateEnabled = true;
  anItemInfo.KeepName = false;
  anItemInfo.Name = TEXT("GZip");
  anItemInfo.Extension = TEXT("gz");
  anInfoList.Add(anItemInfo);
  #endif

  #ifdef FORMAT_TAR
  anItemInfo.UpdateEnabled = true;
  anItemInfo.KeepName = false;
  anItemInfo.Name = TEXT("Tar");
  anItemInfo.Extension = TEXT("tar");
  anInfoList.Add(anItemInfo);
  #endif

  #ifdef FORMAT_ZIP
  anItemInfo.UpdateEnabled = true;
  anItemInfo.KeepName = false;
  anItemInfo.Name = TEXT("Zip");
  anItemInfo.Extension = TEXT("zip");
  anInfoList.Add(anItemInfo);
  #endif

  
  #else

  NSynchronization::CSingleLock aLock(&g_RegistryOperationsCriticalSection, true);

  CKey anArchiversKey;
  if(anArchiversKey.Open(HKEY_LOCAL_MACHINE, GetArchiversKeyName(), KEY_READ) != ERROR_SUCCESS)
    return;
 
  CSysStringVector aClassIDs;
  anArchiversKey.EnumKeys(aClassIDs);
  for(int i = 0; i < aClassIDs.Size(); i++)
  {
    const CSysString aClassIDString = aClassIDs[i];
    CArchiverInfo anItemInfo;
    anItemInfo.UpdateEnabled = false;
    anItemInfo.KeepName = false;
    CKey aClassIDKey;
    if(aClassIDKey.Open(anArchiversKey, aClassIDString, KEY_READ) != ERROR_SUCCESS)
      return;

    if(StringToGUID(aClassIDString, anItemInfo.ClassID) != NOERROR)
      return; // test it maybe creation;
    aClassIDKey.QueryValue(NULL, anItemInfo.Name);
    aClassIDKey.QueryValue(NArchiveType::kExtension, anItemInfo.Extension);
    aClassIDKey.QueryValue(NArchiveType::kUpdate, anItemInfo.UpdateEnabled);
    aClassIDKey.QueryValue(NArchiveType::kKeepName, anItemInfo.KeepName);
    anInfoList.Add(anItemInfo);
  }
  #endif
}

}

