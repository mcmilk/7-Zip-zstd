// MyMessages.h

#ifndef __MYMESSAGES_H
#define __MYMESSAGES_H

#include "Common/MyString.h"
#include "Common/Types.h"

void ShowErrorMessage(HWND window, LPCWSTR message);
inline void ShowErrorMessage(LPCWSTR message) { ShowErrorMessage(0, message); }

void ShowErrorMessageHwndRes(HWND window, UINT resID
    #ifdef LANG
    , UInt32 langID
    #endif
    );

void ShowErrorMessageRes(UINT resID
    #ifdef LANG
    , UInt32 langID
    #endif
    );

// void ShowErrorMessageDWORD(HWND window, DWORD errorCode);
// inline void ErrorMessageDWORD(DWORD errorCode) { ShowErrorMessageDWORD(0, errorCode); }
void ShowLastErrorMessage(HWND window = 0);

#endif
