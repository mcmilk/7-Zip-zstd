// Windows/Error.h

#include "StdAfx.h"

#include "Windows/Error.h"

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

}}
