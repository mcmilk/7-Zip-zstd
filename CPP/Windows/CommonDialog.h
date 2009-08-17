// Windows/CommonDialog.h

#ifndef __WINDOWS_COMMON_DIALOG_H
#define __WINDOWS_COMMON_DIALOG_H

#include "Common/MyString.h"

namespace NWindows{

bool MyGetOpenFileName(HWND hwnd, LPCWSTR title, LPCWSTR fullFileName,
    LPCWSTR s, UString &resPath
    #ifdef UNDER_CE
    , bool openFolder = false
    #endif
);

}

#endif
