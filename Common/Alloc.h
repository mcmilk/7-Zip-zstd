// Common/Alloc.h

#ifndef __COMMON_ALLOC_H
#define __COMMON_ALLOC_H

#include <stddef.h>

void *MyAlloc(size_t size);
void MyFree(void *address);
void *BigAlloc(size_t size);
void BigFree(void *address);
// void *BigAllocE(size_t size);

#endif
