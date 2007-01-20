// Windows/CommonDialog.h

#ifndef __WINDOWS_COMMONDIALOG_H
#define __WINDOWS_COMMONDIALOG_H

#include <windows.h>

#include "Common/String.h"
#include "Windows/Defs.h"

namespace NWindows{

bool MyGetOpenFileName(HWND hwnd, LPCWSTR title, LPCWSTR fullFileName, LPCWSTR s, UString &resPath);

}

#endif
