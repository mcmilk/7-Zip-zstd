// PropVariantConversions.cpp

#include "StdAfx.h"

#include "PropVariantConversions.h"

#include "Windows/NationalTime.h"
#include "Windows/Defs.h"

#include "Common/StringConvert.h"
#include "Common/IntToString.h"

using namespace NWindows;

static UString ConvertUInt64ToString(UINT64 value)
{
  wchar_t buffer[32];
  ConvertUInt64ToString(value, buffer);
  return buffer;
}

static UString ConvertInt64ToString(Int64 value)
{
  wchar_t buffer[32];
  ConvertInt64ToString(value, buffer);
  return buffer;
}

/*
CSysString ConvertFileTimeToString(const FILETIME &fileTime, bool includeTime)
{
  SYSTEMTIME systemTime;
  if(!BOOLToBool(FileTimeToSystemTime(&fileTime, &systemTime)))
    #ifndef _WIN32_WCE
    throw 311907;
    #else
    return CSysString();
    #endif

  const int kBufferSize = 64;
  CSysString stringDate;
  if(!NNational::NTime::MyGetDateFormat(LOCALE_USER_DEFAULT, 
      0, &systemTime, NULL, stringDate))
    #ifndef _WIN32_WCE
    throw 311908;
    #else
    return CSysString();
    #endif
  if (!includeTime)
    return stringDate;
  CSysString stringTime;
  if(!NNational::NTime::MyGetTimeFormat(LOCALE_USER_DEFAULT, 
      0, &systemTime, NULL, stringTime))
    #ifndef _WIN32_WCE
    throw 311909;
    #else
    return CSysString();
    #endif
  return stringDate + _T(" ") + stringTime;
}
*/

UString ConvertFileTimeToString2(const FILETIME &fileTime, 
    bool includeTime, bool includeSeconds)
{
  CSysString string;
  SYSTEMTIME systemTime;
  if(!BOOLToBool(FileTimeToSystemTime(&fileTime, &systemTime)))
    return UString();
  TCHAR buffer[64];
  wsprintf(buffer, TEXT("%04d-%02d-%02d"), systemTime.wYear, systemTime.wMonth, systemTime.wDay);
  if (includeTime)
  {
    wsprintf(buffer + lstrlen(buffer), TEXT(" %02d:%02d"), systemTime.wHour, systemTime.wMinute);
    if (includeSeconds)
      wsprintf(buffer + lstrlen(buffer), TEXT(":%02d"), systemTime.wSecond);
  }
  return GetUnicodeString(buffer);
}
 

UString ConvertPropVariantToString(const PROPVARIANT &propVariant)
{
  switch (propVariant.vt)
  {
    case VT_EMPTY:
      return UString();
    case VT_BSTR:
      return propVariant.bstrVal;
    case VT_UI1:
      return ConvertUInt64ToString(propVariant.bVal);
    case VT_UI2:
      return ConvertUInt64ToString(propVariant.uiVal);
    case VT_UI4:
      return ConvertUInt64ToString(propVariant.ulVal);
    case VT_UI8:
      return ConvertUInt64ToString(*(UINT64 *)(&propVariant.uhVal));
    case VT_FILETIME:
      return ConvertFileTimeToString2(propVariant.filetime, true, true);


    /*
    case VT_I1:
      return ConvertInt64ToString(propVariant.cVal);
    */
    case VT_I2:
      return ConvertInt64ToString(propVariant.iVal);
    case VT_I4:
      return ConvertInt64ToString(propVariant.lVal);
    case VT_I8:
      return ConvertInt64ToString(*(Int64 *)(&propVariant.hVal));

    case VT_BOOL:
      return VARIANT_BOOLToBool(propVariant.boolVal) ? L"1" : L"0";
    default:
      #ifndef _WIN32_WCE
      throw 150245;
      #else
      return CSysString();
      #endif
  }
}

UINT64 ConvertPropVariantToUINT64(const PROPVARIANT &propVariant)
{
  switch (propVariant.vt)
  {
    case VT_UI1:
      return propVariant.bVal;
    case VT_UI2:
      return propVariant.uiVal;
    case VT_UI4:
      return propVariant.ulVal;
    case VT_UI8:
      return (*(UINT64 *)(&propVariant.uhVal));
    default:
      #ifndef _WIN32_WCE
      throw 151199;
      #else
      return 0;
      #endif
  }
}
