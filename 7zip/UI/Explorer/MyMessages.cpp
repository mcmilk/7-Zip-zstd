// MyMessages.cpp

#include "StdAfx.h"

#include "MyMessages.h"
#include "Common/String.h"
#include "Common/StringConvert.h"

#include "Windows/Error.h"
#include "Windows/ResourceString.h"

#ifdef LANG        
#include "../../FileManager/LangUtils.h"
#endif

using namespace NWindows;

void MyMessageBox(HWND aWindow, LPCWSTR aMessage)
{ 
  ::MessageBoxW(aWindow, aMessage, L"7-Zip", 0); 
}

void MyMessageBox(UINT32 anId
    #ifdef LANG        
    ,UINT32 aLangID
    #endif
    )
{
  #ifdef LANG        
  MyMessageBox(LangLoadStringW(anId, aLangID));
  #else
  MyMessageBox(GetUnicodeString(MyLoadString(anId)));
  #endif
}

void ShowErrorMessage(HWND aWindow, DWORD anError)
{
  CSysString aMessage;
  NError::MyFormatMessage(anError, aMessage);
  MyMessageBox(aWindow, GetUnicodeString(aMessage));
}

void ShowLastErrorMessage(HWND aWindow)
{
  ShowErrorMessage(aWindow, ::GetLastError());
}

