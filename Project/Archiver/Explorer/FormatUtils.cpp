// FormatUtils.cpp

#include "StdAfx.h"

#include "FormatUtils.h"
#include "Windows/ResourceString.h"

#ifdef LANG
#include "../Common/LangUtils.h"
#endif

CSysString MyFormat(const CSysString &aFormat, const CSysString &aString)
{
  CSysString aResult;
  _stprintf(aResult.GetBuffer(aFormat.Length() + aString.Length() + 2), 
      aFormat, aString);
  aResult.ReleaseBuffer();
  return aResult;
}

CSysString MyFormat(UINT32 aResourceID, 
    #ifdef LANG
    UINT32 aLangID, 
    #endif
    const CSysString &aString)
{
  return MyFormat(
    #ifdef LANG
    LangLoadString(aResourceID, aLangID), 
    #else
    NWindows::MyLoadString(aResourceID), 
    #endif
    
    aString);
}

CSysString NumberToString(UINT64 aNumber)
{
  TCHAR aTmp[32];
  _stprintf(aTmp, _T("%I64u"), aNumber);
  return aTmp;
}
