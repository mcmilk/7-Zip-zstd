// Common/Types.h

#ifndef __COMMON_TYPES_H
#define __COMMON_TYPES_H

#ifndef _7ZIP_BYTE_DEFINED
#define _7ZIP_BYTE_DEFINED
typedef unsigned char Byte;
#endif 

#ifndef _7ZIP_INT16_DEFINED
#define _7ZIP_INT16_DEFINED
typedef short Int16;
#endif 

#ifndef _7ZIP_UINT16_DEFINED
#define _7ZIP_UINT16_DEFINED
typedef unsigned short UInt16;
#endif 

#ifndef _7ZIP_INT32_DEFINED
#define _7ZIP_INT32_DEFINED
typedef int Int32;
#endif 

#ifndef _7ZIP_UINT32_DEFINED
#define _7ZIP_UINT32_DEFINED
typedef unsigned int UInt32;
#endif 

#ifdef _MSC_VER

#ifndef _7ZIP_INT64_DEFINED
#define _7ZIP_INT64_DEFINED
typedef __int64 Int64;
#endif 

#ifndef _7ZIP_UINT64_DEFINED
#define _7ZIP_UINT64_DEFINED
typedef unsigned __int64 UInt64;
#endif 

#else

#ifndef _7ZIP_INT64_DEFINED
#define _7ZIP_INT64_DEFINED
typedef long long int Int64;
#endif 

#ifndef _7ZIP_UINT64_DEFINED
#define _7ZIP_UINT64_DEFINED
typedef unsigned long long int UInt64;
#endif 

#endif

#endif
