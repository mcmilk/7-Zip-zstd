// Main.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Windows/FileDir.h"

#include "../../ICoder.h"
#include "../../IPassword.h"
#include "../../Archive/IArchive.h"
#include "../../UI/Agent/Agent.h"
#include "../../UI/GUI/Extract.h"
#include "../../UI/Explorer/MyMessages.h"

HINSTANCE g_hInstance;

static void GetArchiveName(
    const CSysString &commandLine, 
    CSysString &archiveName, 
    CSysString &switches)
{
  archiveName.Empty();
  switches.Empty();
  bool quoteMode = false;
  for (int i = 0; i < commandLine.Length(); i++)
  {
    TCHAR c = commandLine[i];
    if (c == '\"')
      quoteMode = !quoteMode;
    else if (c == ' ' && !quoteMode)
    {
      if (!quoteMode)
      {
        i++;
        break;
      }
    }
    else 
      archiveName += c;
  }
  switches = commandLine + i;
}

int APIENTRY WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  g_hInstance = (HINSTANCE)hInstance;
  CSysString archiveName, switches;
  GetArchiveName(GetCommandLine(), archiveName, switches);
  /*
  CSysString fullPath;
  int fileNamePartStartIndex;
  if (!NWindows::NFile::NDirectory::MyGetFullPathName(archiveName, fullPath, fileNamePartStartIndex))
  {
    MessageBox(NULL, "can't get archive name", "7-Zip", 0);
    return 1;
  }
  */
  switches.Trim();
  bool assumeYes = false;
  if (switches == CSysString("-y"))
    assumeYes = true;

  TCHAR path[MAX_PATH + 1];
  ::GetModuleFileName(NULL, path, MAX_PATH);

  CSysString fullPath;
  int fileNamePartStartIndex;
  if (!NWindows::NFile::NDirectory::MyGetFullPathName(path, fullPath, fileNamePartStartIndex))
  {
    MessageBox(NULL, TEXT("Error 1329484"), TEXT("7-Zip"), 0);
    return 1;
  }
  HRESULT result = ExtractArchive(NULL, path, assumeYes, !assumeYes, fullPath.Left(fileNamePartStartIndex));
  if (result == S_FALSE)
    MyMessageBox(L"Archive is not supported");
  else if (result != S_OK && result != E_ABORT)
    ShowErrorMessage(0, result);
  return 0;
}

