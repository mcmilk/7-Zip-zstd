// Common/Vector.h

#pragma once

#ifndef __COMMON_VECTOR_H
#define __COMMON_VECTOR_H

#include "Defs.h"

///////////////////////////
// class CBaseRecordVector

class CBaseRecordVector
{
  void MoveItems(int destIndex, int srcIndex);
protected:
	int _capacity;
  int _size;
	void *_items;
  size_t _itemSize;
	void Grow();
	void ReserveOnePosition();

  void TestIndexAndCorrectNum(int index, int &num) const;
  void InsertOneItem(int index);
public:
	CBaseRecordVector(size_t itemSize);
	virtual ~CBaseRecordVector();
  int Size() const {  return _size; }
	bool IsEmpty() const {  return (_size == 0); }
	void Reserve(int newCapacity);
	void Clear();
	virtual void Delete(int index, int num = 1);
	void DeleteFrom(int index);
	void DeleteBack();
};

//////////////////////////
// class CRecordVector

template <class T>
class CRecordVector: public CBaseRecordVector
{
public:
  CRecordVector():CBaseRecordVector(sizeof(T)){};
  CRecordVector(const CRecordVector &recordVector);
	CRecordVector& operator=(const CRecordVector &recordVector);
  CRecordVector& operator+=(const CRecordVector &recordVector);
  T* GetPointer() const { return (T*)_items; }
  // T operator[](int index) const { return ((T *)_items)[index]; }
  const T& operator[](int index) const { return ((T *)_items)[index]; }
	T& operator[](int index) { return ((T *)_items)[index]; }
	const T& Front() const { return operator[](0); }
  T& Front()   { return operator[](0); }
	const T& Back() const { return operator[](_size - 1); }
  T& Back()   { return operator[](_size - 1); }
	virtual int Add(T item);
	virtual void Insert(int index, T item);
  static int CompareRecordItems(const void *elem1, const void *elem2);
  void Sort();
};

template <class T>
CRecordVector<T>::CRecordVector(const CRecordVector<T> &recordVector):
  CBaseRecordVector(sizeof(T))
{
  *this = recordVector;
}

template <class T>
CRecordVector<T>& CRecordVector<T>::operator=(const CRecordVector<T> &recordVector)
{
  Clear();
  return (*this += recordVector);
}

template <class T>
CRecordVector<T>& CRecordVector<T>::operator+=(const CRecordVector<T> &recordVector)
{
  int size = recordVector.Size();
  Reserve(Size() + size);
  for(int i = 0; i < size; i++)
    Add(recordVector[i]);
  return *this;
}

template <class T>
int CRecordVector<T>::Add(T item)
{
  ReserveOnePosition();
  ((T *)_items)[_size] = item;
  return _size++;
}

template <class T>
void CRecordVector<T>::Insert(int index, T item)
{
  InsertOneItem(index);
  ((T *)_items)[index] = item;
}

template <class T>
int CRecordVector<T>::CompareRecordItems(const void *elem1, const void *elem2)
  {  return MyCompare(*((const T *)elem1), *((const T *)elem2)); }

template <class T>
void CRecordVector<T>::Sort()
{
  qsort(&Front(), Size(), _itemSize, CompareRecordItems);
}

/////////////////////////////
// CRecordVector typedefs

typedef CRecordVector<int> CIntVector;
typedef CRecordVector<unsigned int> CUIntVector;
typedef CRecordVector<bool> CBoolVector;
typedef CRecordVector<unsigned char> CByteVector;
typedef CRecordVector<void *> CPointerVector;

template <class T>
class CUniqueRecordVector: public CRecordVector<T>
{
public:
	virtual int Find(const T& item) const
  {
    for(int i = 0; i < _size; i++)
      if (item == (*this)[i])
        return i;
    return -1;
  }
	virtual int AddUnique(const T& item)
  {
    int index = Find(item);
    if (index >= 0)
      return index;
    return Add(item);
  }
};

typedef CUniqueRecordVector<int> CUniqueIntVector;

//////////////////////////
// class CObjectVector 

template <class T>
class CObjectVector: public CPointerVector
{
public:
  CObjectVector(){};
  ~CObjectVector();
  CObjectVector(const CObjectVector &objectVector);
	CObjectVector& operator=(const CObjectVector &objectVector);
	CObjectVector& operator+=(const CObjectVector &objectVector);
	const T& operator[](int index) const { return *((T *)CPointerVector::operator[](index)); }
	T& operator[](int index) { return *((T *)CPointerVector::operator[](index)); }
	T& Front() { return operator[](0); }
	const T& Front() const { return operator[](0); }
	T& Back() { return operator[](_size - 1); }
	const T& Back() const { return operator[](_size - 1); }
	virtual int Add(const T& item);
	virtual void Insert(int index, const T& item);
	virtual void Delete(int index, int num = 1);
  int Find(const T& item) const;
  int FindInSorted(const T& item) const;
  int AddToSorted(const T& item);
  static int CompareObjectItems(const void *elem1, const void *elem2);
  void Sort();
};

template <class T>
CObjectVector<T>::~CObjectVector()
{
  Clear();
}

template <class T>
CObjectVector<T>::CObjectVector(const CObjectVector<T> &objectVector)
{
  *this = objectVector;
}

template <class T>
CObjectVector<T>& CObjectVector<T>::operator=(const CObjectVector<T> &objectVector)
{
  Clear();
  return (*this += objectVector);
}

template <class T>
CObjectVector<T>& CObjectVector<T>::operator+=(const CObjectVector<T> &objectVector)
{
  int size = objectVector.Size();
  Reserve(Size() + size);
  for(int i = 0; i < size; i++)
    Add(objectVector[i]);
  return *this;
}

template <class T>
int CObjectVector<T>::Add(const T& item)
{
  return CPointerVector::Add(new T(item));
}

template <class T>
void CObjectVector<T>::Insert(int index, const T& item)
{
  CPointerVector::Insert(index, new T(item));
}

template <class T>
void CObjectVector<T>::Delete(int index, int num)
{
  TestIndexAndCorrectNum(index, num);
  for(int i = 0; i < num; i++)
    delete (T *)(((void **)_items)[index + i]);
  CPointerVector::Delete(index, num);
}

template <class T>
int CObjectVector<T>::Find(const T& item) const
{
  for(int i = 0; i < Size(); i++)
    if (item == (*this)[mid])
      return i;
  return -1;
}

template <class T>
int CObjectVector<T>::FindInSorted(const T& item) const
{
  int left = 0, right = Size(); 
  while (left != right)
  {
    int mid = (left + right) / 2;
    const T& midValue = (*this)[mid];
    if (item == midValue)
      return mid;
    if (item < midValue)
      right = mid;
    else
      left = mid + 1;
  }
  return -1;
}

template <class T>
int CObjectVector<T>::AddToSorted(const T& item)
{
  int left = 0, right = Size(); 
  while (left != right)
  {
    int mid = (left + right) / 2;
    const T& midValue = (*this)[mid];
    if (item == midValue)
    {
      right = mid + 1;
      break;
    }
    if (item < midValue)
      right = mid;
    else
      left = mid + 1;
  }
  Insert(right, item);
  return right;
}

template <class T>
int CObjectVector<T>::CompareObjectItems(const void *elem1, const void *elem2)
  {  return MyCompare(*(*((const T **)elem1)), *(*((const T **)elem2))); }

template <class T>
void CObjectVector<T>::Sort()
{
  CPointerVector &pointerVector = *this;
  qsort(&pointerVector[0], Size(), sizeof(void *), CompareObjectItems);
}

#endif 
