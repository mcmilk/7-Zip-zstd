// Common/String.h

#pragma once

#ifndef __COMMON_STRING_H
#define __COMMON_STRING_H

#include "Common/Vector.h"

inline char* MyStringGetNextCharPointer(char *pointer)
  { return (char *)_mbsinc((unsigned char *)pointer); }

inline wchar_t* MyStringGetNextCharPointer(wchar_t *pointer)
  { return (wchar_t*)(pointer + 1); }

inline const char* MyStringGetNextCharPointer(const char *pointer)
  { return (const char *)_mbsinc((const unsigned char *)pointer); }

inline const wchar_t* MyStringGetNextCharPointer(const wchar_t *pointer)
  { return (const wchar_t*)(pointer + 1); }

template <class T>
class CStringBase
{
  void TrimLeftWithCharSet(const CStringBase &charSet);
  void TrimRightWithCharSet(const CStringBase &charSet);
  void InsertSpace(int &index, int size);
  static T *GetNextCharPointer(T *pointer)
    { return MyStringGetNextCharPointer(pointer); }
  static const T *GetNextCharPointer(const T *pointer)
    { return MyStringGetNextCharPointer(pointer); }
protected:
  T *_chars;
  int _length;
	int _capacity;
  void SetCapacity(int newCapacity);
  void GrowLength(int n);
  void CorrectIndex(int &index) const;
public:
  CStringBase();
  CStringBase(T c);
  CStringBase(const T *chars);
  CStringBase(const CStringBase &string);
  ~CStringBase();
  // T operator[](int index) const;
  // T& operator[](int index);
  operator const T*() const { return _chars;} 

  // The minimum size of the character buffer in characters. 
  // This value does not include space for a null terminator.
  T* GetBuffer(int minBufLength);
  void ReleaseBuffer();
  void ReleaseBuffer(int newLength);


  CStringBase& operator=(T c);
  CStringBase& operator=(const T *chars);
  CStringBase& operator=(const CStringBase& string);
  CStringBase& operator+=(T c);
  CStringBase& operator+=(const T *string);
  CStringBase& operator+=(const CStringBase& string);

  void Empty();
  int Length() const { return _length; }
  bool IsEmpty() const {   return (_length == 0); }

  CStringBase Mid(int startIndex) const;
  CStringBase Mid(int startIndex, int count ) const;
  CStringBase Left(int count) const;
  CStringBase Right(int count) const;

  void MakeUpper();
  void MakeLower();

  int Compare(const CStringBase& string) const;
  int CompareNoCase(const CStringBase& string) const;
  int Collate(const CStringBase& string) const;
  int CollateNoCase(const CStringBase& string) const;

  int Find(T c) const;
  int Find(T c, int startIndex) const;
  int Find(const CStringBase &string) const;
  int Find(const CStringBase &string, int startIndex) const;
  int ReverseFind(T c) const;
  int FindOneOf(const CStringBase &string) const;

  void TrimLeft();
  void TrimLeft(T c);
  void TrimRight();
  void TrimRight(T c);
  void Trim();

  int Insert(int index, T c);
  int Insert(int index, const CStringBase &string);

  int Replace(T oldChar, T newChar);
  int Replace(const CStringBase &oldString, const CStringBase &newString);

  int Delete(int index, int count = 1 );
  // void DeleteBack() {   Delete(Length() - 1); }
};

template <class T>
CStringBase<T>  operator+(const CStringBase<T> & string1, const CStringBase<T> & string2);
template <class T>
CStringBase<T>  operator+(const CStringBase<T> & string, T c);
template <class T>
CStringBase<T>  operator+(T c, const CStringBase<T> & string);
template <class T>
CStringBase<T>  operator+(const CStringBase<T> & string, const T * chars);
template <class T>
CStringBase<T>  operator+(const T * chars, const CStringBase<T> & string);

template <class T>
bool operator==(const CStringBase<T> & s1, const CStringBase<T> & s2);
template <class T>
bool operator!=(const CStringBase<T> & s1, const CStringBase<T> & s2);

static const char *kTrimDefaultCharSet  = " \n\t";

inline size_t MyStringLen(const char *string)
  { return strlen(string); }
inline size_t MyStringLen(const wchar_t *string)
  { return wcslen(string); }

inline char * MyStringCopy(char *strDestination, const char *strSource)
  { return strcpy(strDestination, strSource); }
inline wchar_t * MyStringCopy(wchar_t *strDestination, const wchar_t *strSource)
  { return wcscpy(strDestination, strSource); }

inline char * MyStringNCopy(char *strDestination, const char *strSource, size_t count)
  { return strncpy(strDestination, strSource, count); }
inline wchar_t * MyStringNCopy(wchar_t *strDestination, const wchar_t *strSource, size_t count)
  { return wcsncpy(strDestination, strSource, count); }

inline char * MyStringUpperCase(char *string)
  { return (char *)_mbsupr((unsigned char *)string); }
inline wchar_t * MyStringUpperCase(wchar_t *string)
  { return _wcsupr(string); }

inline char * MyStringLowerCase(char *string)
  { return (char *)_mbsupr((unsigned char *)string); }
inline wchar_t * MyStringLowerCase(wchar_t *string)
  { return _wcslwr(string); }

inline int MyStringCompare(const char *string1, const char *string2)
  { return _mbscmp((const unsigned char *)string1, (const unsigned char *)string2); }
inline int MyStringCompare(const wchar_t *string1, const wchar_t *string2)
  { return wcscmp(string1, string2); }

inline int MyStringCompareNoCase(const char *string1, const char *string2)
  { return _mbsicmp((const unsigned char *)string1, (const unsigned char *)string2); }
inline int MyStringCompareNoCase(const wchar_t *string1, const wchar_t *string2)
  { return _wcsicmp(string1, string2); }

#ifndef _WIN32_WCE

inline int MyStringCollate(const char *string1, const char *string2)
  { return _mbscoll((const unsigned char *)string1, (const unsigned char *)string2); }
inline int MyStringCollate(const wchar_t *string1, const wchar_t *string2)
  { return wcscoll(string1, string2); }

inline int MyStringCollateNoCase(const char *string1, const char *string2)
  { return _mbsicoll((const unsigned char *)string1, (const unsigned char *)string2); }
inline int MyStringCollateNoCase(const wchar_t *string1, const wchar_t *string2)
  { return _wcsicoll(string1, string2); }

#endif

inline char* MyStringFindChar(const char *string, int c)
  { return (char *)_mbschr((const unsigned char *)string, c); }
inline wchar_t* MyStringFindChar(const wchar_t *string, wint_t c)
  { return wcschr(string, c); }

inline char* MyStringFindSubString(const char *string, const char *strCharSet)
  { return (char *)_mbsstr((const unsigned char *)string, (const unsigned char *)strCharSet); }  
inline wchar_t* MyStringFindSubString(const wchar_t *string, const wchar_t *strCharSet)
  { return wcsstr(string, strCharSet); }  

inline char* MyStringReverseFind(const char *string, int c)
  { return (char *)_mbsrchr((const unsigned char *)string, c); }
inline wchar_t* MyStringReverseFind(const wchar_t *string, wint_t c)
  { return wcsrchr(string, c); }

 
//////////////////////////////////////////////

template <class T>
void CStringBase<T>::SetCapacity(int newCapacity)
{
  int realCapacity = newCapacity + 1;
  if(realCapacity == _capacity)
    return;
  const kMaxStringSize = 0x20000000;
  #ifndef _WIN32_WCE
  if(newCapacity > kMaxStringSize || newCapacity < _length)
    throw 1052337;
  #endif
  T *newBuffer = new T[realCapacity];
  if(_capacity > 0)
  {
    memmove(newBuffer, _chars, sizeof(T) * (_length + 1));
    delete []_chars;
    _chars = newBuffer;
  }
  else
  {
    _chars = newBuffer;
    _chars[0] = 0;
  }
  _capacity = realCapacity;
}

template <class T>
void CStringBase<T>::GrowLength(int n)
{
  int freeSize = _capacity - _length - 1;
  if (n <= freeSize) 
    return;
  int delta;
  if (_capacity > 64)
    delta = _capacity / 2;
  else if (_capacity > 8)
    delta = 16;
  else
    delta = 4;
  if (freeSize + delta < n)
    delta = n - freeSize;
  SetCapacity(_capacity + delta);
}

template <class T>
CStringBase<T>::CStringBase():
  _chars(NULL),
  _length(0),
  _capacity(0)
{
  SetCapacity(16 - 1);
}

template <class T>
CStringBase<T>::CStringBase(T c):
  _chars(NULL),
  _length(0),
  _capacity(0)
{
  SetCapacity(1);
  _chars[0] = c;
  _chars[1] = 0;
  _length = 1;
}

template <class T>
CStringBase<T>::CStringBase(const T *chars):
  _chars(NULL),
  _length(0),
  _capacity(0)
{
  int length = MyStringLen(chars);
  SetCapacity(length);
  MyStringCopy(_chars, chars); // can be optimized by memove()
  _length = length;
}

template <class T>
CStringBase<T>::CStringBase(const CStringBase<T> &string):
  _chars(NULL),
  _length(0),
  _capacity(0)
{
  SetCapacity(string._length);
  MyStringCopy(_chars, string._chars);
  _length = string._length;
}

template <class T>
CStringBase<T>::~CStringBase()
{
  delete []_chars;
}

template <class T>
void CStringBase<T>::CorrectIndex(int &index) const
{
  if (index > _length)
    index = _length;
}

//////////////////////////////////
// Buffer functions

template <class T>
T* CStringBase<T>::GetBuffer(int minBufLength)
{
  if(minBufLength >= _capacity)
    SetCapacity(minBufLength + 1);
  return _chars;
}

template <class T>
void CStringBase<T>::ReleaseBuffer()
{
  ReleaseBuffer(MyStringLen(_chars));
}

template <class T>
void CStringBase<T>::ReleaseBuffer(int newLength)
{
  #ifndef _WIN32_WCE
  if(newLength >= _capacity)
    throw 282217;
  #endif
  _chars[newLength] = 0;
  _length = newLength;
}

void ReleaseBuffer( int nNewLength = -1 );


////////////////////////
// operator =

template <class T>
CStringBase<T>& CStringBase<T>::operator=(T c)
{
  Empty();
  SetCapacity(1);
  _chars[0] = c;
  _chars[1] = 0;
  _length = 1;
  return *this;
}

template <class T>
CStringBase<T>& CStringBase<T>::operator=(const T *chars)
{
  Empty();
  int length = MyStringLen(chars);
  SetCapacity(length);
  MyStringCopy(_chars, chars);
  _length = length; 
  return *this;
}  

template <class T>
CStringBase<T>& CStringBase<T>::operator=(const CStringBase<T>& string)
{
  if(&string == this)
    return *this;
  Empty();
  SetCapacity(string._length);
  MyStringCopy(_chars, string._chars);
  _length = string._length;
  return *this;
}

////////////////////////
// operator +=

template <class T>
CStringBase<T>& CStringBase<T>::operator+=(T c)
{
  GrowLength(1);
  _chars[_length] = c;
  _chars[++_length] = 0;
  return *this;
}

template <class T>
CStringBase<T>& CStringBase<T>::operator+=(const T *string)
{
  int len = MyStringLen(string);
  GrowLength(len);
  MyStringCopy(_chars + _length, string);
  _length += len;
  return *this;
}

template <class T>
CStringBase<T>& CStringBase<T>::operator+=(const CStringBase<T>& string)
{
  GrowLength(string._length);
  MyStringCopy(_chars + _length, string._chars);
  _length += string._length;
  return *this;
}

//////////////////////////////////////////////

template <class T>
void CStringBase<T>::Empty()
{
  _length = 0;
  _chars[0] = 0;
}

//////////////////////////////////////////////

template <class T>
CStringBase<T> CStringBase<T>::Mid(int startIndex) const
{
  return Mid(startIndex, _length - startIndex);
}

template <class T>
CStringBase<T> CStringBase<T>::Mid(int startIndex, int count) const
{
  if (startIndex + count > _length)
    count = _length - startIndex;

  if (startIndex == 0 && startIndex + count == _length)
    return *this;

  CStringBase<T> result;
  result.SetCapacity(count);
  MyStringNCopy(result._chars, _chars + startIndex, count);
  result._length = count;
  result._chars[count] = 0;
  return result;
}

template <class T>
CStringBase<T> CStringBase<T>::Right(int count) const
{
  if (count > _length)
    count = _length;
  return Mid(_length - count, count);
}

template <class T>
CStringBase<T> CStringBase<T>::Left(int count) const
{
  return Mid(0, count);
}

//////////////////////////////////////////////

template <class T>
void CStringBase<T>::MakeUpper()
{
  MyStringUpperCase(_chars);
}

template <class T>
void CStringBase<T>::MakeLower()
{
  MyStringLowerCase(_chars);
}

//////////////////////////////////////////////
// Compare functions 

template <class T>
int CStringBase<T>::Compare(const CStringBase<T>& string) const
{
  return MyStringCompare(_chars, string._chars);
}

template <class T>
int CStringBase<T>::CompareNoCase(const CStringBase<T>& string) const
{
  return MyStringCompareNoCase(_chars, string._chars);
}

template <class T>
int CStringBase<T>::Collate(const CStringBase<T>& string) const
{
  return MyStringCollate(_chars, string._chars);
}

template <class T>
int CStringBase<T>::CollateNoCase(const CStringBase<T>& string) const
{
  return MyStringCollateNoCase(_chars, string._chars);
}

//----------------------------------------------------------
// Find functions 

template <class T>
int CStringBase<T>::Find(T c) const
{
  return Find(c, 0);
}

template <class T>
int CStringBase<T>::Find(T c, int startIndex) const
{
  const T *pointer = MyStringFindChar(_chars + startIndex, c);
  if (pointer == NULL)
    return -1;
  return pointer - _chars;
}

template <class T>
int CStringBase<T>::Find(const CStringBase<T> &string) const
{
  return Find(string, 0);
}

template <class T>
int CStringBase<T>::Find(const CStringBase<T> &string, int startIndex) const
{
  const T *pointer = MyStringFindSubString(_chars + startIndex, string);
  if (pointer == NULL)
    return -1;
  return pointer - _chars;
}

template <class T>
int CStringBase<T>::ReverseFind(T c) const
{
  const T *pointer = MyStringReverseFind(_chars, c);
  if (pointer == NULL)
    return -1;
  return pointer - _chars;
}

template <class T>
int CStringBase<T>::FindOneOf(const CStringBase<T> &string) const
{
  for(int i = 0; i < _length; i++)
    if (string.Find(_chars[i]) >= 0)
      return i;
  return -1;
}

//----------------------------------------------------------
// Trim functions

template <class T>
void CStringBase<T>::TrimLeft(T c)
{
  const T *pointer = _chars;
	while (c == *pointer)
    pointer = GetNextCharPointer(pointer);
  Delete(0, pointer - _chars);
}

template <class T>
void CStringBase<T>::TrimLeftWithCharSet(const CStringBase<T> &charSet)
{
  const T *pointer = _chars;
	while (charSet.Find(*pointer) >= 0 && (*pointer != 0))
    pointer = GetNextCharPointer(pointer);
  Delete(0, pointer - _chars);
}

template <class T>
void CStringBase<T>::TrimLeft()
{
  CStringBase<T> charSet;
  for(int i = 0; i < sizeof(kTrimDefaultCharSet) /
      sizeof(kTrimDefaultCharSet[0]); i++)
    charSet += kTrimDefaultCharSet[i];
  TrimLeftWithCharSet(charSet);
}

template <class T>
void CStringBase<T>::TrimRight(T c)
{
  const T *pointer = _chars;
  const T *pointerLast = NULL;
	while (*pointer != 0)
	{
		if (*pointer == c)
		{
			if (pointerLast == NULL)
				pointerLast = pointer;
		}
		else
			pointerLast = NULL;
    pointer = GetNextCharPointer(pointer);
	}
  if(pointerLast != NULL)
  {
    int i = pointerLast - _chars;
    Delete(i, _length - i);
  }
}

template <class T>
void CStringBase<T>::TrimRightWithCharSet(const CStringBase<T> &charSet)
{
  const T *pointer = _chars;
  const T *pointerLast = NULL;
	while (*pointer != 0)
	{
		if (charSet.Find(*pointer) >= 0)
		{
			if (pointerLast == NULL)
				pointerLast = pointer;
		}
		else
			pointerLast = NULL;
    pointer = GetNextCharPointer(pointer);
	}
  if(pointerLast != NULL)
  {
    int i = pointerLast - _chars;
    Delete(i, _length - i);
  }
}

template <class T>
void CStringBase<T>::TrimRight()
{
  CStringBase<T> charSet;
  for(int i = 0; i < sizeof(kTrimDefaultCharSet) / 
      sizeof(kTrimDefaultCharSet[0]); i++)
    charSet += kTrimDefaultCharSet[i];
  TrimRightWithCharSet(charSet);
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
void CStringBase<T>::InsertSpace(int &index, int size)
{
  CorrectIndex(index);
  GrowLength(size);
  memmove(_chars + index + size, _chars + index, 
      sizeof(T) * (_length - index + 1));
}

template <class T>
int CStringBase<T>::Insert(int index, T c)
{
  InsertSpace(index, 1);
  _chars[index] = c;
  _length++;
  return _length;
}

template <class T>
int CStringBase<T>::Insert(int index, const CStringBase<T> &string)
{
  CorrectIndex(index);
  if (string.IsEmpty())
    return _length;
  int numInsertChars = string.Length();
  InsertSpace(index, numInsertChars);
  for(int i = 0; i < numInsertChars; i++)
    _chars[index + i] = string[i];
  _length += numInsertChars;
  return _length;
}

//---------------------------------
// Replace functions

// !!!!!!!!!!!!!!! test it if newChar = '\0'
template <class T>
int CStringBase<T>::Replace(T oldChar, T newChar)
{
  if (oldChar == newChar)
    return 0;
  int number  = 0;
  int pos  = 0;
  while (pos < Length())
  {
    pos = Find(oldChar, pos);
    if (pos < 0) 
      break;
    _chars[pos] = newChar;
    pos++;
    number++;
  }
  return number;
  /*
  for (int i = 0; i < _length; i++)
  {
    if (_chars[i] == oldChar)
    {
      _chars[i] = newChar;
      number++;
    }
  }
  */
  return number;
}

template <class T>
int CStringBase<T>::Replace(const CStringBase<T> &oldString, const CStringBase<T> &newString)
{
  if (oldString.IsEmpty())
    return 0;
  if (oldString == newString)
    return 0;
  int oldStringLength = oldString.Length();
  int newStringLength = newString.Length();
  int number  = 0;
  int pos  = 0;
  while (pos < _length)
  {
    pos = Find(oldString, pos);
    if (pos < 0) 
      break;
    Delete(pos, oldStringLength);
    Insert(pos, newString);
    pos += newStringLength;
    number++;
  }
  return number;
}

// -------------------------------------------
// Delete function

template <class T>
int CStringBase<T>::Delete(int index, int count)
{
  if (index + count > _length)
    count = _length - index;
  if (count > 0)
  {
    memmove(_chars + index, _chars + index + count, 
      sizeof(T) * (_length - (index + count) + 1));
    _length -= count;
  }
  return _length;
}

// ------------------------------------
// CStringBase<T> functions

template <class T>
CStringBase<T> operator+(const CStringBase<T>& string1, const CStringBase<T>& string2)
{
  CStringBase<T> result(string1);
  result += string2;
  return result; 
}

template <class T>
CStringBase<T> operator+(const CStringBase<T>& string, T c)
{
  CStringBase<T> result(string);
  result += c;
  return result; 
}

template <class T>
CStringBase<T> operator+(T c, const CStringBase<T>& string)
{
  CStringBase<T> result(c);
  result += string;
  return result; 
}

template <class T>
CStringBase<T> operator+(const CStringBase<T>& string, const T * chars)
{
  CStringBase<T> result(string);
  result += chars;
  return result; 
}

template <class T>
CStringBase<T> operator+(const T * chars, const CStringBase<T>& string)
{
  CStringBase<T> result(chars);
  result += string;
  return result; 
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
