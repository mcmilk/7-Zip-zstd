// Common/Buffer.h

#pragma once

#ifndef __COMMON_BUFFER_H
#define __COMMON_BUFFER_H

#include "Common/Defs.h"

template <class T> class CBuffer
{    
protected:
	size_t m_Capacity;
  T *m_Items;
public:
  CBuffer(): m_Capacity(0), m_Items(0) {};
  CBuffer(const CBuffer &aBuffer);
  CBuffer& operator=(const CBuffer &aBuffer);
  CBuffer(size_t aSize);
  virtual ~CBuffer();
  operator T *() { return m_Items; };
  operator const T *()const { return m_Items; };
  size_t GetCapacity() const { return  m_Capacity; }
  void SetCapacity(size_t aNewCapacity);
};

template <class T>
CBuffer<T>::CBuffer(const CBuffer<T> &aBuffer):
  m_Capacity(0),
  m_Items(0)
{
  *this = aBuffer;
}

template <class T>
CBuffer<T>& CBuffer<T>::operator=(const CBuffer<T> &aBuffer)
{
  if(aBuffer.m_Capacity > 0)
  {
    SetCapacity(aBuffer.m_Capacity);
    memmove(m_Items, aBuffer.m_Items, aBuffer.m_Capacity * sizeof(T));
  }
  return *this;
}

template <class T> 
void CBuffer<T>::SetCapacity(size_t aCapacity)
{
  T *aNewBuffer = new T[aCapacity];
  if(m_Capacity > 0)
    memmove(aNewBuffer, m_Items, MyMin(m_Capacity, aCapacity) * sizeof(T));
  delete []m_Items;
  m_Items = aNewBuffer;
  m_Capacity = aCapacity;
}

template <class T> 
CBuffer<T>::CBuffer(size_t aCapacity):
  m_Items(0),
  m_Capacity(0)
{
  SetCapacity(aCapacity);
}

template <class T> 
CBuffer<T>::~CBuffer()
{
  delete []m_Items;
}
    
typedef CBuffer<char> CCharBuffer;
typedef CBuffer<wchar_t> CWCharBuffer;
typedef CBuffer<unsigned char> CByteBuffer;

#endif
