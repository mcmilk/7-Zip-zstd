// Common/DynamicBuffer.h

#pragma once

#ifndef __DYNAMICBUFFER_H
#define __DYNAMICBUFFER_H

#include "Buffer.h"

template <class T> class CDynamicBuffer: public CBuffer<T>
{    
  void GrowLength(size_t aSize);
public:
  CDynamicBuffer(): CBuffer<T>() {};
  CDynamicBuffer(const CDynamicBuffer &aBuffer): CBuffer<T>(aBuffer) {};
  CDynamicBuffer(size_t aSize): CBuffer<T>(aSize) {};
  CDynamicBuffer& operator=(const CDynamicBuffer &aBuffer);
  void EnsureCapacity(size_t aCapacity);
};

template <class T>
CDynamicBuffer<T>& CDynamicBuffer<T>::operator=(const CDynamicBuffer<T> &aBuffer)
{
  if(aBuffer.m_Capacity > 0)
  {
    SetCapacity(aBuffer.m_Capacity);
    memmove(m_Items, aBuffer.m_Items, aBuffer.m_Capacity * sizeof(T));
  }
  return *this;
}

template <class T>
void CDynamicBuffer<T>::GrowLength(size_t aSize)
{
  size_t aDelta;
  if (m_Capacity > 64)
    aDelta = m_Capacity / 4;
  else if (m_Capacity > 8)
    aDelta = 16;
  else
    aDelta = 4;
  aDelta = MyMax(aDelta, aSize);
  SetCapacity(m_Capacity + aDelta);
}

template <class T> 
void CDynamicBuffer<T>::EnsureCapacity(size_t aCapacity)
{
  if (m_Capacity < aCapacity)
    GrowLength(aCapacity - m_Capacity);
}

typedef CDynamicBuffer<char> CCharDynamicBuffer;
typedef CDynamicBuffer<wchar_t> CWCharDynamicBuffer;
typedef CDynamicBuffer<unsigned char> CByteDynamicBuffer;

#endif
