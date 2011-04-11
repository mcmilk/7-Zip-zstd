// FilePlugins.cpp

#include "StdAfx.h"

#include "../Agent/Agent.h"

#include "FilePlugins.h"
#include "PluginLoader.h"
#include "StringUtils.h"

int CExtDatabase::FindExt(const UString &ext)
{
  for (int i = 0; i < Exts.Size(); i++)
    if (Exts[i].Ext.CompareNoCase(ext) == 0)
      return i;
  return -1;
}

void CExtDatabase::Read()
{
  ReadFileFolderPluginInfoList(Plugins);
  for (int pluginIndex = 0; pluginIndex < Plugins.Size(); pluginIndex++)
  {
    const CPluginInfo &plugin = Plugins[pluginIndex];

    CPluginLibrary pluginLib;
    CMyComPtr<IFolderManager> folderManager;

    if (plugin.FilePath.IsEmpty())
      folderManager = new CArchiveFolderManager;
    else if (pluginLib.LoadAndCreateManager(plugin.FilePath, plugin.ClassID, &folderManager) != S_OK)
      continue;
    CMyComBSTR extBSTR;
    if (folderManager->GetExtensions(&extBSTR) != S_OK)
      return;
    UStringVector exts;
    SplitString((const wchar_t *)extBSTR, exts);
    for (int i = 0; i < exts.Size(); i++)
    {
      const UString &ext = exts[i];
      #ifdef UNDER_CE
      if (ext == L"cab")
        continue;
      #endif

      Int32 iconIndex;
      CMyComBSTR iconPath;
      CPluginToIcon plugPair;
      plugPair.PluginIndex = pluginIndex;
      if (folderManager->GetIconPath(ext, &iconPath, &iconIndex) == S_OK)
        if (iconPath != 0)
        {
          plugPair.IconPath = (const wchar_t *)iconPath;
          plugPair.IconIndex = iconIndex;
        }

      int index = FindExt(ext);
      if (index >= 0)
        Exts[index].Plugins.Add(plugPair);
      else
      {
        CExtPlugins extInfo;
        extInfo.Plugins.Add(plugPair);
        extInfo.Ext = ext;
        Exts.Add(extInfo);
      }
    }
  }
}
