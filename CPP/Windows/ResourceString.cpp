// Windows/ResourceString.cpp

#include "StdAfx.h"

#include "Windows/ResourceString.h"
#ifndef _UNICODE
#include "Common/StringConvert.h"
#endif

extern HINSTANCE g_hInstance;
#ifndef _UNICODE
extern bool g_IsNT;
#endif

namespace NWindows {

CSysString MyLoadString(HINSTANCE hInstance, UINT resourceID)
{
  CSysString s;
  int size = 256;
  int len;
  do
  {
    size += 256;
    len = ::LoadString(hInstance, resourceID, s.GetBuffer(size - 1), size);
  } 
  while (size - len <= 1);
  s.ReleaseBuffer();
  return s;
}

CSysString MyLoadString(UINT resourceID)
{
  return MyLoadString(g_hInstance, resourceID);
}

#ifndef _UNICODE
UString MyLoadStringW(HINSTANCE hInstance, UINT resourceID)
{
  if (g_IsNT)
  {
    UString s;
    int size = 256;
    int len;
    do
    {
      size += 256;
      len = ::LoadStringW(hInstance, resourceID, s.GetBuffer(size - 1), size);
    } 
    while (size - len <= 1);
    s.ReleaseBuffer();
    return s;
  }
  return GetUnicodeString(MyLoadString(hInstance, resourceID));
}

UString MyLoadStringW(UINT resourceID)
{
  return MyLoadStringW(g_hInstance, resourceID);
}

#endif

}
