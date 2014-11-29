// ExtractingFilePath.cpp

#include "StdAfx.h"

#include "../../../Common/Wildcard.h"

#include "../../../Windows/FileName.h"

#include "ExtractingFilePath.h"

static UString ReplaceIncorrectChars(const UString &s, bool repaceColon)
{
  #ifdef _WIN32
  UString res;
  bool beforeColon = true;
  {
    for (unsigned i = 0; i < s.Len(); i++)
    {
      wchar_t c = s[i];
      if (beforeColon)
        if (c == '*' || c == '?' || c < 0x20 || c == '<' || c == '>' || c == '|' || c == '"')
          c = '_';
      if (c == ':')
      {
        if (repaceColon)
          c = '_';
        else
          beforeColon = false;
      }
      res += c;
    }
  }
  if (beforeColon)
  {
    for (int i = res.Len() - 1; i >= 0; i--)
    {
      wchar_t c = res[i];
      if (c != '.' && c != ' ')
        break;
      res.ReplaceOneCharAtPos(i, '_');
    }
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

static bool CheckTail(const UString &name, unsigned len)
{
  int dotPos = name.Find(L'.');
  if (dotPos < 0)
    dotPos = name.Len();
  UString s = name.Left(dotPos);
  s.TrimRight();
  return s.Len() != len;
}

static bool CheckNameNum(const UString &name, const wchar_t *reservedName)
{
  unsigned len = MyStringLen(reservedName);
  if (name.Len() <= len)
    return true;
  if (MyStringCompareNoCase_N(name, reservedName, len) != 0)
    return true;
  wchar_t c = name[len];
  if (c < L'0' || c > L'9')
    return true;
  return CheckTail(name, len + 1);
}

static bool IsSupportedName(const UString &name)
{
  for (unsigned i = 0; i < ARRAY_SIZE(g_ReservedNames); i++)
  {
    const wchar_t *reservedName = g_ReservedNames[i];
    unsigned len = MyStringLen(reservedName);
    if (name.Len() < len)
      continue;
    if (MyStringCompareNoCase_N(name, reservedName, len) != 0)
      continue;
    if (!CheckTail(name, len))
      return false;
  }
  if (!CheckNameNum(name, L"COM"))
    return false;
  return CheckNameNum(name, L"LPT");
}

#endif

static UString GetCorrectFileName(const UString &path, bool repaceColon)
{
  if (path == L".." || path == L".")
    return UString();
  return ReplaceIncorrectChars(path, repaceColon);
}

void MakeCorrectPath(bool isPathFromRoot, UStringVector &pathParts, bool replaceAltStreamColon)
{
  for (unsigned i = 0; i < pathParts.Size();)
  {
    UString &s = pathParts[i];
    #ifdef _WIN32
    bool needReplaceColon = (replaceAltStreamColon || i != pathParts.Size() - 1);
    if (i == 0 && isPathFromRoot && NWindows::NFile::NName::IsDrivePath(s))
    {
      UString s2 = s[0];
      s2 += L'_';
      s2 += GetCorrectFileName(s.Ptr(2), needReplaceColon);
      s = s2;
    }
    else
      s = GetCorrectFileName(s, needReplaceColon);
    #endif

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

UString MakePathNameFromParts(const UStringVector &parts)
{
  UString result;
  FOR_VECTOR (i, parts)
  {
    if (i != 0)
      result += WCHAR_PATH_SEPARATOR;
    result += parts[i];
  }
  return result;
}

static const wchar_t *k_EmptyReplaceName = L"[]";

void Correct_IfEmptyLastPart(UStringVector &parts)
{
  if (parts.IsEmpty())
    parts.Add(k_EmptyReplaceName);
  else
  {
    UString &s = parts.Back();
    if (s.IsEmpty())
      s = k_EmptyReplaceName;
  }
}

UString GetCorrectFsPath(const UString &path)
{
  UString res = GetCorrectFileName(path, true);
  #ifdef _WIN32
  if (!IsSupportedName(res))
    res = (UString)L"_" + res;
  #endif
  if (res.IsEmpty())
    res = k_EmptyReplaceName;
  return res;
}
  
UString GetCorrectFullFsPath(const UString &path)
{
  UStringVector parts;
  SplitPathToParts(path, parts);
  FOR_VECTOR (i, parts)
  {
    UString &s = parts[i];
    #ifdef _WIN32
    while (!s.IsEmpty() && (s.Back() == '.' || s.Back() == ' '))
      s.DeleteBack();
    if (!IsSupportedName(s))
      s.InsertAtFront(L'_');
    #endif
  }
  return MakePathNameFromParts(parts);
}
