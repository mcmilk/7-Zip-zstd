// Common/MyInitGuid.h

#ifndef __COMMON_MYINITGUID_H
#define __COMMON_MYINITGUID_H

#ifdef _WIN32
#include <initguid.h>
#else
#define INITGUID
#include "MyGuidDef.h"
DEFINE_GUID(IID_IUnknown,
0x00000000, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
#endif

#endif
