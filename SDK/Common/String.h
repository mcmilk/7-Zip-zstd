// Common/String.h

#pragma once

#ifndef __COMMON_STRING_H
#define __COMMON_STRING_H

#include "Common/Vector.h"

inline char* MyStringGetNextCharPointer(char *aPointer)
  { return (char *)_mbsinc((unsigned char *)aPointer); }

inline wchar_t* MyStringGetNextCharPointer(wchar_t *aPointer)
  { return (wchar_t*)(aPointer + 1); }

inline const char* MyStringGetNextCharPointer(const char *aPointer)
  { return (const char *)_mbsinc((const unsigned char *)aPointer); }

inline const wchar_t* MyStringGetNextCharPointer(const wchar_t *aPointer)
  { return (const wchar_t*)(aPointer + 1); }

template <class T>
class CStringBase
{
  void TrimLeftWithCharSet(const CStringBase &aCharSet);
  void TrimRightWithCharSet(const CStringBase &aCharSet);
  void InsertSpace(int &anIndex, int aSize);
  static T *GetNextCharPointer(T *aPointer)
    { return MyStringGetNextCharPointer(aPointer); }
  static const T *GetNextCharPointer(const T *aPointer)
    { return MyStringGetNextCharPointer(aPointer); }
protected:
  T *m_Chars;
  int m_Length;
	int m_Capacity;
  void SetCapacity(int aNewCapacity);
  void GrowLength(int n);
  void CorrectIndex(int &anIndex) const;
public:
  CStringBase();
  CStringBase(T aChar);
  CStringBase(const T *aChars);
  CStringBase(const CStringBase &aString);
  ~CStringBase();
  // T operator[](int anIndex) const;
  // T& operator[](int anIndex);
  operator const T*() const { return m_Chars;} 

  // The minimum size of the character buffer in characters. 
  // This value does not include space for a null terminator.
  T* GetBuffer(int aMinBufLength);
  void ReleaseBuffer();
  void ReleaseBuffer(int aNewLength);


  CStringBase& operator=(T aChar);
  CStringBase& operator=(const T *aChars);
  CStringBase& operator=(const CStringBase& aString);
  CStringBase& operator+=(T c);
  CStringBase& operator+=(const T *aString);
  CStringBase& operator+=(const CStringBase& aString);

  void Empty();
  int Length() const { return m_Length; }
  bool IsEmpty() const {   return (m_Length == 0); }

  CStringBase Mid(int aFirst) const;
  CStringBase Mid(int nFirst, int nCount ) const;
  CStringBase Left(int nCount) const;
  CStringBase Right(int nCount) const;

  void MakeUpper();
  void MakeLower();

  int Compare(const CStringBase& aString) const;
  int CompareNoCase(const CStringBase& aString) const;
  int Collate(const CStringBase& aString) const;
  int CollateNoCase(const CStringBase& aString) const;

  int Find(T aChar) const;
  int Find(T aChar, int aStartIndex) const;
  int Find(const CStringBase &aString) const;
  int Find(const CStringBase &aString, int aStartIndex) const;
  int ReverseFind(T aChar) const;
  int FindOneOf(const CStringBase &aString) const;

  void TrimLeft();
  void TrimLeft(T aChar);
  void TrimRight();
  void TrimRight(T aChar);
  void Trim();

  int Insert(int anIndex, T aChar);
  int Insert(int anIndex, const CStringBase &aString);

  int Replace(T anOldChar, T aNewChar);
  int Replace(const CStringBase &anOldString, const CStringBase &aNewString);

  int Delete(int anIndex, int aCount = 1 );
};

template <class T>
CStringBase<T>  operator+(const CStringBase<T> & aString1, const CStringBase<T> & aString2);
template <class T>
CStringBase<T>  operator+(const CStringBase<T> & aString, T aChar);
template <class T>
CStringBase<T>  operator+(T aChar, const CStringBase<T> & aString);
template <class T>
CStringBase<T>  operator+(const CStringBase<T> & aString, const T * aChars);
template <class T>
CStringBase<T>  operator+(const T * aChars, const CStringBase<T> & aString);

template <class T>
bool operator==(const CStringBase<T> & s1, const CStringBase<T> & s2);
template <class T>
bool operator!=(const CStringBase<T> & s1, const CStringBase<T> & s2);

static const char *kTrimDefaultCharSet  = " \n\t";

inline size_t MyStringLen(const char *aString)
  { return strlen(aString); }
inline size_t MyStringLen(const wchar_t *aString)
  { return wcslen(aString); }

inline char * MyStringCopy(char *strDestination, const char *strSource)
  { return strcpy(strDestination, strSource); }
inline wchar_t * MyStringCopy(wchar_t *strDestination, const wchar_t *strSource)
  { return wcscpy(strDestination, strSource); }

inline char * MyStringNCopy(char *strDestination, const char *strSource, size_t count)
  { return strncpy(strDestination, strSource, count); }
inline wchar_t * MyStringNCopy(wchar_t *strDestination, const wchar_t *strSource, size_t count)
  { return wcsncpy(strDestination, strSource, count); }

inline char * MyStringUpperCase(char *aString)
  { return (char *)_mbsupr((unsigned char *)aString); }
inline wchar_t * MyStringUpperCase(wchar_t *aString)
  { return _wcsupr(aString); }

inline char * MyStringLowerCase(char *aString)
  { return (char *)_mbsupr((unsigned char *)aString); }
inline wchar_t * MyStringLowerCase(wchar_t *aString)
  { return _wcslwr(aString); }

inline int MyStringCompare(const char *aString1, const char *aString2)
  { return _mbscmp((const unsigned char *)aString1, (const unsigned char *)aString2); }
inline int MyStringCompare(const wchar_t *aString1, const wchar_t *aString2)
  { return wcscmp(aString1, aString2); }

inline int MyStringCompareNoCase(const char *aString1, const char *aString2)
  { return _mbsicmp((const unsigned char *)aString1, (const unsigned char *)aString2); }
inline int MyStringCompareNoCase(const wchar_t *aString1, const wchar_t *aString2)
  { return _wcsicmp(aString1, aString2); }

#ifndef _WIN32_WCE

inline int MyStringCollate(const char *aString1, const char *aString2)
  { return _mbscoll((const unsigned char *)aString1, (const unsigned char *)aString2); }
inline int MyStringCollate(const wchar_t *aString1, const wchar_t *aString2)
  { return wcscoll(aString1, aString2); }

inline int MyStringCollateNoCase(const char *aString1, const char *aString2)
  { return _mbsicoll((const unsigned char *)aString1, (const unsigned char *)aString2); }
inline int MyStringCollateNoCase(const wchar_t *aString1, const wchar_t *aString2)
  { return _wcsicoll(aString1, aString2); }

#endif

inline char* MyStringFindChar(const char *aString, int aChar)
  { return (char *)_mbschr((const unsigned char *)aString, aChar); }
inline wchar_t* MyStringFindChar(const wchar_t *aString, wint_t aChar)
  { return wcschr(aString, aChar); }

inline char* MyStringFindSubString(const char *aString, const char *aStrCharSet)
  { return (char *)_mbsstr((const unsigned char *)aString, (const unsigned char *)aStrCharSet); }  
inline wchar_t* MyStringFindSubString(const wchar_t *aString, const wchar_t *aStrCharSet)
  { return wcsstr(aString, aStrCharSet); }  

inline char* MyStringReverseFind(const char *aString, int aChar)
  { return (char *)_mbsrchr((const unsigned char *)aString, aChar); }
inline wchar_t* MyStringReverseFind(const wchar_t *aString, wint_t aChar)
  { return wcsrchr(aString, aChar); }

 
//////////////////////////////////////////////

template <class T>
void CStringBase<T>::SetCapacity(int aNewCapacity)
{
  int aRealCapacity = aNewCapacity + 1;
  if(aRealCapacity == m_Capacity)
    return;
  const kMaxStringSize = 0x20000000;
  #ifndef _WIN32_WCE
  if(aNewCapacity > kMaxStringSize || aNewCapacity < m_Length)
    throw 1052337;
  #endif
  T *aNewBuffer = new T[aRealCapacity];
  if(m_Capacity > 0)
  {
    memmove(aNewBuffer, m_Chars, sizeof(T) * (m_Length + 1));
    delete []m_Chars;
    m_Chars = aNewBuffer;
  }
  else
  {
    m_Chars = aNewBuffer;
    m_Chars[0] = 0;
  }
  m_Capacity = aRealCapacity;
}

template <class T>
void CStringBase<T>::GrowLength(int n)
{
  int aFreeSize = m_Capacity - m_Length - 1;
  if (n <= aFreeSize) 
    return;
  int aDelta;
  if (m_Capacity > 64)
    aDelta = m_Capacity / 2;
  else if (m_Capacity > 8)
    aDelta = 16;
  else
    aDelta = 4;
  if (aFreeSize + aDelta < n)
    aDelta = n - aFreeSize;
  SetCapacity(m_Capacity + aDelta);
}

template <class T>
CStringBase<T>::CStringBase():
  m_Chars(NULL),
  m_Length(0),
  m_Capacity(0)
{
  SetCapacity(16 - 1);
}

template <class T>
CStringBase<T>::CStringBase(T aChar):
  m_Chars(NULL),
  m_Length(0),
  m_Capacity(0)
{
  SetCapacity(1);
  m_Chars[0] = aChar;
  m_Chars[1] = 0;
  m_Length = 1;
}

template <class T>
CStringBase<T>::CStringBase(const T *aChars):
  m_Chars(NULL),
  m_Length(0),
  m_Capacity(0)
{
  int aLength = MyStringLen(aChars);
  SetCapacity(aLength);
  MyStringCopy(m_Chars, aChars); // can be optimized by memove()
  m_Length = aLength;
}

template <class T>
CStringBase<T>::CStringBase(const CStringBase<T> &aString):
  m_Chars(NULL),
  m_Length(0),
  m_Capacity(0)
{
  SetCapacity(aString.m_Length);
  MyStringCopy(m_Chars, aString.m_Chars);
  m_Length = aString.m_Length;
}

template <class T>
CStringBase<T>::~CStringBase()
{
  delete []m_Chars;
}

template <class T>
void CStringBase<T>::CorrectIndex(int &anIndex) const
{
  if (anIndex > m_Length)
    anIndex = m_Length;
}

//////////////////////////////////
// Buffer functions

template <class T>
T* CStringBase<T>::GetBuffer(int aMinBufLength)
{
  if(aMinBufLength >= m_Capacity)
    SetCapacity(aMinBufLength + 1);
  return m_Chars;
}

template <class T>
void CStringBase<T>::ReleaseBuffer()
{
  ReleaseBuffer(MyStringLen(m_Chars));
}

template <class T>
void CStringBase<T>::ReleaseBuffer(int aNewLength)
{
  #ifndef _WIN32_WCE
  if(aNewLength >= m_Capacity)
    throw 282217;
  #endif
  m_Chars[aNewLength] = 0;
  m_Length = aNewLength;
}

void ReleaseBuffer( int nNewLength = -1 );


////////////////////////
// operator =

template <class T>
CStringBase<T>& CStringBase<T>::operator=(T aChar)
{
  Empty();
  SetCapacity(1);
  m_Chars[0] = aChar;
  m_Chars[1] = 0;
  m_Length = 1;
  return *this;
}

template <class T>
CStringBase<T>& CStringBase<T>::operator=(const T *aChars)
{
  Empty();
  int aLength = MyStringLen(aChars);
  SetCapacity(aLength);
  MyStringCopy(m_Chars, aChars);
  m_Length = aLength; 
  return *this;
}  

template <class T>
CStringBase<T>& CStringBase<T>::operator=(const CStringBase<T>& aString)
{
  if(&aString == this)
    return *this;
  Empty();
  SetCapacity(aString.m_Length);
  MyStringCopy(m_Chars, aString.m_Chars);
  m_Length = aString.m_Length;
  return *this;
}

////////////////////////
// operator +=

template <class T>
CStringBase<T>& CStringBase<T>::operator+=(T c)
{
  GrowLength(1);
  m_Chars[m_Length] = c;
  m_Chars[++m_Length] = 0;
  return *this;
}

template <class T>
CStringBase<T>& CStringBase<T>::operator+=(const T *aString)
{
  int aLen = MyStringLen(aString);
  GrowLength(aLen);
  MyStringCopy(m_Chars + m_Length, aString);
  m_Length += aLen;
  return *this;
}

template <class T>
CStringBase<T>& CStringBase<T>::operator+=(const CStringBase<T>& aString)
{
  GrowLength(aString.m_Length);
  MyStringCopy(m_Chars + m_Length, aString.m_Chars);
  m_Length += aString.m_Length;
  return *this;
}

//////////////////////////////////////////////

template <class T>
void CStringBase<T>::Empty()
{
  m_Length = 0;
  m_Chars[0] = 0;
}

//////////////////////////////////////////////

template <class T>
CStringBase<T> CStringBase<T>::Mid(int aFirst) const
{
  return Mid(aFirst, m_Length - aFirst);
}

template <class T>
CStringBase<T> CStringBase<T>::Mid(int aFirst, int aCount) const
{
  if (aFirst + aCount > m_Length)
    aCount = m_Length - aFirst;

  if (aFirst == 0 && aFirst + aCount == m_Length)
    return *this;

  CStringBase<T> aResult;
  aResult.SetCapacity(aCount);
  MyStringNCopy(aResult.m_Chars, m_Chars + aFirst, aCount);
  aResult.m_Length = aCount;
  aResult.m_Chars[aCount] = 0;
  return aResult;
}

template <class T>
CStringBase<T> CStringBase<T>::Right(int aCount) const
{
  if (aCount > m_Length)
    aCount = m_Length;
  return Mid(m_Length - aCount, aCount);
}

template <class T>
CStringBase<T> CStringBase<T>::Left(int aCount) const
{
  return Mid(0, aCount);
}

//////////////////////////////////////////////

template <class T>
void CStringBase<T>::MakeUpper()
{
  MyStringUpperCase(m_Chars);
}

template <class T>
void CStringBase<T>::MakeLower()
{
  MyStringLowerCase(m_Chars);
}

//////////////////////////////////////////////
// Compare functions 

template <class T>
int CStringBase<T>::Compare(const CStringBase<T>& aString) const
{
  return MyStringCompare(m_Chars, aString.m_Chars);
}

template <class T>
int CStringBase<T>::CompareNoCase(const CStringBase<T>& aString) const
{
  return MyStringCompareNoCase(m_Chars, aString.m_Chars);
}

template <class T>
int CStringBase<T>::Collate(const CStringBase<T>& aString) const
{
  return MyStringCollate(m_Chars, aString.m_Chars);
}

template <class T>
int CStringBase<T>::CollateNoCase(const CStringBase<T>& aString) const
{
  return MyStringCollateNoCase(m_Chars, aString.m_Chars);
}

//----------------------------------------------------------
// Find functions 

template <class T>
int CStringBase<T>::Find(T aChar) const
{
  return Find(aChar, 0);
}

template <class T>
int CStringBase<T>::Find(T aChar, int aStartIndex) const
{
  const T *aPointer = MyStringFindChar(m_Chars + aStartIndex, aChar);
  if (aPointer == NULL)
    return -1;
  return aPointer - m_Chars;
}

template <class T>
int CStringBase<T>::Find(const CStringBase<T> &aString) const
{
  return Find(aString, 0);
}

template <class T>
int CStringBase<T>::Find(const CStringBase<T> &aString, int aStartIndex) const
{
  const T *aPointer = MyStringFindSubString(m_Chars + aStartIndex, aString);
  if (aPointer == NULL)
    return -1;
  return aPointer - m_Chars;
}

template <class T>
int CStringBase<T>::ReverseFind(T aChar) const
{
  const T *aPointer = MyStringReverseFind(m_Chars, aChar);
  if (aPointer == NULL)
    return -1;
  return aPointer - m_Chars;
}

template <class T>
int CStringBase<T>::FindOneOf(const CStringBase<T> &aString) const
{
  for(int i = 0; i < m_Length; i++)
    if (aString.Find(m_Chars[i]) >= 0)
      return i;
  return -1;
}

//----------------------------------------------------------
// Trim functions

template <class T>
void CStringBase<T>::TrimLeft(T aChar)
{
  const T *aPointer = m_Chars;
	while (aChar == *aPointer)
    aPointer = GetNextCharPointer(aPointer);
  Delete(0, aPointer - m_Chars);
}

template <class T>
void CStringBase<T>::TrimLeftWithCharSet(const CStringBase<T> &aCharSet)
{
  const T *aPointer = m_Chars;
	while (aCharSet.Find(*aPointer) >= 0 && (*aPointer != 0))
    aPointer = GetNextCharPointer(aPointer);
  Delete(0, aPointer - m_Chars);
}

template <class T>
void CStringBase<T>::TrimLeft()
{
  CStringBase<T> aCharSet;
  for(int i = 0; i < sizeof(kTrimDefaultCharSet) /
      sizeof(kTrimDefaultCharSet[0]); i++)
    aCharSet += kTrimDefaultCharSet[i];
  TrimLeftWithCharSet(aCharSet);
}

template <class T>
void CStringBase<T>::TrimRight(T aChar)
{
  const T *aPointer = m_Chars;
  const T *aPointerLast = NULL;
	while (*aPointer != 0)
	{
		if (*aPointer == aChar)
		{
			if (aPointerLast == NULL)
				aPointerLast = aPointer;
		}
		else
			aPointerLast = NULL;
    aPointer = GetNextCharPointer(aPointer);
	}
  if(aPointerLast != NULL)
  {
    int i = aPointerLast - m_Chars;
    Delete(i, m_Length - i);
  }
}

template <class T>
void CStringBase<T>::TrimRightWithCharSet(const CStringBase<T> &aCharSet)
{
  const T *aPointer = m_Chars;
  const T *aPointerLast = NULL;
	while (*aPointer != 0)
	{
		if (aCharSet.Find(*aPointer) >= 0)
		{
			if (aPointerLast == NULL)
				aPointerLast = aPointer;
		}
		else
			aPointerLast = NULL;
    aPointer = GetNextCharPointer(aPointer);
	}
  if(aPointerLast != NULL)
  {
    int i = aPointerLast - m_Chars;
    Delete(i, m_Length - i);
  }
}

template <class T>
void CStringBase<T>::TrimRight()
{
  CStringBase<T> aCharSet;
  for(int i = 0; i < sizeof(kTrimDefaultCharSet) / 
      sizeof(kTrimDefaultCharSet[0]); i++)
    aCharSet += kTrimDefaultCharSet[i];
  TrimRightWithCharSet(aCharSet);
}

template <class T>
void CStringBase<T>::Trim()
{
  TrimRight();
  TrimLeft();
}

//---------------------------------
// Insert functions

template <class T>
void CStringBase<T>::InsertSpace(int &anIndex, int aSize)
{
  CorrectIndex(anIndex);
  GrowLength(aSize);
  memmove(m_Chars + anIndex + aSize, m_Chars + anIndex, 
      sizeof(T) * (m_Length - anIndex + 1));
}

template <class T>
int CStringBase<T>::Insert(int anIndex, T aChar)
{
  InsertSpace(anIndex, 1);
  m_Chars[anIndex] = aChar;
  m_Length++;
  return m_Length;
}

template <class T>
int CStringBase<T>::Insert(int anIndex, const CStringBase<T> &aString)
{
  CorrectIndex(anIndex);
  if (aString.IsEmpty())
    return m_Length;
  int aNumInsertChars = aString.Length();
  InsertSpace(anIndex, aNumInsertChars);
  for(int i = 0; i < aNumInsertChars; i++)
    m_Chars[anIndex + i] = aString[i];
  m_Length += aNumInsertChars;
  return m_Length;
}

//---------------------------------
// Replace functions

// !!!!!!!!!!!!!!! test it if aNewChar = '\0'
template <class T>
int CStringBase<T>::Replace(T anOldChar, T aNewChar)
{
  if (anOldChar == aNewChar)
    return 0;
  int aNumber  = 0;
  for (int i = 0; i < m_Length; i++)
  {
    if (m_Chars[i] == anOldChar)
    {
      m_Chars[i] = aNewChar;
      aNumber++;
    }
  }
  return aNumber;
}

template <class T>
int CStringBase<T>::Replace(const CStringBase<T> &anOldString, const CStringBase<T> &aNewString)
{
  if (anOldString.IsEmpty())
    return 0;
  if (anOldString == aNewString)
    return 0;
  int anOldStringLength = anOldString.Length();
  int aNewStringLength = aNewString.Length();
  int aNumber  = 0;
  int aPos  = 0;
  while (aPos < m_Length)
  {
    aPos = Find(anOldString, aPos);
    if (aPos < 0) 
      break;
    Delete(aPos, anOldStringLength);
    Insert(aPos, aNewString);
    aPos += aNewStringLength;
    aNumber++;
  }
  return aNumber;
}

// -------------------------------------------
// Delete function

template <class T>
int CStringBase<T>::Delete(int anIndex, int aCount)
{
  if (anIndex + aCount > m_Length)
    aCount = m_Length - anIndex;
  if (aCount > 0)
  {
    memmove(m_Chars + anIndex, m_Chars + anIndex + aCount, 
      sizeof(T) * (m_Length - (anIndex + aCount) + 1));
    m_Length -= aCount;
  }
  return m_Length;
}

// ------------------------------------
// CStringBase<T> functions

template <class T>
CStringBase<T> operator+(const CStringBase<T>& aString1, const CStringBase<T>& aString2)
{
  CStringBase<T> aResult(aString1);
  aResult += aString2;
  return aResult; 
}

template <class T>
CStringBase<T> operator+(const CStringBase<T>& aString, T aChar)
{
  CStringBase<T> aResult(aString);
  aResult += aChar;
  return aResult; 
}

template <class T>
CStringBase<T> operator+(T aChar, const CStringBase<T>& aString)
{
  CStringBase<T> aResult(aChar);
  aResult += aString;
  return aResult; 
}

template <class T>
CStringBase<T> operator+(const CStringBase<T>& aString, const T * aChars)
{
  CStringBase<T> aResult(aString);
  aResult += aChars;
  return aResult; 
}

template <class T>
CStringBase<T> operator+(const T * aChars, const CStringBase<T>& aString)
{
  CStringBase<T> aResult(aChars);
  aResult += aString;
  return aResult; 
}

template <class T>
bool operator==(const CStringBase<T>& s1, const CStringBase<T>& s2)
{
  return (s1.Compare(s2) == 0);
}

template <class T>
bool operator<(const CStringBase<T>& s1, const CStringBase<T>& s2)
  { return (s1.Compare(s2) < 0); }

template <class T>
bool operator==(const T *s1, const CStringBase<T>& s2)
  { return (s2.Compare(s1) == 0); }

template <class T>
bool operator==(const CStringBase<T>& s1, const T *s2)
  { return (s1.Compare(s2) == 0); }

template <class T>
bool operator!=(const CStringBase<T>& s1, const CStringBase<T>& s2)
  { return (s1.Compare(s2) != 0); }

template <class T>
bool operator!=(const T *s1, const CStringBase<T>& s2)
  { return (s2.Compare(s1) != 0); }

template <class T>
bool operator!=(const CStringBase<T>& s1, const T *s2)
  { return (s1.Compare(s2) != 0); }

typedef CStringBase<char> AString;
typedef CStringBase<wchar_t> UString;

typedef CObjectVector<AString> AStringVector;
typedef CObjectVector<UString> UStringVector;

#ifdef _UNICODE
  typedef UString CSysString;
#else
  typedef AString CSysString;
#endif

typedef CObjectVector<CSysString> CSysStringVector;

#endif
