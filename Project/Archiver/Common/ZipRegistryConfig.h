// ZipRegistryConfig.h

#pragma once

#ifndef __ZIPREGISTRYCONFIG_H
#define __ZIPREGISTRYCONFIG_H

#include "ZipRegistryMain.h"

namespace NZipRootRegistry {

  ///////////////////////////
  // ContextMenu

  bool CheckContextMenuHandler();
  void AddContextMenuHandler();
  void DeleteContextMenuHandler();

}

bool GetProgramDirPrefix(CSysString &folder);


#endif