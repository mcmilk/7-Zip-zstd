// FormatUtils.cpp

#include "StdAfx.h"

#include "FormatUtils.h"
#include "Windows/ResourceString.h"

CSysString MyFormat(const CSysString &aFormat, const CSysString &aString)
{
  CSysString aResult;
  _stprintf(aResult.GetBuffer(aFormat.Length() + aString.Length() + 2), 
      aFormat, aString);
  aResult.ReleaseBuffer();
  return aResult;
}

CSysString MyFormat(UINT32 anID, const CSysString &aString)
{
  return MyFormat(NWindows::MyLoadString(anID), aString);
}

CSysString NumberToString(UINT64 aNumber)
{
  TCHAR aTmp[32];
  _stprintf(aTmp, _T("%I64u"), aNumber);
  return aTmp;
}
