// Common/Types.h

#pragma once

#ifndef __COMMON_TYPES_H
#define __COMMON_TYPES_H

#include <basetsd.h>

typedef unsigned char   UINT8;
typedef unsigned short  UINT16;
typedef short INT16;
#ifndef _WINDOWS_ 
  // typedef unsigned long UINT32;
  typedef UINT8 BYTE;
#endif

#endif

