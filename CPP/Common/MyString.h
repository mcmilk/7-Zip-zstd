// Common/String.h

#ifndef __COMMON_STRING_H
#define __COMMON_STRING_H

#include <string.h>

#ifndef _WIN32
#include <wctype.h>
#include <wchar.h>
#endif

#include "MyTypes.h"
#include "MyVector.h"

inline unsigned MyStringLen(const char *s)
{
  unsigned i;
  for (i = 0; s[i] != 0; i++);
  return i;
}

inline void MyStringCopy(char *dest, const char *src)
{
  while ((*dest++ = *src++) != 0);
}

inline char *MyStpCpy(char *dest, const char *src)
{
  for (;;)
  {
    char c = *src;
    *dest = c;
    if (c == 0)
      return dest;
    src++;
    dest++;
  }
}

inline unsigned MyStringLen(const wchar_t *s)
{
  unsigned i;
  for (i = 0; s[i] != 0; i++);
  return i;
}

inline void MyStringCopy(wchar_t *dest, const wchar_t *src)
{
  while ((*dest++ = *src++) != 0);
}

int FindCharPosInString(const char *s, char c) throw();
int FindCharPosInString(const wchar_t *s, wchar_t c) throw();

#ifdef _WIN32
  #ifndef _UNICODE
    #define STRING_UNICODE_THROW
  #endif
#endif

#ifndef STRING_UNICODE_THROW
  #define STRING_UNICODE_THROW throw()
#endif

/*
inline char MyCharUpper_Ascii(char c)
{
  if (c >= 'a' && c <= 'z')
    return (char)(c - 0x20);
  return c;
}
inline wchar_t MyCharUpper_Ascii(wchar_t c)
{
  if (c >= 'a' && c <= 'z')
    return (wchar_t)(c - 0x20);
  return c;
}
*/

inline char MyCharLower_Ascii(char c)
{
  if (c >= 'A' && c <= 'Z')
    return (char)(c + 0x20);
  return c;
}

inline wchar_t MyCharLower_Ascii(wchar_t c)
{
  if (c >= 'A' && c <= 'Z')
    return (wchar_t)(c + 0x20);
  return c;
}

wchar_t MyCharUpper_WIN(wchar_t c) throw();

inline wchar_t MyCharUpper(wchar_t c) throw()
{
  if (c < 'a') return c;
  if (c <= 'z') return (wchar_t)(c - 0x20);
  if (c <= 0x7F) return c;
  #ifdef _WIN32
    #ifdef _UNICODE
      return (wchar_t)(unsigned)(UINT_PTR)CharUpperW((LPWSTR)(UINT_PTR)(unsigned)c);
    #else
      return (wchar_t)MyCharUpper_WIN(c);
    #endif
  #else
    return (wchar_t)towupper(c);
  #endif
}

/*
wchar_t MyCharLower_WIN(wchar_t c) throw();

inline wchar_t MyCharLower(wchar_t c) throw()
{
  if (c < 'A') return c;
  if (c <= 'Z') return (wchar_t)(c + 0x20);
  if (c <= 0x7F) return c;
  #ifdef _WIN32
    #ifdef _UNICODE
      return (wchar_t)(unsigned)(UINT_PTR)CharLowerW((LPWSTR)(UINT_PTR)(unsigned)c);
    #else
      return (wchar_t)MyCharLower_WIN(c);
    #endif
  #else
    return (wchar_t)tolower(c);
  #endif
}
*/

// char *MyStringUpper(char *s) throw();
// char *MyStringLower(char *s) throw();

// void MyStringUpper_Ascii(wchar_t *s) throw();
void MyStringLower_Ascii(wchar_t *s) throw();
// wchar_t *MyStringUpper(wchar_t *s) STRING_UNICODE_THROW;
// wchar_t *MyStringLower(wchar_t *s) STRING_UNICODE_THROW;

bool StringsAreEqualNoCase(const wchar_t *s1, const wchar_t *s2) throw();

bool IsString1PrefixedByString2(const char *s1, const char *s2) throw();
bool IsString1PrefixedByString2(const wchar_t *s1, const wchar_t *s2) throw();

int MyStringCompareNoCase(const wchar_t *s1, const wchar_t *s2) throw();
int MyStringCompareNoCase_N(const wchar_t *s1, const wchar_t *s2, unsigned num) throw();

// ---------- ASCII ----------
// char values in ASCII strings must be less then 128
bool StringsAreEqual_Ascii(const wchar_t *u, const char *a) throw();
bool StringsAreEqualNoCase_Ascii(const char *s1, const char *s2) throw();
bool StringsAreEqualNoCase_Ascii(const wchar_t *s1, const char *s2) throw();
bool StringsAreEqualNoCase_Ascii(const wchar_t *s1, const wchar_t *s2) throw();

#define MY_STRING_DELETE(_p_) delete []_p_;
// #define MY_STRING_DELETE(_p_) my_delete(_p_);

class AString
{
  char *_chars;
  unsigned _len;
  unsigned _limit;

  void MoveItems(unsigned dest, unsigned src)
  {
    memmove(_chars + dest, _chars + src, (size_t)(_len - src + 1) * sizeof(char));
  }
  
  void InsertSpace(unsigned &index, unsigned size);
  
  void ReAlloc(unsigned newLimit);
  void SetStartLen(unsigned len);
  void Grow_1();
  void Grow(unsigned n);

  // AString(unsigned num, const char *s);
  AString(unsigned num, const AString &s);
  AString(const AString &s, char c); // it's for String + char
  AString(const char *s1, unsigned num1, const char *s2, unsigned num2);

  friend AString operator+(const AString &s, char c) { return AString(s, c); } ;
  // friend AString operator+(char c, const AString &s); // is not supported

  friend AString operator+(const AString &s1, const AString &s2);
  friend AString operator+(const AString &s1, const char    *s2);
  friend AString operator+(const char    *s1, const AString &s2);

public:
  AString();
  AString(char c);
  AString(const char *s);
  AString(const AString &s);
  ~AString() { MY_STRING_DELETE(_chars); }

  unsigned Len() const { return _len; }
  bool IsEmpty() const { return _len == 0; }
  void Empty() { _len = 0; _chars[0] = 0; }

  operator const char *() const { return _chars; }
  const char *Ptr() const { return _chars; }
  const char *Ptr(unsigned pos) const { return _chars + pos; }
  const char *RightPtr(unsigned num) const { return _chars + _len - num; }
  char Back() const { return _chars[_len - 1]; }

  void ReplaceOneCharAtPos(unsigned pos, char c) { _chars[pos] = c; }

  // The minimum size of the character buffer in characters.
  // This value does not include space for a null terminator.
  char *GetBuffer(unsigned minBufLen)
  {
    if (minBufLen > _limit)
      ReAlloc(minBufLen);
    return _chars;
  }
  void ReleaseBuffer() { ReleaseBuffer(MyStringLen(_chars)); }
  void ReleaseBuffer(unsigned newLen) { _len = newLen; _chars[newLen] = 0; }

  AString &operator=(char c);
  AString &operator=(const char *s);
  AString &operator=(const AString &s);

  AString &operator+=(char c)
  {
    if (_limit == _len)
      Grow_1();
    unsigned len = _len;
    char *chars = _chars;
    chars[len++] = c;
    chars[len] = 0;
    _len = len;
    return *this;
  }

  AString &operator+=(const char *s);
  AString &operator+=(const AString &s);

  void SetFrom(const char *s, unsigned len); // no check
  // AString Mid(unsigned startIndex, unsigned count) const { return AString(count, _chars + startIndex); }
  AString Left(unsigned count) const { return AString(count, *this); }

  // void MakeUpper() { MyStringUpper(_chars); }
  // void MakeLower() { MyStringLower(_chars); }


  // int Compare(const char *s) const { return MyStringCompare(_chars, s); }
  // int Compare(const AString &s) const { return MyStringCompare(_chars, s._chars); }
  // int CompareNoCase(const char *s) const { return MyStringCompareNoCase(_chars, s); }
  // int CompareNoCase(const AString &s) const { return MyStringCompareNoCase(_chars, s._chars); }
  bool IsPrefixedBy(const char *s) const { return IsString1PrefixedByString2(_chars, s); }
  bool IsPrefixedBy_Ascii_NoCase(const char *s) const throw();
 
  int Find(char c) const { return FindCharPosInString(_chars, c); }
  int Find(char c, unsigned startIndex) const
  {
    int pos = FindCharPosInString(_chars + startIndex, c);
    return pos < 0 ? -1 : (int)startIndex + pos;
  }
  int ReverseFind(char c) const throw();
  int Find(const AString &s) const { return Find(s, 0); }
  int Find(const AString &s, unsigned startIndex) const throw();

  void TrimLeft() throw();
  void TrimRight() throw();
  void Trim()
  {
    TrimRight();
    TrimLeft();
  }

  void InsertAtFront(char c);
  // void Insert(unsigned index, char c);
  void Insert(unsigned index, const char *s);
  void Insert(unsigned index, const AString &s);

  void RemoveChar(char ch) throw();
  void Replace(char oldChar, char newChar) throw();
  void Replace(const AString &oldString, const AString &newString);
  
  void Delete(unsigned index) throw();
  void Delete(unsigned index, unsigned count) throw();
  void DeleteFrontal(unsigned num) throw();
  void DeleteBack() { _chars[--_len] = 0; }
  void DeleteFrom(unsigned index)
  {
    if (index < _len)
    {
      _len = index;
      _chars[index] = 0;
    }
  }
};

bool operator<(const AString &s1, const AString &s2);
bool operator>(const AString &s1, const AString &s2);

/*
bool operator==(const AString &s1, const AString &s2);
bool operator==(const AString &s1, const char    *s2);
bool operator==(const char    *s1, const AString &s2);

bool operator!=(const AString &s1, const AString &s2);
bool operator!=(const AString &s1, const char    *s2);
bool operator!=(const char    *s1, const AString &s2);
*/

inline bool operator==(const AString &s1, const AString &s2) { return s1.Len() == s2.Len() && strcmp(s1, s2) == 0; }
inline bool operator==(const AString &s1, const char    *s2) { return strcmp(s1, s2) == 0; }
inline bool operator==(const char    *s1, const AString &s2) { return strcmp(s1, s2) == 0; }

inline bool operator!=(const AString &s1, const AString &s2) { return s1.Len() != s2.Len() || strcmp(s1, s2) != 0; }
inline bool operator!=(const AString &s1, const char    *s2) { return strcmp(s1, s2) != 0; }
inline bool operator!=(const char    *s1, const AString &s2) { return strcmp(s1, s2) != 0; }



class UString
{
  wchar_t *_chars;
  unsigned _len;
  unsigned _limit;

  void MoveItems(unsigned dest, unsigned src)
  {
    memmove(_chars + dest, _chars + src, (size_t)(_len - src + 1) * sizeof(wchar_t));
  }
  
  void InsertSpace(unsigned index, unsigned size);
  
  void ReAlloc(unsigned newLimit);
  void SetStartLen(unsigned len);
  void Grow_1();
  void Grow(unsigned n);

  UString(unsigned num, const wchar_t *s); // for Mid
  UString(unsigned num, const UString &s); // for Left
  UString(const UString &s, wchar_t c); // it's for String + char
  UString(const wchar_t *s1, unsigned num1, const wchar_t *s2, unsigned num2);

  friend UString operator+(const UString &s, wchar_t c) { return UString(s, c); } ;
  // friend UString operator+(wchar_t c, const UString &s); // is not supported

  friend UString operator+(const UString &s1, const UString &s2);
  friend UString operator+(const UString &s1, const wchar_t *s2);
  friend UString operator+(const wchar_t *s1, const UString &s2);

public:
  UString();
  UString(wchar_t c);
  UString(const wchar_t *s);
  UString(const UString &s);
  ~UString() { MY_STRING_DELETE(_chars); }

  unsigned Len() const { return _len; }
  bool IsEmpty() const { return _len == 0; }
  void Empty() { _len = 0; _chars[0] = 0; }

  operator const wchar_t *() const { return _chars; }
  const wchar_t *Ptr() const { return _chars; }
  const wchar_t *Ptr(unsigned pos) const { return _chars + pos; }
  const wchar_t *RightPtr(unsigned num) const { return _chars + _len - num; }
  wchar_t Back() const { return _chars[_len - 1]; }

  void ReplaceOneCharAtPos(unsigned pos, wchar_t c) { _chars[pos] = c; }

  // The minimum size of the character buffer in characters.
  // This value does not include space for a null terminator.
  wchar_t *GetBuffer(unsigned minBufLen)
  {
    if (minBufLen > _limit)
      ReAlloc(minBufLen);
    return _chars;
  }
  void ReleaseBuffer() { ReleaseBuffer(MyStringLen(_chars)); }
  void ReleaseBuffer(unsigned newLen) { _len = newLen; _chars[newLen] = 0; }

  UString &operator=(wchar_t c);
  UString &operator=(const wchar_t *s);
  UString &operator=(const UString &s);

  UString &operator+=(wchar_t c)
  {
    if (_limit == _len)
      Grow_1();
    unsigned len = _len;
    wchar_t *chars = _chars;
    chars[len++] = c;
    chars[len] = 0;
    _len = len;
    return *this;
  }

  UString &operator+=(const wchar_t *s);
  UString &operator+=(const UString &s);

  void SetFrom(const wchar_t *s, unsigned len); // no check

  void SetFromAscii(const char *s);
  void AddAsciiStr(const char *s);

  UString Mid(unsigned startIndex, unsigned count) const { return UString(count, _chars + startIndex); }
  UString Left(unsigned count) const { return UString(count, *this); }

  // void MakeUpper() { MyStringUpper(_chars); }
  // void MakeUpper() { MyStringUpper_Ascii(_chars); }
  // void MakeUpper_Ascii() { MyStringUpper_Ascii(_chars); }
  void MakeLower_Ascii() { MyStringLower_Ascii(_chars); }

  bool IsEqualTo(const char *s) const { return StringsAreEqual_Ascii(_chars, s); }
  bool IsEqualToNoCase(const wchar_t *s) const { return StringsAreEqualNoCase(_chars, s); }
  int Compare(const wchar_t *s) const { return wcscmp(_chars, s); }
  // int Compare(const UString &s) const { return MyStringCompare(_chars, s._chars); }
  // int CompareNoCase(const wchar_t *s) const { return MyStringCompareNoCase(_chars, s); }
  // int CompareNoCase(const UString &s) const { return MyStringCompareNoCase(_chars, s._chars); }
  bool IsPrefixedBy(const wchar_t *s) const { return IsString1PrefixedByString2(_chars, s); };
  bool IsPrefixedBy_Ascii_NoCase(const char *s) const throw();

  int Find(wchar_t c) const { return FindCharPosInString(_chars, c); }
  int Find(wchar_t c, unsigned startIndex) const
  {
    int pos = FindCharPosInString(_chars + startIndex, c);
    return pos < 0 ? -1 : (int)startIndex + pos;
  }
  int Find(const UString &s) const { return Find(s, 0); }
  int Find(const UString &s, unsigned startIndex) const throw();
  int ReverseFind(wchar_t c) const throw();

  void TrimLeft() throw();
  void TrimRight() throw();
  void Trim()
  {
    TrimRight();
    TrimLeft();
  }

  void InsertAtFront(wchar_t c);
  // void Insert(unsigned index, wchar_t c);
  void Insert(unsigned index, const wchar_t *s);
  void Insert(unsigned index, const UString &s);

  void RemoveChar(wchar_t ch) throw();
  void Replace(wchar_t oldChar, wchar_t newChar) throw();
  void Replace(const UString &oldString, const UString &newString);
  
  void Delete(unsigned index) throw();
  void Delete(unsigned index, unsigned count) throw();
  void DeleteFrontal(unsigned num) throw();
  void DeleteBack() { _chars[--_len] = 0; }
  void DeleteFrom(unsigned index)
  {
    if (index < _len)
    {
      _len = index;
      _chars[index] = 0;
    }
  }
};

bool operator<(const UString &s1, const UString &s2);
bool operator>(const UString &s1, const UString &s2);

inline bool operator==(const UString &s1, const UString &s2) { return s1.Len() == s2.Len() && wcscmp(s1, s2) == 0; }
inline bool operator==(const UString &s1, const wchar_t *s2) { return wcscmp(s1, s2) == 0; }
inline bool operator==(const wchar_t *s1, const UString &s2) { return wcscmp(s1, s2) == 0; }

inline bool operator!=(const UString &s1, const UString &s2) { return s1.Len() != s2.Len() || wcscmp(s1, s2) != 0; }
inline bool operator!=(const UString &s1, const wchar_t *s2) { return wcscmp(s1, s2) != 0; }
inline bool operator!=(const wchar_t *s1, const UString &s2) { return wcscmp(s1, s2) != 0; }


typedef CObjectVector<AString> AStringVector;
typedef CObjectVector<UString> UStringVector;

#ifdef _UNICODE
  typedef UString CSysString;
#else
  typedef AString CSysString;
#endif

typedef CObjectVector<CSysString> CSysStringVector;


// ---------- FString ----------

#ifdef _WIN32
  #define USE_UNICODE_FSTRING
#endif

#ifdef USE_UNICODE_FSTRING

  #define __FTEXT(quote) L##quote

  typedef wchar_t FChar;
  typedef UString FString;

  #define fs2us(_x_) (_x_)
  #define us2fs(_x_) (_x_)
  FString fas2fs(const AString &s);
  AString fs2fas(const FChar *s);

#else

  #define __FTEXT(quote) quote

  typedef char FChar;
  typedef AString FString;

  UString fs2us(const FString &s);
  FString us2fs(const wchar_t *s);
  #define fas2fs(_x_) (_x_)
  #define fs2fas(_x_) (_x_)

#endif

#define FTEXT(quote) __FTEXT(quote)

#define FCHAR_PATH_SEPARATOR FTEXT(CHAR_PATH_SEPARATOR)
#define FSTRING_PATH_SEPARATOR FTEXT(STRING_PATH_SEPARATOR)
#define FCHAR_ANY_MASK FTEXT('*')
#define FSTRING_ANY_MASK FTEXT("*")
typedef const FChar *CFSTR;

typedef CObjectVector<FString> FStringVector;

#endif
