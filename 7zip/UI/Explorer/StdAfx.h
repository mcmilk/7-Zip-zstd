// stdafx.h : include file for standard system include files,

#ifndef __STDAFX_H
#define __STDAFX_H


// #define _WIN32_IE 0x0500
#include <windows.h>
#include <CommCtrl.h>
#include <shlobj.h>
#include <tchar.h>

#include <stddef.h>
#include <string.h>
#include <mbstring.h>
#include <wchar.h>

#define _ATL_APARTMENT_THREADED

#define _ATL_NO_UUIDOF

#include <atlbase.h>

extern CComModule _Module;

#include <atlcom.h>
#include <shlguid.h>
#include <regstr.h>

// #include <new.h>

#undef _MT

#include <vector>
#include <map>
#include <algorithm>

#define _MT

#endif 
