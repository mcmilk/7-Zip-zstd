// SysIconUtils.h

#include "StdAfx.h"

#include "SysIconUtils.h"
#ifndef _UNICODE
#include "Common/StringConvert.h"
#endif

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

DWORD_PTR GetRealIconIndex(LPCTSTR path, UINT32 attributes, int &iconIndex)
{
  SHFILEINFO shellInfo;
  DWORD_PTR res = ::SHGetFileInfo(path, FILE_ATTRIBUTE_NORMAL | attributes, &shellInfo, 
      sizeof(shellInfo), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX);
  iconIndex = shellInfo.iIcon;
  return res;
}

#ifndef _UNICODE
static inline UINT GetCurrentCodePage() 
  { return ::AreFileApisANSI() ? CP_ACP : CP_OEMCP; } 
DWORD_PTR GetRealIconIndex(LPCWSTR path, UINT32 attributes, int &iconIndex)
{
  SHFILEINFOW shellInfo;
  DWORD_PTR res = ::SHGetFileInfoW(path, FILE_ATTRIBUTE_NORMAL | attributes, &shellInfo, 
      sizeof(shellInfo), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX);
  if (res == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    return GetRealIconIndex(UnicodeStringToMultiByte(path, GetCurrentCodePage()), attributes, iconIndex);
  iconIndex = shellInfo.iIcon;
  return res;
}
#endif

DWORD_PTR GetRealIconIndex(const CSysString &fileName, UINT32 attributes, 
    int &iconIndex, CSysString &typeName)
{
  SHFILEINFO shellInfo;
  DWORD_PTR res = ::SHGetFileInfo(fileName, FILE_ATTRIBUTE_NORMAL | attributes, &shellInfo, 
      sizeof(shellInfo), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX 
      | SHGFI_TYPENAME);
  typeName = shellInfo.szTypeName;
  iconIndex = shellInfo.iIcon;
  return res;
}

int CExtToIconMap::GetIconIndex(UINT32 attributes, const CSysString &fileNameSpec,
    CSysString &typeName)
{
  CSysString fileName = fileNameSpec;
  if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
  {
    fileName = TEXT("__Fldr__");
    if (_dirIconIndex < 0)
      GetRealIconIndex(fileName, attributes, _dirIconIndex, _dirTypeName);
    typeName = _dirTypeName;
    return _dirIconIndex;
  }
  int dotPos = fileName.ReverseFind('.');
  if (dotPos < 0)
  {
    fileName = TEXT("__File__");
    if (_noExtIconIndex < 0)
    {
      int iconIndexTemp;
      GetRealIconIndex(fileName, attributes, iconIndexTemp, _noExtTypeName);
    }
    typeName = _noExtTypeName;
    return _noExtIconIndex;
  }
  CExtIconPair extIconPair;
  extIconPair.Ext = fileName.Mid(dotPos + 1);
  int anIndex = _map.FindInSorted(extIconPair);
  if (anIndex >= 0)
    return _map[anIndex].IconIndex;
  fileName = fileName.Mid(dotPos);
  GetRealIconIndex(fileName, attributes, extIconPair.IconIndex, extIconPair.TypeName);
  _map.AddToSorted(extIconPair);
  typeName = extIconPair.TypeName;
  return extIconPair.IconIndex;
}

int CExtToIconMap::GetIconIndex(UINT32 attributes, const CSysString &fileName)
{
  CSysString typeName;
  return GetIconIndex(attributes, fileName, typeName);
}
