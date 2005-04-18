// Common/Alloc.cpp

#include "StdAfx.h"

#ifdef _WIN32
#include "MyWindows.h"
#else
#include <stdlib.h>
#endif

#include "Alloc.h"

/* #define _SZ_ALLOC_DEBUG */
/* use _SZ_ALLOC_DEBUG to debug alloc/free operations */
#ifdef _SZ_ALLOC_DEBUG
#include <stdio.h>
int g_allocCount = 0;
int g_allocCountBig = 0;
#endif

void *MyAlloc(size_t size)
{
  #ifdef _SZ_ALLOC_DEBUG
  fprintf(stderr, "\nAlloc %10d bytes; count = %10d", size, g_allocCount++);
  #endif

  return ::malloc(size);
}

void MyFree(void *address)
{
  #ifdef _SZ_ALLOC_DEBUG
  if (address != 0)
    fprintf(stderr, "\nFree; count = %10d", --g_allocCount);
  #endif
  
  ::free(address);
}

void *BigAlloc(size_t size)
{
  #ifdef _SZ_ALLOC_DEBUG
  fprintf(stderr, "\nAlloc_Big %10d bytes;  count = %10d", size, g_allocCountBig++);
  #endif
  
  #ifdef _WIN32
  return ::VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
  #else
  return ::malloc(size);
  #endif
}

void BigFree(void *address)
{
  #ifdef _SZ_ALLOC_DEBUG
  if (address != 0)
    fprintf(stderr, "\nFree_Big; count = %10d", --g_allocCountBig);
  #endif
  
  if (address == 0)
    return;
  #ifdef _WIN32
  ::VirtualFree(address, 0, MEM_RELEASE);
  #else
  ::free(address);
  #endif
}

/*
void *BigAllocE(size_t size)
{
  void *res = BigAlloc(size);
  #ifndef _NO_EXCEPTIONS
  if (res == 0)
    throw CNewException();
  #endif
  return res;
}
*/
