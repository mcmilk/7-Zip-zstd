// Main.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Common/StringConvert.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"

#include "../../ICoder.h"
#include "../../IPassword.h"
#include "../../Archive/IArchive.h"
#include "../../UI/Agent/Agent.h"
#include "../../UI/GUI/Extract.h"
#include "../../UI/Explorer/MyMessages.h"

HINSTANCE g_hInstance;

void SplitStringToSubstrings(const UString &srcString, 
    UStringVector &destStrings)
{
  UString s;
  bool quoteMode = false;
  for (int i = 0; i < srcString.Length(); i++)
  {
    wchar_t c = srcString[i];
    if (c == L'\"')
      quoteMode = !quoteMode;
    else if (c == L' ' && !quoteMode)
    {
      if (!s.IsEmpty())
      {
        destStrings.Add(s);
        s.Empty();
      }
    }
    else 
      s += c;
  }
  if (!s.IsEmpty())
    destStrings.Add(s);
}

int APIENTRY WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  g_hInstance = (HINSTANCE)hInstance;
  bool assumeYes = false;
  bool outputFolderDefined = false;
  CSysString outputFolder;
  UStringVector subStrings;
  SplitStringToSubstrings(GetCommandLineW(), subStrings);
  for (int i = 1; i < subStrings.Size(); i++)
  {
    const UString &s = subStrings[i];
    if (s.CompareNoCase(L"-y") == 0)
      assumeYes = true;
    else if (s.Left(2).CompareNoCase(L"-o") == 0)
    {
      outputFolder = GetSystemString(s.Mid(2));
      NWindows::NFile::NName::NormalizeDirPathPrefix(outputFolder);
      outputFolderDefined = !outputFolder.IsEmpty();
    }
  }

  TCHAR path[MAX_PATH + 1];
  ::GetModuleFileName(NULL, path, MAX_PATH);

  CSysString fullPath;
  int fileNamePartStartIndex;
  if (!NWindows::NFile::NDirectory::MyGetFullPathName(path, fullPath, fileNamePartStartIndex))
  {
    MyMessageBox(L"Error 1329484");
    return 1;
  }
  HRESULT result = ExtractArchive(NULL, path, assumeYes, !assumeYes, 
    outputFolderDefined ? outputFolder : 
      fullPath.Left(fileNamePartStartIndex));
  if (result == S_FALSE)
    MyMessageBox(L"Archive is not supported");
  else if (result != S_OK && result != E_ABORT)
    ShowErrorMessage(0, result);
  return 0;
}
