// Main.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Common/StringConvert.h"
#include "Common/CommandLineParser.h"

#include "Windows/FileDir.h"
#include "Windows/FileName.h"
#include "Windows/DLL.h"

#include "../../ICoder.h"
#include "../../IPassword.h"
#include "../../Archive/IArchive.h"
#include "../../UI/Agent/Agent.h"
#include "../../UI/GUI/Extract.h"
#include "../../UI/Explorer/MyMessages.h"

HINSTANCE g_hInstance;

int APIENTRY WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  g_hInstance = (HINSTANCE)hInstance;
  bool assumeYes = false;
  bool outputFolderDefined = false;
  UString outputFolder;
  UStringVector subStrings;
  NCommandLineParser::SplitCommandLine(GetCommandLineW(), subStrings);
  for (int i = 1; i < subStrings.Size(); i++)
  {
    const UString &s = subStrings[i];
    if (s.CompareNoCase(L"-y") == 0)
      assumeYes = true;
    else if (s.Left(2).CompareNoCase(L"-o") == 0)
    {
      outputFolder = s.Mid(2);
      NWindows::NFile::NName::NormalizeDirPathPrefix(outputFolder);
      outputFolderDefined = !outputFolder.IsEmpty();
    }
  }

  UString path;
  NWindows::NDLL::MyGetModuleFileName(g_hInstance, path);

  UString fullPath;
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
