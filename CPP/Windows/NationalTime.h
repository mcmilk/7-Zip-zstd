// Windows/NationalTime.h

#ifndef __WINDOWS_NATIONALTIME_H
#define __WINDOWS_NATIONALTIME_H

#include "Common/String.h"

namespace NWindows {
namespace NNational {
namespace NTime {

bool MyGetTimeFormat(LCID locale, DWORD flags, CONST SYSTEMTIME *time,
    LPCTSTR format, CSysString &resultString);

bool MyGetDateFormat(LCID locale, DWORD flags, CONST SYSTEMTIME *time,
    LPCTSTR format, CSysString &resultString);

}}}

#endif
