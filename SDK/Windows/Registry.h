// Windows/Registry.h

#pragma once

#ifndef __WINDOWS_REGISTRY_H
#define __WINDOWS_REGISTRY_H

#include "Common/DynamicBuffer.h"
#include "Common/String.h"

namespace NWindows {
namespace NRegistry {

const TCHAR kKeyNameDelimiter = _T('\\');

LONG SetValue(HKEY aParentKey, LPCTSTR aKeyName,
    LPCTSTR aValueName, LPCTSTR aValue);

class CKey
{
  HKEY m_Object;
public:
  CKey(): m_Object(NULL) {}
  ~CKey();

  operator HKEY() const { return m_Object; }

  HKEY Detach();
  void Attach(HKEY aKey);
  LONG Create(HKEY aParentKey, LPCTSTR aKeyName,
      LPTSTR aClass = REG_NONE, DWORD anOptions = REG_OPTION_NON_VOLATILE,
      REGSAM anAccessMask = KEY_ALL_ACCESS,
      LPSECURITY_ATTRIBUTES aSecurityAttributes = NULL,
      LPDWORD aDisposition = NULL);
  LONG Open(HKEY aParentKey, LPCTSTR aKeyName,
      REGSAM anAccessMask = KEY_ALL_ACCESS);

  LONG Close();

  LONG DeleteSubKey(LPCTSTR aSubKeyName);
  LONG RecurseDeleteKey(LPCTSTR aSubKeyName);

  LONG DeleteValue(LPCTSTR aValue);
  LONG SetValue(LPCTSTR aValueName, UINT32 aValue);
  LONG SetValue(LPCTSTR aValueName, bool aValue);
  LONG SetValue(LPCTSTR aValueName, LPCTSTR aValue);
  LONG SetValue(LPCTSTR aValueName, const CSysString &aValue);
  LONG SetValue(LPCTSTR aValueName, const void *aValue, UINT32 aSize);

  LONG SetKeyValue(LPCTSTR aKeyName, LPCTSTR aValueName, LPCTSTR aValue);

  LONG QueryValue(LPCTSTR aValueName, UINT32 &aValue);
  LONG QueryValue(LPCTSTR aValueName, bool &aValue);
  LONG QueryValue(LPCTSTR aValueName, LPTSTR szValue, UINT32 &aDataSize);
  LONG QueryValue(LPCTSTR aValueName, CSysString &aValue);

  LONG QueryValue(LPCTSTR aValueName, void *aValue, UINT32 &aDataSize);
  LONG QueryValue(LPCTSTR aValueName, CByteDynamicBuffer &aValue,  
      UINT32 &aDataSize);

  LONG EnumKeys(CSysStringVector &aKeyNames);
};

}}

#endif
