// CoderLoader.h

#ifndef __CODERLOADER_H
#define __CODERLOADER_H

#include "../../../Common/String.h"
#include "../../../Common/MyCom.h"
#include "../../../Windows/DLL.h"
#include "../../ICoder.h"

typedef UInt32 (WINAPI * CreateObjectPointer)(
    const GUID *clsID, 
    const GUID *interfaceID, 
    void **outObject);

class CCoderLibrary: public NWindows::NDLL::CLibrary 
{
public:
  HRESULT CreateObject(REFGUID clsID, REFGUID iid, void **obj)
  {
    CreateObjectPointer createObject = (CreateObjectPointer)
      GetProcAddress("CreateObject");
    if (createObject == NULL)
      return GetLastError();
    return createObject(&clsID, &iid, obj);
  }

  HRESULT CreateFilter(REFGUID clsID, ICompressFilter **filter)
  {
    return CreateObject(clsID, IID_ICompressFilter, (void **)filter);
  }

  HRESULT CreateCoder(REFGUID clsID, ICompressCoder **coder)
  {
    return CreateObject(clsID, IID_ICompressCoder, (void **)coder);
  }

  HRESULT CreateCoderSpec(REFGUID clsID, ICompressCoder **coder);
  
  HRESULT LoadAndCreateFilter(LPCTSTR filePath, REFGUID clsID, ICompressFilter **filter)
  {
    CCoderLibrary libTemp;
    if (!libTemp.Load(filePath))
      return GetLastError();
    RINOK(libTemp.CreateFilter(clsID, filter));
    Attach(libTemp.Detach());
    return S_OK;
  }
  

  HRESULT LoadAndCreateCoder(LPCTSTR filePath, REFGUID clsID, ICompressCoder **coder)
  {
    CCoderLibrary libTemp;
    if (!libTemp.Load(filePath))
      return GetLastError();
    RINOK(libTemp.CreateCoder(clsID, coder));
    Attach(libTemp.Detach());
    return S_OK;
  }

  HRESULT LoadAndCreateCoderSpec(LPCTSTR filePath, REFGUID clsID, ICompressCoder **coder);
  HRESULT CreateCoder2(REFGUID clsID, ICompressCoder2 **coder)
  {
    CreateObjectPointer createObject = (CreateObjectPointer)
        GetProcAddress("CreateObject");
    if (createObject == NULL)
      return GetLastError();
    return createObject(&clsID, &IID_ICompressCoder2, (void **)coder);
  }
  HRESULT LoadAndCreateCoder2(LPCTSTR filePath, REFGUID clsID, ICompressCoder2 **coder)
  {
    CCoderLibrary libTemp;
    if (!libTemp.Load(filePath))
      return GetLastError();
    RINOK(libTemp.CreateCoder2(clsID, coder));
    Attach(libTemp.Detach());
    return S_OK;
  }
};


class CCoderLibraries
{
  struct CPathToLibraryPair
  {
    CSysString Path;
    CCoderLibrary Libary;
  };
  CObjectVector<CPathToLibraryPair> Pairs;
public:
  int FindPath(LPCTSTR filePath)
  {
    for (int i = 0; i < Pairs.Size(); i++)
      if (Pairs[i].Path.CollateNoCase(filePath) == 0)
        return i;
    return -1;
  }

  HRESULT CreateCoder(LPCTSTR filePath, REFGUID clsID, ICompressCoder **coder)
  {
    int index = FindPath(filePath);
    if (index < 0)
    {
      CPathToLibraryPair pair;
      RINOK(pair.Libary.LoadAndCreateCoder(filePath, clsID, coder));
      pair.Path = filePath;
      Pairs.Add(pair);
      pair.Libary.Detach();
      return S_OK;
    }
    return Pairs[index].Libary.CreateCoder(clsID, coder);
  }

  HRESULT CreateCoderSpec(LPCTSTR filePath, REFGUID clsID, ICompressCoder **coder)
  {
    int index = FindPath(filePath);
    if (index < 0)
    {
      CPathToLibraryPair pair;
      RINOK(pair.Libary.LoadAndCreateCoderSpec(filePath, clsID, coder));
      pair.Path = filePath;
      Pairs.Add(pair);
      pair.Libary.Detach();
      return S_OK;
    }
    return Pairs[index].Libary.CreateCoderSpec(clsID, coder);
  }

  HRESULT CreateCoder2(LPCTSTR filePath, REFGUID clsID, ICompressCoder2 **coder)
  {
    int index = FindPath(filePath);
    if (index < 0)
    {
      CPathToLibraryPair pair;
      RINOK(pair.Libary.LoadAndCreateCoder2(filePath, clsID, coder));
      pair.Path = filePath;
      Pairs.Add(pair);
      pair.Libary.Detach();
      return S_OK;
    }
    return Pairs[index].Libary.CreateCoder2(clsID, coder);
  }
};


#endif

