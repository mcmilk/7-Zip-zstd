// FilePlugins.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"

#include "FilePlugins.h"
#include "FolderInterface.h"
#include "StringUtils.h"

using namespace NRegistryAssociations;

int CExtDatabase::FindExtInfoBig(const UString &ext)
{
  for (int i = 0; i < _extBigItems.Size(); i++)
    if (_extBigItems[i].Ext.CompareNoCase(ext) == 0)
      return i;
  return -1;
}

int CExtDatabase::FindPlugin(const UString &plugin)
{
  for (int i = 0; i < _plugins.Size(); i++)
    if (_plugins[i].Name.CompareNoCase(plugin) == 0)
      return i;
  return -1;
}

void CExtDatabase::Read()
{
  CObjectVector<CExtInfo> extItems;
  ReadInternalAssociations(extItems);
  ReadFileFolderPluginInfoList(_plugins);
  for (int i = 0; i < extItems.Size(); i++)
  {
    const CExtInfo &extInfo = extItems[i];
    CExtInfoBig extInfoBig;
    extInfoBig.Ext = extInfo.Ext;
    extInfoBig.Associated = false;
    for (int p = 0; p < extInfo.Plugins.Size(); p++)
    {
      int pluginIndex = FindPlugin(extInfo.Plugins[p]);
      if (pluginIndex >= 0)
        extInfoBig.PluginsPairs.Add(CPluginEnabledPair(pluginIndex, true));
    }
    _extBigItems.Add(extInfoBig);
  }
  for (int pluginIndex = 0; pluginIndex < _plugins.Size(); pluginIndex++)
  {
    const CPluginInfo &pluginInfo = _plugins[pluginIndex];
    if (!pluginInfo.OptionsClassIDDefined)
      continue;
    CComPtr<IFolderManager> folderManager;
    if (folderManager.CoCreateInstance(pluginInfo.ClassID) != S_OK)
      continue;
    CComBSTR typesString;
    if (folderManager->GetTypes(&typesString) != S_OK)
      continue;
    UStringVector types;
    SplitString((const wchar_t *)typesString, types);
    for (int typeIndex = 0; typeIndex < types.Size(); typeIndex++)
    {
      CComBSTR extTemp;
      if (folderManager->GetExtension(types[typeIndex], &extTemp) != S_OK)
        continue;
      const UString ext = (const wchar_t *)extTemp;
      int index = FindExtInfoBig(ext);
      if (index < 0)
      {
        CExtInfoBig extInfo;
        extInfo.PluginsPairs.Add(CPluginEnabledPair(pluginIndex, false));
        extInfo.Associated = false;
        extInfo.Ext = ext;
        _extBigItems.Add(extInfo);
      }
      else
      {
        CExtInfoBig &extInfo = _extBigItems[index];
        int pluginIndexIndex = extInfo.FindPlugin(pluginIndex);
        if (pluginIndexIndex < 0)
          extInfo.PluginsPairs.Add(CPluginEnabledPair(pluginIndex, false));
      }
    }
  }
}

void CExtDatabase::Save()
{
  CObjectVector<CExtInfo> extItems;
  for (int i = 0; i < _extBigItems.Size(); i++)
  {
    const CExtInfoBig &extInfoBig = _extBigItems[i];
    CExtInfo extInfo;
    // extInfo.Enabled = extInfoBig.Associated;
    extInfo.Ext = extInfoBig.Ext;
    for (int p = 0; p < extInfoBig.PluginsPairs.Size(); p++)
    {
      CPluginEnabledPair pluginPair = extInfoBig.PluginsPairs[p];
      if (pluginPair.Enabled)
        extInfo.Plugins.Add(_plugins[pluginPair.Index].Name);
    }
    extItems.Add(extInfo);
  }
  WriteInternalAssociations(extItems);
}


