// Main.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Windows/FileDir.h"

#include "Interface/CryptoInterface.h"

#include "../../../Compress/Interface/CompressInterface.h"
#include "../../../Crypto/Cipher/Common/CipherInterface.h"
#include "../../Format/Common/ArchiveInterface.h"
#include "../../Agent/Handler.h"
#include "../../GUI/ExtractEngine.h"
#include "../../Explorer/MyMessages.h"

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
  CSysString aFullPath;
  int aFileNamePartStartIndex;
  if (!NWindows::NFile::NDirectory::MyGetFullPathName(archiveName, aFullPath, aFileNamePartStartIndex))
  {
    MessageBox(NULL, "can't get archive name", "7-Zip", 0);
    return 1;
  }
  switches.Trim();
  bool anAssumeYes = false;
  if (switches == CSysString("-y"))
    anAssumeYes = true;
  HRESULT result = ExtractArchive(NULL, aFullPath, anAssumeYes);
  if (result == S_FALSE)
    MyMessageBox(L"Archive is not supported");
  else if (result != S_OK && result != E_ABORT)
    ShowErrorMessage(0, result);
  return 0;
}

