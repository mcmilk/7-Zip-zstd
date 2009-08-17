// SysIconUtils.cpp

#include "StdAfx.h"

#ifndef _UNICODE
#include "Common/StringConvert.h"
#endif

#include "SysIconUtils.h"

#ifndef _UNICODE
extern bool g_IsNT;
#endif

int GetIconIndexForCSIDL(int csidl)
{
  LPITEMIDLIST pidl = 0;
  SHGetSpecialFolderLocation(NULL, csidl, &pidl);
  if (pidl)
  {
    SHFILEINFO shellInfo;
    SHGetFileInfo(LPCTSTR(pidl), FILE_ATTRIBUTE_NORMAL,
      &shellInfo, sizeof(shellInfo),
      SHGFI_PIDL | SHGFI_SYSICONINDEX);
    IMalloc  *pMalloc;
    SHGetMalloc(&pMalloc);
    if(pMalloc)
    {
      pMalloc->Free(pidl);
      pMalloc->Release();
    }
    return shellInfo.iIcon;
  }
  return 0;
}

DWORD_PTR GetRealIconIndex(LPCTSTR path, DWORD attrib, int &iconIndex)
{
  SHFILEINFO shellInfo;
  DWORD_PTR res = ::SHGetFileInfo(path, FILE_ATTRIBUTE_NORMAL | attrib, &shellInfo,
      sizeof(shellInfo), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX);
  iconIndex = shellInfo.iIcon;
  return res;
}


#ifndef _UNICODE
typedef int (WINAPI * SHGetFileInfoWP)(LPCWSTR pszPath, DWORD attrib, SHFILEINFOW *psfi, UINT cbFileInfo, UINT uFlags);

struct CSHGetFileInfoInit
{
  SHGetFileInfoWP shGetFileInfoW;
  CSHGetFileInfoInit()
  {
    shGetFileInfoW = (SHGetFileInfoWP)
    ::GetProcAddress(::GetModuleHandleW(L"shell32.dll"), "SHGetFileInfoW");
  }
} g_SHGetFileInfoInit;
#endif

static DWORD_PTR MySHGetFileInfoW(LPCWSTR pszPath, DWORD attrib, SHFILEINFOW *psfi, UINT cbFileInfo, UINT uFlags)
{
  #ifdef _UNICODE
  return SHGetFileInfo(
  #else
  if (g_SHGetFileInfoInit.shGetFileInfoW == 0)
    return 0;
  return g_SHGetFileInfoInit.shGetFileInfoW(
  #endif
  pszPath, attrib, psfi, cbFileInfo, uFlags);
}

#ifndef _UNICODE
// static inline UINT GetCurrentCodePage() { return ::AreFileApisANSI() ? CP_ACP : CP_OEMCP; }
DWORD_PTR GetRealIconIndex(LPCWSTR path, DWORD attrib, int &iconIndex)
{
  if(g_IsNT)
  {
    SHFILEINFOW shellInfo;
    DWORD_PTR res = ::MySHGetFileInfoW(path, FILE_ATTRIBUTE_NORMAL | attrib, &shellInfo,
      sizeof(shellInfo), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX);
    iconIndex = shellInfo.iIcon;
    return res;
  }
  else
    return GetRealIconIndex(UnicodeStringToMultiByte(path), attrib, iconIndex);
}
#endif

DWORD_PTR GetRealIconIndex(const UString &fileName, DWORD attrib,
    int &iconIndex, UString &typeName)
{
  #ifndef _UNICODE
  if(!g_IsNT)
  {
    SHFILEINFO shellInfo;
    shellInfo.szTypeName[0] = 0;
    DWORD_PTR res = ::SHGetFileInfoA(GetSystemString(fileName), FILE_ATTRIBUTE_NORMAL | attrib, &shellInfo,
        sizeof(shellInfo), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_TYPENAME);
    typeName = GetUnicodeString(shellInfo.szTypeName);
    iconIndex = shellInfo.iIcon;
    return res;
  }
  else
  #endif
  {
    SHFILEINFOW shellInfo;
    shellInfo.szTypeName[0] = 0;
    DWORD_PTR res = ::MySHGetFileInfoW(fileName, FILE_ATTRIBUTE_NORMAL | attrib, &shellInfo,
        sizeof(shellInfo), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_TYPENAME);
    typeName = shellInfo.szTypeName;
    iconIndex = shellInfo.iIcon;
    return res;
  }
}

int CExtToIconMap::GetIconIndex(DWORD attrib, const UString &fileName, UString &typeName)
{
  int dotPos = fileName.ReverseFind(L'.');
  if ((attrib & FILE_ATTRIBUTE_DIRECTORY) != 0 || dotPos < 0)
  {
    CAttribIconPair pair;
    pair.Attrib = attrib;
    int index = _attribMap.FindInSorted(pair);
    if (index >= 0)
    {
      typeName = _attribMap[index].TypeName;
      return _attribMap[index].IconIndex;
    }
    GetRealIconIndex(
        #ifdef UNDER_CE
        L"\\"
        #endif
        L"__File__"
        , attrib, pair.IconIndex, pair.TypeName);
    _attribMap.AddToSorted(pair);
    typeName = pair.TypeName;
    return pair.IconIndex;
  }

  CExtIconPair pair;
  pair.Ext = fileName.Mid(dotPos + 1);
  int index = _extMap.FindInSorted(pair);
  if (index >= 0)
  {
    typeName = _extMap[index].TypeName;
    return _extMap[index].IconIndex;
  }
  GetRealIconIndex(fileName.Mid(dotPos), attrib, pair.IconIndex, pair.TypeName);
  _extMap.AddToSorted(pair);
  typeName = pair.TypeName;
  return pair.IconIndex;
}

int CExtToIconMap::GetIconIndex(DWORD attrib, const UString &fileName)
{
  UString typeName;
  return GetIconIndex(attrib, fileName, typeName);
}
