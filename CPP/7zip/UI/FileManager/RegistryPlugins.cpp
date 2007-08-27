// RegistryPlugins.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
// #include "Windows/Registry.h"
// #include "Windows/Synchronization.h"

#include "Windows/DLL.h"
#include "Windows/PropVariant.h"
#include "Windows/FileFind.h"

#include "RegistryPlugins.h"
#include "IFolder.h"

using namespace NWindows;
using namespace NFile;
// using namespace NRegistry;
// using namespace NCOM;

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

static NSynchronization::CCriticalSection g_CriticalSection;
*/
typedef UINT32 (WINAPI * GetPluginPropertyFunc)(
    PROPID propID, PROPVARIANT *value);

static bool ReadPluginInfo(CPluginInfo &pluginInfo)
{
  {
    NDLL::CLibrary library;
    if (!library.LoadEx(pluginInfo.FilePath, LOAD_LIBRARY_AS_DATAFILE))
      return false;
  }
  NDLL::CLibrary library;
  if (!library.Load(pluginInfo.FilePath))
    return false;
  GetPluginPropertyFunc getPluginProperty = (GetPluginPropertyFunc)
    library.GetProcAddress("GetPluginProperty");
  if (getPluginProperty == NULL)
    return false;
  
  NCOM::CPropVariant propVariant;
  if (getPluginProperty(NPlugin::kName, &propVariant) != S_OK)
    return false;
  if (propVariant.vt != VT_BSTR)
    return false;
  pluginInfo.Name = propVariant.bstrVal;
  propVariant.Clear();
  
  if (getPluginProperty(NPlugin::kClassID, &propVariant) != S_OK)
    return false;
  if (propVariant.vt == VT_EMPTY)
    pluginInfo.ClassIDDefined = false;
  else if (propVariant.vt != VT_BSTR)
    return false;
  else
  {
    pluginInfo.ClassIDDefined = true;
    pluginInfo.ClassID = *(const GUID *)propVariant.bstrVal;
  }
  propVariant.Clear();
  
  if (getPluginProperty(NPlugin::kOptionsClassID, &propVariant) != S_OK)
    return false;
  if (propVariant.vt == VT_EMPTY)
    pluginInfo.OptionsClassIDDefined = false;
  else if (propVariant.vt != VT_BSTR)
    return false;
  else
  {
    pluginInfo.OptionsClassIDDefined = true;
    pluginInfo.OptionsClassID = *(const GUID *)propVariant.bstrVal;
  }
  propVariant.Clear();

  if (getPluginProperty(NPlugin::kType, &propVariant) != S_OK)
    return false;
  if (propVariant.vt == VT_EMPTY)
    pluginInfo.Type = kPluginTypeFF;
  else if (propVariant.vt == VT_UI4)
    pluginInfo.Type = (EPluginType)propVariant.ulVal;
  else
    return false;
  return true;
}

UString GetProgramFolderPrefix();

void ReadPluginInfoList(CObjectVector<CPluginInfo> &plugins)
{
  plugins.Clear();

  UString baseFolderPrefix = GetProgramFolderPrefix();
  {
    CPluginInfo pluginInfo;
    pluginInfo.FilePath = baseFolderPrefix + L"7-zip.dll";
    if (::ReadPluginInfo(pluginInfo))
      plugins.Add(pluginInfo);
  }
  UString folderPath = baseFolderPrefix + L"Plugins\\";
  NFind::CEnumeratorW enumerator(folderPath + L"*");
  NFind::CFileInfoW fileInfo;
  while (enumerator.Next(fileInfo))
  {
    if (fileInfo.IsDirectory())
      continue;
    CPluginInfo pluginInfo;
    pluginInfo.FilePath = folderPath + fileInfo.Name;
    if (::ReadPluginInfo(pluginInfo))
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
