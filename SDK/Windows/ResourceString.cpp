// Windows/ResourceString.cpp

#include "StdAfx.h"

#include "Windows/ResourceString.h"

extern HINSTANCE g_hInstance;

namespace NWindows {

CSysString MyLoadString(UINT resourceID)
{
  CSysString string;
  int size = 256;
  int len;
  do
  {
    size += 256;
    len = ::LoadString(g_hInstance, resourceID, string.GetBuffer(size - 1), size);
  } while (size - len <= 1);
  string.ReleaseBuffer();
  return string;
}

}