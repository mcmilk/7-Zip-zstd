// Windows/Error.h

#pragma once

#ifndef __WINDOWS_ERROR_H
#define __WINDOWS_ERROR_H

#include "Common/String.h"

namespace NWindows {
namespace NError {

bool MyFormatMessage(DWORD aMessageID, CSysString &aMessage);
inline CSysString MyFormatMessage(DWORD aMessageID)
{
  CSysString aMessage;
  MyFormatMessage(aMessageID, aMessage);
  return aMessage;
}

}}

#endif
