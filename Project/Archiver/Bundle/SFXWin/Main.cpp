// MainCommandLineAr.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Windows/FileDir.h"

#include "../../../Compress/Interface/CompressInterface.h"
#include "../../Format/Common/FormatCryptoInterface.h"
#include "../../Common/IArchiveHandler2.h"
#include "../../Agent/Handler.h"
#include "../../Explorer/ExtractEngine.h"

HINSTANCE g_hInstance;

static CSysString GetArchiveName(const CSysString &aCommandLine)
{
  CSysString aResult;
  bool aQuoteMode = false;
  for (int i = 0; i < aCommandLine.Length(); i++)
  {
    TCHAR aChar = aCommandLine[i];
    if (aChar == '\"')
      aQuoteMode = !aQuoteMode;
    else if (aChar == ' ' && !aQuoteMode)
    {
      if (!aQuoteMode)
        break;
    }
    else 
      aResult += aChar;
  }
  return aResult;
}

int APIENTRY WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  g_hInstance = (HINSTANCE)hInstance;
  CSysString anArchiveName = GetArchiveName(GetCommandLine());
  CSysString aFullPath;
  if (!NWindows::NFile::NDirectory::MyGetFullPathName(anArchiveName, aFullPath))
  {
    MessageBox(NULL, "can't get archive name", "7-Zip", 0);
    return 1;
  }
  ExtractArchive(NULL, aFullPath);
  return 0;
}

