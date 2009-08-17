// RegistryContextMenu.cpp

#include "StdAfx.h"

#include "Windows/Registry.h"
#include "Windows/Synchronization.h"

#include "RegistryContextMenu.h"

using namespace NWindows;
using namespace NRegistry;

namespace NZipRootRegistry {
  
#ifndef UNDER_CE

static NSynchronization::CCriticalSection g_CS;
  
static const TCHAR *kContextMenuKeyName  = TEXT("\\shellex\\ContextMenuHandlers\\7-Zip");
static const TCHAR *kDragDropMenuKeyName = TEXT("\\shellex\\DragDropHandlers\\7-Zip");

static const TCHAR *kExtensionCLSID = TEXT("{23170F69-40C1-278A-1000-000100020000}");

static const TCHAR *kRootKeyNameForFile = TEXT("*");
static const TCHAR *kRootKeyNameForFolder = TEXT("Folder");
static const TCHAR *kRootKeyNameForDirectory = TEXT("Directory");
static const TCHAR *kRootKeyNameForDrive = TEXT("Drive");

static CSysString GetFullContextMenuKeyName(const CSysString &keyName)
  { return (keyName + kContextMenuKeyName); }

static CSysString GetFullDragDropMenuKeyName(const CSysString &keyName)
  { return (keyName + kDragDropMenuKeyName); }

static bool CheckHandlerCommon(const CSysString &keyName)
{
  NSynchronization::CCriticalSectionLock lock(g_CS);
  CKey key;
  if (key.Open(HKEY_CLASSES_ROOT, keyName, KEY_READ) != ERROR_SUCCESS)
    return false;
  CSysString value;
  if (key.QueryValue(NULL, value) != ERROR_SUCCESS)
    return false;
  value.MakeUpper();
  return (value.Compare(kExtensionCLSID) == 0);
}

bool CheckContextMenuHandler()
{
  return
    // CheckHandlerCommon(GetFullContextMenuKeyName(kRootKeyNameForFolder)) &&
    CheckHandlerCommon(GetFullContextMenuKeyName(kRootKeyNameForDirectory))  &&
    CheckHandlerCommon(GetFullContextMenuKeyName(kRootKeyNameForFile)) &&
    CheckHandlerCommon(GetFullDragDropMenuKeyName(kRootKeyNameForDirectory))  &&
    CheckHandlerCommon(GetFullDragDropMenuKeyName(kRootKeyNameForDrive));
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
  NSynchronization::CCriticalSectionLock lock(g_CS);
  CKey key;
  key.Create(HKEY_CLASSES_ROOT, GetFullContextMenuKeyName(keyName));
  key.SetValue(NULL, kExtensionCLSID);
}

static void AddDragDropMenuHandlerCommon(const CSysString &keyName)
{
  DeleteDragDropMenuHandlerCommon(keyName);
  NSynchronization::CCriticalSectionLock lock(g_CS);
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

#endif

}
