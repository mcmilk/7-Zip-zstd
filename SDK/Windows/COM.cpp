// Windows/COM.cpp

#include "StdAfx.h"

#include "Windows/COM.h"
#include "Common/StringConvert.h"

namespace NWindows {
namespace NCOM {

//////////////////////////////////
// GUID <--> String Conversions
  
// CoInitialize (NULL); must be called!

UString GUIDToStringW(REFGUID aGUID)
{
  UString aString;
  const kStringSize = 48;
  StringFromGUID2(aGUID, aString.GetBuffer(kStringSize), kStringSize);
  aString.ReleaseBuffer();
  return aString;
}

AString GUIDToStringA(REFGUID aGUID)
{
  return GetAnsiString(GUIDToStringW(aGUID));
}

HRESULT StringToGUIDW(const wchar_t *aString, GUID &aClassID)
{
  return CLSIDFromString((wchar_t *)aString, &aClassID);
}

HRESULT StringToGUIDA(const char *aString, GUID &aClassID)
{
  return StringToGUIDW(MultiByteToUnicodeString(aString), aClassID);
}

}}