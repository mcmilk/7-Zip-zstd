// Windows/Registry.h

#pragma once

#ifndef __WINDOWS_REGISTRY_H
#define __WINDOWS_REGISTRY_H

#include "Common/Buffer.h"
#include "Common/String.h"

namespace NWindows {
namespace NRegistry {

const TCHAR kKeyNameDelimiter = TEXT('\\');

LONG SetValue(HKEY parentKey, LPCTSTR keyName, 
    LPCTSTR valueName, LPCTSTR value);

class CKey
{
  HKEY _object;
public:
  CKey(): _object(NULL) {}
  ~CKey();

  operator HKEY() const { return _object; }

  HKEY Detach();
  void Attach(HKEY key);
  LONG Create(HKEY parentKey, LPCTSTR keyName,
      LPTSTR keyClass = REG_NONE, DWORD options = REG_OPTION_NON_VOLATILE,
      REGSAM accessMask = KEY_ALL_ACCESS,
      LPSECURITY_ATTRIBUTES securityAttributes = NULL,
      LPDWORD disposition = NULL);
  LONG Open(HKEY parentKey, LPCTSTR keyName,
      REGSAM accessMask = KEY_ALL_ACCESS);

  LONG Close();

  LONG DeleteSubKey(LPCTSTR subKeyName);
  LONG RecurseDeleteKey(LPCTSTR subKeyName);

  LONG DeleteValue(LPCTSTR value);
  LONG SetValue(LPCTSTR valueName, UINT32 value);
  LONG SetValue(LPCTSTR valueName, bool value);
  LONG SetValue(LPCTSTR valueName, LPCTSTR value);
  LONG SetValue(LPCTSTR valueName, const CSysString &value);
  LONG SetValue(LPCTSTR valueName, const void *value, UINT32 size);

  LONG SetKeyValue(LPCTSTR keyName, LPCTSTR valueName, LPCTSTR value);

  LONG QueryValue(LPCTSTR valueName, UINT32 &value);
  LONG QueryValue(LPCTSTR valueName, bool &value);
  LONG QueryValue(LPCTSTR valueName, LPTSTR value, UINT32 &dataSize);
  LONG QueryValue(LPCTSTR valueName, CSysString &value);

  LONG QueryValue(LPCTSTR valueName, void *value, UINT32 &dataSize);
  LONG QueryValue(LPCTSTR valueName, CByteBuffer &value, UINT32 &dataSize);

  LONG EnumKeys(CSysStringVector &keyNames);
};

}}

#endif
