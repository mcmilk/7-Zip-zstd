// FilePlugins.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Common/MyCom.h"

#include "IFolder.h"
#include "FilePlugins.h"
#include "StringUtils.h"
#include "PluginLoader.h"
#include "../Agent/Agent.h"

using namespace NRegistryAssociations;

int CExtDatabase::FindExtInfoBig(const UString &ext)
{
  for (int i = 0; i < ExtBigItems.Size(); i++)
    if (ExtBigItems[i].Ext.CompareNoCase(ext) == 0)
      return i;
  return -1;
}

int CExtDatabase::FindPlugin(const UString &plugin)
{
  for (int i = 0; i < Plugins.Size(); i++)
    if (Plugins[i].Name.CompareNoCase(plugin) == 0)
      return i;
  return -1;
}

void CExtDatabase::Read()
{
  CObjectVector<CExtInfo> extItems;
  ReadInternalAssociations(extItems);
  ReadFileFolderPluginInfoList(Plugins);
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
    ExtBigItems.Add(extInfoBig);
  }
  for (int pluginIndex = 0; pluginIndex < Plugins.Size(); pluginIndex++)
  {
    const CPluginInfo &pluginInfo = Plugins[pluginIndex];

    CPluginLibrary pluginLibrary;
    CMyComPtr<IFolderManager> folderManager;

    if (pluginInfo.FilePath.IsEmpty())
      folderManager = new CArchiveFolderManager;
    else if (pluginLibrary.LoadAndCreateManager(pluginInfo.FilePath, 
         pluginInfo.ClassID, &folderManager) != S_OK)
      continue;
    CMyComBSTR extBSTR;
    if (folderManager->GetExtensions(&extBSTR) != S_OK)
      return;
    const UString ext2 = (const wchar_t *)extBSTR;
    UStringVector exts;
    SplitString(ext2, exts);
    for (int i = 0; i < exts.Size(); i++)
    {
      const UString &ext = exts[i];
      int index = FindExtInfoBig(ext);
      if (index < 0)
      {
        CExtInfoBig extInfo;
        extInfo.PluginsPairs.Add(CPluginEnabledPair(pluginIndex, false));
        extInfo.Associated = false;
        extInfo.Ext = ext;
        ExtBigItems.Add(extInfo);
      }
      else
      {
        CExtInfoBig &extInfo = ExtBigItems[index];
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
  for (int i = 0; i < ExtBigItems.Size(); i++)
  {
    const CExtInfoBig &extInfoBig = ExtBigItems[i];
    CExtInfo extInfo;
    // extInfo.Enabled = extInfoBig.Associated;
    extInfo.Ext = extInfoBig.Ext;
    for (int p = 0; p < extInfoBig.PluginsPairs.Size(); p++)
    {
      CPluginEnabledPair pluginPair = extInfoBig.PluginsPairs[p];
      if (pluginPair.Enabled)
        extInfo.Plugins.Add(Plugins[pluginPair.Index].Name);
    }
    extItems.Add(extInfo);
  }
  WriteInternalAssociations(extItems);
}


