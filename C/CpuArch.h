/* CpuArch.h
2008-03-26
Igor Pavlov
Public domain */

#ifndef __CPUARCH_H
#define __CPUARCH_H

/* 
LITTLE_ENDIAN_UNALIGN means:
  1) CPU is LITTLE_ENDIAN
  2) it's allowed to make unaligned memory accesses
if LITTLE_ENDIAN_UNALIGN is not defined, it means that we don't know 
about these properties of platform.
*/

#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64) || defined(__i386__) || defined(__x86_64__)
#define LITTLE_ENDIAN_UNALIGN
#endif

#ifdef LITTLE_ENDIAN_UNALIGN

#define GetUi16(p) (*(const UInt16 *)(p))
#define GetUi32(p) (*(const UInt32 *)(p))
#define GetUi64(p) (*(const UInt64 *)(p))
#define SetUi32(p, d) *(UInt32 *)(p) = d;

#else

#define GetUi16(p) (((const Byte *)(p))[0] | \
           ((UInt16)((const Byte *)(p))[1] << 8))

#define GetUi32(p) (((const Byte *)(p))[0]        | \
           ((UInt32)((const Byte *)(p))[1] << 8 ) | \
           ((UInt32)((const Byte *)(p))[2] << 16) | \
           ((UInt32)((const Byte *)(p))[3] << 24))

#define GetUi64(p) (GetUi32(p) | (UInt64)GetUi32(((const Byte *)(p)) + 4) << 32)

#define SetUi32(p, d) { UInt32 _x_ = (d); \
    ((Byte *)(p))[0] = (Byte)_x_; \
    ((Byte *)(p))[1] = (Byte)(_x_ >> 8); \
    ((Byte *)(p))[2] = (Byte)(_x_ >> 16); \
    ((Byte *)(p))[3] = (Byte)(_x_ >> 24); }

#endif

#endif
