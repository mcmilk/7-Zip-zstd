// stdafx.h

#ifndef __STDAFX_H
#define __STDAFX_H

#include <windows.h>

#define _ATL_APARTMENT_THREADED

#define _ATL_NO_UUIDOF

#include <atlbase.h>

extern CComModule _Module;

#include <atlcom.h>

#include <crtdbg.h>
#include <string.h>
#include <mbstring.h>
#include <tchar.h>
#include <locale.h>
#include <shlobj.h>
#include <shlguid.h>
#include <regstr.h>

#include <new.h>

#pragma warning(disable:4786)

#include <vector>
#include <map>
#include <algorithm>

#endif
