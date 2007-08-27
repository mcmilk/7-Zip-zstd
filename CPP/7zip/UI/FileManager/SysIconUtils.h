// SysIconUtils.h

#ifndef __SYSICONUTILS_H
#define __SYSICONUTILS_H

#include "Common/MyString.h"

struct CExtIconPair
{
  UString Ext;
  int IconIndex;
  UString TypeName;

};

inline bool operator==(const CExtIconPair &a1, const CExtIconPair &a2)
{
  return (a1.Ext == a2.Ext);
}

inline bool operator<(const CExtIconPair &a1, const CExtIconPair &a2)
{
  return (a1.Ext < a2.Ext);
}

class CExtToIconMap
{
  int _dirIconIndex;
  UString _dirTypeName;
  int _noExtIconIndex;
  UString _noExtTypeName;
  CObjectVector<CExtIconPair> _map;
public:
  CExtToIconMap(): _dirIconIndex(-1), _noExtIconIndex(-1) {}
  void Clear() 
  {
    _dirIconIndex = -1;
    _noExtIconIndex = -1;
    _map.Clear();
  }
  int GetIconIndex(UINT32 attributes, const UString &fileName, UString &typeName);
  int GetIconIndex(UINT32 attributes, const UString &fileName);
};

DWORD_PTR GetRealIconIndex(LPCTSTR path, UINT32 attributes, int &iconIndex);
#ifndef _UNICODE
DWORD_PTR GetRealIconIndex(LPCWSTR path, UINT32 attributes, int &iconIndex);
#endif
int GetIconIndexForCSIDL(int aCSIDL);

#endif
