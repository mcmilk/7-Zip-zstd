// MyMessages.cpp

#include "StdAfx.h"

#include "MyMessages.h"

#include "Windows/Error.h"
#include "Windows/ResourceString.h"

#ifdef LANG
#include "../FileManager/LangUtils.h"
#endif

using namespace NWindows;

void ShowErrorMessage(HWND window, LPCWSTR message)
{
  ::MessageBoxW(window, message, L"7-Zip", MB_OK | MB_ICONSTOP);
}

void ShowErrorMessageHwndRes(HWND window, UINT resID
    #ifdef LANG
    , UInt32 langID
    #endif
    )
{
  ShowErrorMessage(window,
  #ifdef LANG
  LangString(resID, langID)
  #else
  MyLoadStringW(resID)
  #endif
  );
}

void ShowErrorMessageRes(UINT resID
    #ifdef LANG
    , UInt32 langID
    #endif
    )
{
  ShowErrorMessageHwndRes(0, resID
  #ifdef LANG
  , langID
  #endif
  );
}

void ShowErrorMessageDWORD(HWND window, DWORD errorCode)
{
  ShowErrorMessage(window, NError::MyFormatMessageW(errorCode));
}

void ShowLastErrorMessage(HWND window)
{
  ShowErrorMessageDWORD(window, ::GetLastError());
}

