// AlignedBuffer.h

#ifndef __ALIGNBUFFER_H
#define __ALIGNBUFFER_H

class CAlignedBuffer
{
  unsigned char *m_Buffer;
public:
  CAlignedBuffer(): m_Buffer(0) {};
  ~CAlignedBuffer();
  void *Allocate(size_t aNumItems, size_t anItemSize, size_t anAlignValue);
  void Free();
};

#endif
