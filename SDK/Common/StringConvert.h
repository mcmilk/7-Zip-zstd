// Common/StringConvert.h

#pragma once

#ifndef __COMMON_STRINGCONVERT_H
#define __COMMON_STRINGCONVERT_H

#include "Common/String.h"

UString MultiByteToUnicodeString(const AString &aMultiByteString,
    UINT aCodePage = CP_ACP);

AString UnicodeStringToMultiByte(const UString &aFromString, 
    unsigned int aCodePage = CP_ACP);

inline const wchar_t* GetUnicodeString(const wchar_t* anUnicodeString)
  { return anUnicodeString; }
inline const UString& GetUnicodeString(const UString &anUnicodeString)
  { return anUnicodeString; }
inline UString GetUnicodeString(const AString &anAnsiString)
  { return MultiByteToUnicodeString(anAnsiString); }
inline UString GetUnicodeString(const AString &aMultiByteString, UINT aCodePage)
  { return MultiByteToUnicodeString(aMultiByteString, aCodePage); }
inline const wchar_t* GetUnicodeString(const wchar_t* anUnicodeString, UINT aCodePage)
  { return anUnicodeString; }
inline const UString& GetUnicodeString(const UString &anUnicodeString, UINT aCodePage)
  { return anUnicodeString; }

inline const char* GetAnsiString(const char* anAnsiString)
  { return anAnsiString; }
inline const AString& GetAnsiString(const AString &anAnsiString)
  { return anAnsiString; }
inline AString GetAnsiString(const UString &anUnicodeString)
  { return UnicodeStringToMultiByte(anUnicodeString); }

inline const char* GetOemString(const char* anOemString)
  { return anOemString; }
inline const AString& GetOemString(const AString &anOemString)
  { return anOemString; }
inline AString GetOemString(const UString &anUnicodeString)
  { return UnicodeStringToMultiByte(anUnicodeString, CP_OEMCP); }


#ifdef _UNICODE
  inline const wchar_t* GetSystemString(const wchar_t* anUnicodeString)
    { return anUnicodeString;}
  inline const UString& GetSystemString(const UString &anUnicodeString)
    { return anUnicodeString;}
  inline const wchar_t* GetSystemString(const wchar_t* anUnicodeString, UINT aCodePage)
    { return anUnicodeString;}
  inline const UString& GetSystemString(const UString &anUnicodeString, UINT aCodePage)
    { return anUnicodeString;}
  inline UString GetSystemString(const AString &aMultiByteString, UINT aCodePage)
    { return MultiByteToUnicodeString(aMultiByteString, aCodePage);}
  inline UString GetSystemString(const AString &aMultiByteString)
    { return MultiByteToUnicodeString(aMultiByteString);}
#else
  inline const char* GetSystemString(const char *anAnsiString)
    { return anAnsiString; }
  inline const AString& GetSystemString(const AString &aMultiByteString, UINT aCodePage)
    { return aMultiByteString; }
  inline const char * GetSystemString(const char *aMultiByteString, UINT aCodePage)
    { return aMultiByteString; }
  inline AString GetSystemString(const UString &anUnicodeString)
    { return UnicodeStringToMultiByte(anUnicodeString); }
  inline AString GetSystemString(const UString &anUnicodeString, UINT aCodePage)
    { return UnicodeStringToMultiByte(anUnicodeString, aCodePage); }
#endif

#ifndef _WIN32_WCE
AString SystemStringToOemString(const CSysString &aSrcString);
#endif

#endif
