/* 7zTypes.h */

#ifndef __COMMON_TYPES_H
#define __COMMON_TYPES_H

#ifndef UInt32
#ifdef _LZMA_UINT32_IS_ULONG
#define UInt32 unsigned long
#else
#define UInt32 unsigned int
#endif
#endif

#ifndef Byte
#define Byte unsigned char
#endif

#ifndef UInt16
#define UInt16 unsigned short
#endif

/* #define _SZ_NO_INT_64 */
/* define it your compiler doesn't support long long int */

#ifdef _SZ_NO_INT_64
#define UInt64 unsigned long
#else
#ifdef _MSC_VER
#define UInt64 unsigned __int64
#else
#define UInt64 unsigned long long int
#endif
#endif


/* #define _SZ_FILE_SIZE_64 */
/* Use _SZ_FILE_SIZE_64 if you need support for files larger than 4 GB*/

#ifndef CFileSize
#ifdef _SZ_FILE_SIZE_64
#define CFileSize UInt64
#else
#define CFileSize UInt32
#endif
#endif

#define SZ_RESULT int

#define SZ_OK (0)
#define SZE_DATA_ERROR (1)
#define SZE_OUTOFMEMORY (2)
#define SZE_CRC_ERROR (3)

#define SZE_NOTIMPL (4)
#define SZE_FAIL (5)

#define SZE_ARCHIVE_ERROR (6)

#define RINOK(x) { int __result_ = (x); if(__result_ != 0) return __result_; }

#endif
