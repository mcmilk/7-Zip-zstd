// MyMessages.h

#pragma once

#ifndef __MYMESSAGES_H
#define __MYMESSAGES_H

#include "Common/String.h"

void MyMessageBox(HWND aWindow, LPCTSTR aMessage);

inline void MyMessageBox(LPCTSTR aMessage)
  {  MyMessageBox(0, aMessage); }

void MyMessageBox(UINT32 anId);

void ShowErrorMessage(HWND aWindow, DWORD anError);
inline void ShowErrorMessage(DWORD anError)
  { ShowErrorMessage(0, anError); }
void ShowLastErrorMessage(HWND aWindow = 0);

#endif
