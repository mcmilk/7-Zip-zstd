// RegistryAssociations.cpp

#include "StdAfx.h"

#include "RegistryAssociations.h"

#include "Common/StringConvert.h"

#include "Windows/COM.h"
#include "Windows/Synchronization.h"
#include "Windows/Registry.h"
#include "Windows/System.h"

#include "Windows/FileName.h"

#include "StringUtils.h"

using namespace NWindows;
using namespace NCOM;
using namespace NRegistry;


namespace NRegistryAssociations {
  
static NSynchronization::CCriticalSection g_CriticalSection;

static const TCHAR *kCUKeyPath = _T("Software\\7-ZIP\\FM");
static const TCHAR *kAssociations = _T("Associations");
static const TCHAR *kExtPlugins = _T("Plugins");
static const TCHAR *kExtEnabled = _T("Enabled");

bool ReadInternalAssociation(const TCHAR *ext, CExtInfo &extInfo)
{
  NSynchronization::CSingleLock lock(&g_CriticalSection, true);
  CKey key;
  if(key.Open(HKEY_CURRENT_USER, CSysString(kCUKeyPath) 
      + CSysString('\\') + CSysString(kAssociations)
      + CSysString('\\') + CSysString(ext), KEY_READ) != ERROR_SUCCESS)
    return false;
  CSysString pluginsString;
  key.QueryValue(kExtPlugins, pluginsString);
  SplitString(GetUnicodeString(pluginsString), extInfo.Plugins);
  return true;
}

void ReadInternalAssociations(CObjectVector<CExtInfo> &items)
{
  items.Clear();
  NSynchronization::CSingleLock lock(&g_CriticalSection, true);
  CKey associationsKey;
  if(associationsKey.Open(HKEY_CURRENT_USER, CSysString(kCUKeyPath) 
      + CSysString('\\') + CSysString(kAssociations), KEY_READ) != ERROR_SUCCESS)
    return;
  CSysStringVector extNames;
  associationsKey.EnumKeys(extNames);
  for(int i = 0; i < extNames.Size(); i++)
  {
    const CSysString extName = extNames[i];
    CExtInfo extInfo;
    // extInfo.Enabled = false;
    extInfo.Ext = GetUnicodeString(extName);
    CKey key;
    if(key.Open(associationsKey, extName, KEY_READ) != ERROR_SUCCESS)
      return;
    CSysString pluginsString;
    key.QueryValue(kExtPlugins, pluginsString);
    SplitString(GetUnicodeString(pluginsString), extInfo.Plugins);
    /*
    if (key.QueryValue(kExtEnabled, extInfo.Enabled) != ERROR_SUCCESS)
      extInfo.Enabled = false;
    */
    items.Add(extInfo);
  }
}

void WriteInternalAssociations(const CObjectVector<CExtInfo> &items)
{
  NSynchronization::CSingleLock lock(&g_CriticalSection, true);
  CKey mainKey;
  mainKey.Create(HKEY_CURRENT_USER, kCUKeyPath);
  mainKey.RecurseDeleteKey(kAssociations);
  CKey associationsKey;
  associationsKey.Create(mainKey, kAssociations);
  for(int i = 0; i < items.Size(); i++)
  {
    const CExtInfo &extInfo = items[i];
    CKey key;
    key.Create(associationsKey, GetSystemString(extInfo.Ext));
    key.SetValue(kExtPlugins, GetSystemString(JoinStrings(extInfo.Plugins)));
    // key.SetValue(kExtEnabled, extInfo.Enabled);
  }
}

///////////////////////////////////
// External 

static const TCHAR *kShellNewKeyName = _T("ShellNew");
static const TCHAR *kShellNewDataValueName = _T("Data");
  
static const TCHAR *kDefaultIconKeyName = _T("DefaultIcon");
static const TCHAR *kShellKeyName = _T("shell");
static const TCHAR *kOpenKeyName = _T("open");
static const TCHAR *kCommandKeyName = _T("command");

static const TCHAR *kOpenCommandValue = _T("7zFMn.exe \"%1\"");

static CSysString GetExtensionKeyName(const CSysString &extension)
{
  return CSysString(_T(".")) + extension;
}

static CSysString GetExtProgramKeyName(const CSysString &extension)
{
  return CSysString(_T("7-Zip.")) + extension;
}

bool CheckShellExtensionInfo(const CSysString &extension)
{
  NSynchronization::CSingleLock lock(&g_CriticalSection, true);
  CKey extKey;
  if (extKey.Open(HKEY_CLASSES_ROOT, GetExtensionKeyName(extension), KEY_READ) != ERROR_SUCCESS)
    return false;
  CSysString programNameValue;
  if (extKey.QueryValue(NULL, programNameValue) != ERROR_SUCCESS)
    return false;
  CSysString extProgramKeyName = GetExtProgramKeyName(extension);
  if (programNameValue.CollateNoCase(extProgramKeyName) != 0)
    return false;
  CKey extProgKey;
  return (extProgKey.Open(HKEY_CLASSES_ROOT, extProgramKeyName, KEY_READ) == ERROR_SUCCESS);
}

static void DeleteShellExtensionKey(const CSysString &extension)
{
  NSynchronization::CSingleLock lock(&g_CriticalSection, true);
  CKey rootKey;
  rootKey.Attach(HKEY_CLASSES_ROOT);
  rootKey.RecurseDeleteKey(GetExtensionKeyName(extension));
  rootKey.Detach();
}

static void DeleteShellExtensionProgramKey(const CSysString &extension)
{
  NSynchronization::CSingleLock lock(&g_CriticalSection, true);
  CKey rootKey;
  rootKey.Attach(HKEY_CLASSES_ROOT);
  rootKey.RecurseDeleteKey(GetExtProgramKeyName(extension));
  rootKey.Detach();
}

void DeleteShellExtensionInfo(const CSysString &extension)
{
  if (CheckShellExtensionInfo(extension))
    DeleteShellExtensionKey(extension);
  DeleteShellExtensionProgramKey(extension);
}

void AddShellExtensionInfo(const CSysString &extension,
    const CSysString &programTitle, 
    const CSysString &programOpenCommand, 
    const CSysString &iconPath,
    const void *shellNewData, int shellNewDataSize)
{
  DeleteShellExtensionKey(extension);
  DeleteShellExtensionProgramKey(extension);
  NSynchronization::CSingleLock lock(&g_CriticalSection, true);
  CSysString programKeyName = GetExtProgramKeyName(extension);
  {
    CKey extKey;
    extKey.Create(HKEY_CLASSES_ROOT, GetExtensionKeyName(extension));
    extKey.SetValue(NULL, programKeyName);
    if (shellNewData != NULL)
    {
      CKey shellNewKey;
      shellNewKey.Create(extKey, kShellNewKeyName);
      shellNewKey.SetValue(kShellNewDataValueName, shellNewData, shellNewDataSize);
    }
  }
  CKey programKey;
  programKey.Create(HKEY_CLASSES_ROOT, programKeyName);
  programKey.SetValue(NULL, programTitle);
  {
    CKey iconKey;
    iconKey.Create(programKey, kDefaultIconKeyName);
    iconKey.SetValue(NULL, iconPath);
  }

  CKey shellKey;
  shellKey.Create(programKey, kShellKeyName);
  shellKey.SetValue(NULL, _T(""));

  CKey openKey;
  openKey.Create(shellKey, kOpenKeyName);
  openKey.SetValue(NULL, _T(""));
  
  CKey commandKey;
  commandKey.Create(openKey, kCommandKeyName);

  CSysString params;
  /*
  if (!NSystem::MyGetWindowsDirectory(aParams))
  {
    aParams.Empty();
    // return;
  }
  else
    NFile::NName::NormalizeDirPathPrefix(aParams);
  */
  // aParams += kOpenCommandValue;
  HRESULT result = commandKey.SetValue(NULL, programOpenCommand);
}

///////////////////////////
// ContextMenu
/*

static const TCHAR *kContextMenuKeyName = _T("\\shellex\\ContextMenuHandlers\\7-ZIP");
static const TCHAR *kContextMenuHandlerCLASSIDValue = 
    _T("{23170F69-40C1-278A-1000-000100020000}");
static const TCHAR *kRootKeyNameForFile = _T("*");
static const TCHAR *kRootKeyNameForFolder = _T("Folder");

static CSysString GetFullContextMenuKeyName(const CSysString &aKeyName)
  { return (aKeyName + kContextMenuKeyName); }

static bool CheckContextMenuHandlerCommon(const CSysString &aKeyName)
{
  NSynchronization::CSingleLock lock(&g_CriticalSection, true);
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
  CKey rootKey;
  rootKey.Attach(HKEY_CLASSES_ROOT);
  rootKey.RecurseDeleteKey(GetFullContextMenuKeyName(aKeyName));
  rootKey.Detach();
}

void DeleteContextMenuHandler()
{ 
  DeleteContextMenuHandlerCommon(kRootKeyNameForFile); 
  DeleteContextMenuHandlerCommon(kRootKeyNameForFolder);
}

static void AddContextMenuHandlerCommon(const CSysString &aKeyName)
{
  DeleteContextMenuHandlerCommon(aKeyName);
  NSynchronization::CSingleLock lock(&g_CriticalSection, true);
  CKey aKey;
  aKey.Create(HKEY_CLASSES_ROOT, GetFullContextMenuKeyName(aKeyName));
  aKey.SetValue(NULL, kContextMenuHandlerCLASSIDValue);
}

void AddContextMenuHandler()
{ 
  AddContextMenuHandlerCommon(kRootKeyNameForFile); 
  AddContextMenuHandlerCommon(kRootKeyNameForFolder);
}
*/

}
