// AlignedBuffer.h

#ifndef __ALIGNBUFFER_H
#define __ALIGNBUFFER_H

class CAlignedBuffer
{
  unsigned char *m_Buffer;
public:
  CAlignedBuffer(): m_Buffer(0) {};
  ~CAlignedBuffer() { Free(); }
  void *Allocate(size_t numItems, size_t itemSize, size_t alignValue);
  void Free();
};

#endif
