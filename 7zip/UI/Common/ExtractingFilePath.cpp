// ExtractingFilePath.cpp

#include "StdAfx.h"
#include "ExtractingFilePath.h"

UString GetCorrectPath(const UString &path)
{
  UString result = path;
  int first;
  for (first = 0; first < result.Length(); first++)
    if (result[first] != ' ')
      break;
  while(result.Length() > first)
  {

    if (result[first] == L'\\' || result[first] == L'/')
    {
      result.Delete(first);
      continue;
    }
    break;
  }
  result.Replace(L"..\\", L"");
  result.Replace(L"../", L"");

  if (result.Length() > 1)
  {
    if (result[first + 1] == L':')
    {
      result.Delete(first + 1);
      // result.Insert(first + 1, L'_');
    }
  }
  return result;

}

