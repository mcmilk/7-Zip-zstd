// HandlerLoader.h

#ifndef __HANDLERLOADER_H
#define __HANDLERLOADER_H

#include "../../ICoder.h"
#include "Windows/DLL.h"

typedef UInt32 (WINAPI * CreateObjectFunc)(
    const GUID *clsID, 
    const GUID *interfaceID, 
    void **outObject);

class CHandlerLoader: public NWindows::NDLL::CLibrary
{
public:
  HRESULT CreateHandler(LPCWSTR filepath, REFGUID clsID, 
      void **archive, bool outHandler)
  {
    if (!Load(filepath))
      return GetLastError();
    CreateObjectFunc createObject = (CreateObjectFunc)
        GetProcAddress("CreateObject");
    if (createObject == NULL)
    {
      HRESULT res = ::GetLastError();
      Free();
      return res;
    }
    HRESULT res = createObject(&clsID, 
        outHandler ? &IID_IOutArchive : &IID_IInArchive, (void **)archive);
    if (res != 0)
      Free();
    return res;
  }
};

#endif
