// Windows/Registry.cpp

#include "StdAfx.h"

#include "Windows/Registry.h"

namespace NWindows {
namespace NRegistry {

#define MYASSERT(expr) _ASSERTE(expr)

CKey::~CKey()
{ 
  Close(); 
}

HKEY CKey::Detach()
{
  HKEY hKey = m_Object;
  m_Object = NULL;
  return hKey;
}

void CKey::Attach(HKEY hKey)
{
  MYASSERT(m_Object == NULL);
  m_Object = hKey;
}

LONG CKey::Create(HKEY aParentKey, LPCTSTR aKeyName,
    LPTSTR aClass, DWORD anOptions, REGSAM anAccessMask,
    LPSECURITY_ATTRIBUTES aSecurityAttributes, LPDWORD aDisposition)
{
  MYASSERT(aParentKey != NULL);
  DWORD aDispositionReal;
  HKEY aKey = NULL;
  LONG aRes = RegCreateKeyEx(aParentKey, aKeyName, 0,
    aClass, anOptions, anAccessMask, aSecurityAttributes, &aKey, &aDispositionReal);
  if (aDisposition != NULL)
    *aDisposition = aDispositionReal;
  if (aRes == ERROR_SUCCESS)
  {
    aRes = Close();
    m_Object = aKey;
  }
  return aRes;
}

LONG CKey::Open(HKEY aParentKey, LPCTSTR aKeyName, REGSAM anAccessMask)
{
  MYASSERT(aParentKey != NULL);
  HKEY aKey = NULL;
  LONG aRes = RegOpenKeyEx(aParentKey, aKeyName, 0, anAccessMask, &aKey);
  if (aRes == ERROR_SUCCESS)
  {
    aRes = Close();
    MYASSERT(aRes == ERROR_SUCCESS);
    m_Object = aKey;
  }
  return aRes;
}

LONG CKey::Close()
{
  LONG aRes = ERROR_SUCCESS;
  if (m_Object != NULL)
  {
    aRes = RegCloseKey(m_Object);
    m_Object = NULL;
  }
  return aRes;
}

// win95, win98: deletes sunkey and all its subkeys
// winNT to be deleted must not have subkeys
LONG CKey::DeleteSubKey(LPCTSTR aSubKeyName)
{
  MYASSERT(m_Object != NULL);
  return RegDeleteKey(m_Object, aSubKeyName);
}

LONG CKey::RecurseDeleteKey(LPCTSTR aSubKeyName)
{
  CKey aKey;
  LONG aRes = aKey.Open(m_Object, aSubKeyName, KEY_READ | KEY_WRITE);
  if (aRes != ERROR_SUCCESS)
    return aRes;
  FILETIME aTime;
  const UINT32 kBufferSize = MAX_PATH + 1; // 256 in ATL
  DWORD aSize = kBufferSize;
  TCHAR aBuffer[kBufferSize];
  while (RegEnumKeyEx(aKey.m_Object, 0, aBuffer, &aSize, NULL, NULL, NULL,
      &aTime) == ERROR_SUCCESS)
  {
    aRes = aKey.RecurseDeleteKey(aBuffer);
    if (aRes != ERROR_SUCCESS)
      return aRes;
    aSize = kBufferSize;
  }
  aKey.Close();
  return DeleteSubKey(aSubKeyName);
}


/////////////////////////
// Value Functions

static inline UINT32 BoolToDWORD(bool aValue)
  {  return (aValue ? 1: 0); }

static inline bool DWORDToBool(UINT32 aValue)
  {  return (aValue != 0); }


LONG CKey::DeleteValue(LPCTSTR aValue)
{
  MYASSERT(m_Object != NULL);
  return RegDeleteValue(m_Object, (LPTSTR)aValue);
}

LONG CKey::SetValue(LPCTSTR aValueName, UINT32 aValue)
{
  MYASSERT(m_Object != NULL);
  return RegSetValueEx(m_Object, aValueName, NULL, REG_DWORD,
      (BYTE * const)&aValue, sizeof(UINT32));
}

LONG CKey::SetValue(LPCTSTR aValueName, bool aValue)
{
  return SetValue(aValueName, BoolToDWORD(aValue));
}

LONG CKey::SetValue(LPCTSTR aValueName, LPCTSTR aValue)
{
  MYASSERT(aValue != NULL);
  MYASSERT(m_Object != NULL);
  return RegSetValueEx(m_Object, aValueName, NULL, REG_SZ,
      (const BYTE * )aValue, (lstrlen(aValue) + 1) * sizeof(TCHAR));
}

LONG CKey::SetValue(LPCTSTR aValueName, const CSysString &aValue)
{
  MYASSERT(aValue != NULL);
  MYASSERT(m_Object != NULL);
  return RegSetValueEx(m_Object, aValueName, NULL, REG_SZ,
      (const BYTE *)(const TCHAR *)aValue, (aValue.Length() + 1) * sizeof(TCHAR));
}

LONG CKey::SetValue(LPCTSTR aValueName, const void *aValue, UINT32 aSize)
{
  MYASSERT(aValue != NULL);
  MYASSERT(m_Object != NULL);
  return RegSetValueEx(m_Object, aValueName, NULL, REG_BINARY,
      (const BYTE *)aValue, aSize);
}

LONG SetValue(HKEY aParentKey, LPCTSTR aKeyName, 
    LPCTSTR aValueName, LPCTSTR aValue)
{
  MYASSERT(aValue != NULL);
  CKey aKey;
  LONG aRes = aKey.Create(aParentKey, aKeyName);
  if (aRes == ERROR_SUCCESS)
    aRes = aKey.SetValue(aValueName, aValue);
  return aRes;
}

LONG CKey::SetKeyValue(LPCTSTR aKeyName, LPCTSTR aValueName, LPCTSTR aValue)
{
  MYASSERT(aValue != NULL);
  CKey aKey;
  LONG aRes = aKey.Create(m_Object, aKeyName);
  if (aRes == ERROR_SUCCESS)
    aRes = aKey.SetValue(aValueName, aValue);
  return aRes;
}

LONG CKey::QueryValue(LPCTSTR aValueName, UINT32 &aValue)
{
  DWORD dwType = NULL;
  DWORD dwCount = sizeof(DWORD);
  LONG aRes = RegQueryValueEx(m_Object, (LPTSTR)aValueName, NULL, &dwType,
    (LPBYTE)&aValue, &dwCount);
  MYASSERT((aRes!=ERROR_SUCCESS) || (dwType == REG_DWORD));
  MYASSERT((aRes!=ERROR_SUCCESS) || (dwCount == sizeof(UINT32)));
  return aRes;
}

LONG CKey::QueryValue(LPCTSTR aValueName, bool &aValue)
{
  UINT32 aDWORDValue = BoolToDWORD(aValue);
  LONG aRes = QueryValue(aValueName, aDWORDValue);
  aValue = DWORDToBool(aDWORDValue);
  return aRes;
}

LONG CKey::QueryValue(LPCTSTR aValueName, LPTSTR szValue, UINT32 &aCount)
{
  MYASSERT(aCount != NULL);
  DWORD dwType = NULL;
  LONG aRes = RegQueryValueEx(m_Object, (LPTSTR)aValueName, NULL, &dwType,
    (LPBYTE)szValue, (DWORD *)&aCount);
  MYASSERT((aRes!=ERROR_SUCCESS) || (dwType == REG_SZ) ||
       (dwType == REG_MULTI_SZ) || (dwType == REG_EXPAND_SZ));
  return aRes;
}

LONG CKey::QueryValue(LPCTSTR aValueName, CSysString &aValue)
{
  aValue.Empty();
  DWORD aType = NULL;
  UINT32 aCurrentSize = 0;
  LONG aRes = RegQueryValueEx(m_Object, (LPTSTR)aValueName, NULL, &aType,
      NULL, (DWORD *)&aCurrentSize);
  if (aRes != ERROR_SUCCESS && aRes != ERROR_MORE_DATA)
    return aRes;
  aRes = QueryValue(aValueName, aValue.GetBuffer(aCurrentSize), aCurrentSize);
  aValue.ReleaseBuffer();
  return aRes;
}

LONG CKey::QueryValue(LPCTSTR aValueName, void *aValue, UINT32 &aCount)
{
  MYASSERT(aCount != NULL);
  DWORD dwType = NULL;
  LONG aRes = RegQueryValueEx(m_Object, (LPTSTR)aValueName, NULL, &dwType,
    (LPBYTE)aValue, (DWORD *)&aCount);
  MYASSERT((aRes!=ERROR_SUCCESS) || (dwType == REG_BINARY));
  return aRes;
}


LONG CKey::QueryValue(LPCTSTR aValueName, CByteBuffer &aValue, UINT32 &aDataSize)
{
  DWORD aType = NULL;
  aDataSize = 0;
  LONG aRes = RegQueryValueEx(m_Object, (LPTSTR)aValueName, NULL, &aType,
      NULL, (DWORD *)&aDataSize);
  if (aRes != ERROR_SUCCESS && aRes != ERROR_MORE_DATA)
    return aRes;
  aValue.SetCapacity(aDataSize);
  return QueryValue(aValueName, (BYTE *)aValue, aDataSize);
}

LONG CKey::EnumKeys(CSysStringVector &aKeyNames)
{
  aKeyNames.Clear();
  CSysString aKeyName;
  for(UINT32 anIndex = 0; true; anIndex++)
  {
    const UINT32 kBufferSize = MAX_PATH + 1; // 256 in ATL
    FILETIME aLastWriteTime;
    UINT32 aNameSize = kBufferSize;
    LONG aResult = ::RegEnumKeyEx(m_Object, anIndex, aKeyName.GetBuffer(kBufferSize), 
        (DWORD *)&aNameSize, NULL, NULL, NULL, &aLastWriteTime);
    aKeyName.ReleaseBuffer();
    if(aResult == ERROR_NO_MORE_ITEMS)
      break;
    if(aResult != ERROR_SUCCESS)
      return aResult;
    aKeyNames.Add(aKeyName);
  }
  return ERROR_SUCCESS;
}


}}
