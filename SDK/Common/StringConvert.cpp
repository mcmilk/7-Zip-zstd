// Common/StringConvert.cpp

#include "StdAfx.h"

#include "StringConvert.h"

UString MultiByteToUnicodeString(const AString &aSrcString, 
    UINT aCodePage)
{
  UString aResultString;
  if(!aSrcString.IsEmpty())
  {
    int aNumChars = MultiByteToWideChar(aCodePage, 0, aSrcString, 
      aSrcString.Length(), aResultString.GetBuffer(aSrcString.Length()), 
      aSrcString.Length() + 1);
    #ifndef _WIN32_WCE
    if(aNumChars == 0)
      throw 282228;
    #endif
    aResultString.ReleaseBuffer(aNumChars);
  }
  return aResultString;
}

AString UnicodeStringToMultiByte(const UString &aSrcString, 
    unsigned int aCodePage)
{
  AString aResultString;
  if(!aSrcString.IsEmpty())
  {
    int aNumRequiredBytes = aSrcString.Length() * 2;
    int aNumChars = WideCharToMultiByte(aCodePage, 0, aSrcString, 
      aSrcString.Length(), aResultString.GetBuffer(aNumRequiredBytes), 
      aNumRequiredBytes + 1, NULL, NULL);
    #ifndef _WIN32_WCE
    if(aNumChars == 0)
      throw 282229;
    #endif
    aResultString.ReleaseBuffer(aNumChars);
  }
  return aResultString;
}

#ifndef _WIN32_WCE
AString SystemStringToOemString(const CSysString &aSrcString)
{
  AString aResult;
  CharToOem(aSrcString, aResult.GetBuffer(aSrcString.Length() * 2));
  aResult.ReleaseBuffer();
  return aResult;
}
#endif
