// Windows/ResourceString.cpp

#include "StdAfx.h"

#include "Windows/ResourceString.h"
#ifndef _UNICODE
#include "Common/StringConvert.h"
#endif

extern HINSTANCE g_hInstance;

namespace NWindows {

CSysString MyLoadString(UINT resourceID)
{
  CSysString s;
  int size = 256;
  int len;
  do
  {
    size += 256;
    len = ::LoadString(g_hInstance, resourceID, s.GetBuffer(size - 1), size);
  } while (size - len <= 1);
  s.ReleaseBuffer();
  return s;
}

#ifndef _UNICODE
UString MyLoadStringW(UINT resourceID)
{
  UString s;
  int size = 256;
  int len;
  do
  {
    size += 256;
    len = ::LoadStringW(g_hInstance, resourceID, s.GetBuffer(size - 1), size);
    if (len == 0)
    {
      if (::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
        break;
      return GetUnicodeString(MyLoadString(resourceID));
    }
  } while (size - len <= 1);
  s.ReleaseBuffer();
  return s;
}
#endif

}