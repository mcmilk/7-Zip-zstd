// ProcessMessages.cpp

#include "StdAfx.h"

#include "ProcessMessages.h"

void ProcessMessages(HWND aWindow) 
{
  MSG msg;
  while (::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE) ) 
  { 
    if (aWindow == (HWND) NULL || !IsDialogMessage(aWindow, &msg)) 
    { 
      TranslateMessage(&msg); 
      DispatchMessage(&msg); 
    } 
  } 
}
