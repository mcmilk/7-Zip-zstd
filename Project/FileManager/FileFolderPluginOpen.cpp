// FileFolderPluginOpen.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"

#include "Windows/Defs.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"

#include "FolderInterface.h"
#include "RegistryAssociations.h"
#include "RegistryPlugins.h"

using namespace NWindows;
using namespace NRegistryAssociations;

static int FindPlugin(const CObjectVector<CPluginInfo> &plugins, 
    const UString &pluginName)
{
  for (int i = 0; i < plugins.Size(); i++)
    if (plugins[i].Name.CompareNoCase(pluginName) == 0)
      return i;
  return -1;
}


HRESULT OpenFileFolderPlugin(const UString &path, IFolderFolder **resultFolder)
{
  CObjectVector<CPluginInfo> plugins;
  ReadFileFolderPluginInfoList(plugins);

  CSysString extension;
  CSysString name, pureName, dot;
  if(!NFile::NDirectory::GetOnlyName(GetSystemString(path), name))
    return E_FAIL;
  NFile::NName::SplitNameToPureNameAndExtension(name, pureName, dot, extension);
  if (!extension.IsEmpty())
  {
    CExtInfo extInfo;
    if (ReadInternalAssociation(extension, extInfo))
    {
      for (int i = extInfo.Plugins.Size() - 1; i >= 0; i--)
      {
        int pluginIndex = FindPlugin(plugins, extInfo.Plugins[i]);
        if (pluginIndex >= 0)
        {
          const CPluginInfo plugin = plugins[pluginIndex];
          plugins.Delete(pluginIndex);
          plugins.Insert(0, plugin);
        }
      }
    }
  }

  for (int i = 0; i < plugins.Size(); i++)
  {
    const CPluginInfo &plugin = plugins[i];
    CComPtr<IFolderManager> folderManager;
    CComPtr<IFolderFolder> folder;
    HRESULT result = folderManager.CoCreateInstance(plugin.ClassID);
    if (result != S_OK)
      continue;
    result = folderManager->OpenFolderFile(path, &folder, NULL);
    if (result == S_OK)
    {
      *resultFolder = folder.Detach();
      return S_OK;
    }
    continue;

    /*
    if (result != S_FALSE)
      return result;
    */
  }
  return S_FALSE;
}