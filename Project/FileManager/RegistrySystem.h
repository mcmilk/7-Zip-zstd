// ZipRegistryMain.h

#pragma once

#ifndef __ZIPREGISTRYMAIN_H
#define __ZIPREGISTRYMAIN_H

#include "ZipSettings.h"

namespace NZipRootRegistry {

  struct CArchiverInfo
  {
    #ifndef NO_REGISTRY
    CLSID ClassID;
    #endif
    CSysString Name;
    CSysString Extension;
    CSysString AddExtension;
    bool UpdateEnabled;
    bool KeepName;
  };

  void ReadArchiverInfoList(CObjectVector<CArchiverInfo> &anInfoList);
}


#endif