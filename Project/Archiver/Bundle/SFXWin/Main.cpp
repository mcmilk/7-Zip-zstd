// MainCommandLineAr.cpp

#include "StdAfx.h"

#include <initguid.h>

#include "Windows/FileDir.h"

#include "Interface/CryptoInterface.h"

#include "../../../Compress/Interface/CompressInterface.h"
#include "../../../Crypto/Cipher/Common/CipherInterface.h"
#include "../../Format/Common/ArchiveInterface.h"
#include "../../Agent/Handler.h"
#include "../../Explorer/ExtractEngine.h"
#include "../../Explorer/MyMessages.h"

HINSTANCE g_hInstance;

static void GetArchiveName(
    const CSysString &aCommandLine, 
    CSysString &anArchiveName, 
    CSysString &aSwitches)
{
  anArchiveName.Empty();
  aSwitches.Empty();
  bool aQuoteMode = false;
  for (int i = 0; i < aCommandLine.Length(); i++)
  {
    TCHAR aChar = aCommandLine[i];
    if (aChar == '\"')
      aQuoteMode = !aQuoteMode;
    else if (aChar == ' ' && !aQuoteMode)
    {
      if (!aQuoteMode)
      {
        i++;
        break;
      }
    }
    else 
      anArchiveName += aChar;
  }
  aSwitches = aCommandLine + i;
}

int APIENTRY WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  g_hInstance = (HINSTANCE)hInstance;
  CSysString anArchiveName, aSwitches;
  GetArchiveName(GetCommandLine(), anArchiveName, aSwitches);
  CSysString aFullPath;
  int aFileNamePartStartIndex;
  if (!NWindows::NFile::NDirectory::MyGetFullPathName(anArchiveName, aFullPath, aFileNamePartStartIndex))
  {
    MessageBox(NULL, "can't get archive name", "7-Zip", 0);
    return 1;
  }
  aSwitches.Trim();
  bool anAssumeYes = false;
  if (aSwitches == CSysString("-y"))
    anAssumeYes = true;
  HRESULT aResult = ExtractArchive(NULL, aFullPath, anAssumeYes);
  if (aResult == S_FALSE)
    MyMessageBox(L"Archive is not supported");
  else if (aResult != S_OK)
    ShowErrorMessage(0, aResult);
  return 0;
}

