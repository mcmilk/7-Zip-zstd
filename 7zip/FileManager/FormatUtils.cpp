// FormatUtils.cpp

#include "StdAfx.h"

#include "FormatUtils.h"
#include "Windows/ResourceString.h"
#include "Common/IntToString.h"
#include "Common/StringConvert.h"

#ifdef LANG
#include "LangUtils.h"
#endif

/*
CSysString MyFormat(const CSysString &format, const CSysString &argument)
{
  CSysString result;
  _stprintf(result.GetBuffer(format.Length() + argument.Length() + 2), 
      format, argument);
  result.ReleaseBuffer();
  return result;
}

CSysString MyFormat(UINT32 resourceID, 
    #ifdef LANG
    UINT32 aLangID, 
    #endif
    const CSysString &argument)
{
  return MyFormat(
    #ifdef LANG
    LangLoadString(resourceID, aLangID), 
    #else
    NWindows::MyLoadString(resourceID), 
    #endif
    
    argument);
}
*/

CSysString NumberToString(UINT64 number)
{
  TCHAR temp[32];
  ConvertUInt64ToString(number, temp);
  return temp;
}

UString NumberToStringW(UINT64 number)
{
  wchar_t numberString[32];
  ConvertUInt64ToString(number, numberString);
  return numberString;
}

UString MyFormatNew(const UString &format, const UString &argument)
{
  UString result = format;
  result.Replace(L"{0}", argument);
  return result;
}


UString MyFormatNew(UINT32 resourceID, 
    #ifdef LANG
    UINT32 aLangID, 
    #endif
    const UString &argument)
{
  return MyFormatNew(
    #ifdef LANG
    LangLoadStringW(resourceID, aLangID), 
    #else
    NWindows::MyLoadStringW(resourceID), 
    #endif
    argument);
}
