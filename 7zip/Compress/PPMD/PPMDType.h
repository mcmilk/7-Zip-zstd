/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2001                                                               *
 *  Contents: compilation parameters and miscelaneous definitions           *
 *  Comments: system & compiler dependent file                              *
 ****************************************************************************/
#if !defined(_PPMDTYPE_H_)
#define _PPMDTYPE_H_

#include <stdio.h>

const UINT32 kMinMemSize = (1 << 11); 
const UINT32 kMinOrder = 2;
const int kMaxOrderCompress = 32;
const int MAX_O=255;                         /* maximum allowed model order */

#define _WIN32_ENVIRONMENT_
//#define _DOS32_ENVIRONMENT_
//#define _POSIX_ENVIRONMENT_
//#define _UNKNOWN_ENVIRONMENT_
#if defined(_WIN32_ENVIRONMENT_)+defined(_DOS32_ENVIRONMENT_)+defined(_POSIX_ENVIRONMENT_)+defined(_UNKNOWN_ENVIRONMENT_) != 1
#error Only one environment must be defined
#endif /* defined(_WIN32_ENVIRONMENT_)+defined(_DOS32_ENVIRONMENT_)+defined(_POSIX_ENVIRONMENT_)+defined(_UNKNOWN_ENVIRONMENT_) != 1 */

#if defined(_WIN32_ENVIRONMENT_)
#include <wtypes.h>
#else /* _DOS32_ENVIRONMENT_ || _POSIX_ENVIRONMENT_ || _UNKNOWN_ENVIRONMENT_ */
typedef int   BOOL;
#define FALSE 0
#define TRUE  1
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
typedef unsigned int   UINT;
#endif /* defined(_WIN32_ENVIRONMENT_)  */

#if !defined(_UNKNOWN_ENVIRONMENT_) && !defined(__GNUC__)
#define _FASTCALL __fastcall
#define _STDCALL  __stdcall
#else
#define _FASTCALL
#define _STDCALL
#endif /* !defined(_UNKNOWN_ENVIRONMENT_) && !defined(__GNUC__) */

#if defined(__GNUC__)
#define _PACK_ATTR __attribute__ ((packed))
#else
#define _PACK_ATTR
#endif /* defined(__GNUC__) */

// PPMd module works with file streams via ...GETC/...PUTC macros only
typedef FILE _PPMD_FILE;
#define _PPMD_E_GETC(fp)   getc(fp)
#define _PPMD_E_PUTC(c,fp) putc((c),fp)
#define _PPMD_D_GETC(fp)   getc(fp)
#define _PPMD_D_PUTC(c,fp) putc((c),fp)

template <class T>
inline void _PPMD_SWAP(T& t1,T& t2) { T tmp=t1; t1=t2; t2=tmp; }

#endif /* !defined(_PPMDTYPE_H_) */
