// SysIconUtils.h

#include "StdAfx.h"

#include "SysIconUtils.h"

int GetIconIndexForCSIDL(int aCSIDL)
{
  LPITEMIDLIST  pidlMyComputer = 0;
  SHGetSpecialFolderLocation(NULL, aCSIDL, &pidlMyComputer);
  if (pidlMyComputer)
  {
    SHFILEINFO shellInfo;
    SHGetFileInfo(LPCTSTR(pidlMyComputer),  FILE_ATTRIBUTE_NORMAL, 
      &shellInfo, sizeof(shellInfo), 
      SHGFI_PIDL | SHGFI_SYSICONINDEX);
    IMalloc  *pMalloc;
    SHGetMalloc(&pMalloc);
    if(pMalloc)
    {
      pMalloc->Free(pidlMyComputer);
      pMalloc->Release();
    }
    return shellInfo.iIcon;
  }
  return 0;
}

int GetRealIconIndex(UINT32 attributes, const CSysString &fileName)
{
  SHFILEINFO shellInfo;
  ::SHGetFileInfo(fileName, FILE_ATTRIBUTE_NORMAL | attributes, &shellInfo, 
      sizeof(shellInfo), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX);
  return shellInfo.iIcon;
}

int GetRealIconIndex(UINT32 attributes, const CSysString &fileName, 
    CSysString &typeName)
{
  SHFILEINFO shellInfo;
  ::SHGetFileInfo(fileName, FILE_ATTRIBUTE_NORMAL | attributes, &shellInfo, 
      sizeof(shellInfo), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX 
      | SHGFI_TYPENAME);
  typeName = shellInfo.szTypeName;
  return shellInfo.iIcon;
}

int CExtToIconMap::GetIconIndex(UINT32 attributes, const CSysString &fileName,
    CSysString &typeName)
{
  if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
  {
    if (_dirIconIndex < 0)
      _dirIconIndex = GetRealIconIndex(attributes, fileName, _dirTypeName);
    typeName = _dirTypeName;
    return _dirIconIndex;
  }
  int dotPos = fileName.ReverseFind('.');
  if (dotPos < 0)
  {
    if (_noExtIconIndex < 0)
      _noExtIconIndex = GetRealIconIndex(attributes, fileName, _noExtTypeName);
    typeName = _noExtTypeName;
    return _noExtIconIndex;
  }
  CExtIconPair extIconPair;
  extIconPair.Ext = fileName.Mid(dotPos + 1);
  int anIndex = _map.FindInSorted(extIconPair);
  if (anIndex >= 0)
    return _map[anIndex].IconIndex;
  extIconPair.IconIndex = GetRealIconIndex(attributes, fileName, extIconPair.TypeName);
  _map.AddToSorted(extIconPair);
  typeName = extIconPair.TypeName;
  return extIconPair.IconIndex;
}

int CExtToIconMap::GetIconIndex(UINT32 attributes, const CSysString &fileName)
{
  CSysString typeName;
  return GetIconIndex(attributes, fileName, typeName);
}
