// Windows/Control/ComboBox.cpp

// #define _UNICODE
// #define UNICODE

#include "StdAfx.h"

#ifndef _UNICODE
#include "Common/StringConvert.h"
#endif

#include "Windows/Control/ComboBox.h"
#include "Windows/Defs.h"

#ifndef _UNICODE
extern bool g_IsNT;
#endif

namespace NWindows {
namespace NControl {

LRESULT CComboBox::GetLBText(int index, CSysString &s)
{
  s.Empty();
  LRESULT len = GetLBTextLen(index);
  if (len == CB_ERR)
    return len;
  len = GetLBText(index, s.GetBuffer((int)len + 1));
  s.ReleaseBuffer();
  return len;
}

#ifndef _UNICODE
LRESULT CComboBox::AddString(LPCWSTR s)
{ 
  if (g_IsNT)
    return SendMessageW(CB_ADDSTRING, 0, (LPARAM)s);
  return AddString(GetSystemString(s));
}

LRESULT CComboBox::GetLBText(int index, UString &s)
{
  s.Empty();
  if (g_IsNT)
  {
    LRESULT len = SendMessageW(CB_GETLBTEXTLEN, index, 0);
    if (len == CB_ERR)
      return len;
    len = SendMessageW(CB_GETLBTEXT, index, (LPARAM)s.GetBuffer((int)len + 1));
    s.ReleaseBuffer();
    return len;
  }
  AString sa;
  LRESULT len = GetLBText(index, sa);
  if (len == CB_ERR)
    return len;
  s = GetUnicodeString(sa);
  return s.Length();
}
#endif


}}
