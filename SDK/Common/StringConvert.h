// Common/StringConvert.h

#pragma once

#ifndef __COMMON_STRINGCONVERT_H
#define __COMMON_STRINGCONVERT_H

#include "Common/String.h"

UString MultiByteToUnicodeString(const AString &aMultiByteString,
    UINT aCodePage = CP_ACP);

AString UnicodeStringToMultiByte(const UString &aFromString, 
    unsigned int aCodePage = CP_ACP);

//#ifdef _UNICODE
  inline const wchar_t* GetUnicodeString(const wchar_t* anUnicodeString)
    { return anUnicodeString; }
  inline const UString& GetUnicodeString(const UString &anUnicodeString)
    { return anUnicodeString; }
// #else
  inline UString GetUnicodeString(const AString &anAnsiString)
    { return MultiByteToUnicodeString(anAnsiString); }
// #endif

  inline const wchar_t* GetUnicodeString(const wchar_t* anUnicodeString, UINT aCodePage)
    { return anUnicodeString; }
  inline const UString& GetUnicodeString(const UString &anUnicodeString, UINT aCodePage)
    { return anUnicodeString; }

  inline UString GetUnicodeString(const AString &anAnsiString, UINT aCodePage)
    { return MultiByteToUnicodeString(anAnsiString, aCodePage); }

  inline const char* GetAnsiString(const char* anAnsiString)
    { return anAnsiString; }
  inline const AString& GetAnsiString(const AString &anAnsiString)
    { return anAnsiString; }
  inline AString GetAnsiString(const UString &anUnicodeString)
    { return UnicodeStringToMultiByte(anUnicodeString); }


#ifdef _UNICODE
  inline const wchar_t* GetSystemString(const wchar_t* anUnicodeString)
    { return anUnicodeString;}
  inline const UString& GetSystemString(const UString &anUnicodeString)
    { return anUnicodeString;}
  inline const wchar_t* GetSystemString(const wchar_t* anUnicodeString, UINT aCodePage)
    { return anUnicodeString;}
  inline const UString& GetSystemString(const UString &anUnicodeString, UINT aCodePage)
    { return anUnicodeString;}
#else
  inline const char* GetSystemString(const char *anAnsiString)
    { return anAnsiString; }
  inline const AString& GetSystemString(const AString &anAnsiString)
    { return anAnsiString; }
  inline AString GetSystemString(const UString &anUnicodeString)
    { return UnicodeStringToMultiByte(anUnicodeString); }
  inline AString GetSystemString(const UString &anUnicodeString, UINT aCodePage)
    { return UnicodeStringToMultiByte(anUnicodeString, aCodePage); }
#endif

#ifndef _WIN32_WCE
AString SystemStringToOemString(const CSysString &aSrcString);
#endif

#endif