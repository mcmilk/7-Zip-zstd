// Windows/Error.h

#include "StdAfx.h"

#include "Windows/Error.h"
#ifndef _UNICODE
#include "Common/StringConvert.h"
#endif

namespace NWindows {
namespace NError {

bool MyFormatMessage(DWORD messageID, CSysString &message)
{
  LPVOID msgBuf;
  if(::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | 
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      messageID,
      0, // Default language
      (LPTSTR) &msgBuf,
      0,
      NULL) == 0)
    return false;

  message = (LPCTSTR)msgBuf;
  ::LocalFree(msgBuf);
  return true;
}

#ifndef _UNICODE
bool MyFormatMessage(DWORD messageID, UString &message)
{
  LPVOID msgBuf;
  if(::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | 
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      messageID,
      0, // Default language
      (LPWSTR) &msgBuf,
      0,
      NULL) == 0)
  {
    if (::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
      return false;
    CSysString messageSys;
    bool result = MyFormatMessage(messageID, messageSys);
    message = GetUnicodeString(messageSys);
    return result;
  }
  message = (LPCWSTR)msgBuf;
  ::LocalFree(msgBuf);
  return true;
}
#endif

}}
