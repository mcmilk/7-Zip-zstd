// RegistryPlugins.h

#ifndef __REGISTRYPLUGINS_H
#define __REGISTRYPLUGINS_H

#include "Common/Vector.h"
#include "Common/String.h"

enum EPluginType 
{
  kPluginTypeFF = 0
};

struct CPluginInfo
{
  CSysString FilePath;
  EPluginType Type;
  UString Name;
  CLSID ClassID;
  CLSID OptionsClassID;
  bool OptionsClassIDDefined;

  // CSysString Extension;
  // CSysString AddExtension;
  // bool UpdateEnabled;
  // bool KeepName;
};

void ReadPluginInfoList(CObjectVector<CPluginInfo> &plugins);
void ReadFileFolderPluginInfoList(CObjectVector<CPluginInfo> &plugins);

#endif
