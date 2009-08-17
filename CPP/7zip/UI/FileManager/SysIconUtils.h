// SysIconUtils.h

#ifndef __SYS_ICON_UTILS_H
#define __SYS_ICON_UTILS_H

#include "Common/MyString.h"

struct CExtIconPair
{
  UString Ext;
  int IconIndex;
  UString TypeName;
};

struct CAttribIconPair
{
  DWORD Attrib;
  int IconIndex;
  UString TypeName;
};

inline bool operator==(const CExtIconPair &a1, const CExtIconPair &a2) { return a1.Ext == a2.Ext; }
inline bool operator< (const CExtIconPair &a1, const CExtIconPair &a2) { return a1.Ext < a2.Ext; }

inline bool operator==(const CAttribIconPair &a1, const CAttribIconPair &a2) { return a1.Attrib == a2.Attrib; }
inline bool operator< (const CAttribIconPair &a1, const CAttribIconPair &a2) { return a1.Attrib < a2.Attrib; }

class CExtToIconMap
{
  CObjectVector<CExtIconPair> _extMap;
  CObjectVector<CAttribIconPair> _attribMap;
public:
  void Clear()
  {
    _extMap.Clear();
    _attribMap.Clear();
  }
  int GetIconIndex(DWORD attrib, const UString &fileName, UString &typeName);
  int GetIconIndex(DWORD attrib, const UString &fileName);
};

DWORD_PTR GetRealIconIndex(LPCTSTR path, DWORD attrib, int &iconIndex);
#ifndef _UNICODE
DWORD_PTR GetRealIconIndex(LPCWSTR path, DWORD attrib, int &iconIndex);
#endif
int GetIconIndexForCSIDL(int csidl);

inline HIMAGELIST GetSysImageList(bool smallIcons)
{
  SHFILEINFO shellInfo;
  return (HIMAGELIST)SHGetFileInfo(TEXT(""),
      FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_DIRECTORY,
      &shellInfo, sizeof(shellInfo),
      SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | (smallIcons ? SHGFI_SMALLICON : SHGFI_ICON));
}

#endif
