// FileFolderPluginOpen.cpp

#include "StdAfx.h"

#include "Common/StringConvert.h"
#include "Windows/Defs.h"
#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/DLL.h"

#include "IFolder.h"
#include "RegistryAssociations.h"
#include "RegistryPlugins.h"

#include "OpenCallback.h"
#include "PluginLoader.h"
#include "../Agent/Agent.h"

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

HRESULT OpenFileFolderPlugin(
    const UString &path, 
    HMODULE *module,
    IFolderFolder **resultFolder, 
    HWND parentWindow, 
    bool &encrypted)
{
  encrypted = false;
  CObjectVector<CPluginInfo> plugins;
  ReadFileFolderPluginInfoList(plugins);

  UString extension;
  UString name, pureName, dot;

  if(!NFile::NDirectory::GetOnlyName(path, name))
    return E_FAIL;
  NFile::NName::SplitNameToPureNameAndExtension(name, pureName, dot, extension);


  int slashPos = path.ReverseFind(L'\\');
  UString dirPrefix;
  UString fileName;
  if (slashPos >= 0)
  {
    dirPrefix = path.Left(slashPos + 1);
    fileName = path.Mid(slashPos + 1);
  }
  else
    fileName = path;

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
    if (!plugin.ClassIDDefined)
      continue;
    CPluginLibrary library;
    CMyComPtr<IFolderManager> folderManager;
    CMyComPtr<IFolderFolder> folder;
    if (plugin.FilePath.IsEmpty())
      folderManager = new CArchiveFolderManager;
    else if (library.LoadAndCreateManager(plugin.FilePath, plugin.ClassID, &folderManager) != S_OK)
      continue;

    COpenArchiveCallback *openCallbackSpec = new COpenArchiveCallback;
    CMyComPtr<IProgress> openCallback = openCallbackSpec;
    openCallbackSpec->PasswordIsDefined = false;
    openCallbackSpec->ParentWindow = parentWindow;
    openCallbackSpec->LoadFileInfo(dirPrefix, fileName);
    HRESULT result = folderManager->OpenFolderFile(path, &folder, openCallback);
    if (openCallbackSpec->PasswordWasAsked)
      encrypted = true;
    if (result == S_OK)
    {
      *module = library.Detach();
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