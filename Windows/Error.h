// Windows/Error.h

#pragma once

#ifndef __WINDOWS_ERROR_H
#define __WINDOWS_ERROR_H

#include "Common/String.h"

namespace NWindows {
namespace NError {

bool MyFormatMessage(DWORD messageID, CSysString &message);
inline CSysString MyFormatMessage(DWORD messageID)
{
  CSysString message;
  MyFormatMessage(messageID, message);
  return message;
}

}}

#endif
