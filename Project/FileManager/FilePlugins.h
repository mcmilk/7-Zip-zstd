// FilePlugins.h

#pragma once 

#ifndef __FILEPLUGINS_H
#define __FILEPLUGINS_H

#include "RegistryPlugins.h"
#include "RegistryAssociations.h"

struct CPluginEnabledPair
{
  int Index;
  bool Enabled;
  CPluginEnabledPair(int index, bool enabled): Index(index),Enabled(enabled) {}
};

struct CExtInfoBig
{
  UString Ext;
  bool Associated;
  CRecordVector<CPluginEnabledPair> PluginsPairs;
  int FindPlugin(int pluginIndex)
  {
    for (int i = 0; i < PluginsPairs.Size(); i++)
      if (PluginsPairs[i].Index == pluginIndex)
        return i;
      return -1;
  }
};

class CExtDatabase
{
public:
  CObjectVector<CExtInfoBig> _extBigItems;
  CObjectVector<CPluginInfo> _plugins;
  int FindExtInfoBig(const UString &ext);
  int FindPlugin(const UString &plugin);

  UString GetMainPluginNameForExtItem(int extIndex) const
  {
    const CExtInfoBig &extInfo = _extBigItems[extIndex];
    if (extInfo.PluginsPairs.IsEmpty())
      return UString();
    else
      return _plugins[extInfo.PluginsPairs.Front().Index].Name;
  }

  void Read();
  void Save();
};


#endif


