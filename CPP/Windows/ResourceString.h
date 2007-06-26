// Windows/ResourceString.h

#ifndef __WINDOWS_RESOURCESTRING_H
#define __WINDOWS_RESOURCESTRING_H

#include "Common/MyString.h"

namespace NWindows {

CSysString MyLoadString(HINSTANCE hInstance, UINT resourceID);
CSysString MyLoadString(UINT resourceID);
#ifdef _UNICODE
inline UString MyLoadStringW(HINSTANCE hInstance, UINT resourceID) { return MyLoadString(hInstance, resourceID); }
inline UString MyLoadStringW(UINT resourceID) { return MyLoadString(resourceID); }
#else
UString MyLoadStringW(HINSTANCE hInstance, UINT resourceID);
UString MyLoadStringW(UINT resourceID);
#endif

}

#endif
