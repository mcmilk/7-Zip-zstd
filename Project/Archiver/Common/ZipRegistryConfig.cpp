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
static const TCHAR *kRootKeyNameForDirectory = _T("Directory");

static CSysString GetFullContextMenuKeyName(const CSysString &keyName)
  { return (keyName + kContextMenuKeyName); }

static bool CheckContextMenuHandlerCommon(const CSysString &keyName)
{
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey key;
  if (key.Open(HKEY_CLASSES_ROOT, GetFullContextMenuKeyName(keyName), KEY_READ)
      != ERROR_SUCCESS)
    return false;
  CSysString value;
  if (key.QueryValue(NULL, value) != ERROR_SUCCESS)
    return false;
  return (value.CollateNoCase(kContextMenuHandlerCLASSIDValue) == 0);
}

bool CheckContextMenuHandler()
{ 
  return CheckContextMenuHandlerCommon(kRootKeyNameForFile) &&
    CheckContextMenuHandlerCommon(kRootKeyNameForFolder) &&
    CheckContextMenuHandlerCommon(kRootKeyNameForDirectory);
}

static void DeleteContextMenuHandlerCommon(const CSysString &keyName)
{
  CKey rootKey;
  rootKey.Attach(HKEY_CLASSES_ROOT);
  rootKey.RecurseDeleteKey(GetFullContextMenuKeyName(keyName));
  rootKey.Detach();
}

void DeleteContextMenuHandler()
{ 
  DeleteContextMenuHandlerCommon(kRootKeyNameForFile); 
  DeleteContextMenuHandlerCommon(kRootKeyNameForFolder);
  DeleteContextMenuHandlerCommon(kRootKeyNameForDirectory);
}

static void AddContextMenuHandlerCommon(const CSysString &keyName)
{
  DeleteContextMenuHandlerCommon(keyName);
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey key;
  key.Create(HKEY_CLASSES_ROOT, GetFullContextMenuKeyName(keyName));
  key.SetValue(NULL, kContextMenuHandlerCLASSIDValue);
}

void AddContextMenuHandler()
{ 
  AddContextMenuHandlerCommon(kRootKeyNameForFile); 
  AddContextMenuHandlerCommon(kRootKeyNameForFolder);
  AddContextMenuHandlerCommon(kRootKeyNameForDirectory);
}

}
