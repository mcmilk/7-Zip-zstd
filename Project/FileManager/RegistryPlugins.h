// RegistryPlugins.h

#pragma once

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
  UString Name;
  CLSID ClassID;
  CLSID OptionsClassID;
  bool OptionsClassIDDefined;
  EPluginType Type;

  // CSysString Extension;
  // CSysString AddExtension;
  // bool UpdateEnabled;
  // bool KeepName;
};

void ReadPluginInfoList(CObjectVector<CPluginInfo> &plugins);
void ReadFileFolderPluginInfoList(CObjectVector<CPluginInfo> &plugins);

#endif

