// RegistryPlugins.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"

#include "Windows/Registry.h"
#include "Windows/Synchronization.h"
#include "Windows/COM.h"


#include "RegistryPlugins.h"

using namespace NWindows;
using namespace NRegistry;
using namespace NCOM;

static const TCHAR *kLMBasePath = _T("Software\\7-Zip\\FM");

static const TCHAR *kPluginsKeyName = _T("Plugins");

static const TCHAR *kPluginsOpenClassIDValue = _T("CLSID");
static const TCHAR *kPluginsOptionsClassIDValue = _T("Options");
static const TCHAR *kPluginsTypeValue = _T("Type");

static CSysString GetFileFolderPluginsKeyName()
{
  return CSysString(kLMBasePath) + CSysString(TEXT('\\')) + 
      CSysString(kPluginsKeyName);
}

static NSynchronization::CCriticalSection g_CriticalSection;

void ReadPluginInfoList(CObjectVector<CPluginInfo> &plugins)
{
  plugins.Clear();
  
  NSynchronization::CSingleLock aLock(&g_CriticalSection, true);

  CKey aPluginsKey;
  if(aPluginsKey.Open(HKEY_LOCAL_MACHINE, GetFileFolderPluginsKeyName(), 
      KEY_READ) != ERROR_SUCCESS)
    return;
 
  CSysStringVector pluginsNames;
  aPluginsKey.EnumKeys(pluginsNames);
  for(int i = 0; i < pluginsNames.Size(); i++)
  {
    const CSysString pluginName = pluginsNames[i];
    CPluginInfo plugin;
    plugin.Name = GetUnicodeString(pluginName);
    CKey pluginKey;
    if(pluginKey.Open(aPluginsKey, pluginName, KEY_READ) != ERROR_SUCCESS)
      return;
    
    CSysString classID;
    if (pluginKey.QueryValue(kPluginsOpenClassIDValue, classID) != ERROR_SUCCESS)
      continue;
    if(StringToGUID(classID, plugin.ClassID) != NOERROR)
      continue;
    plugin.OptionsClassIDDefined = false;
    if (pluginKey.QueryValue(kPluginsOptionsClassIDValue, classID) == ERROR_SUCCESS)
    {
      plugin.OptionsClassIDDefined = 
          (StringToGUID(classID, plugin.OptionsClassID) == NOERROR);
    }
    plugin.Type = kPluginTypeFF;
    UINT32 type;
    if (pluginKey.QueryValue(kPluginsTypeValue, type) == ERROR_SUCCESS)
    {
      if (type <= kPluginTypeFF)
        plugin.Type = EPluginType(type);
    }
    plugins.Add(plugin);
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
}

