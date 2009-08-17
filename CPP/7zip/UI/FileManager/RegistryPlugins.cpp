// RegistryPlugins.cpp

#include "StdAfx.h"

#include "Windows/DLL.h"
#include "Windows/PropVariant.h"
#include "Windows/FileFind.h"

#include "ProgramLocation.h"
#include "RegistryPlugins.h"
#include "IFolder.h"

using namespace NWindows;
using namespace NFile;

/*
static const TCHAR *kLMBasePath = TEXT("Software\\7-Zip\\FM");

static const TCHAR *kPluginsKeyName = TEXT("Plugins");
static const TCHAR *kPluginsOpenClassIDValue = TEXT("CLSID");
static const TCHAR *kPluginsOptionsClassIDValue = TEXT("Options");
static const TCHAR *kPluginsTypeValue = TEXT("Type");

static CSysString GetFileFolderPluginsKeyName()
{
  return CSysString(kLMBasePath) + CSysString(TEXT('\\')) +
      CSysString(kPluginsKeyName);
}

*/

typedef UINT32 (WINAPI * GetPluginPropertyFunc)(PROPID propID, PROPVARIANT *value);

static bool ReadPluginInfo(CPluginInfo &pluginInfo, bool needCheckDll)
{
  if (needCheckDll)
  {
    NDLL::CLibrary lib;
    if (!lib.LoadEx(pluginInfo.FilePath, LOAD_LIBRARY_AS_DATAFILE))
      return false;
  }
  NDLL::CLibrary lib;
  if (!lib.Load(pluginInfo.FilePath))
    return false;
  GetPluginPropertyFunc getPluginProperty = (GetPluginPropertyFunc)lib.GetProc("GetPluginProperty");
  if (getPluginProperty == NULL)
    return false;
  
  NCOM::CPropVariant prop;
  if (getPluginProperty(NPlugin::kName, &prop) != S_OK)
    return false;
  if (prop.vt != VT_BSTR)
    return false;
  pluginInfo.Name = prop.bstrVal;
  prop.Clear();
  
  if (getPluginProperty(NPlugin::kClassID, &prop) != S_OK)
    return false;
  if (prop.vt == VT_EMPTY)
    pluginInfo.ClassIDDefined = false;
  else if (prop.vt != VT_BSTR)
    return false;
  else
  {
    pluginInfo.ClassIDDefined = true;
    pluginInfo.ClassID = *(const GUID *)prop.bstrVal;
  }
  prop.Clear();
  
  if (getPluginProperty(NPlugin::kOptionsClassID, &prop) != S_OK)
    return false;
  if (prop.vt == VT_EMPTY)
    pluginInfo.OptionsClassIDDefined = false;
  else if (prop.vt != VT_BSTR)
    return false;
  else
  {
    pluginInfo.OptionsClassIDDefined = true;
    pluginInfo.OptionsClassID = *(const GUID *)prop.bstrVal;
  }
  prop.Clear();

  if (getPluginProperty(NPlugin::kType, &prop) != S_OK)
    return false;
  if (prop.vt == VT_EMPTY)
    pluginInfo.Type = kPluginTypeFF;
  else if (prop.vt == VT_UI4)
    pluginInfo.Type = (EPluginType)prop.ulVal;
  else
    return false;
  return true;
}

UString GetProgramFolderPrefix();

void ReadPluginInfoList(CObjectVector<CPluginInfo> &plugins)
{
  plugins.Clear();

  UString baseFolderPrefix;
  GetProgramFolderPath(baseFolderPrefix);
  {
    CPluginInfo pluginInfo;
    pluginInfo.FilePath = baseFolderPrefix + L"7-zip.dll";
    if (::ReadPluginInfo(pluginInfo, false))
      plugins.Add(pluginInfo);
  }
  UString folderPath = baseFolderPrefix + L"Plugins" WSTRING_PATH_SEPARATOR;
  NFind::CEnumeratorW enumerator(folderPath + L"*");
  NFind::CFileInfoW fileInfo;
  while (enumerator.Next(fileInfo))
  {
    if (fileInfo.IsDir())
      continue;
    CPluginInfo pluginInfo;
    pluginInfo.FilePath = folderPath + fileInfo.Name;
    if (::ReadPluginInfo(pluginInfo, true))
      plugins.Add(pluginInfo);
  }
}

void ReadFileFolderPluginInfoList(CObjectVector<CPluginInfo> &plugins)
{
  ReadPluginInfoList(plugins);
  for (int i = 0; i < plugins.Size();)
    if (plugins[i].Type != kPluginTypeFF)
      plugins.Delete(i);
    else
      i++;
  {
    CPluginInfo p;
    // p.FilePath.Empty();
    p.Type = kPluginTypeFF;
    p.Name = L"7-Zip";
    // p.ClassID = CLSID_CAgentArchiveHandler;
    p.ClassIDDefined = true;
    // p.OptionsClassID;
    p.OptionsClassIDDefined = false;
    plugins.Add(p);
  }
}
