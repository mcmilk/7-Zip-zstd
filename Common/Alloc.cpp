// Common/Alloc.cpp

#include "StdAfx.h"

#ifdef _WIN32
#include "MyWindows.h"
#else
#include <stdlib.h>
#endif

#include "Alloc.h"

void *MyAlloc(size_t size)
{
  return ::malloc(size);
}

void MyFree(void *address)
{
  ::free(address);
}

void *BigAlloc(size_t size)
{
  #ifdef _WIN32
  return ::VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
  #else
  return ::malloc(size);
  #endif
}

void BigFree(void *address)
{
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
