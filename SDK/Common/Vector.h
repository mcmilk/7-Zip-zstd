// Common/Vector.h

#pragma once

#ifndef __COMMON_VECTOR_H
#define __COMMON_VECTOR_H

///////////////////////////
// class CBaseRecordVector

class CBaseRecordVector
{
  void MoveItems(int aDestinationIndex, int aSourceIndex);
protected:
	int m_Capacity;
  int m_Size;
	void *m_Items;
  size_t m_ItemSize;
	void Grow();
	void ReserveOnePosition();

  void TestIndexAndCorrectNum(int anIndex, int &aNum) const;
  void InsertOneItem(int anIndex);
public:
	CBaseRecordVector(size_t anItemSize);
	virtual ~CBaseRecordVector();
  int Size() const {  return m_Size; }
	bool IsEmpty() const {  return (m_Size == 0); }
	void Reserve(int aNewCapacity);
	void Clear();
	virtual void Delete(int anIndex, int aNum = 1);
	void DeleteFrom(int anIndex);
	void DeleteBack();
};

//////////////////////////
// class CRecordVector

template <class T>
class CRecordVector: public CBaseRecordVector
{
public:
  CRecordVector():CBaseRecordVector(sizeof(T)){};
  CRecordVector(const CRecordVector &aRecordVector);
	CRecordVector& operator=(const CRecordVector &aRecordVector);
  CRecordVector& operator+=(const CRecordVector &aRecordVector);
  T* GetPointer() const { return (T*)m_Items; }
  // T operator[](int anIndex) const { return ((T *)m_Items)[anIndex]; }
  const T& operator[](int anIndex) const { return ((T *)m_Items)[anIndex]; }
	T& operator[](int anIndex) { return ((T *)m_Items)[anIndex]; }
	const T& Front() const { return operator[](0); }
  T& Front()   { return operator[](0); }
	const T& Back() const { return operator[](m_Size - 1); }
  T& Back()   { return operator[](m_Size - 1); }
	virtual int Add(T anItem);
	virtual void Insert(int anIndex, T anItem);
};

template <class T>
CRecordVector<T>::CRecordVector(const CRecordVector<T> &aRecordVector):
  CBaseRecordVector(sizeof(T))
{
  *this = aRecordVector;
}

template <class T>
CRecordVector<T>& CRecordVector<T>::operator=(const CRecordVector<T> &aRecordVector)
{
  Clear();
  return (*this += aRecordVector);
}

template <class T>
CRecordVector<T>& CRecordVector<T>::operator+=(const CRecordVector<T> &aRecordVector)
{
  int aSize = aRecordVector.Size();
  Reserve(Size() + aSize);
  for(int i = 0; i < aSize; i++)
    Add(aRecordVector[i]);
  return *this;
}

template <class T>
int CRecordVector<T>::Add(T anItem)
{
  ReserveOnePosition();
  ((T *)m_Items)[m_Size] = anItem;
  return m_Size++;
}

template <class T>
void CRecordVector<T>::Insert(int anIndex, T anItem)
{
  InsertOneItem(anIndex);
  ((T *)m_Items)[anIndex] = anItem;
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
	virtual int Find(const T& anItem) const
  {
    for(int i = 0; i < m_Size; i++)
      if (anItem == (*this)[i])
        return i;
    return -1;
  }
	virtual int AddUnique(const T& anItem)
  {
    int anIndex = Find(anItem);
    if (anIndex >= 0)
      return anIndex;
    return Add(anItem);
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
  CObjectVector(const CObjectVector &anObjectVector);
	CObjectVector& operator=(const CObjectVector &anObjectVector);
	CObjectVector& operator+=(const CObjectVector &anObjectVector);
	const T& operator[](int anIndex) const { return *((T *)CPointerVector::operator[](anIndex)); }
	T& operator[](int anIndex) { return *((T *)CPointerVector::operator[](anIndex)); }
	T& Front() { return operator[](0); }
	const T& Front() const { return operator[](0); }
	T& Back() { return operator[](m_Size - 1); }
	const T& Back() const { return operator[](m_Size - 1); }
	virtual int Add(const T& anItem);
	virtual void Insert(int anIndex, const T& anItem);
	virtual void Delete(int anIndex, int aNum = 1);
  int Find(const T& anItem) const;
  int FindInSorted(const T& anItem) const;
  int AddToSorted(const T& anItem);
  static int CompareStringItems(const void *anElem1, const void *anElem2);
  void Sort();
};

template <class T>
CObjectVector<T>::~CObjectVector()
{
  Clear();
}

template <class T>
CObjectVector<T>::CObjectVector(const CObjectVector<T> &anObjectVector)
{
  *this = anObjectVector;
}

template <class T>
CObjectVector<T>& CObjectVector<T>::operator=(const CObjectVector<T> &anObjectVector)
{
  Clear();
  return (*this += anObjectVector);
}

template <class T>
CObjectVector<T>& CObjectVector<T>::operator+=(const CObjectVector<T> &anObjectVector)
{
  int aSize = anObjectVector.Size();
  Reserve(Size() + aSize);
  for(int i = 0; i < aSize; i++)
    Add(anObjectVector[i]);
  return *this;
}

template <class T>
int CObjectVector<T>::Add(const T& anItem)
{
  return CPointerVector::Add(new T(anItem));
}

template <class T>
void CObjectVector<T>::Insert(int anIndex, const T& anItem)
{
  CPointerVector::Insert(anIndex, new T(anItem));
}

template <class T>
void CObjectVector<T>::Delete(int anIndex, int aNum)
{
  TestIndexAndCorrectNum(anIndex, aNum);
  for(int i = 0; i < aNum; i++)
    delete (T *)(((void **)m_Items)[anIndex + i]);
  CPointerVector::Delete(anIndex, aNum);
}

template <class T>
int CObjectVector<T>::Find(const T& anItem) const
{
  for(int i = 0; i < Size(); i++)
    if (anItem == (*this)[aMid])
      return i;
  return -1;
}

template <class T>
int CObjectVector<T>::FindInSorted(const T& anItem) const
{
  int aLeft = 0, aRight = Size(); 
  while (aLeft != aRight)
  {
    int aMid = (aLeft + aRight) / 2;
    const T& aMidValue = (*this)[aMid];
    if (anItem == aMidValue)
      return aMid;
    if (anItem < aMidValue)
      aRight = aMid;
    else
      aLeft = aMid + 1;
  }
  return -1;
}

template <class T>
int CObjectVector<T>::AddToSorted(const T& anItem)
{
  int aLeft = 0, aRight = Size(); 
  while (aLeft != aRight)
  {
    int aMid = (aLeft + aRight) / 2;
    const T& aMidValue = (*this)[aMid];
    if (anItem == aMidValue)
    {
      aRight = aMid + 1;
      break;
    }
    if (anItem < aMidValue)
      aRight = aMid;
    else
      aLeft = aMid + 1;
  }
  Insert(aRight, anItem);
  return aRight;
}

template <class T>
int CObjectVector<T>::CompareStringItems(const void *anElem1, const void *anElem2)
  {  return MyCompare(*(*((const T **)anElem1)), *(*((const T **)anElem2))); }

template <class T>
void CObjectVector<T>::Sort()
{
  CPointerVector &aPointerVector = *this;
  qsort(&aPointerVector[0], Size(), sizeof(void *), CompareStringItems);
}

#endif 
