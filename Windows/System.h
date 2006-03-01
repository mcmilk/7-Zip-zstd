// Windows/System.h

#ifndef __WINDOWS_SYSTEM_H
#define __WINDOWS_SYSTEM_H

#include "..\Common\Types.h"

namespace NWindows {
namespace NSystem {

static UInt32 GetNumberOfProcessors()
{
  SYSTEM_INFO systemInfo;
  GetSystemInfo(&systemInfo);
  return (UInt32)systemInfo.dwNumberOfProcessors;
}


}}

#endif
