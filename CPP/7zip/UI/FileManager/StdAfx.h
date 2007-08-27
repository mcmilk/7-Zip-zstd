// stdafx.h

#ifndef __STDAFX_H
#define __STDAFX_H

#define _WIN32_WINNT 0x0400

// it's for Windows NT supporting (MENUITEMINFOW)
#define WINVER 0x0400

#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#include <ShlObj.h>
#include <limits.h>
#include <tchar.h>
#include <shlwapi.h>

// #define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers

#include "Common/NewHandler.h"

#endif
