// Common/DynamicBuffer.h

#pragma once

#ifndef __COMMON_DYNAMICBUFFER_H
#define __COMMON_DYNAMICBUFFER_H

#include "Buffer.h"

template <class T> class CDynamicBuffer: public CBuffer<T>
{    
  void GrowLength(size_t size)
  {
    size_t delta;
    if (_capacity > 64)
      delta = _capacity / 4;
    else if (_capacity > 8)
      delta = 16;
    else
      delta = 4;
    delta = MyMax(delta, size);
    SetCapacity(_capacity + delta);
  }
public:
  CDynamicBuffer(): CBuffer<T>() {};
  CDynamicBuffer(const CDynamicBuffer &buffer): CBuffer<T>(buffer) {};
  CDynamicBuffer(size_t size): CBuffer<T>(size) {};
  CDynamicBuffer& operator=(const CDynamicBuffer &buffer)
  {
    Free();
    if(buffer._capacity > 0)
    {
      SetCapacity(buffer._capacity);
      memmove(_items, buffer._items, buffer._capacity * sizeof(T));
    }
    return *this;
  }
  void EnsureCapacity(size_t capacity)
  {
    if (_capacity < capacity)
      GrowLength(capacity - _capacity);
  }
};

typedef CDynamicBuffer<char> CCharDynamicBuffer;
typedef CDynamicBuffer<wchar_t> CWCharDynamicBuffer;
typedef CDynamicBuffer<unsigned char> CByteDynamicBuffer;

#endif
