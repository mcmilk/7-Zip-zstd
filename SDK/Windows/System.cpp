// Windows/System.h

#include "StdAfx.h"

#include "Windows/System.h"

namespace NWindows {
namespace NSystem {

bool MyGetWindowsDirectory(CSysString &aPath)
{
  DWORD aNeedLength = ::GetWindowsDirectory(aPath.GetBuffer(MAX_PATH), MAX_PATH);
  aPath.ReleaseBuffer();
  return (aNeedLength < MAX_PATH);
}

bool MyGetSystemDirectory(CSysString &aPath)
{
  DWORD aNeedLength = ::GetSystemDirectory(aPath.GetBuffer(MAX_PATH), MAX_PATH);
  aPath.ReleaseBuffer();
  return (aNeedLength < MAX_PATH);
}

}}

