// MyMessages.h

#pragma once

#ifndef __MYMESSAGES_H
#define __MYMESSAGES_H

#include "Common/String.h"

void MyMessageBox(HWND aWindow, LPCWSTR aMessage);

inline void MyMessageBox(LPCWSTR aMessage)
  {  MyMessageBox(0, aMessage); }

void MyMessageBox(UINT32 anId
    #ifdef LANG        
    ,UINT32 aLangID
    #endif
    );

void ShowErrorMessage(HWND aWindow, DWORD anError);
inline void ShowErrorMessage(DWORD anError)
  { ShowErrorMessage(0, anError); }
void ShowLastErrorMessage(HWND aWindow = 0);

#endif
