// ExtractingFilePath.cpp

#include "StdAfx.h"
#include "ExtractingFilePath.h"

static UString ReplaceIncorrectChars(const UString &s)
{
  #ifdef _WIN32
  UString res;
  for (int i = 0; i < s.Length(); i++)
  {
    wchar_t c = s[i];
    if (c < 0x20 || c == '*' || c == '?' || c == '<' || c == '>'  || c == '|' || c == ':' || c == '"')
      c = '_';
    res += c;
  }
  return res;
  #else
  return s;
  #endif
}

#ifdef _WIN32
static const wchar_t *g_ReservedNames[] =
{
  L"CON", L"PRN", L"AUX", L"NUL"
};

static bool CheckTail(const UString &name, int len)
{
  int dotPos = name.Find(L'.');
  if (dotPos < 0)
    dotPos = name.Length();
  UString s = name.Left(dotPos);
  s.TrimRight();
  return (s.Length() != len);
}

static bool CheckNameNum(const UString &name, const wchar_t *reservedName)
{
  int len = MyStringLen(reservedName);
  if (name.Length() <= len)
    return true;
  if (name.Left(len).CompareNoCase(reservedName) != 0)
    return true;
  wchar_t c = name[len];
  if (c < L'0' || c > L'9')
    return true;
  return CheckTail(name, len + 1);
}

static bool IsSupportedName(const UString &name)
{
  for (int i = 0; i < sizeof(g_ReservedNames) / sizeof(g_ReservedNames[0]); i++)
  {
    const wchar_t *reservedName = g_ReservedNames[i];
    int len = MyStringLen(reservedName);
    if (name.Length() < len)
      continue;
    if (name.Left(len).CompareNoCase(reservedName) != 0)
      continue;
    if (!CheckTail(name, len))
      return false;
  }
  if (!CheckNameNum(name, L"COM"))
    return false;
  return CheckNameNum(name, L"LPT");
}
#endif

static UString GetCorrectFileName(const UString &path)
{
  if (path == L".." || path == L".")
    return UString();
  return ReplaceIncorrectChars(path);
}

void MakeCorrectPath(UStringVector &pathParts)
{
  for (int i = 0; i < pathParts.Size();)
  {
    UString &s = pathParts[i];
    s = GetCorrectFileName(s);
    if (s.IsEmpty())
      pathParts.Delete(i);
    else
    {
      #ifdef _WIN32
      if (!IsSupportedName(s))
        s = (UString)L"_" + s;
      #endif
      i++;
    }
  }
}

