// PropVariantConversions.cpp

#include "StdAfx.h"

#include "PropVariantConversions.h"

#include "Windows/NationalTime.h"
#include "Windows/Defs.h"

#include "Common/StringConvert.h"
#include "Common/IntToString.h"

using namespace NWindows;

static CSysString ConvertUINT64ToString(UINT64 aValue)
{
  TCHAR aBuffer[32];
  ConvertUINT64ToString(aValue, aBuffer);
  return aBuffer;
}

static CSysString ConvertINT64ToString(INT64 aValue)
{
  TCHAR aBuffer[32];
  ConvertINT64ToString(aValue, aBuffer);
  return aBuffer;
}

static CSysString ConvertUINT32ToString(UINT32 aValue)
{
  TCHAR aBuffer[16];
  return _ultot(aValue, aBuffer, 10);
}

static CSysString ConvertINT32ToString(INT32 aValue)
{
  TCHAR aBuffer[16];
  return _ultot(aValue, aBuffer, 10);
}


/*
CSysString ConvertFileTimeToString(const FILETIME &aFileTime, bool anIncludeTime)
{
  SYSTEMTIME aSystemTime;
  if(!BOOLToBool(FileTimeToSystemTime(&aFileTime, &aSystemTime)))
    #ifndef _WIN32_WCE
    throw 311907;
    #else
    return CSysString();
    #endif

  const kBufferSize = 64;
  CSysString aStringDate;
  if(!NNational::NTime::MyGetDateFormat(LOCALE_USER_DEFAULT, 
      0, &aSystemTime, NULL, aStringDate))
    #ifndef _WIN32_WCE
    throw 311908;
    #else
    return CSysString();
    #endif
  if (!anIncludeTime)
    return aStringDate;
  CSysString aStringTime;
  if(!NNational::NTime::MyGetTimeFormat(LOCALE_USER_DEFAULT, 
      0, &aSystemTime, NULL, aStringTime))
    #ifndef _WIN32_WCE
    throw 311909;
    #else
    return CSysString();
    #endif
  return aStringDate + _T(" ") + aStringTime;
}
*/

CSysString ConvertFileTimeToString2(const FILETIME &aFileTime, 
    bool anIncludeTime, bool anIncludeSeconds)
{
  CSysString aString;
  SYSTEMTIME aTime;
  if(!BOOLToBool(FileTimeToSystemTime(&aFileTime, &aTime)))
    return aString;
  TCHAR aBuffer[64];
  _stprintf(aBuffer, TEXT("%04d-%02d-%02d"), aTime.wYear, aTime.wMonth, aTime.wDay);
  if (anIncludeTime)
  {
    _stprintf(aBuffer + _tcslen(aBuffer), TEXT(" %02d:%02d"), aTime.wHour, aTime.wMinute);
    if (anIncludeSeconds)
      _stprintf(aBuffer + _tcslen(aBuffer), TEXT(":%02d"), aTime.wSecond);
  }
  return aBuffer;
}
 

CSysString ConvertPropVariantToString(const PROPVARIANT &aPropVariant)
{
  switch (aPropVariant.vt)
  {
    case VT_EMPTY:
      return CSysString();
    case VT_BSTR:
      return GetSystemString(aPropVariant.bstrVal);
    case VT_UI1:
      return ConvertUINT32ToString(aPropVariant.bVal);
    case VT_UI2:
      return ConvertUINT32ToString(aPropVariant.uiVal);
    case VT_UI4:
      return ConvertUINT32ToString(aPropVariant.ulVal);
    case VT_UI8:
      return ConvertUINT64ToString(*(UINT64 *)(&aPropVariant.uhVal));
    case VT_FILETIME:
      return ConvertFileTimeToString2(aPropVariant.filetime, true, true);


    /*
    case VT_I1:
      return ConvertINT64ToString(aPropVariant.cVal);
    */
    case VT_I2:
      return ConvertINT32ToString(aPropVariant.iVal);
    case VT_I4:
      return ConvertINT32ToString(aPropVariant.lVal);
    case VT_I8:
      return ConvertINT64ToString(*(INT64 *)(&aPropVariant.hVal));

    case VT_BOOL:
      return VARIANT_BOOLToBool(aPropVariant.boolVal) ? _T("1") : _T("0");
    default:
      #ifndef _WIN32_WCE
      throw 150245;
      #else
      return CSysString();
      #endif
  }
}

UINT64 ConvertPropVariantToUINT64(const PROPVARIANT &aPropVariant)
{
  switch (aPropVariant.vt)
  {
    case VT_UI1:
      return aPropVariant.bVal;
    case VT_UI2:
      return aPropVariant.uiVal;
    case VT_UI4:
      return aPropVariant.ulVal;
    case VT_UI8:
      return (*(UINT64 *)(&aPropVariant.uhVal));
    default:
      #ifndef _WIN32_WCE
      throw 151199;
      #else
      return 0;
      #endif
  }
}
