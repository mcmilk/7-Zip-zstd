// Common/AlignedBuffer.h

#ifndef __COMMON_ALIGNEDBUFFER_H
#define __COMMON_ALIGNEDBUFFER_H

#include <stddef.h>

class CAlignedBuffer
{
  unsigned char *m_Buffer;
public:
  CAlignedBuffer(): m_Buffer(0) {};
  ~CAlignedBuffer() { Free(); }
  void *Allocate(size_t size, size_t mask);
  void Free();
};

#endif
