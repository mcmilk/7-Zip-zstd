// ZipRegistryMain.cpp

#include "StdAfx.h"

#include "ZipRegistryConfig.h"
#include "Windows/COM.h"
#include "Windows/Synchronization.h"
#include "Windows/Registry.h"
#include "Windows/System.h"

#include "Windows/FileName.h"

using namespace NZipSettings;

using namespace NWindows;
using namespace NCOM;
using namespace NRegistry;

////////////////////////////////////////////////////////////
// CZipRegistryClassesRootManager

namespace NZipRootRegistry {
  
static NSynchronization::CCriticalSection g_RegistryOperationsCriticalSection;
  
static const TCHAR *kShellNewKeyName = _T("ShellNew");
static const TCHAR *kShellNewDataValueName = _T("Data");
  
static const TCHAR *kDefaultIconKeyName = _T("DefaultIcon");
static const TCHAR *kShellKeyName = _T("shell");
static const TCHAR *kOpenKeyName = _T("open");
static const TCHAR *kCommandKeyName = _T("command");
static const TCHAR *kOpenCommandValue = 
    _T("Explorer /e,/root,{23170F69-40C1-278A-1000-000100010000}, %1");


CSysString GetExtensionKeyName(const CSysString &anExtension)
{
  return CSysString(_T(".")) + anExtension;
}

CSysString GetProgramKeyName(const CSysString &anExtension)
{
  return CSysString(_T("7-")) + anExtension;
}

bool CheckShellExtensionInfo(const CSysString &anExtension)
{
  NSynchronization::CSingleLock aLock(&g_RegistryOperationsCriticalSection, true);
  CSysString aProgramKeyName = GetProgramKeyName(anExtension);

  CKey anExtensionArchiversKey;
  if (anExtensionArchiversKey.Open(HKEY_CLASSES_ROOT, 
     GetExtensionKeyName(anExtension), KEY_READ)  != ERROR_SUCCESS)
    return false;
  CSysString aProgramNameValue;
  if (anExtensionArchiversKey.QueryValue(NULL, aProgramNameValue) != ERROR_SUCCESS)
    return false;
  return (aProgramNameValue.CollateNoCase(aProgramKeyName) == 0);
}


void DeleteShellExtensionInfoAlways(const CSysString &anExtension)
{
  NSynchronization::CSingleLock aLock(&g_RegistryOperationsCriticalSection, true);
  CKey aRootKey;
  aRootKey.Attach(HKEY_CLASSES_ROOT);
  aRootKey.RecurseDeleteKey(GetExtensionKeyName(anExtension));
  aRootKey.RecurseDeleteKey(GetProgramKeyName(anExtension));
  aRootKey.Detach();
}

void DeleteShellExtensionInfo(const CSysString &anExtension)
{
  if (!CheckShellExtensionInfo(anExtension))
    return;
  DeleteShellExtensionInfoAlways(anExtension);
}

void AddShellExtensionInfo(const CSysString &anExtension,
    const CSysString &aProgramTitle, const CSysString &anIconFilePath,
    const void *aShellNewData, int aShellNewDataSize)
{
  DeleteShellExtensionInfoAlways(anExtension);
  NSynchronization::CSingleLock aLock(&g_RegistryOperationsCriticalSection, true);
  CSysString aProgramKeyName = GetProgramKeyName(anExtension);
  {
    CKey anExtensionArchiversKey;
    anExtensionArchiversKey.Create(HKEY_CLASSES_ROOT, GetExtensionKeyName(anExtension));
    anExtensionArchiversKey.SetValue(NULL, aProgramKeyName);
    if (aShellNewData != NULL)
    {
      CKey aShellNewKey;
      aShellNewKey.Create(anExtensionArchiversKey, kShellNewKeyName);
      aShellNewKey.SetValue(kShellNewDataValueName, aShellNewData, aShellNewDataSize);
    }
  }
  CKey aProgramKey;
  aProgramKey.Create(HKEY_CLASSES_ROOT, aProgramKeyName);
  aProgramKey.SetValue(NULL, aProgramTitle);

  {
    CKey aDefaultIcon;
    aDefaultIcon.Create(aProgramKey, kDefaultIconKeyName);
    aDefaultIcon.SetValue(NULL, anIconFilePath);
  }

  CKey aShellKey;
  aShellKey.Create(aProgramKey, kShellKeyName);
  aShellKey.SetValue(NULL, _T(""));

  CKey anOpenKey;
  anOpenKey.Create(aShellKey, kOpenKeyName);
  anOpenKey.SetValue(NULL, _T(""));
  
  CKey aCommandKey;
  aCommandKey.Create(anOpenKey, kCommandKeyName);

  CSysString aParams;
  if (!NSystem::MyGetWindowsDirectory(aParams))
  {
    aParams.Empty();
    // return;
  }
  else
    NFile::NName::NormalizeDirPathPrefix(aParams);
  aParams += kOpenCommandValue;
  aCommandKey.SetValue(NULL, aParams);
}

static const TCHAR *kCLSIDKeyName = _T("CLSID");
static const TCHAR *kInprocServer32KeyName = _T("InprocServer32");

// static const TCHAR *kIconExtension = _T(".ico");

void AddShellExtensionInfo(const CSysString &anExtension,
    const CSysString &aProgramTitle, const CLSID &aClassID,
    const void *aShellNewData, int aShellNewDataSize)
{
  CSysString anIconPath;
  {
    CKey aKey;
    CSysString aKeyPath = kCLSIDKeyName;
    aKeyPath += kKeyNameDelimiter;
    aKeyPath += GUIDToString(aClassID);
    aKeyPath += kKeyNameDelimiter;
    aKeyPath += kInprocServer32KeyName;
    if (aKey.Open(HKEY_CLASSES_ROOT, aKeyPath, KEY_READ) == ERROR_SUCCESS)
    {
      CSysString aPath;
      if (aKey.QueryValue(NULL, aPath) == ERROR_SUCCESS)
      {
        anIconPath = aPath;
        /*
        CSysString aDirPrefix;
        if (NFile::NDirectory::GetOnlyDirPrefix(aPath, aDirPrefix))
        {
        anIconPath = aDirPrefix;
        anIconPath += anExtension;
        anIconPath += kIconExtension;
        }
        */
      }
    }
  }
  AddShellExtensionInfo(anExtension, aProgramTitle, anIconPath,
      aShellNewData, aShellNewDataSize);
}



///////////////////////////
// ContextMenu

static const TCHAR *kContextMenuKeyName = _T("\\shellex\\ContextMenuHandlers\\7-ZIP");
static const TCHAR *kContextMenuHandlerCLASSIDValue = 
    _T("{23170F69-40C1-278A-1000-000100020000}");
static const TCHAR *kRootKeyNameForFile = _T("*");
static const TCHAR *kRootKeyNameForFolder = _T("Folder");

static CSysString GetFullContextMenuKeyName(const CSysString &aKeyName)
  { return (aKeyName + kContextMenuKeyName); }

static bool CheckContextMenuHandlerCommon(const CSysString &aKeyName)
{
  NSynchronization::CSingleLock aLock(&g_RegistryOperationsCriticalSection, true);
  CKey aKey;
  if (aKey.Open(HKEY_CLASSES_ROOT, GetFullContextMenuKeyName(aKeyName), KEY_READ)
      != ERROR_SUCCESS)
    return false;
  CSysString aValue;
  if (aKey.QueryValue(NULL, aValue) != ERROR_SUCCESS)
    return false;
  return (aValue.CollateNoCase(kContextMenuHandlerCLASSIDValue) == 0);
}

bool CheckContextMenuHandler()
{ 
  return CheckContextMenuHandlerCommon(kRootKeyNameForFile) &&
    CheckContextMenuHandlerCommon(kRootKeyNameForFolder);
}

static void DeleteContextMenuHandlerCommon(const CSysString &aKeyName)
{
  CKey aRootKey;
  aRootKey.Attach(HKEY_CLASSES_ROOT);
  aRootKey.RecurseDeleteKey(GetFullContextMenuKeyName(aKeyName));
  aRootKey.Detach();
}

void DeleteContextMenuHandler()
{ 
  DeleteContextMenuHandlerCommon(kRootKeyNameForFile); 
  DeleteContextMenuHandlerCommon(kRootKeyNameForFolder);
}

static void AddContextMenuHandlerCommon(const CSysString &aKeyName)
{
  DeleteContextMenuHandlerCommon(aKeyName);
  NSynchronization::CSingleLock aLock(&g_RegistryOperationsCriticalSection, true);
  CKey aKey;
  aKey.Create(HKEY_CLASSES_ROOT, GetFullContextMenuKeyName(aKeyName));
  aKey.SetValue(NULL, kContextMenuHandlerCLASSIDValue);
}

void AddContextMenuHandler()
{ 
  AddContextMenuHandlerCommon(kRootKeyNameForFile); 
  AddContextMenuHandlerCommon(kRootKeyNameForFolder);
}

}
