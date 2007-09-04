// Windows/Clipboard.cpp

#include "StdAfx.h"

#include "Windows/Clipboard.h"
#include "Windows/Defs.h"
#include "Windows/Memory.h"
#include "Windows/Shell.h"
#include "Windows/Memory.h"

#include "Common/StringConvert.h"

namespace NWindows {

bool CClipboard::Open(HWND wndNewOwner)
{  
  m_Open = BOOLToBool(::OpenClipboard(wndNewOwner)); 
  return m_Open; 
}

CClipboard::~CClipboard()
{
  Close();
}

bool CClipboard::Close() 
{
  if (!m_Open)
    return true;
  m_Open = !BOOLToBool(CloseClipboard()); 
  return !m_Open;
}

bool ClipboardIsFormatAvailableHDROP()
{
  return BOOLToBool(IsClipboardFormatAvailable(CF_HDROP));
}

/*
bool ClipboardGetTextString(AString &s)
{
  s.Empty();
  if (!IsClipboardFormatAvailable(CF_TEXT)) 
    return false; 
  CClipboard clipboard;

  if (!clipboard.Open(NULL)) 
    return false; 

  HGLOBAL h = ::GetClipboardData(CF_TEXT);
  if (h != NULL) 
  { 
    NMemory::CGlobalLock globalLock(h); 
    const char *p = (const char *)globalLock.GetPointer(); 
    if (p != NULL)
    {
      s = p;
      return true;
    }
  } 
  return false;
}
*/

/*
bool ClipboardGetFileNames(UStringVector &names)
{
  names.Clear();
  if (!IsClipboardFormatAvailable(CF_HDROP)) 
    return false; 
  CClipboard clipboard;

  if (!clipboard.Open(NULL)) 
    return false; 

  HGLOBAL h = ::GetClipboardData(CF_HDROP);
  if (h != NULL) 
  { 
    NMemory::CGlobalLock globalLock(h); 
    void *p = (void *)globalLock.GetPointer(); 
    if (p != NULL)
    {
      NShell::CDrop drop(false);
      drop.Attach((HDROP)p);
      drop.QueryFileNames(names);
      return true;
    }
  } 
  return false;
}
*/

static bool ClipboardSetData(UINT uFormat, const void *data, size_t size)
{
  NMemory::CGlobal global;
  if (!global.Alloc(GMEM_DDESHARE | GMEM_MOVEABLE, size))
    return false; 
  {
    NMemory::CGlobalLock globalLock(global);
    LPVOID p = globalLock.GetPointer();
    if (p == NULL)
      return false;
    memcpy(p, data, size);
  }
  if (::SetClipboardData(uFormat, global) == NULL)
    return false;
  global.Detach();
  return true;
}

bool ClipboardSetText(HWND owner, const UString &s)
{
  CClipboard clipboard;
  if (!clipboard.Open(owner)) 
    return false; 
  if (!::EmptyClipboard())
    return false; 

  bool res;
  res = ClipboardSetData(CF_UNICODETEXT, (const wchar_t *)s, (s.Length() + 1) * sizeof(wchar_t));
  #ifndef _UNICODE
  AString a;
  a = UnicodeStringToMultiByte(s, CP_ACP);
  res |=  ClipboardSetData(CF_TEXT, (const char *)a, (a.Length() + 1) * sizeof(char));
  a = UnicodeStringToMultiByte(s, CP_OEMCP);
  res |=  ClipboardSetData(CF_OEMTEXT, (const char *)a, (a.Length() + 1) * sizeof(char));
  #endif
  return res;
} 
 
}
