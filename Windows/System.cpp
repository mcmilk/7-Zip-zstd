// Windows/System.h

#include "StdAfx.h"

#include "System.h"

namespace NWindows {
namespace NSystem {

bool MyGetWindowsDirectory(CSysString &path)
{
  DWORD needLength = ::GetWindowsDirectory(path.GetBuffer(MAX_PATH), MAX_PATH);
  path.ReleaseBuffer();
  return (needLength < MAX_PATH);
}

bool MyGetSystemDirectory(CSysString &path)
{
  DWORD needLength = ::GetSystemDirectory(path.GetBuffer(MAX_PATH), MAX_PATH);
  path.ReleaseBuffer();
  return (needLength < MAX_PATH);
}

}}

