// Windows/NationalTime.h

#pragma once

#ifndef __WINDOWS_NATIONALTIME_H
#define __WINDOWS_NATIONALTIME_H

#include "Common/String.h"

namespace NWindows {
namespace NNational {
namespace NTime {


bool MyGetTimeFormat(LCID aLocale, DWORD aFlags, CONST SYSTEMTIME *aTime, 
    LPCTSTR aFormat, CSysString &aResultString);

bool MyGetDateFormat(LCID aLocale, DWORD aFlags, CONST SYSTEMTIME *aTime, 
    LPCTSTR aFormat, CSysString &aResultString);

}}}

#endif
