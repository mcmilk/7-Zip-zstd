/* CpuArch.h */

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

#endif
