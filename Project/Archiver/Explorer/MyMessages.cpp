// MyMessages.cpp

#include "StdAfx.h"

#include "MyMessages.h"
#include "Common/String.h"

#include "Windows/Error.h"
#include "Windows/ResourceString.h"

using namespace NWindows;

void MyMessageBox(HWND aWindow, LPCTSTR aMessage)
{ 
  ::MessageBox(aWindow, aMessage, _T("7-Zip"), 0); 
}

void MyMessageBox(UINT32 anId)
{
  MyMessageBox(MyLoadString(anId));
}

void ShowErrorMessage(HWND aWindow, DWORD anError)
{
  CSysString aMessage;
  NError::MyFormatMessage(anError, aMessage);
  MyMessageBox(aWindow, aMessage);
}

void ShowLastErrorMessage(HWND aWindow)
{
  ShowErrorMessage(aWindow, ::GetLastError());
}

