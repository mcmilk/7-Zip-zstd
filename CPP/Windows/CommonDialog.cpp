// Windows/CommonDialog.cpp

#include "StdAfx.h"

#ifdef UNDER_CE
#include <commdlg.h>
#endif

#ifndef _UNICODE
#include "Common/StringConvert.h"
#endif
#include "Common/MyCom.h"

#include "Windows/Defs.h"

#include "CommonDialog.h"

#ifndef _UNICODE
extern bool g_IsNT;
#endif

namespace NWindows{

#ifndef _UNICODE
class CDoubleZeroStringListA
{
  CRecordVector<int> m_Indexes;
  AString m_String;
public:
  void Add(LPCSTR s);
  void SetForBuffer(LPSTR buffer);
};

void CDoubleZeroStringListA::Add(LPCSTR s)
{
  m_String += s;
  m_Indexes.Add(m_String.Length());
  m_String += ' ';
}

void CDoubleZeroStringListA::SetForBuffer(LPSTR buffer)
{
  MyStringCopy(buffer, (const char *)m_String);
  for (int i = 0; i < m_Indexes.Size(); i++)
    buffer[m_Indexes[i]] = '\0';
}
#endif

class CDoubleZeroStringListW
{
  CRecordVector<int> m_Indexes;
  UString m_String;
public:
  void Add(LPCWSTR s);
  void SetForBuffer(LPWSTR buffer);
};

void CDoubleZeroStringListW::Add(LPCWSTR s)
{
  m_String += s;
  m_Indexes.Add(m_String.Length());
  m_String += L' ';
}

void CDoubleZeroStringListW::SetForBuffer(LPWSTR buffer)
{
  MyStringCopy(buffer, (const wchar_t *)m_String);
  for (int i = 0; i < m_Indexes.Size(); i++)
    buffer[m_Indexes[i]] = L'\0';
}

#define MY_OFN_PROJECT 0x00400000
#define MY_OFN_SHOW_ALL 0x01000000

bool MyGetOpenFileName(HWND hwnd, LPCWSTR title, LPCWSTR fullFileName,
    LPCWSTR s, UString &resPath
    #ifdef UNDER_CE
    , bool openFolder
    #endif
    )
{
  const int kBufferSize = MAX_PATH * 2;
  #ifndef _UNICODE
  if (!g_IsNT)
  {
    CHAR buffer[kBufferSize];
    MyStringCopy(buffer, (const char *)GetSystemString(fullFileName));
    OPENFILENAME info;
    info.lStructSize = sizeof(info);
    info.hwndOwner = hwnd;
    info.hInstance = 0;
    const int kFilterBufferSize = MAX_PATH;
    CHAR filterBuffer[kFilterBufferSize];
    CDoubleZeroStringListA doubleZeroStringList;
    doubleZeroStringList.Add(GetSystemString(s));
    doubleZeroStringList.Add("*.*");
    doubleZeroStringList.SetForBuffer(filterBuffer);
    info.lpstrFilter = filterBuffer;
    
    info.lpstrCustomFilter = NULL;
    info.nMaxCustFilter = 0;
    info.nFilterIndex = 0;
    
    info.lpstrFile = buffer;
    info.nMaxFile = kBufferSize;
    
    info.lpstrFileTitle = NULL;
    info.nMaxFileTitle = 0;
    
    info.lpstrInitialDir= NULL;

    info.lpstrTitle = 0;
    AString titleA;
    if (title != 0)
    {
      titleA = GetSystemString(title);
      info.lpstrTitle = titleA;
    }
    
    info.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
    info.nFileOffset = 0;
    info.nFileExtension = 0;
    info.lpstrDefExt = NULL;
    
    info.lCustData = 0;
    info.lpfnHook = NULL;
    info.lpTemplateName = NULL;
    
    bool res = BOOLToBool(::GetOpenFileNameA(&info));
    resPath = GetUnicodeString(buffer);
    return res;
  }
  else
  #endif
  {
    WCHAR buffer[kBufferSize];
    MyStringCopy(buffer, fullFileName);
    OPENFILENAMEW info;
    info.lStructSize = sizeof(info);
    info.hwndOwner = hwnd;
    info.hInstance = 0;
    const int kFilterBufferSize = MAX_PATH;
    WCHAR filterBuffer[kFilterBufferSize];
    CDoubleZeroStringListW doubleZeroStringList;
    doubleZeroStringList.Add(s);
    doubleZeroStringList.Add(L"*.*");
    doubleZeroStringList.SetForBuffer(filterBuffer);
    info.lpstrFilter = filterBuffer;
        
    info.lpstrCustomFilter = NULL;
    info.nMaxCustFilter = 0;
    info.nFilterIndex = 0;
    
    info.lpstrFile = buffer;
    info.nMaxFile = kBufferSize;
    
    info.lpstrFileTitle = NULL;
    info.nMaxFileTitle = 0;
    
    info.lpstrInitialDir= NULL;
       
    info.lpstrTitle = title;
    
    info.Flags = OFN_EXPLORER | OFN_HIDEREADONLY
        #ifdef UNDER_CE
        | (openFolder ? (MY_OFN_PROJECT | MY_OFN_SHOW_ALL) : 0)
        #endif
    ;

    info.nFileOffset = 0;
    info.nFileExtension = 0;
    info.lpstrDefExt = NULL;
    
    info.lCustData = 0;
    info.lpfnHook = NULL;
    info.lpTemplateName = NULL;
    
    bool res = BOOLToBool(::GetOpenFileNameW(&info));
    resPath = buffer;
    return res;
  }
}

}
