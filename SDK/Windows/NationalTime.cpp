// Windows/NationalTime.cpp

#include "StdAfx.h"

#include "Windows/NationalTime.h"

namespace NWindows {
namespace NNational {
namespace NTime {

bool MyGetTimeFormat(LCID aLocale, DWORD aFlags, CONST SYSTEMTIME *aTime, 
    LPCTSTR aFormat, CSysString &aResultString)
{
  aResultString.Empty();
  int aNumChars = ::GetTimeFormat(aLocale, aFlags, aTime, aFormat,
      NULL, 0);
  if(aNumChars == 0)
    return false;
  aNumChars = ::GetTimeFormat(aLocale, aFlags, aTime, aFormat,
      aResultString.GetBuffer(aNumChars), aNumChars + 1);
  aResultString.ReleaseBuffer();
  return (aNumChars != 0);
}

bool MyGetDateFormat(LCID aLocale, DWORD aFlags, CONST SYSTEMTIME *aTime, 
    LPCTSTR aFormat, CSysString &aResultString)
{
  aResultString.Empty();
  int aNumChars = ::GetDateFormat(aLocale, aFlags, aTime, aFormat,
      NULL, 0);
  if(aNumChars == 0)
    return false;
  aNumChars = ::GetDateFormat(aLocale, aFlags, aTime, aFormat,
      aResultString.GetBuffer(aNumChars), aNumChars + 1);
  aResultString.ReleaseBuffer();
  return (aNumChars != 0);
}

}}}
