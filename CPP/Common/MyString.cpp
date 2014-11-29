// Common/MyString.cpp

#include "StdAfx.h"

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#else
#include <ctype.h>
#endif

#if !defined(_UNICODE) || !defined(USE_UNICODE_FSTRING)
#include "StringConvert.h"
#endif

#include "MyString.h"

#define MY_STRING_NEW(_T_, _size_) new _T_[_size_]
// #define MY_STRING_NEW(_T_, _size_) ((_T_ *)my_new((size_t)(_size_) * sizeof(_T_)))

/*
inline const char* MyStringGetNextCharPointer(const char *p) throw()
{
  #if defined(_WIN32) && !defined(UNDER_CE)
  return CharNextA(p);
  #else
  return p + 1;
  #endif
}
*/

int FindCharPosInString(const char *s, char c)
{
  for (const char *p = s;; p++)
  {
    if (*p == c)
      return (int)(p - s);
    if (*p == 0)
      return -1;
    // MyStringGetNextCharPointer(p);
  }
}

int FindCharPosInString(const wchar_t *s, wchar_t c)
{
  for (const wchar_t *p = s;; p++)
  {
    if (*p == c)
      return (int)(p - s);
    if (*p == 0)
      return -1;
  }
}

/*
void MyStringUpper_Ascii(wchar_t *s)
{
  for (;;)
  {
    wchar_t c = *s;
    if (c == 0)
      return;
    *s++ = MyCharUpper_Ascii(c);
  }
}
*/

void MyStringLower_Ascii(wchar_t *s)
{
  for (;;)
  {
    wchar_t c = *s;
    if (c == 0)
      return;
    *s++ = MyCharLower_Ascii(c);
  }
}

#ifdef _WIN32

#ifdef _UNICODE

// wchar_t * MyStringUpper(wchar_t *s) { return CharUpperW(s); }
// wchar_t * MyStringLower(wchar_t *s) { return CharLowerW(s); }
// for WinCE - FString - char
// const char *MyStringGetPrevCharPointer(const char * /* base */, const char *p) { return p - 1; }

#else

// const char * MyStringGetPrevCharPointer(const char *base, const char *p) throw() { return CharPrevA(base, p); }
// char * MyStringUpper(char *s) { return CharUpperA(s); }
// char * MyStringLower(char *s) { return CharLowerA(s); }

wchar_t MyCharUpper_WIN(wchar_t c)
{
  wchar_t *res = CharUpperW((LPWSTR)(UINT_PTR)(unsigned)c);
  if (res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return (wchar_t)(unsigned)(UINT_PTR)res;
  const int kBufSize = 4;
  char s[kBufSize + 1];
  int numChars = ::WideCharToMultiByte(CP_ACP, 0, &c, 1, s, kBufSize, 0, 0);
  if (numChars == 0 || numChars > kBufSize)
    return c;
  s[numChars] = 0;
  ::CharUpperA(s);
  ::MultiByteToWideChar(CP_ACP, 0, s, numChars, &c, 1);
  return c;
}

/*
wchar_t MyCharLower_WIN(wchar_t c)
{
  wchar_t *res = CharLowerW((LPWSTR)(UINT_PTR)(unsigned)c);
  if (res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return (wchar_t)(unsigned)(UINT_PTR)res;
  const int kBufSize = 4;
  char s[kBufSize + 1];
  int numChars = ::WideCharToMultiByte(CP_ACP, 0, &c, 1, s, kBufSize, 0, 0);
  if (numChars == 0 || numChars > kBufSize)
    return c;
  s[numChars] = 0;
  ::CharLowerA(s);
  ::MultiByteToWideChar(CP_ACP, 0, s, numChars, &c, 1);
  return c;
}
*/

/*
wchar_t * MyStringUpper(wchar_t *s)
{
  if (s == 0)
    return 0;
  wchar_t *res = CharUpperW(s);
  if (res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return res;
  AString a = UnicodeStringToMultiByte(s);
  a.MakeUpper();
  MyStringCopy(s, (const wchar_t *)MultiByteToUnicodeString(a));
  return s;
}
*/

/*
wchar_t * MyStringLower(wchar_t *s)
{
  if (s == 0)
    return 0;
  wchar_t *res = CharLowerW(s);
  if (res != 0 || ::GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    return res;
  AString a = UnicodeStringToMultiByte(s);
  a.MakeLower();
  MyStringCopy(s, (const wchar_t *)MultiByteToUnicodeString(a));
  return s;
}
*/

#endif

#endif

bool IsString1PrefixedByString2(const char *s1, const char *s2)
{
  for (;;)
  {
    unsigned char c2 = (unsigned char)*s2++; if (c2 == 0) return true;
    unsigned char c1 = (unsigned char)*s1++; if (c1 != c2) return false;
  }
}

bool StringsAreEqualNoCase(const wchar_t *s1, const wchar_t *s2)
{
  for (;;)
  {
    wchar_t c1 = *s1++;
    wchar_t c2 = *s2++;
    if (c1 != c2 && MyCharUpper(c1) != MyCharUpper(c2)) return false;
    if (c1 == 0) return true;
  }
}

// ---------- ASCII ----------

bool AString::IsPrefixedBy_Ascii_NoCase(const char *s) const
{
  const char *s1 = _chars;
  for (;;)
  {
    char c2 = *s++;
    if (c2 == 0)
      return true;
    char c1 = *s1++;
    if (MyCharLower_Ascii(c1) !=
        MyCharLower_Ascii(c2))
      return false;
  }
}

bool UString::IsPrefixedBy_Ascii_NoCase(const char *s) const
{
  const wchar_t *s1 = _chars;
  for (;;)
  {
    char c2 = *s++;
    if (c2 == 0)
      return true;
    wchar_t c1 = *s1++;
    if (MyCharLower_Ascii(c1) !=
        MyCharLower_Ascii(c2))
      return false;
  }
}

bool StringsAreEqual_Ascii(const wchar_t *u, const char *a)
{
  for (;;)
  {
    unsigned char c = *a;
    if (c != *u)
      return false;
    if (c == 0)
      return true;
    a++;
    u++;
  }
}

bool StringsAreEqualNoCase_Ascii(const char *s1, const char *s2)
{
  for (;;)
  {
    char c1 = *s1++;
    char c2 = *s2++;
    if (c1 != c2 && MyCharLower_Ascii(c1) != MyCharLower_Ascii(c2))
      return false;
    if (c1 == 0)
      return true;
  }
}

bool StringsAreEqualNoCase_Ascii(const wchar_t *s1, const wchar_t *s2)
{
  for (;;)
  {
    wchar_t c1 = *s1++;
    wchar_t c2 = *s2++;
    if (c1 != c2 && MyCharLower_Ascii(c1) != MyCharLower_Ascii(c2))
      return false;
    if (c1 == 0)
      return true;
  }
}

bool StringsAreEqualNoCase_Ascii(const wchar_t *s1, const char *s2)
{
  for (;;)
  {
    wchar_t c1 = *s1++;
    char c2 = *s2++;
    if (c1 != c2 && (c1 > 0x7F || MyCharLower_Ascii(c1) != MyCharLower_Ascii(c2)))
      return false;
    if (c1 == 0)
      return true;
  }
}

bool IsString1PrefixedByString2(const wchar_t *s1, const wchar_t *s2)
{
  for (;;)
  {
    wchar_t c2 = *s2++; if (c2 == 0) return true;
    wchar_t c1 = *s1++; if (c1 != c2) return false;
  }
}

// NTFS order: uses upper case
int MyStringCompareNoCase(const wchar_t *s1, const wchar_t *s2)
{
  for (;;)
  {
    wchar_t c1 = *s1++;
    wchar_t c2 = *s2++;
    if (c1 != c2)
    {
      wchar_t u1 = MyCharUpper(c1);
      wchar_t u2 = MyCharUpper(c2);
      if (u1 < u2) return -1;
      if (u1 > u2) return 1;
    }
    if (c1 == 0) return 0;
  }
}

int MyStringCompareNoCase_N(const wchar_t *s1, const wchar_t *s2, unsigned num)
{
  for (; num != 0; num--)
  {
    wchar_t c1 = *s1++;
    wchar_t c2 = *s2++;
    if (c1 != c2)
    {
      wchar_t u1 = MyCharUpper(c1);
      wchar_t u2 = MyCharUpper(c2);
      if (u1 < u2) return -1;
      if (u1 > u2) return 1;
    }
    if (c1 == 0) return 0;
  }
  return 0;
}


// ---------- AString ----------

void AString::InsertSpace(unsigned &index, unsigned size)
{
  Grow(size);
  MoveItems(index + size, index);
}

void AString::ReAlloc(unsigned newLimit)
{
  if (newLimit < _len || newLimit >= 0x20000000) throw 20130220;
  // MY_STRING_REALLOC(_chars, char, newLimit + 1, _len + 1);
  char *newBuf = MY_STRING_NEW(char, newLimit + 1);
  memcpy(newBuf, _chars, (size_t)(_len + 1)); \
  MY_STRING_DELETE(_chars);
  _chars = newBuf;

  _limit = newLimit;
}

void AString::SetStartLen(unsigned len)
{
  _chars = 0;
  _chars = MY_STRING_NEW(char, len + 1);
  _len = len;
  _limit = len;
}

void AString::Grow_1()
{
  unsigned next = _len;
  next += next / 2;
  next += 16;
  next &= ~(unsigned)15;
  ReAlloc(next - 1);
}

void AString::Grow(unsigned n)
{
  unsigned freeSize = _limit - _len;
  if (n <= freeSize)
    return;
  
  unsigned next = _len + n;
  next += next / 2;
  next += 16;
  next &= ~(unsigned)15;
  ReAlloc(next - 1);
}

/*
AString::AString(unsigned num, const char *s)
{
  unsigned len = MyStringLen(s);
  if (num > len)
    num = len;
  SetStartLen(num);
  memcpy(_chars, s, num);
  _chars[num] = 0;
}
*/

AString::AString(unsigned num, const AString &s)
{
  if (num > s._len)
    num = s._len;
  SetStartLen(num);
  memcpy(_chars, s._chars, num);
  _chars[num] = 0;
}

AString::AString(const AString &s, char c)
{
  SetStartLen(s.Len() + 1);
  char *chars = _chars;
  unsigned len = s.Len();
  memcpy(chars, s, len);
  chars[len] = c;
  chars[len + 1] = 0;
}

AString::AString(const char *s1, unsigned num1, const char *s2, unsigned num2)
{
  SetStartLen(num1 + num2);
  char *chars = _chars;
  memcpy(chars, s1, num1);
  memcpy(chars + num1, s2, num2 + 1);
}

AString operator+(const AString &s1, const AString &s2) { return AString(s1, s1.Len(), s2, s2.Len()); }
AString operator+(const AString &s1, const char    *s2) { return AString(s1, s1.Len(), s2, MyStringLen(s2)); }
AString operator+(const char    *s1, const AString &s2) { return AString(s1, MyStringLen(s1), s2, s2.Len()); }

AString::AString()
{
  _chars = 0;
  _chars = MY_STRING_NEW(char, 4);
  _len = 0;
  _limit = 4 - 1;
  _chars[0] = 0;
}

AString::AString(char c)
{
  SetStartLen(1);
  _chars[0] = c;
  _chars[1] = 0;
}

AString::AString(const char *s)
{
  SetStartLen(MyStringLen(s));
  MyStringCopy(_chars, s);
}

AString::AString(const AString &s)
{
  SetStartLen(s._len);
  MyStringCopy(_chars, s._chars);
}

AString &AString::operator=(char c)
{
  if (1 > _limit)
  {
    char *newBuf = MY_STRING_NEW(char, 1 + 1);
    MY_STRING_DELETE(_chars);
    _chars = newBuf;
    _limit = 1;
  }
  _len = 1;
  _chars[0] = c;
  _chars[1] = 0;
  return *this;
}

AString &AString::operator=(const char *s)
{
  unsigned len = MyStringLen(s);
  if (len > _limit)
  {
    char *newBuf = MY_STRING_NEW(char, len + 1);
    MY_STRING_DELETE(_chars);
    _chars = newBuf;
    _limit = len;
  }
  _len = len;
  MyStringCopy(_chars, s);
  return *this;
}

AString &AString::operator=(const AString &s)
{
  if (&s == this)
    return *this;
  unsigned len = s._len;
  if (len > _limit)
  {
    char *newBuf = MY_STRING_NEW(char, len + 1);
    MY_STRING_DELETE(_chars);
    _chars = newBuf;
    _limit = len;
  }
  _len = len;
  MyStringCopy(_chars, s._chars);
  return *this;
}

AString &AString::operator+=(const char *s)
{
  unsigned len = MyStringLen(s);
  Grow(len);
  MyStringCopy(_chars + _len, s);
  _len += len;
  return *this;
}

AString &AString::operator+=(const AString &s)
{
  Grow(s._len);
  MyStringCopy(_chars + _len, s._chars);
  _len += s._len;
  return *this;
}

void AString::SetFrom(const char *s, unsigned len) // no check
{
  if (len > _limit)
  {
    char *newBuf = MY_STRING_NEW(char, len + 1);
    MY_STRING_DELETE(_chars);
    _chars = newBuf;
    _limit = len;
  }
  memcpy(_chars, s, len);
  _chars[len] = 0;
  _len = len;
}

int AString::Find(const AString &s, unsigned startIndex) const
{
  if (s.IsEmpty())
    return startIndex;
  for (; startIndex < _len; startIndex++)
  {
    unsigned j;
    for (j = 0; j < s._len && startIndex + j < _len; j++)
      if (_chars[startIndex + j] != s._chars[j])
        break;
    if (j == s._len)
      return (int)startIndex;
  }
  return -1;
}

int AString::ReverseFind(char c) const
{
  if (_len == 0)
    return -1;
  const char *p = _chars + _len - 1;
  for (;;)
  {
    if (*p == c)
      return (int)(p - _chars);
    if (p == _chars)
      return -1;
    p--; // p = GetPrevCharPointer(_chars, p);
  }
}

void AString::TrimLeft()
{
  const char *p = _chars;
  for (;; p++)
  {
    char c = *p;
    if (c != ' ' && c != '\n' && c != '\t')
      break;
  }
  unsigned pos = (unsigned)(p - _chars);
  if (pos != 0)
  {
    MoveItems(0, pos);
    _len -= pos;
  }
}

void AString::TrimRight()
{
  const char *p = _chars;
  int i;
  for (i = _len - 1; i >= 0; i--)
  {
    char c = p[i];
    if (c != ' ' && c != '\n' && c != '\t')
      break;
  }
  i++;
  if ((unsigned)i != _len)
  {
    _chars[i] = 0;
    _len = i;
  }
}

void AString::InsertAtFront(char c)
{
  if (_limit == _len)
    Grow_1();
  MoveItems(1, 0);
  _chars[0] = c;
  _len++;
}

/*
void AString::Insert(unsigned index, char c)
{
  InsertSpace(index, 1);
  _chars[index] = c;
  _len++;
}
*/

void AString::Insert(unsigned index, const char *s)
{
  unsigned num = MyStringLen(s);
  if (num != 0)
  {
    InsertSpace(index, num);
    memcpy(_chars + index, s, num);
    _len += num;
  }
}

void AString::Insert(unsigned index, const AString &s)
{
  unsigned num = s.Len();
  if (num != 0)
  {
    InsertSpace(index, num);
    memcpy(_chars + index, s, num);
    _len += num;
  }
}

void AString::RemoveChar(char ch)
{
  int pos = Find(ch);
  if (pos < 0)
    return;
  const char *src = _chars;
  char *dest = _chars + pos;
  pos++;
  unsigned len = _len;
  for (; (unsigned)pos < len; pos++)
  {
    char c = src[(unsigned)pos];
    if (c != ch)
      *dest++ = c;
  }
  *dest = 0;
  _len = (unsigned)(dest - _chars);
}

// !!!!!!!!!!!!!!! test it if newChar = '\0'
void AString::Replace(char oldChar, char newChar)
{
  if (oldChar == newChar)
    return; // 0;
  // unsigned number = 0;
  int pos = 0;
  while ((unsigned)pos < _len)
  {
    pos = Find(oldChar, pos);
    if (pos < 0)
      break;
    _chars[pos] = newChar;
    pos++;
    // number++;
  }
  return; //  number;
}

void AString::Replace(const AString &oldString, const AString &newString)
{
  if (oldString.IsEmpty())
    return;  //  0;
  if (oldString == newString)
    return; // 0;
  unsigned oldLen = oldString.Len();
  unsigned newLen = newString.Len();
  // unsigned number = 0;
  int pos = 0;
  while ((unsigned)pos < _len)
  {
    pos = Find(oldString, pos);
    if (pos < 0)
      break;
    Delete(pos, oldLen);
    Insert(pos, newString);
    pos += newLen;
    // number++;
  }
  // return number;
}

void AString::Delete(unsigned index)
{
  MoveItems(index, index + 1);
  _len--;
}

void AString::Delete(unsigned index, unsigned count)
{
  if (index + count > _len)
    count = _len - index;
  if (count > 0)
  {
    MoveItems(index, index + count);
    _len -= count;
  }
}

void AString::DeleteFrontal(unsigned num)
{
  if (num != 0)
  {
    MoveItems(0, num);
    _len -= num;
  }
}

/*
AString operator+(const AString &s1, const AString &s2)
{
  AString result(s1);
  result += s2;
  return result;
}

AString operator+(const AString &s, const char *chars)
{
  AString result(s);
  result += chars;
  return result;
}

AString operator+(const char *chars, const AString &s)
{
  AString result(chars);
  result += s;
  return result;
}

AString operator+(const AString &s, char c)
{
  AString result(s);
  result += c;
  return result;
}
*/

/*
AString operator+(char c, const AString &s)
{
  AString result(c);
  result += s;
  return result;
}
*/




// ---------- UString ----------

void UString::InsertSpace(unsigned index, unsigned size)
{
  Grow(size);
  MoveItems(index + size, index);
}

void UString::ReAlloc(unsigned newLimit)
{
  if (newLimit < _len || newLimit >= 0x20000000) throw 20130221;
  // MY_STRING_REALLOC(_chars, wchar_t, newLimit + 1, _len + 1);
  wchar_t *newBuf = MY_STRING_NEW(wchar_t, newLimit + 1);
  wmemcpy(newBuf, _chars, _len + 1);
  MY_STRING_DELETE(_chars);
  _chars = newBuf;

  _limit = newLimit;
}

void UString::SetStartLen(unsigned len)
{
  _chars = 0;
  _chars = MY_STRING_NEW(wchar_t, len + 1);
  _len = len;
  _limit = len;
}

void UString::Grow_1()
{
  unsigned next = _len;
  next += next / 2;
  next += 16;
  next &= ~(unsigned)15;
  ReAlloc(next - 1);
}

void UString::Grow(unsigned n)
{
  unsigned freeSize = _limit - _len;
  if (n <= freeSize)
    return;
  
  unsigned next = _len + n;
  next += next / 2;
  next += 16;
  next &= ~(unsigned)15;
  ReAlloc(next - 1);
}


UString::UString(unsigned num, const wchar_t *s)
{
  unsigned len = MyStringLen(s);
  if (num > len)
    num = len;
  SetStartLen(num);
  wmemcpy(_chars, s, num);
  _chars[num] = 0;
}


UString::UString(unsigned num, const UString &s)
{
  if (num > s._len)
    num = s._len;
  SetStartLen(num);
  wmemcpy(_chars, s._chars, num);
  _chars[num] = 0;
}

UString::UString(const UString &s, wchar_t c)
{
  SetStartLen(s.Len() + 1);
  wchar_t *chars = _chars;
  unsigned len = s.Len();
  wmemcpy(chars, s, len);
  chars[len] = c;
  chars[len + 1] = 0;
}

UString::UString(const wchar_t *s1, unsigned num1, const wchar_t *s2, unsigned num2)
{
  SetStartLen(num1 + num2);
  wchar_t *chars = _chars;
  wmemcpy(chars, s1, num1);
  wmemcpy(chars + num1, s2, num2 + 1);
}

UString operator+(const UString &s1, const UString &s2) { return UString(s1, s1.Len(), s2, s2.Len()); }
UString operator+(const UString &s1, const wchar_t *s2) { return UString(s1, s1.Len(), s2, MyStringLen(s2)); }
UString operator+(const wchar_t *s1, const UString &s2) { return UString(s1, MyStringLen(s1), s2, s2.Len()); }

UString::UString()
{
  _chars = 0;
  _chars = MY_STRING_NEW(wchar_t, 4);
  _len = 0;
  _limit = 4 - 1;
  _chars[0] = 0;
}

UString::UString(wchar_t c)
{
  SetStartLen(1);
  _chars[0] = c;
  _chars[1] = 0;
}

UString::UString(const wchar_t *s)
{
  SetStartLen(MyStringLen(s));
  MyStringCopy(_chars, s);
}

UString::UString(const UString &s)
{
  SetStartLen(s._len);
  MyStringCopy(_chars, s._chars);
}

UString &UString::operator=(wchar_t c)
{
  if (1 > _limit)
  {
    wchar_t *newBuf = MY_STRING_NEW(wchar_t, 1 + 1);
    MY_STRING_DELETE(_chars);
    _chars = newBuf;
    _limit = 1;
  }
  _len = 1;
  _chars[0] = c;
  _chars[1] = 0;
  return *this;
}

UString &UString::operator=(const wchar_t *s)
{
  unsigned len = MyStringLen(s);
  if (len > _limit)
  {
    wchar_t *newBuf = MY_STRING_NEW(wchar_t, len + 1);
    MY_STRING_DELETE(_chars);
    _chars = newBuf;
    _limit = len;
  }
  _len = len;
  MyStringCopy(_chars, s);
  return *this;
}

UString &UString::operator=(const UString &s)
{
  if (&s == this)
    return *this;
  unsigned len = s._len;
  if (len > _limit)
  {
    wchar_t *newBuf = MY_STRING_NEW(wchar_t, len + 1);
    MY_STRING_DELETE(_chars);
    _chars = newBuf;
    _limit = len;
  }
  _len = len;
  MyStringCopy(_chars, s._chars);
  return *this;
}

UString &UString::operator+=(const wchar_t *s)
{
  unsigned len = MyStringLen(s);
  Grow(len);
  MyStringCopy(_chars + _len, s);
  _len += len;
  return *this;
}

UString &UString::operator+=(const UString &s)
{
  Grow(s._len);
  MyStringCopy(_chars + _len, s._chars);
  _len += s._len;
  return *this;
}

void UString::SetFrom(const wchar_t *s, unsigned len) // no check
{
  if (len > _limit)
  {
    wchar_t *newBuf = MY_STRING_NEW(wchar_t, len + 1);
    MY_STRING_DELETE(_chars);
    _chars = newBuf;
    _limit = len;
  }
  wmemcpy(_chars, s, len);
  _chars[len] = 0;
  _len = len;
}

void UString::SetFromAscii(const char *s)
{
  unsigned len = MyStringLen(s);
  if (len > _limit)
  {
    wchar_t *newBuf = MY_STRING_NEW(wchar_t, len + 1);
    MY_STRING_DELETE(_chars);
    _chars = newBuf;
    _limit = len;
  }
  wchar_t *chars = _chars;
  for (unsigned i = 0; i < len; i++)
    chars[i] = s[i];
  chars[len] = 0;
  _len = len;
}

void UString::AddAsciiStr(const char *s)
{
  unsigned len = MyStringLen(s);
  Grow(len);
  wchar_t *chars = _chars + _len;
  for (unsigned i = 0; i < len; i++)
    chars[i] = s[i];
  chars[len] = 0;
  _len += len;
}



int UString::Find(const UString &s, unsigned startIndex) const
{
  if (s.IsEmpty())
    return startIndex;
  for (; startIndex < _len; startIndex++)
  {
    unsigned j;
    for (j = 0; j < s._len && startIndex + j < _len; j++)
      if (_chars[startIndex + j] != s._chars[j])
        break;
    if (j == s._len)
      return (int)startIndex;
  }
  return -1;
}

int UString::ReverseFind(wchar_t c) const
{
  if (_len == 0)
    return -1;
  const wchar_t *p = _chars + _len - 1;
  for (;;)
  {
    if (*p == c)
      return (int)(p - _chars);
    if (p == _chars)
      return -1;
    p--;
  }
}

void UString::TrimLeft()
{
  const wchar_t *p = _chars;
  for (;; p++)
  {
    wchar_t c = *p;
    if (c != ' ' && c != '\n' && c != '\t')
      break;
  }
  unsigned pos = (unsigned)(p - _chars);
  if (pos != 0)
  {
    MoveItems(0, pos);
    _len -= pos;
  }
}

void UString::TrimRight()
{
  const wchar_t *p = _chars;
  int i;
  for (i = _len - 1; i >= 0; i--)
  {
    wchar_t c = p[i];
    if (c != ' ' && c != '\n' && c != '\t')
      break;
  }
  i++;
  if ((unsigned)i != _len)
  {
    _chars[i] = 0;
    _len = i;
  }
}

void UString::InsertAtFront(wchar_t c)
{
  if (_limit == _len)
    Grow_1();
  MoveItems(1, 0);
  _chars[0] = c;
  _len++;
}

/*
void UString::Insert(unsigned index, wchar_t c)
{
  InsertSpace(index, 1);
  _chars[index] = c;
  _len++;
}
*/

void UString::Insert(unsigned index, const wchar_t *s)
{
  unsigned num = MyStringLen(s);
  if (num != 0)
  {
    InsertSpace(index, num);
    wmemcpy(_chars + index, s, num);
    _len += num;
  }
}

void UString::Insert(unsigned index, const UString &s)
{
  unsigned num = s.Len();
  if (num != 0)
  {
    InsertSpace(index, num);
    wmemcpy(_chars + index, s, num);
    _len += num;
  }
}

void UString::RemoveChar(wchar_t ch)
{
  int pos = Find(ch);
  if (pos < 0)
    return;
  const wchar_t *src = _chars;
  wchar_t *dest = _chars + pos;
  pos++;
  unsigned len = _len;
  for (; (unsigned)pos < len; pos++)
  {
    wchar_t c = src[(unsigned)pos];
    if (c != ch)
      *dest++ = c;
  }
  *dest = 0;
  _len = (unsigned)(dest - _chars);
}

// !!!!!!!!!!!!!!! test it if newChar = '\0'
void UString::Replace(wchar_t oldChar, wchar_t newChar)
{
  if (oldChar == newChar)
    return; // 0;
  // unsigned number = 0;
  int pos = 0;
  while ((unsigned)pos < _len)
  {
    pos = Find(oldChar, pos);
    if (pos < 0)
      break;
    _chars[pos] = newChar;
    pos++;
    // number++;
  }
  return; //  number;
}

void UString::Replace(const UString &oldString, const UString &newString)
{
  if (oldString.IsEmpty())
    return;  //  0;
  if (oldString == newString)
    return; // 0;
  unsigned oldLen = oldString.Len();
  unsigned newLen = newString.Len();
  // unsigned number = 0;
  int pos = 0;
  while ((unsigned)pos < _len)
  {
    pos = Find(oldString, pos);
    if (pos < 0)
      break;
    Delete(pos, oldLen);
    Insert(pos, newString);
    pos += newLen;
    // number++;
  }
  // return number;
}

void UString::Delete(unsigned index)
{
  MoveItems(index, index + 1);
  _len--;
}

void UString::Delete(unsigned index, unsigned count)
{
  if (index + count > _len)
    count = _len - index;
  if (count > 0)
  {
    MoveItems(index, index + count);
    _len -= count;
  }
}

void UString::DeleteFrontal(unsigned num)
{
  if (num != 0)
  {
    MoveItems(0, num);
    _len -= num;
  }
}


// ----------------------------------------

/*
int MyStringCompareNoCase(const char *s1, const char *s2)
{
  return MyStringCompareNoCase(MultiByteToUnicodeString(s1), MultiByteToUnicodeString(s2));
}
*/

static inline UINT GetCurrentCodePage()
{
  #if defined(UNDER_CE) || !defined(_WIN32)
  return CP_ACP;
  #else
  return ::AreFileApisANSI() ? CP_ACP : CP_OEMCP;
  #endif
}

#ifdef USE_UNICODE_FSTRING

#ifndef _UNICODE

AString fs2fas(CFSTR s)
{
  return UnicodeStringToMultiByte(s, GetCurrentCodePage());
}

FString fas2fs(const AString &s)
{
  return MultiByteToUnicodeString(s, GetCurrentCodePage());
}

#endif

#else

UString fs2us(const FString &s)
{
  return MultiByteToUnicodeString((AString)s, GetCurrentCodePage());
}

FString us2fs(const wchar_t *s)
{
  return UnicodeStringToMultiByte(s, GetCurrentCodePage());
}

#endif
