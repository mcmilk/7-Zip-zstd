// ZipRegistryConfig.h

#pragma once

#ifndef __ZIPREGISTRYCONFIG_H
#define __ZIPREGISTRYCONFIG_H

#include "ZipRegistryMain.h"

namespace NZipRootRegistry {


  bool CheckShellExtensionInfo(const CSysString &anExtension);

  void AddShellExtensionInfo(const CSysString &anExtension,
      const CSysString &aProgramTitle, const CLSID &aClassID,
      const void *aShellNewData, int aShellNewDataSize);

  // void ReadCompressionInfo(NZipSettings::NCompression::CInfo &anInfo, 
  void DeleteShellExtensionInfo(const CSysString &anExtension);

  ///////////////////////////
  // ContextMenu

  bool CheckContextMenuHandler();
  void AddContextMenuHandler();
  void DeleteContextMenuHandler();

}

bool GetProgramDirPrefix(CSysString &aFolder);


#endif