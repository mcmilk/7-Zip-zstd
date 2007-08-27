// FilePlugins.h

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
  CObjectVector<CExtInfoBig> ExtBigItems;
  CObjectVector<CPluginInfo> Plugins;
  int FindExtInfoBig(const UString &ext);
  int FindPlugin(const UString &plugin);

  UString GetMainPluginNameForExtItem(int extIndex) const
  {
    const CExtInfoBig &extInfo = ExtBigItems[extIndex];
    if (extInfo.PluginsPairs.IsEmpty())
      return UString();
    else
      return Plugins[extInfo.PluginsPairs.Front().Index].Name;
  }

  void Read();
  void Save();
};


#endif


