// ExtractingFilePath.cpp

#include "StdAfx.h"
#include "ExtractingFilePath.h"

static void ReplaceDisk(UString &s)
{
  int i;
  for (i = 0; i < s.Length(); i++)
    if (s[i] != ' ')
      break;
  if (s.Length() > i + 1)
  {
    if (s[i + 1] == L':')
    {
      s.Delete(i + 1);
      // s.Insert(i + 1, L'_');
    }
  }
}

UString GetCorrectFileName(const UString &path)
{
  UString result = path;
  {
    UString test = path;
    test.Trim();
    if (test == L"..")
      result.Replace(L"..", L"");
  }
  ReplaceDisk(result);
  return result;
}

UString GetCorrectPath(const UString &path)
{
  UString result = path;
  int first;
  for (first = 0; first < result.Length(); first++)
    if (result[first] != ' ')
      break;
  while(result.Length() > first)
  {
    if (
      #ifdef _WIN32
      result[first] == L'\\' || 
      #endif
      result[first] == L'/')
    {
      result.Delete(first);
      continue;
    }
    break;
  }
  #ifdef _WIN32
  result.Replace(L"..\\", L"");
  #endif
  result.Replace(L"../", L"");

  ReplaceDisk(result);
  return result;
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
      i++;
  }
}
