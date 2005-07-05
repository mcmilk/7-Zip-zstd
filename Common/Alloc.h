// Common/Alloc.h

#ifndef __COMMON_ALLOC_H
#define __COMMON_ALLOC_H

#include <stddef.h>

void *MyAlloc(size_t size) throw();
void MyFree(void *address) throw();
void *BigAlloc(size_t size) throw();
void BigFree(void *address) throw();
// void *BigAllocE(size_t size);

#endif
