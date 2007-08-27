// PluginLoader.h

#ifndef __PLUGINLOADER_H
#define __PLUGINLOADER_H

#include "Windows/DLL.h"

typedef UINT32 (WINAPI * CreateObjectPointer)(
    const GUID *clsID, 
    const GUID *interfaceID, 
    void **outObject);

class CPluginLibrary: public NWindows::NDLL::CLibrary 
{
public:
  HRESULT CreateManager(REFGUID clsID, IFolderManager **manager)
  {
    CreateObjectPointer createObject = (CreateObjectPointer)
        GetProcAddress("CreateObject");
    if (createObject == NULL)
      return GetLastError();
    return createObject(&clsID, &IID_IFolderManager, (void **)manager);
  }
  HRESULT LoadAndCreateManager(LPCWSTR filePath, REFGUID clsID, IFolderManager **manager)
  {
    if (!Load(filePath))
      return GetLastError();
    return CreateManager(clsID, manager);
  }
};

#endif
