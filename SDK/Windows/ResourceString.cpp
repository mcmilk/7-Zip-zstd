// Windows/ResourceString.cpp

#include "StdAfx.h"

#include "Windows/ResourceString.h"

extern HINSTANCE g_hInstance;

namespace NWindows {

CSysString MyLoadString(UINT anID)
{
  CSysString aString;
  int aSize = 256;
  int aLen;
  do
  {
    aSize += 256;
    aLen = ::LoadString(g_hInstance, anID, aString.GetBuffer(aSize - 1), aSize);
  } while (aSize - aLen <= 1);
  aString.ReleaseBuffer();
  return aString;
}

}