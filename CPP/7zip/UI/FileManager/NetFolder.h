// NetFolder.h

#ifndef __NETFOLDER_H
#define __NETFOLDER_H

#include "Common/MyString.h"
#include "Common/Buffer.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "Windows/Net.h"

#include "IFolder.h"

struct CResourceEx: public NWindows::NNet::CResourceW
{
  UString Name;
};

class CNetFolder: 
  public IFolderFolder,
  public IFolderGetSystemIconIndex,
  public CMyUnknownImp
{
public:
  MY_UNKNOWN_IMP1(
    IFolderGetSystemIconIndex
  )

  INTERFACE_FolderFolder(;)

  STDMETHOD(GetSystemIconIndex)(UInt32 index, INT32 *iconIndex);

private:
  NWindows::NNet::CResourceW _netResource;
  NWindows::NNet::CResourceW *_netResourcePointer;

  CObjectVector<CResourceEx> _items;

  CMyComPtr<IFolderFolder> _parentFolder;
  UString _path;
  
public:
  void Init(const UString &path);
  void Init(const NWindows::NNet::CResourceW *netResource, 
      IFolderFolder *parentFolder, const UString &path);
  CNetFolder(): _netResourcePointer(0) {}
};

#endif
