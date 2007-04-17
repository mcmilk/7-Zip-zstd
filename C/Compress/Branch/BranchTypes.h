/* BranchTypes.h */

#ifndef __BRANCHTYPES_H
#define __BRANCHTYPES_H

#ifndef _7ZIP_BYTE_DEFINED
#define _7ZIP_BYTE_DEFINED
typedef unsigned char Byte;
#endif 

#ifndef _7ZIP_UINT16_DEFINED
#define _7ZIP_UINT16_DEFINED
typedef unsigned short UInt16;
#endif 

#ifndef _7ZIP_UINT32_DEFINED
#define _7ZIP_UINT32_DEFINED
#ifdef _LZMA_UINT32_IS_ULONG
typedef unsigned long UInt32;
#else
typedef unsigned int UInt32;
#endif
#endif

#ifndef _7ZIP_UINT64_DEFINED
#define _7ZIP_UINT64_DEFINED
#ifdef _SZ_NO_INT_64
typedef unsigned long UInt64;
#else
#ifdef _MSC_VER
typedef unsigned __int64 UInt64;
#else
typedef unsigned long long int UInt64;
#endif
#endif
#endif

#endif
