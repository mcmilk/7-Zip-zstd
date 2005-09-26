// RegistryContextMenu.cpp

#include "StdAfx.h"

#include "RegistryContextMenu.h"
#include "Windows/COM.h"
#include "Windows/Synchronization.h"
#include "Windows/Registry.h"
#include "Windows/FileName.h"

using namespace NWindows;
using namespace NCOM;
using namespace NRegistry;

namespace NZipRootRegistry {
  
static NSynchronization::CCriticalSection g_RegistryOperationsCriticalSection;
  
///////////////////////////
// ContextMenu

static const TCHAR *kContextMenuKeyName  = TEXT("\\shellex\\ContextMenuHandlers\\7-ZIP");
static const TCHAR *kDragDropMenuKeyName = TEXT("\\shellex\\DragDropHandlers\\7-ZIP");

static const TCHAR *kExtensionCLSID = TEXT("{23170F69-40C1-278A-1000-000100020000}");

static const TCHAR *kRootKeyNameForFile = TEXT("*");
static const TCHAR *kRootKeyNameForFolder = TEXT("Folder");
static const TCHAR *kRootKeyNameForDirectory = TEXT("Directory");
static const TCHAR *kRootKeyNameForDrive = TEXT("Drive");

static CSysString GetFullContextMenuKeyName(const CSysString &keyName)
  { return (keyName + kContextMenuKeyName); }

static CSysString GetFullDragDropMenuKeyName(const CSysString &keyName)
  { return (keyName + kDragDropMenuKeyName); }

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
  return (value.CompareNoCase(kExtensionCLSID) == 0);
}

static bool CheckDragDropMenuHandlerCommon(const CSysString &keyName)
{
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey key;
  if (key.Open(HKEY_CLASSES_ROOT, GetFullDragDropMenuKeyName(keyName), KEY_READ) != ERROR_SUCCESS)
    return false;
  CSysString value;
  if (key.QueryValue(NULL, value) != ERROR_SUCCESS)
    return false;
  return (value.CompareNoCase(kExtensionCLSID) == 0);
}

bool CheckContextMenuHandler()
{ 
  return CheckContextMenuHandlerCommon(kRootKeyNameForFile) &&
    // CheckContextMenuHandlerCommon(kRootKeyNameForFolder) &&
    CheckContextMenuHandlerCommon(kRootKeyNameForDirectory)  &&
    CheckDragDropMenuHandlerCommon(kRootKeyNameForDirectory)  &&
    CheckDragDropMenuHandlerCommon(kRootKeyNameForDrive);
}

static void DeleteContextMenuHandlerCommon(const CSysString &keyName)
{
  CKey rootKey;
  rootKey.Attach(HKEY_CLASSES_ROOT);
  rootKey.RecurseDeleteKey(GetFullContextMenuKeyName(keyName));
  rootKey.Detach();
}

static void DeleteDragDropMenuHandlerCommon(const CSysString &keyName)
{
  CKey rootKey;
  rootKey.Attach(HKEY_CLASSES_ROOT);
  rootKey.RecurseDeleteKey(GetFullDragDropMenuKeyName(keyName));
  rootKey.Detach();
}

void DeleteContextMenuHandler()
{ 
  DeleteContextMenuHandlerCommon(kRootKeyNameForFile); 
  DeleteContextMenuHandlerCommon(kRootKeyNameForFolder);
  DeleteContextMenuHandlerCommon(kRootKeyNameForDirectory);
  DeleteContextMenuHandlerCommon(kRootKeyNameForDrive);
  DeleteDragDropMenuHandlerCommon(kRootKeyNameForFile); 
  DeleteDragDropMenuHandlerCommon(kRootKeyNameForFolder);
  DeleteDragDropMenuHandlerCommon(kRootKeyNameForDirectory);
  DeleteDragDropMenuHandlerCommon(kRootKeyNameForDrive);
}

static void AddContextMenuHandlerCommon(const CSysString &keyName)
{
  DeleteContextMenuHandlerCommon(keyName);
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey key;
  key.Create(HKEY_CLASSES_ROOT, GetFullContextMenuKeyName(keyName));
  key.SetValue(NULL, kExtensionCLSID);
}

static void AddDragDropMenuHandlerCommon(const CSysString &keyName)
{
  DeleteDragDropMenuHandlerCommon(keyName);
  NSynchronization::CCriticalSectionLock lock(g_RegistryOperationsCriticalSection);
  CKey key;
  key.Create(HKEY_CLASSES_ROOT, GetFullDragDropMenuKeyName(keyName));
  key.SetValue(NULL, kExtensionCLSID);
}

void AddContextMenuHandler()
{ 
  AddContextMenuHandlerCommon(kRootKeyNameForFile); 
  // AddContextMenuHandlerCommon(kRootKeyNameForFolder);
  AddContextMenuHandlerCommon(kRootKeyNameForDirectory);

  AddDragDropMenuHandlerCommon(kRootKeyNameForDirectory);
  AddDragDropMenuHandlerCommon(kRootKeyNameForDrive);
}

}
