// PropVariantConversions.cpp

#include "StdAfx.h"

#include "PropVariantConversions.h"

#include "Windows/NationalTime.h"
#include "Windows/Defs.h"

#include "Common/StringConvert.h"
#include "Common/IntToString.h"

using namespace NWindows;

static CSysString ConvertUINT64ToString(UINT64 value)
{
  TCHAR buffer[32];
  ConvertUINT64ToString(value, buffer);
  return buffer;
}

static CSysString ConvertINT64ToString(INT64 value)
{
  TCHAR buffer[32];
  ConvertINT64ToString(value, buffer);
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

CSysString ConvertFileTimeToString2(const FILETIME &fileTime, 
    bool includeTime, bool includeSeconds)
{
  CSysString string;
  SYSTEMTIME systemTime;
  if(!BOOLToBool(FileTimeToSystemTime(&fileTime, &systemTime)))
    return string;
  TCHAR buffer[64];
  wsprintf(buffer, TEXT("%04d-%02d-%02d"), systemTime.wYear, systemTime.wMonth, systemTime.wDay);
  if (includeTime)
  {
    wsprintf(buffer + lstrlen(buffer), TEXT(" %02d:%02d"), systemTime.wHour, systemTime.wMinute);
    if (includeSeconds)
      wsprintf(buffer + lstrlen(buffer), TEXT(":%02d"), systemTime.wSecond);
  }
  return buffer;
}
 

CSysString ConvertPropVariantToString(const PROPVARIANT &propVariant)
{
  switch (propVariant.vt)
  {
    case VT_EMPTY:
      return CSysString();
    case VT_BSTR:
      return GetSystemString(propVariant.bstrVal);
    case VT_UI1:
      return ConvertUINT64ToString(propVariant.bVal);
    case VT_UI2:
      return ConvertUINT64ToString(propVariant.uiVal);
    case VT_UI4:
      return ConvertUINT64ToString(propVariant.ulVal);
    case VT_UI8:
      return ConvertUINT64ToString(*(UINT64 *)(&propVariant.uhVal));
    case VT_FILETIME:
      return ConvertFileTimeToString2(propVariant.filetime, true, true);


    /*
    case VT_I1:
      return ConvertINT64ToString(propVariant.cVal);
    */
    case VT_I2:
      return ConvertINT64ToString(propVariant.iVal);
    case VT_I4:
      return ConvertINT64ToString(propVariant.lVal);
    case VT_I8:
      return ConvertINT64ToString(*(INT64 *)(&propVariant.hVal));

    case VT_BOOL:
      return VARIANT_BOOLToBool(propVariant.boolVal) ? TEXT("1") : TEXT("0");
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
