// Windows/Error.h

#include "StdAfx.h"

#include "Windows/Error.h"

namespace NWindows {
namespace NError {

bool MyFormatMessage(DWORD aMessageID, CSysString &aMessage)
{
  LPVOID lpMsgBuf;
  if(::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | 
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      aMessageID,
      0, // Default language
      (LPTSTR) &lpMsgBuf,
      0,
      NULL) == 0)
    return false;

  aMessage = (LPCTSTR)lpMsgBuf;
  ::LocalFree( lpMsgBuf );
  return true;
}


}}
