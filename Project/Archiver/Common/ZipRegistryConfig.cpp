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

namespace NZipRootRegistry {
  
static NSynchronization::CCriticalSection g_RegistryOperationsCriticalSection;
  
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
