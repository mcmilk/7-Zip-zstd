// MyMessages.cpp

#include "StdAfx.h"

#include "MyMessages.h"

#include "Windows/Error.h"
#include "Windows/ResourceString.h"

#ifdef LANG        
#include "../FileManager/LangUtils.h"
#endif

using namespace NWindows;

void MyMessageBox(HWND window, LPCWSTR message)
{ 
  ::MessageBoxW(window, message, L"7-Zip", 0); 
}

void MyMessageBoxResource(HWND window, UINT32 id
    #ifdef LANG        
    ,UINT32 langID
    #endif
    )
{
  #ifdef LANG        
  MyMessageBox(window, LangString(id, langID));
  #else
  MyMessageBox(window, MyLoadStringW(id));
  #endif
}

void MyMessageBox(UINT32 id
    #ifdef LANG        
    ,UINT32 langID
    #endif
    )
{
  MyMessageBoxResource(0, id
  #ifdef LANG        
  , langID
  #endif
  );
}

void ShowErrorMessage(HWND window, DWORD message)
{
  MyMessageBox(window, NError::MyFormatMessageW(message));
}

void ShowLastErrorMessage(HWND window)
{
  ShowErrorMessage(window, ::GetLastError());
}

